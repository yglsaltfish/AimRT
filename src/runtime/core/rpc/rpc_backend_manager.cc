#include "core/rpc/rpc_backend_manager.h"

#include <regex>
#include <vector>

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
      "Method can only be called when state is 'Init'.");

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

void RpcBackendManager::SetClientsBackendsRules(
    const std::vector<std::pair<std::string, std::vector<std::string>>>& rules) {
  AIMRT_CHECK_ERROR_THROW(
      state_.load() == State::PreInit,
      "Method can only be called when state is 'PreInit'.");

  clients_backends_rules_ = rules;
}

void RpcBackendManager::SetServersBackendsRules(
    const std::vector<std::pair<std::string, std::vector<std::string>>>& rules) {
  AIMRT_CHECK_ERROR_THROW(
      state_.load() == State::PreInit,
      "Method can only be called when state is 'PreInit'.");

  servers_backends_rules_ = rules;
}

void RpcBackendManager::RegisterRpcBackend(RpcBackendBase* rpc_backend_ptr) {
  AIMRT_CHECK_ERROR_THROW(
      state_.load() == State::PreInit,
      "Method can only be called when state is 'PreInit'.");

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

  std::string_view func_name = service_func_wrapper_ref.func_name;

  auto backend_ptr_vec = GetBackendsByRules(func_name, servers_backends_rules_);

  bool ret = true;
  for (auto& itr : backend_ptr_vec) {
    ret &= itr->RegisterServiceFunc(service_func_wrapper_ref);
  }

  servers_backend_index_map_.emplace(func_name, std::move(backend_ptr_vec));

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

  std::string_view func_name = client_func_wrapper_ref.func_name;

  auto backend_itr = clients_backend_index_map_.find(func_name);
  if (backend_itr == clients_backend_index_map_.end()) {
    auto backend_ptr_vec = GetBackendsByRules(func_name, clients_backends_rules_);
    auto emplace_ret = clients_backend_index_map_.emplace(func_name, std::move(backend_ptr_vec));
    backend_itr = emplace_ret.first;
  }

  bool ret = true;
  for (auto& itr : backend_itr->second) {
    AIMRT_TRACE("Register client func '{}' to backend '{}'.", func_name, itr->Name());
    ret &= itr->RegisterClientFunc(client_func_wrapper_ref);
  }
  return ret;
}

void RpcBackendManager::Invoke(ClientInvokeWrapper&& client_invoke_wrapper) {
  if (state_.load() != State::Start) [[unlikely]] {
    AIMRT_WARN("Method can only be called when state is 'Start'.");
    return;
  }

  auto client_invoke_wrapper_ptr =
      std::make_shared<ClientInvokeWrapper>(std::move(client_invoke_wrapper));

  // 未设置timeout时，默认60s超时
  if (client_invoke_wrapper_ptr->ctx_ref.Timeout().count() == 0) {
    client_invoke_wrapper_ptr->ctx_ref.SetTimeout(std::chrono::seconds(60));
  }

  std::string_view func_name = client_invoke_wrapper_ptr->func_name;

  auto find_itr = clients_backend_index_map_.find(func_name);

  if (find_itr == clients_backend_index_map_.end()) [[unlikely]] {
    AIMRT_ERROR("Rpc call found no backend to handle, func name '{}'.",
                client_invoke_wrapper_ptr->func_name);
    client_invoke_wrapper_ptr->callback(AIMRT_RPC_STATUS_CLI_NO_BACKEND_TO_HANDLE);
    return;
  }

  const auto& backend_ptr_vec = find_itr->second;

  // 如果ctx中指定了后端，则使用指定的后端
  std::string_view to_addr(client_invoke_wrapper_ptr->ctx_ref.GetToAddr());
  if (!to_addr.empty()) {
    // to_addr格式：backend_name://url_str
    AIMRT_TRACE("Rpc call use the specified address '{}', func name '{}'.",
                to_addr, client_invoke_wrapper_ptr->func_name);
    auto pos = to_addr.find("://");
    if (pos != std::string_view::npos) {
      auto addr_backend = to_addr.substr(0, pos);

      auto backend_itr = rpc_backend_index_map_.find(addr_backend);
      if (backend_itr != rpc_backend_index_map_.end()) {
        auto backend_ptr = backend_itr->second;

        if (std::find(backend_ptr_vec.begin(), backend_ptr_vec.end(), backend_ptr) != backend_ptr_vec.end()) {
          if (backend_ptr->TryInvoke(client_invoke_wrapper_ptr)) {
            return;
          }
        }
      }
    }
    AIMRT_ERROR("Rpc call address '{}' is invalid, func name '{}'.", to_addr,
                client_invoke_wrapper_ptr->func_name);
    client_invoke_wrapper_ptr->callback(AIMRT_RPC_STATUS_CLI_INVALID_ADDR);
    return;
  }

  // 根据配置的顺序进行尝试
  for (auto& backend : backend_ptr_vec) {
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

std::vector<RpcBackendBase*> RpcBackendManager::GetBackendsByRules(
    std::string_view func_name,
    const std::vector<std::pair<std::string, std::vector<std::string>>>& rules) {
  for (const auto& item : rules) {
    const auto& func_regex = item.first;
    const auto& enable_backends = item.second;

    try {
      if (std::regex_match(func_name.begin(), func_name.end(), std::regex(func_regex, std::regex::ECMAScript))) {
        std::vector<RpcBackendBase*> backend_ptr_vec;

        for (const auto& backend_name : enable_backends) {
          auto itr = std::find_if(
              rpc_backend_index_vec_.begin(), rpc_backend_index_vec_.end(),
              [&backend_name](const RpcBackendBase* backend_ptr) -> bool {
                return backend_ptr->Name() == backend_name;
              });

          if (itr == rpc_backend_index_vec_.end()) [[unlikely]] {
            AIMRT_WARN("Can not find '{}' in backend list.", backend_name);
            continue;
          }

          backend_ptr_vec.emplace_back(*itr);
        }

        return backend_ptr_vec;
      }
    } catch (const std::exception& e) {
      AIMRT_WARN("Regex get exception, expr: {}, string: {}, exception info: {}",
                 func_regex, func_name, e.what());
    }
  }

  return {};
}

std::unordered_map<std::string_view, std::vector<std::string_view>>
RpcBackendManager::GetClientsBackendInfo() const {
  std::unordered_map<std::string_view, std::vector<std::string_view>> result;
  for (auto& itr : clients_backend_index_map_) {
    std::vector<std::string_view> backends_name;
    for (auto& item : itr.second)
      backends_name.emplace_back(item->Name());

    result.emplace(itr.first, std::move(backends_name));
  }

  return result;
}

std::unordered_map<std::string_view, std::vector<std::string_view>>
RpcBackendManager::GetServersBackendInfo() const {
  std::unordered_map<std::string_view, std::vector<std::string_view>> result;
  for (auto& itr : servers_backend_index_map_) {
    std::vector<std::string_view> backends_name;
    for (auto& item : itr.second)
      backends_name.emplace_back(item->Name());

    result.emplace(itr.first, std::move(backends_name));
  }

  return result;
}

}  // namespace aimrt::runtime::core::rpc