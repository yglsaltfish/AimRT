#include "core/rpc/local_rpc_backend.h"
#include "aimrt_module_cpp_interface/rpc/rpc_status.h"
#include "aimrt_module_cpp_interface/util/buffer.h"
#include "aimrt_module_cpp_interface/util/type_support.h"
#include "core/util/thread_tools.h"
#include "util/string_util.h"
#include "util/url_parser.h"

#define TO_AIMRT_BUFFER_ARRAY_VIEW(__aimrt_buffer_array__) \
  (static_cast<const aimrt_buffer_array_view_t*>(          \
      static_cast<const void*>(__aimrt_buffer_array__)))

namespace YAML {
template <>
struct convert<aimrt::runtime::core::rpc::LocalRpcBackend::Options> {
  using Options = aimrt::runtime::core::rpc::LocalRpcBackend::Options;

  static Node encode(const Options& rhs) {
    Node node;

    return node;
  }

  static bool decode(const Node& node, Options& rhs) {
    return true;
  }
};
}  // namespace YAML

namespace aimrt::runtime::core::rpc {

void LocalRpcBackend::Initialize(YAML::Node options_node,
                                 const RpcRegistry* rpc_registry_ptr) {
  AIMRT_CHECK_ERROR_THROW(
      std::atomic_exchange(&state_, State::Init) == State::PreInit,
      "Local rpc backend can only be initialized once.");

  if (options_node && !options_node.IsNull())
    options_ = options_node.as<Options>();

  rpc_registry_ptr_ = rpc_registry_ptr;

  options_node = options_;
}

void LocalRpcBackend::Start() {
  AIMRT_CHECK_ERROR_THROW(
      std::atomic_exchange(&state_, State::Start) == State::Init,
      "Function can only be called when state is 'Init'.");
}

void LocalRpcBackend::Shutdown() {
  if (std::atomic_exchange(&state_, State::Shutdown) == State::Shutdown)
    return;

  service_func_register_index_.clear();
}

bool LocalRpcBackend::RegisterServiceFunc(
    const ServiceFuncWrapper& service_func_wrapper) noexcept {
  if (state_.load() != State::Init) {
    AIMRT_ERROR("Service func can only be registered when state is 'Init'.");
    return false;
  }

  std::string_view pkg_path = service_func_wrapper.pkg_path;
  std::string_view module_name = service_func_wrapper.module_name;
  std::string_view func_name = service_func_wrapper.func_name;

  service_func_register_index_[func_name][pkg_path].emplace(module_name);

  return true;
}

bool LocalRpcBackend::RegisterClientFunc(
    const ClientFuncWrapper& client_func_wrapper) noexcept {
  if (state_.load() != State::Init) {
    AIMRT_ERROR("Client func can only be registered when state is 'Init'.");
    return false;
  }

  return true;
}

bool LocalRpcBackend::TryInvoke(
    const std::shared_ptr<ClientInvokeWrapper>& client_invoke_wrapper_ptr) noexcept {
  assert(state_.load() == State::Start);

  std::string_view pkg_path = client_invoke_wrapper_ptr->pkg_path;
  std::string_view module_name = client_invoke_wrapper_ptr->module_name;
  std::string_view func_name = client_invoke_wrapper_ptr->func_name;

  const auto& service_func_wrapper_map = rpc_registry_ptr_->GetServiceFuncWrapperMap();
  const auto& client_func_wrapper_map = rpc_registry_ptr_->GetClientFuncWrapperMap();

  auto service_func_register_index_find_func_itr = service_func_register_index_.find(func_name);
  if (service_func_register_index_find_func_itr == service_func_register_index_.end()) {
    AIMRT_TRACE("Service func '{}' is not registered in local rpc backend.", func_name);
    return false;
  }

  // 找本地service注册表中符合条件的
  std::string_view service_pkg_path, service_module_name;

  // url: local://rpc/func_name?pkg_path=xxxx&module_name=yyyy
  std::string_view to_addr = client_invoke_wrapper_ptr->ctx_ref.GetToAddr();
  if (!to_addr.empty()) {
    auto url = aimrt::common::util::ParseUrl<std::string_view>(to_addr);
    if (url) {
      assert(url->protocol == Name());
      service_pkg_path = aimrt::common::util::GetValueFromStrKV(url->query, "pkg_path");
      service_module_name = aimrt::common::util::GetValueFromStrKV(url->query, "module_name");
    }
  }

  if (service_pkg_path.empty()) {
    if (service_module_name.empty()) {
      auto service_func_register_index_find_pkg_itr =
          service_func_register_index_find_func_itr->second.begin();

      service_pkg_path = service_func_register_index_find_pkg_itr->first;
      service_module_name = *(service_func_register_index_find_pkg_itr->second.begin());
    } else {
      // pkg未指定，但指定了module。遍历所有pkg，找到第一个符合条件的module
      for (const auto& itr : service_func_register_index_find_func_itr->second) {
        if (itr.second.find(service_module_name) != itr.second.end()) {
          service_pkg_path = itr.first;
          break;
        }
      }
      if (service_pkg_path.empty()) {
        AIMRT_WARN("Can not find service func '{}' in module '{}'. Addr: {}",
                   func_name, service_module_name, to_addr);

        client_invoke_wrapper_ptr->callback(AIMRT_RPC_STATUS_CLI_INVALID_ADDR);

        return true;
      }
    }

  } else {
    auto service_func_register_index_find_pkg_itr =
        service_func_register_index_find_func_itr->second.find(service_pkg_path);

    if (service_func_register_index_find_pkg_itr ==
        service_func_register_index_find_func_itr->second.end()) {
      AIMRT_WARN("Can not find service func '{}' in pkg '{}'. Addr: {}",
                 func_name, service_pkg_path, to_addr);

      client_invoke_wrapper_ptr->callback(AIMRT_RPC_STATUS_CLI_INVALID_ADDR);

      return true;
    }

    if (service_module_name.empty()) {
      service_module_name = *(service_func_register_index_find_pkg_itr->second.begin());
    } else {
      auto service_func_register_index_find_module_itr =
          service_func_register_index_find_pkg_itr->second.find(service_module_name);

      if (service_func_register_index_find_module_itr ==
          service_func_register_index_find_pkg_itr->second.end()) {
        AIMRT_WARN("Can not find service func '{}' in pkg '{}' module '{}'. Addr: {}",
                   func_name, service_pkg_path, service_module_name, to_addr);

        client_invoke_wrapper_ptr->callback(AIMRT_RPC_STATUS_CLI_INVALID_ADDR);

        return true;
      }
    }
  }

  AIMRT_TRACE("Invoke rpc func '{}' in pkg '{}' module '{}'.", func_name,
              service_pkg_path, service_module_name);

  // 找注册的service方法
  auto get_service_func_wrapper_ptr_func = [&]() -> const ServiceFuncWrapper* {
    auto find_pkg_itr = service_func_wrapper_map.find(service_pkg_path);
    if (find_pkg_itr == service_func_wrapper_map.end()) return nullptr;

    auto find_module_itr = find_pkg_itr->second.find(service_module_name);
    if (find_module_itr == find_pkg_itr->second.end()) return nullptr;

    auto find_func_itr = find_module_itr->second.find(func_name);
    if (find_func_itr == find_module_itr->second.end()) return nullptr;

    return find_func_itr->second.get();
  };

  const auto* service_func_wrapper_ptr = get_service_func_wrapper_ptr_func();

  // 在同一个pkg内，直接调用，无需序列化
  if (service_pkg_path == client_invoke_wrapper_ptr->pkg_path) {
    service_func_wrapper_ptr->service_func(
        client_invoke_wrapper_ptr->ctx_ref.NativeHandle(),
        client_invoke_wrapper_ptr->req_ptr, client_invoke_wrapper_ptr->rsp_ptr,
        client_invoke_wrapper_ptr->callback.NativeHandle());
    return true;
  }

  // 不在一个pkg内，需要经过序列化
  std::string serialization_type(
      client_invoke_wrapper_ptr->ctx_ref.GetSerializationType());

  // 找注册的client方法
  auto get_client_func_wrapper_ptr_func = [&]() -> const ClientFuncWrapper* {
    auto find_pkg_itr = client_func_wrapper_map.find(pkg_path);
    if (find_pkg_itr == client_func_wrapper_map.end()) return nullptr;

    auto find_module_itr = find_pkg_itr->second.find(module_name);
    if (find_module_itr == find_pkg_itr->second.end()) return nullptr;

    auto find_func_itr = find_module_itr->second.find(func_name);
    if (find_func_itr == find_module_itr->second.end()) return nullptr;

    return find_func_itr->second.get();
  };
  const auto* client_func_wrapper_ptr = get_client_func_wrapper_ptr_func();

  aimrt::util::BufferArray buffer_array;
  auto client_req_type_support_ref = aimrt::util::TypeSupportRef(client_func_wrapper_ptr->req_type_support);

  // client req序列化
  bool serialize_ret = client_req_type_support_ref.Serialize(
      serialization_type, client_invoke_wrapper_ptr->req_ptr, buffer_array.AllocatorNativeHandle(), buffer_array.BufferArrayNativeHandle());

  if (!serialize_ret) {
    // 序列化失败
    AIMRT_ERROR(
        "Req serialization failed in local rpc backend, serialization_type {}, "
        "pkg_path: {}, module_name: {}, func_name: {}",
        serialization_type, pkg_path, module_name, func_name);

    client_invoke_wrapper_ptr->callback(AIMRT_RPC_STATUS_CLI_SERIALIZATION_FAILDE);

    return true;
  }

  auto service_req_type_support_ref = aimrt::util::TypeSupportRef(service_func_wrapper_ptr->req_type_support);

  // service req反序列化
  std::shared_ptr<void> service_req_ptr = service_req_type_support_ref.CreateSharedPtr();

  bool deserialize_ret = service_req_type_support_ref.Deserialize(
      serialization_type, *TO_AIMRT_BUFFER_ARRAY_VIEW(buffer_array.BufferArrayNativeHandle()), service_req_ptr.get());

  if (!deserialize_ret) {
    // 反序列化失败
    AIMRT_FATAL(
        "Rsp deserialization failed in local rpc backend, serialization_type {}, "
        "pkg_path: {}, module_name: {}, func_name: {}",
        serialization_type, pkg_path, module_name, func_name);

    client_invoke_wrapper_ptr->callback(AIMRT_RPC_STATUS_CLI_DESERIALIZATION_FAILDE);

    return true;
  }

  auto service_rsp_type_support_ref = aimrt::util::TypeSupportRef(service_func_wrapper_ptr->rsp_type_support);

  // service rsp 创建
  std::shared_ptr<void> service_rsp_ptr = service_rsp_type_support_ref.CreateSharedPtr();

  // service rpc调用
  aimrt::util::Function<aimrt_function_service_callback_ops_t> service_callback(
      [this,
       service_func_wrapper_ptr,
       client_func_wrapper_ptr,
       client_invoke_wrapper_ptr,
       service_req_ptr,
       service_rsp_ptr,
       serialization_type{std::move(serialization_type)}](uint32_t code) {
        aimrt::util::BufferArray buffer_array;

        auto service_rsp_type_support_ref = aimrt::util::TypeSupportRef(service_func_wrapper_ptr->rsp_type_support);

        // service rsp 序列化
        bool serialize_ret = service_rsp_type_support_ref.Serialize(
            serialization_type, service_rsp_ptr.get(), buffer_array.AllocatorNativeHandle(), buffer_array.BufferArrayNativeHandle());

        if (!serialize_ret) {
          // 序列化失败
          AIMRT_ERROR(
              "Rsp serialization failed in local rpc backend, serialization_type {}, "
              "pkg_path: {}, module_name: {}, func_name: {}",
              serialization_type, service_func_wrapper_ptr->pkg_path,
              service_func_wrapper_ptr->module_name,
              service_func_wrapper_ptr->func_name);

          client_invoke_wrapper_ptr->callback(AIMRT_RPC_STATUS_SVR_SERIALIZATION_FAILDE);

          return;
        }

        auto client_rsp_type_support_ref = aimrt::util::TypeSupportRef(client_func_wrapper_ptr->rsp_type_support);

        // client rsp 反序列化
        bool deserialize_ret = client_rsp_type_support_ref.Deserialize(
            serialization_type, *TO_AIMRT_BUFFER_ARRAY_VIEW(buffer_array.BufferArrayNativeHandle()), client_invoke_wrapper_ptr->rsp_ptr);

        if (!deserialize_ret) {
          // 反序列化失败
          AIMRT_FATAL(
              "Req deserialization failed in local rpc backend, serialization_type {}, "
              "pkg_path: {}, module_name: {}, func_name: {}",
              serialization_type, service_func_wrapper_ptr->pkg_path,
              service_func_wrapper_ptr->module_name,
              service_func_wrapper_ptr->func_name);

          client_invoke_wrapper_ptr->callback(AIMRT_RPC_STATUS_SVR_DESERIALIZATION_FAILDE);

          return;
        }

        // 调用回调
        client_invoke_wrapper_ptr->callback(code);
      });
  service_func_wrapper_ptr->service_func(
      client_invoke_wrapper_ptr->ctx_ref.NativeHandle(), service_req_ptr.get(),
      service_rsp_ptr.get(), service_callback.NativeHandle());

  return true;
}

}  // namespace aimrt::runtime::core::rpc
