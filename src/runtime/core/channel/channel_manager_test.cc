#include "channel_manager.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "core/channel/channel_manager.h"

namespace aimrt::runtime::core::channel {

YAML::Node options_node_test = YAML::Load(R"str(
aimrt:
  channel: # 【可选】Channel配置根节点
    backends: # 【可选】Channel后端列表
      - type: mock_backend_test # 【必选】Channel后端类型
    pub_topics_options: # 【可选】Channel Pub Topic配置
      - topic_name: "(.*)" # 【必选】Channel Pub Topic名称，支持正则表达式
        enable_backends: [mock_backend_test] # 【必选】Channel Pub Topic允许使用的Channel后端列表
    sub_topics_options: # 【可选】Channel Sub Topic配置
      - topic_name: "(.*)" # 【必选】Channel Sub Topic名称，支持正则表达式
        enable_backends: [mock_backend_test] # 【必选】Channel Sub Topic允许使用的Channel后端列表
)str");

#include <gmock/gmock.h>

// 模拟的通道后端类，继承自ChannelBackendBase
class MockChannelBackendBase : public ChannelBackendBase {
 public:
  std::string_view Name() const noexcept override { return "mock_backend_test"; }
  MOCK_METHOD2(Initialize, void(YAML::Node options_node, const ChannelRegistry* channel_registry_ptr));
  MOCK_METHOD0(Start, void());
  MOCK_METHOD0(Shutdown, void());
  bool RegisterPublishType(
      const PublishTypeWrapper& publish_type_wrapper) noexcept { return false; }
  bool Subscribe(const SubscribeWrapper& subscribe_wrapper) noexcept { return false; }
  void Publish(const PublishWrapper& publish_wrapper) noexcept { return; }
};

class ChannelManagerTest : public ::testing::Test {
 protected:
  void SetUp() override {
    EXPECT_EQ(channel_manager_.GetState(), ChannelManager::State::PreInit);
    channel_manager_.RegisterChannelBackend(std::move(channel_backend_test_ptr_));

    // 初始化ChannelManager
    channel_manager_.Initialize(options_node_test["aimrt"]["channel"]);
    EXPECT_EQ(channel_manager_.GetState(), ChannelManager::State::Init);
  }
  // 测试用例结束后清理工作
  void TearDown() override {
    channel_manager_.Shutdown();
    EXPECT_EQ(channel_manager_.GetState(), ChannelManager::State::Shutdown);
  }

  ChannelManager channel_manager_;
  std::unique_ptr<MockChannelBackendBase> channel_backend_test_ptr_ = std::make_unique<MockChannelBackendBase>();
};

// 测试Initialize、RegisterChannelBackend、GetChannelBackendNameList
TEST_F(ChannelManagerTest, GetChannelBackendNameList) {
  auto channel_backend_name_test_list = channel_manager_.GetChannelBackendNameList();
  // 验证返回的名称列表是否包含我们模拟的名称
  EXPECT_EQ(channel_backend_name_test_list.size(), 1);
  EXPECT_EQ(channel_backend_name_test_list[0], "mock_backend_test");
}

}  // namespace aimrt::runtime::core::channel
