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

  virtual std::string_view Name() const = 0;  // It should always return the same value

  virtual void Initialize(YAML::Node options_node,
                          const RpcRegistry* rpc_registry_ptr) = 0;
  virtual void Start() = 0;
  virtual void Shutdown() = 0;

  /**
   * @brief Register service func
   * @note
   * 1. This method will only be called after 'Initialize' and before 'Start'.
   *
   * @param service_func_wrapper
   * @return Register result
   */
  virtual bool RegisterServiceFunc(
      const ServiceFuncWrapper& service_func_wrapper) noexcept = 0;

  /**
   * @brief Register client func
   * @note
   * 1. This method will only be called after 'Initialize' and before 'Start'.
   *
   * @param client_func_wrapper
   * @return Register result
   */
  virtual bool RegisterClientFunc(
      const ClientFuncWrapper& client_func_wrapper) noexcept = 0;

  /**
   * @brief Try to invoke a rpc call
   * @note
   * 1. This method will only be called after 'Start' and before 'Shutdown'.
   * 2. If this backend decided to handle this rpc call, return true to notify the manager,
   * and return the actual rpc invoke result by the callback.
   *
   * @param client_invoke_wrapper_ptr
   * @return If this backend handle this rpc call
   */
  virtual bool TryInvoke(
      const std::shared_ptr<ClientInvokeWrapper>& client_invoke_wrapper_ptr) noexcept = 0;
};

}  // namespace aimrt::runtime::core::rpc
