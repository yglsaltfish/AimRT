#include <gtest/gtest.h>
#include <fstream>
#include <iostream>

#include "core/configurator/configurator_manager.h"
#include "core/util/module_detail_info.h"

namespace aimrt::runtime::core::configurator {

const std::filesystem::path configurator_manager_test_path = "./configurator_manager_test_cfg.yaml";

class ConfiguratorManagerTest : public ::testing::Test {
 protected:
  void SetUp() override {
    configurator_manager_.Initialize(configurator_manager_test_path);

    YAML::Node configurator_options_node = configurator_manager_.GetAimRTOptionsNode("configurator");
    EXPECT_EQ(configurator_options_node.IsNull(), false);
    EXPECT_EQ(configurator_options_node.IsDefined(), true);
  }

  void TearDown() override {
    configurator_manager_.Shutdown();
  }

  static void SetUpTestCase() {
    auto cfg_content = R"str(
aimrt:
  configurator:
    temp_cfg_path: ./cfg/tmp # 生成的临时模块配置文件存放路径
  module: # 模块配置
    modules: # 模块
      - name: ConfiguratorManagerTest # 模块Name接口返回的名称
        log_lvl: INFO # 模块日志级别
# 模块自定义配置，框架会为每个模块生成临时配置文件，开发者通过Configurator接口获取该配置文件路径
ConfiguratorManagerTest:
  key1: val1
  key2: val2
)str";

    std::ofstream outfile;
    outfile.open(configurator_manager_test_path, std::ios::out);
    outfile << cfg_content;

    outfile.close();
  }

  static void TearDownTestCase() {
    std::error_code error;
    auto file_status = std::filesystem::status(configurator_manager_test_path, error);

    if (std::filesystem::exists(file_status)) {
      std::remove(configurator_manager_test_path.c_str());
    }
  }

  ConfiguratorManager configurator_manager_;
};

TEST_F(ConfiguratorManagerTest, initialize) {
  YAML::Node configurator_ori_options_node = configurator_manager_.GetOriRootOptionsNode();
  EXPECT_EQ(configurator_ori_options_node.IsNull(), false);
  EXPECT_EQ(configurator_ori_options_node.IsDefined(), true);

  YAML::Node configurator_root_options_node = configurator_manager_.DumpRootOptionsNode();
  EXPECT_EQ(configurator_root_options_node.IsNull(), false);
  EXPECT_EQ(configurator_root_options_node.IsDefined(), true);
}

TEST_F(ConfiguratorManagerTest, start) {
  configurator_manager_.Start();

  configurator_manager_.Shutdown();
}

TEST_F(ConfiguratorManagerTest, get_configuratorProxy_with_legal_module_name) {
  util::ModuleDetailInfo detail_info = {
      .name = "ConfiguratorManagerTest",
      .cfg_file_path = "./cfg/tmp",
  };
  ASSERT_NE(configurator_manager_.GetConfiguratorProxy(detail_info).NativeHandle(), nullptr);
  EXPECT_EQ(configurator_manager_.GetConfiguratorProxy(detail_info).GetConfigFilePath(), "./cfg/tmp");
}

TEST_F(ConfiguratorManagerTest, get_configuratorProxy_with_illegal_module_name) {
  util::ModuleDetailInfo detail_info = {
      .name = "IllegalTest",
  };
  ASSERT_NE(configurator_manager_.GetConfiguratorProxy(detail_info).NativeHandle(), nullptr);
  EXPECT_EQ(configurator_manager_.GetConfiguratorProxy(detail_info).GetConfigFilePath(), "");
}

TEST_F(ConfiguratorManagerTest, get_configuratorProxy_with_configured_module_name) {
  util::ModuleDetailInfo detail_info = {
      .name = "ConfiguratorManagerTest",
  };
  ASSERT_NE(configurator_manager_.GetConfiguratorProxy(detail_info).NativeHandle(), nullptr);
  EXPECT_EQ(configurator_manager_.GetConfiguratorProxy(detail_info).GetConfigFilePath(),
            "./cfg/tmp/temp_cfg_file_for_ConfiguratorManagerTest.yaml");
}

}  // namespace aimrt::runtime::core::configurator