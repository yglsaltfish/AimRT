
#include "sm_rpc_backend.h"

namespace YAML {
template <>
struct convert<aimrt::plugins::sm_plugin::SmRpcBackend::Options> {
  using Options = aimrt::plugins::sm_plugin::SmRpcBackend::Options;

  static Node encode(const Options& rhs) {
    Node node;

    return node;
  }

  static bool decode(const Node& node, Options& rhs) {
    return true;
  }
};
}  // namespace YAML

namespace aimrt::plugins::sm_plugin {

void SmRpcBackend::Initialize(YAML::Node options_node,
                              const runtime::core::rpc::RpcRegistry* rpc_registry_ptr,
                              runtime::core::rpc::ContextManager* context_manager_ptr) {
}

void SmRpcBackend::Start() {
}

void SmRpcBackend::Shutdown() {
}

bool SmRpcBackend::RegisterServiceFunc(const runtime::core::rpc::ServiceFuncWrapper& service_func_wrapper) noexcept {
  return true;
}

bool SmRpcBackend::RegisterClientFunc(const runtime::core::rpc::ClientFuncWrapper& client_func_wrapper) noexcept {
  return true;
}

bool SmRpcBackend::TryInvoke(const std::shared_ptr<runtime::core::rpc::ClientInvokeWrapper>& client_invoke_wrapper_ptr) noexcept {
  return true;
}

}  // namespace aimrt::plugins::sm_plugin