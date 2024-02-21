#include "core/rpc/rpc_backend_manager.h"
#include "aimrt_module_cpp_interface/rpc/rpc_status.h"
#include "util/stl_tool.h"

namespace aimrt::runtime::core::rpc {

void RpcBackendManager::Initialize(RpcRegistry* rpc_registry_ptr) {
  AIMRT_CHECK_ERROR_THROW(
      std::atomic_exchange(&state_, State::Init) == State::PreInit,
      "Rpc backend manager can only be initialized once.");

  rpc_registry_ptr_ = rpc_registry_ptr;
}

void RpcBackendManager::Start() {
  AIMRT_CHECK_ERROR_THROW(
      std::atomic_exchange(&state_, State::Start) == State::Init,
      "Function can only be called when state is 'Init'.");

  for (auto& backend : rpc_backend_index_vec_) {
    AIMRT_TRACE("Start rpc backend '{}'.", backend->Name());
    backend->Start();
  }
}

void RpcBackendManager::Shutdown() {
  if (std::atomic_exchange(&state_, State::Shutdown) == State::Shutdown)
    return;

  for (auto& backend : rpc_backend_index_vec_) {
    AIMRT_TRACE("Shutdown rpc backend '{}'.", backend->Name());
    backend->Shutdown();
  }

  rpc_backend_index_map_.clear();
  rpc_backend_index_vec_.clear();
}

void RpcBackendManager::RegisterRpcBackend(RpcBackendBase* rpc_backend_ptr) {
  AIMRT_CHECK_ERROR_THROW(
      state_.load() == State::PreInit,
      "Function can only be called when state is 'PreInit'.");

  rpc_backend_index_vec_.emplace_back(rpc_backend_ptr);
  rpc_backend_index_map_.emplace(rpc_backend_ptr->Name(), rpc_backend_ptr);
}

bool RpcBackendManager::RegisterServiceFunc(
    ServiceFuncWrapper&& service_func_wrapper) {
  if (state_.load() != State::Init) {
    AIMRT_ERROR("Service func can only be registered when state is 'Init'.");
    return false;
  }

  auto service_func_wrapper_ptr =
      std::make_unique<ServiceFuncWrapper>(std::move(service_func_wrapper));
  const auto& service_func_wrapper_ref = *service_func_wrapper_ptr;

  if (!rpc_registry_ptr_->RegisterServiceFunc(std::move(service_func_wrapper_ptr)))
    return false;

  bool ret = true;
  for (auto& itr : rpc_backend_index_vec_) {
    ret &= itr->RegisterServiceFunc(service_func_wrapper_ref);
  }
  return ret;
}

bool RpcBackendManager::RegisterClientFunc(ClientFuncWrapper&& client_func_wrapper) {
  if (state_.load() != State::Init) {
    AIMRT_ERROR("Client func can only be registered when state is 'Init'.");
    return false;
  }

  auto client_func_wrapper_ptr =
      std::make_unique<ClientFuncWrapper>(std::move(client_func_wrapper));
  const auto& client_func_wrapper_ref = *client_func_wrapper_ptr;

  if (!rpc_registry_ptr_->RegisterClientFunc(std::move(client_func_wrapper_ptr)))
    return false;

  bool ret = true;
  for (auto& itr : rpc_backend_index_vec_) {
    ret &= itr->RegisterClientFunc(client_func_wrapper_ref);
  }
  return ret;
}

void RpcBackendManager::Invoke(ClientInvokeWrapper&& client_invoke_wrapper) {
  assert(state_.load() == State::Start);

  auto client_invoke_wrapper_ptr =
      std::make_shared<ClientInvokeWrapper>(std::move(client_invoke_wrapper));

  // 如果ctx中指定了后端，则使用指定的后端
  std::string_view to_addr(client_invoke_wrapper_ptr->ctx_ref.GetToAddr());
  if (!to_addr.empty()) {
    // to_addr格式：backend_name://url_str
    AIMRT_TRACE("Rpc call use the specified address '{}', func name '{}'.",
                to_addr, client_invoke_wrapper_ptr->func_name);
    auto pos = to_addr.find("://");
    if (pos != std::string_view::npos) {
      auto backend_itr = rpc_backend_index_map_.find(to_addr.substr(0, pos));
      if (backend_itr != rpc_backend_index_map_.end()) {
        if (backend_itr->second->TryInvoke(client_invoke_wrapper_ptr)) {
          return;
        }
      }
    }
    AIMRT_ERROR("Rpc call address '{}' is invalid, func name '{}'.", to_addr,
                client_invoke_wrapper_ptr->func_name);
    client_invoke_wrapper_ptr->callback(AIMRT_RPC_STATUS_CLI_INVALID_ADDR);
    return;
  }

  // 根据配置的顺序进行尝试
  for (auto& backend : rpc_backend_index_vec_) {
    AIMRT_TRACE("Rpc call try backend '{}', func name '{}'.",
                backend->Name(), client_invoke_wrapper_ptr->func_name);
    if (backend->TryInvoke(client_invoke_wrapper_ptr)) {
      return;
    }
  }

  AIMRT_ERROR("Rpc call found no backend to handle, func name '{}'.",
              client_invoke_wrapper_ptr->func_name);
  client_invoke_wrapper_ptr->callback(AIMRT_RPC_STATUS_CLI_NO_BACKEND_TO_HANDLE);
}

}  // namespace aimrt::runtime::core::rpc