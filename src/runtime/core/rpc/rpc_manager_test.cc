#include "core/rpc/rpc_manager.h"
#include "gtest/gtest.h"

namespace aimrt::runtime::core::rpc {

TEST(RpcManagerTest, RpcManagerTest) {
  class MockRpcBackend : public RpcBackendBase {
   public:
    std::string_view Name() const override { return "MockRpcBackend"; }
    void Initialize(YAML::Node options_node, const RpcRegistry* rpc_registry_ptr) override {}
    void Start() override {}
    void Shutdown() override {}

    bool RegisterServiceFunc(const ServiceFuncWrapper& service_func_wrapper) noexcept override { return true; }

    bool RegisterClientFunc(const ClientFuncWrapper& client_func_wrapper) noexcept override { return true; }

    bool TryInvoke(const std::shared_ptr<ClientInvokeWrapper>& client_invoke_wrapper_ptr) noexcept override { return true; }
  };
  std::unique_ptr<MockRpcBackend> rpc_backend_ptr = std::make_unique<MockRpcBackend>();

  RpcManager rpc_manager;

  YAML::Node options_node_test = YAML::Load(R"str(
aimrt:
  rpc:
    backends:
      - type: MockRpcBackend
    clients_options:
      - func_name: "(.*)"
        enable_backends: [MockRpcBackend]
    servers_options:
      - func_name: "(.*)"
        enable_backends: [MockRpcBackend]
)str");

  const util::ModuleDetailInfo module_info = {.name = "module_name"};

  rpc_manager.RegisterRpcBackend(std::move(rpc_backend_ptr));
  rpc_manager.Initialize(options_node_test["aimrt"]["rpc"]);
  EXPECT_EQ(rpc_manager.GetRpcBackendNameList().size(), 1);
  EXPECT_EQ(rpc_manager.GetState(), RpcManager::State::Init);
  EXPECT_NE(rpc_manager.GetRpcHandleProxy(module_info).NativeHandle(), nullptr);

  rpc_manager.Shutdown();
  EXPECT_EQ(rpc_manager.GetState(), RpcManager::State::Shutdown);
}

}  // namespace aimrt::runtime::core::rpc