#include "core/rpc/rpc_registry.h"
#include "gtest/gtest.h"

namespace aimrt::runtime::core::rpc {

class RpcRegistryTest : public ::testing::Test {
 protected:
  void SetUp() override {
    service_func_wrapper_1_->func_name = "ServiceFuncTest";
    service_func_wrapper_1_->module_name = "ServiceModulecTest1";
    service_func_wrapper_1_->pkg_path = "ServicePkgPathTest1";
    service_func_wrapper_1_->service_func = ServiceFunc();

    service_func_wrapper_2_->func_name = "ServiceFuncTest";
    service_func_wrapper_2_->module_name = "ServiceModulecTest2";
    service_func_wrapper_2_->pkg_path = "ServicePkgPathTest2";
    service_func_wrapper_2_->service_func = ServiceFunc();

    client_func_wrapper_->func_name = "ClientFuncTest";
    client_func_wrapper_->module_name = "ClientModuleTest";
    client_func_wrapper_->pkg_path = "ClientPkgPathTest";
  }

  RpcRegistry rpc_registry_;
  std::unique_ptr<ServiceFuncWrapper> service_func_wrapper_1_ = std::make_unique<ServiceFuncWrapper>();
  std::unique_ptr<ServiceFuncWrapper> service_func_wrapper_2_ = std::make_unique<ServiceFuncWrapper>();
  std::unique_ptr<ClientFuncWrapper> client_func_wrapper_ = std::make_unique<ClientFuncWrapper>();
};

// 测试RegisterServiceFunc
TEST_F(RpcRegistryTest, RegisterServiceFunc) {
  auto& service_func_wrapper_map = rpc_registry_.GetServiceFuncWrapperMap();
  EXPECT_EQ(service_func_wrapper_map.size(), 0);
  rpc_registry_.RegisterServiceFunc(std::move(service_func_wrapper_1_));
  EXPECT_EQ(service_func_wrapper_map.size(), 1);
}

// 测试RegisterClientFunc
TEST_F(RpcRegistryTest, RegisterClientFunc) {
  auto& service_func_wrapper_map = rpc_registry_.GetClientFuncWrapperMap();
  EXPECT_EQ(service_func_wrapper_map.size(), 0);
  rpc_registry_.RegisterClientFunc(std::move(client_func_wrapper_));
  EXPECT_EQ(service_func_wrapper_map.size(), 1);
}

// 测试GetServiceIndexMa
TEST_F(RpcRegistryTest, GetServiceIndexMap) {
  auto& service_index_map = rpc_registry_.GetServiceIndexMap();
  EXPECT_EQ(service_index_map.size(), 0);
  rpc_registry_.RegisterServiceFunc(std::move(service_func_wrapper_1_));
  EXPECT_EQ(service_index_map.find("ServiceFuncTest")->second.size(), 1);
  rpc_registry_.RegisterServiceFunc(std::move(service_func_wrapper_2_));
  EXPECT_EQ(service_index_map.find("ServiceFuncTest")->second.size(), 2);
}

// 测试GetCLientIndexMap
TEST_F(RpcRegistryTest, GetCLientIndexMap) {
  auto& client_index_map = rpc_registry_.GetClientIndexMap();
  EXPECT_EQ(client_index_map.size(), 0);
  rpc_registry_.RegisterClientFunc(std::move(client_func_wrapper_));
  EXPECT_EQ(client_index_map.size(), 1);
}

}  // namespace aimrt::runtime::core::rpc