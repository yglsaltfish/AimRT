#include "core/rpc/rpc_backend_manager.h"
#include "gtest/gtest.h"

namespace aimrt::runtime::core::rpc {

class RpcBackendManagerTest : public ::testing::Test {
 protected:
  void SetUp() override {
    std::vector<std::pair<std::string, std::vector<std::string>>> serviers_backend_rules = {{"(.*)", {"MockRpcBackendTest"}}};
    std::vector<std::pair<std::string, std::vector<std::string>>> clients_backend_rules = {{"(.*)", {"MockRpcBackendTest"}}};

    service_func_wrapper.func_name = "ServiceFuncTest";
    service_func_wrapper.module_name = "ServiceModulecTest";
    service_func_wrapper.pkg_path = "ServicePkgPathTest";
    service_func_wrapper.service_func = ServiceFunc();

    client_func_wrapper.func_name = "ClientFuncTest";
    client_func_wrapper.module_name = "ClientModuleTest";
    client_func_wrapper.pkg_path = "ClientPkgPathTest";

    client_invoke_wrapper.func_name = "ClientFuncTest";
    client_invoke_wrapper.pkg_path = "ClientModuleTest";
    client_invoke_wrapper.module_name = "ClientPkgPathTest";

    client_invoke_wrapper.ctx_ref = aimrt::rpc::ContextRef(ctx_ptr);
    client_invoke_wrapper.req_ptr = nullptr;
    client_invoke_wrapper.rsp_ptr = nullptr;
    client_invoke_wrapper.callback = std::function<void(aimrt::rpc::Status)>();

    rpc_backend_manager_.RegisterRpcBackend(mock_rpc_backend.get());
    rpc_backend_manager_.SetClientsBackendsRules(clients_backend_rules);
    rpc_backend_manager_.SetServersBackendsRules(serviers_backend_rules);
    rpc_backend_manager_.Initialize(rpc_registry.get());
    EXPECT_EQ(rpc_backend_manager_.GetState(), RpcBackendManager::State::Init);
  }
  void TearDown() override {
    EXPECT_EQ(mock_rpc_backend->is_shutdown_, false);
    rpc_backend_manager_.Shutdown();
    EXPECT_EQ(rpc_backend_manager_.GetState(), RpcBackendManager::State::Shutdown);
    EXPECT_EQ(mock_rpc_backend->is_shutdown_, true);
  }

  // 模拟的Rpc后端
  class MockRpcBackend : public RpcBackendBase {
   public:
    std::string_view Name() const override { return "MockRpcBackendTest"; }
    void Initialize(YAML::Node options_node, const RpcRegistry* rpc_registry_ptr) override {}
    void Start() override { is_started_ = true; }
    void Shutdown() override { is_shutdown_ = true; }

    bool RegisterServiceFunc(const ServiceFuncWrapper& service_func_wrapper) noexcept override {
      is_service_func_registered_ = true;
      return true;
    }

    bool RegisterClientFunc(const ClientFuncWrapper& client_func_wrapper) noexcept override {
      is_client_func_registered_ = true;
      return true;
    }

    bool TryInvoke(const std::shared_ptr<ClientInvokeWrapper>& client_invoke_wrapper_ptr) noexcept override {
      is_invoked_ = true;
      return true;
    }

    bool is_started_ = false;
    bool is_shutdown_ = false;
    bool is_invoked_ = false;
    bool is_service_func_registered_ = false;
    bool is_client_func_registered_ = false;
  };

  std::unique_ptr<MockRpcBackend> mock_rpc_backend = std::make_unique<MockRpcBackend>();
  std::unique_ptr<RpcRegistry> rpc_registry = std::make_unique<RpcRegistry>();
  RpcBackendManager rpc_backend_manager_;
  ServiceFuncWrapper service_func_wrapper;
  ClientFuncWrapper client_func_wrapper;
  ClientInvokeWrapper client_invoke_wrapper;
  aimrt::rpc::Context ctx_ptr;
};

// 测试 Start方法
TEST_F(RpcBackendManagerTest, Start) {
  EXPECT_EQ(mock_rpc_backend->is_started_, false);
  rpc_backend_manager_.Start();
  EXPECT_EQ(rpc_backend_manager_.GetState(), RpcBackendManager::State::Start);
  EXPECT_EQ(mock_rpc_backend->is_started_, true);
}

// 测试 RegisterServiceFunc & GetServersBackendInfo
TEST_F(RpcBackendManagerTest, RegisterServiceFunc) {
  EXPECT_EQ(rpc_registry->GetServiceIndexMap().size(), 0);
  EXPECT_EQ(mock_rpc_backend->is_service_func_registered_, false);
  EXPECT_EQ(rpc_backend_manager_.GetServersBackendInfo()["ServiceFuncTest"].size(), 0);
  rpc_backend_manager_.RegisterServiceFunc(std::move(service_func_wrapper));
  EXPECT_EQ(rpc_registry->GetServiceIndexMap().find("ServiceFuncTest")->second.size(), 1);
  EXPECT_EQ(mock_rpc_backend->is_service_func_registered_, true);
  EXPECT_EQ(rpc_backend_manager_.GetServersBackendInfo()["ServiceFuncTest"].size(), 1);
}

// 测试 RegisterClientFunc & GetClientsBackendInfo
TEST_F(RpcBackendManagerTest, RegisterClientFunc) {
  EXPECT_EQ(rpc_registry->GetClientIndexMap().size(), 0);
  EXPECT_EQ(mock_rpc_backend->is_client_func_registered_, false);
  EXPECT_EQ(rpc_backend_manager_.GetClientsBackendInfo()["ClientFuncTest"].size(), 0);
  rpc_backend_manager_.RegisterClientFunc(std::move(client_func_wrapper));
  EXPECT_EQ(rpc_registry->GetClientIndexMap().find("ClientFuncTest")->second.size(), 1);
  EXPECT_EQ(mock_rpc_backend->is_client_func_registered_, true);
  EXPECT_EQ(rpc_backend_manager_.GetClientsBackendInfo()["ClientFuncTest"].size(), 1);
}

// 测试 Invoke不用指定后端地址
TEST_F(RpcBackendManagerTest, InvokeWithoutSpecifiedAddress) {
  rpc_backend_manager_.RegisterClientFunc(std::move(client_func_wrapper));
  rpc_backend_manager_.Start();
  EXPECT_EQ(mock_rpc_backend->is_invoked_, false);
  rpc_backend_manager_.Invoke(std::move(client_invoke_wrapper));
  EXPECT_EQ(mock_rpc_backend->is_invoked_, true);
}

// 测试 Invoke使用指定后端地址
TEST_F(RpcBackendManagerTest, InvokeWithSpecifiedAddress) {
  client_invoke_wrapper.ctx_ref.SetToAddr("MockRpcBackendTest://url_str");
  rpc_backend_manager_.RegisterClientFunc(std::move(client_func_wrapper));
  rpc_backend_manager_.Start();
  EXPECT_EQ(mock_rpc_backend->is_invoked_, false);
  rpc_backend_manager_.Invoke(std::move(client_invoke_wrapper));
  EXPECT_EQ(mock_rpc_backend->is_invoked_, true);
}

}  // namespace aimrt::runtime::core::rpc