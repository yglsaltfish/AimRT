// Copyright (c) 2023, AgiBot Inc.
// All rights reserved

#include "core/plugin/plugin_manager.h"
#include <gtest/gtest.h>
#include "aimrt_core.h"

namespace aimrt::runtime::core::plugin {

// 测试没有注册AimRT core时会初始化失败
TEST(PluginManagerTest, Initialize1) {
  PluginManager plugin_manager_;
  YAML::Node options_node_test = YAML::Load(R"str(
)str");
  EXPECT_THROW(plugin_manager_.Initialize(options_node_test), aimrt::common::util::AimRTException);
}

// 测试注册AimRT core后初始化成功,且可以通过直接注册容器的方式注册插件并调用该插件的接口 并成功运行
TEST(PluginManagerTest, Initialize2) {
  // 定义一个 MockPlugin
  class MockPlugin : public AimRTCorePluginBase {
   public:
    std::string_view Name() const noexcept override { return "test_plugin"; }
    bool Initialize(runtime::core::AimRTCore* core_ptr) noexcept override {
      is_initialized_ = true;
      return true;
    }
    void Shutdown() noexcept override { is_shutdown_ = true; }

    bool is_initialized_ = false;
    bool is_shutdown_ = false;
  };
  std::shared_ptr<MockPlugin> mock_plugin_ptr_ = std::make_shared<MockPlugin>();
  AimRTCore* core_ptr_ = new AimRTCore;

  PluginManager plugin_manager_;

  YAML::Node options_node_test = YAML::Load(R"str(
plugins: 
    - name: test_plugin 
      path: ""
)str");

  plugin_manager_.RegisterCorePtr(core_ptr_);
  plugin_manager_.RegisterPlugin(mock_plugin_ptr_.get());
  EXPECT_EQ(plugin_manager_.GetState(), PluginManager::State::PreInit);
  plugin_manager_.Initialize(options_node_test);
  EXPECT_EQ(plugin_manager_.GetState(), PluginManager::State::Init);
  EXPECT_TRUE(mock_plugin_ptr_->is_initialized_);

  plugin_manager_.Start();
  EXPECT_EQ(plugin_manager_.GetState(), PluginManager::State::Start);
}

}  // namespace aimrt::runtime::core::plugin
