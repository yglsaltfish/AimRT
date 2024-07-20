#include "core/channel/channel_backend_manager.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace aimrt::runtime::core::channel {

class MockChannelBackend : public ChannelBackendBase {
 public:
  std::string_view Name() const override { return "mock_backend"; }

  void Initialize(YAML::Node options_node, const ChannelRegistry* channel_registry_ptr) noexcept override {
    is_initialized_ = true;
  }
  void Start() override { is_statrted_ = true; }

  void Shutdown() override { is_shutdowned_ = false; }

  bool RegisterPublishType(const PublishTypeWrapper& publish_type_wrapper) noexcept override {
    is_registered_publish_type_ = true;
    return true;
  }

  bool Subscribe(const SubscribeWrapper& subscribe_wrapper) noexcept override {
    is_subscribed = true;
    return true;
  }

  void Publish(const PublishWrapper& publish_wrapper) noexcept override {
    is_published = true;
  }

  bool is_initialized_ = false;
  bool is_statrted_ = false;
  bool is_shutdowned_ = false;

  bool is_registered_publish_type_ = false;
  bool is_subscribed = false;
  bool is_published = false;
};

class ChannelBackendManagerTest : public ::testing::Test {
 protected:
  void SetUp() override {
    EXPECT_EQ(channel_backend_manager_.GetState(), ChannelBackendManager::State::PreInit);
    channel_backend_manager_.RegisterChannelBackend(mock_backend_ptr_.get());

    channel_backend_manager_.SetPubTopicsBackendsRules({
        {"(.*)", {"mock_backend"}},
    });
    channel_backend_manager_.SetSubTopicsBackendsRules({
        {"(.*)", {"mock_backend"}},
    });

    channel_backend_manager_.Initialize(channel_registry_test_ptr_.get());
    EXPECT_EQ(channel_backend_manager_.GetState(), ChannelBackendManager::State::Init);
  }

  // 测试用例结束后清理工作 测试Shutdown
  void TearDown() override {
    channel_backend_manager_.Shutdown();
    EXPECT_EQ(channel_backend_manager_.GetState(), ChannelBackendManager::State::Shutdown);
  }

  std::unique_ptr<MockChannelBackend> mock_backend_ptr_ = std::make_unique<MockChannelBackend>();
  std::shared_ptr<ChannelRegistry> channel_registry_test_ptr_ = std::make_shared<ChannelRegistry>();

  ChannelBackendManager channel_backend_manager_;
};

// 测试 RegisterChannelBackend 、Start
TEST_F(ChannelBackendManagerTest, RegisterChannelBackend_Start) {
  EXPECT_EQ(mock_backend_ptr_->is_statrted_, false);
  channel_backend_manager_.Start();
  EXPECT_EQ(mock_backend_ptr_->is_statrted_, true);
}

// 测试 RegisterPublishType
TEST_F(ChannelBackendManagerTest, RegisterPublishType) {
  EXPECT_EQ(mock_backend_ptr_->is_registered_publish_type_, false);
  EXPECT_EQ(channel_registry_test_ptr_->GetPubTopicIndexMap().size(), 0);

  EXPECT_TRUE(channel_backend_manager_.RegisterPublishType(PublishTypeWrapper{
      .msg_type = "publish_msg_type_test",
      .pkg_path = "publish_pkg_path_test",
      .module_name = "publish_module_name_test",
      .topic_name = "publish_topic_name_test",
  }));
  EXPECT_EQ(mock_backend_ptr_->is_registered_publish_type_, true);
  EXPECT_EQ(channel_registry_test_ptr_->GetPubTopicIndexMap().find("publish_topic_name_test")->second.size(), 1);
}

// 测试 Subscribe
TEST_F(ChannelBackendManagerTest, Subscribe) {
  EXPECT_EQ(mock_backend_ptr_->is_subscribed, false);
  EXPECT_EQ(channel_registry_test_ptr_->GetSubTopicIndexMap().size(), 0);

  EXPECT_TRUE(channel_backend_manager_.Subscribe(SubscribeWrapper{
      .msg_type = "subscribe_msg_type_test",
      .pkg_path = "subscribe_pkg_path_test",
      .module_name = "subscribe_module_name_test",
      .topic_name = "subscribe_topic_name_test",
  }));
  EXPECT_EQ(mock_backend_ptr_->is_subscribed, true);
  EXPECT_EQ(channel_registry_test_ptr_->GetSubTopicIndexMap().find("subscribe_topic_name_test")->second.size(), 1);
}

// 测试 Publish
TEST_F(ChannelBackendManagerTest, Publish) {
  EXPECT_EQ(mock_backend_ptr_->is_published, false);

  EXPECT_TRUE(channel_backend_manager_.RegisterPublishType(PublishTypeWrapper{
      .msg_type = "publish_msg_type_test",
      .pkg_path = "publish_pkg_path_test",
      .module_name = "publish_module_name_test",
      .topic_name = "publish_topic_name_test",
  }));

  channel_backend_manager_.Start();

  channel_backend_manager_.Publish(PublishWrapper{
      .msg_type = "publish_msg_type_test",
      .pkg_path = "publish_pkg_path_test",
      .module_name = "publish_module_name_test",
      .topic_name = "publish_topic_name_test",
      .ctx_ref = aimrt::channel::ContextRef(),
  });
  EXPECT_EQ(mock_backend_ptr_->is_published, true);
}

}  // namespace aimrt::runtime::core::channel