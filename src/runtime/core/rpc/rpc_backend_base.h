#pragma once

#include <memory>
#include <string_view>

#include "aimrt_module_cpp_interface/rpc/rpc_context.h"
#include "aimrt_module_cpp_interface/rpc/rpc_handle.h"
#include "aimrt_module_cpp_interface/util/function.h"
#include "core/rpc/rpc_registry.h"

#include "yaml-cpp/yaml.h"

namespace aimrt::runtime::core::rpc {

struct ClientInvokeWrapper {
  std::string_view func_name;
  std::string_view pkg_path;
  std::string_view module_name;
  aimrt::rpc::ContextRef ctx_ref;
  const void* req_ptr;
  void* rsp_ptr;
  aimrt::rpc::ClientCallback callback;
};

class RpcBackendBase {
 public:
  RpcBackendBase() = default;
  virtual ~RpcBackendBase() = default;

  RpcBackendBase(const RpcBackendBase&) = delete;
  RpcBackendBase& operator=(const RpcBackendBase&) = delete;

  virtual std::string_view Name() const = 0;

  virtual void Initialize(YAML::Node options_node,
                          const RpcRegistry* rpc_registry_ptr) = 0;
  virtual void Start() = 0;
  virtual void Shutdown() = 0;

  virtual bool RegisterServiceFunc(
      const ServiceFuncWrapper& service_func_wrapper) noexcept = 0;
  virtual bool RegisterClientFunc(
      const ClientFuncWrapper& client_func_wrapper) noexcept = 0;
  virtual bool TryInvoke(
      const std::shared_ptr<ClientInvokeWrapper>& client_invoke_wrapper_ptr) noexcept = 0;
};

}  // namespace aimrt::runtime::core::rpc
