// Copyright (c) 2023, AgiBot Inc.
// All rights reserved.

#include <gtest/gtest.h>
#include "aimrt_module_cpp_interface/module_base.h"
#include "core/module/module_manager.h"

#define TESTMODULE "TestModule"

namespace aimrt::runtime::core::module {

TEST(ModuleManagerTest, ModuleManagerTest) {
  // 模拟一个模块继承模块基
  class MockModule : public aimrt::ModuleBase {
   public:
    ModuleInfo Info() const override {
      ModuleInfo info;
      info.name = TESTMODULE;
      return info;
    }

    bool Initialize(CoreRef core) override {
      is_initialized_ = true;
      return true;
    }

    bool Start() override {
      is_start_ = true;
      return true;
    }

    void Shutdown() override {
      is_shutdown_ = true;
    }

    bool is_initialized_ = false;
    bool is_start_ = false;
    bool is_shutdown_ = false;
  };
  std::unique_ptr<MockModule> module_test_ptr = std::make_unique<MockModule>();

  ModuleManager module_manager_;

  // 测试 Initialize 正常初始化操作
  YAML::Node options_node_test = YAML::Load(R"str(
)str");
  ModuleManager::CoreProxyConfigurator module_proxy_configurator = [](const util::ModuleDetailInfo& info, CoreProxy& proxy) {};
  module_manager_.RegisterCoreProxyConfigurator(std::move(module_proxy_configurator));
  EXPECT_EQ(module_manager_.GetState(), ModuleManager::State::PreInit);
  module_manager_.RegisterModule(module_test_ptr->NativeHandle());
  module_manager_.Initialize(options_node_test);
  EXPECT_EQ(module_manager_.GetState(), ModuleManager::State::Init);
  EXPECT_EQ(module_test_ptr->is_initialized_, true);
  EXPECT_EQ(module_manager_.GetModuleNameList().size(), 1);
  EXPECT_EQ(module_manager_.GetModuleDetailInfoList()[0]->name, TESTMODULE);

  // 测试Start正常启动操作
  EXPECT_EQ(module_test_ptr->is_start_, false);
  module_manager_.Start();
  EXPECT_EQ(module_manager_.GetState(), ModuleManager::State::Start);
  EXPECT_EQ(module_test_ptr->is_start_, true);

  // 测试Shutdown正常关闭操作
  EXPECT_EQ(module_test_ptr->is_shutdown_, false);
  module_manager_.Shutdown();
  EXPECT_EQ(module_manager_.GetState(), ModuleManager::State::Shutdown);
  EXPECT_EQ(module_test_ptr->is_shutdown_, true);
}

}  // namespace aimrt::runtime::core::module
