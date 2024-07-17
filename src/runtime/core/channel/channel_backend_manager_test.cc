#include "core/channel/channel_backend_manager.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace aimrt::runtime::core::channel {
// 模拟的通道后端类，继承自ChannelBackendBase
class MockChannelBackendBase : public ChannelBackendBase {
 public:
  std::string_view Name() const override { return "mock_backend_test"; }
  MOCK_METHOD2(Initialize, void(YAML::Node options_node, const ChannelRegistry* channel_registry_ptr));
  void Start() override { is_statrted_ = true; }
  MOCK_METHOD0(Shutdown, void());

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

  // 用于记录多态调用的状态
  bool is_statrted_ = false;
  bool is_registered_publish_type_ = false;
  bool is_subscribed = false;
  bool is_published = false;
};

class ChannelBackendManagerTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // 定义发布主题后端规则
    std::vector<std::pair<std::string, std::vector<std::string>>> pub_topics_backends_rules_test_ = {
        {"(.*)", {"mock_backend_test"}},
    };
    // 定义订阅主题后端规则
    std::vector<std::pair<std::string, std::vector<std::string>>> sub_topics_backends_rules_test_ = {
        {"(.*)", {"mock_backend_test"}},
    };

    ChannelRegistry* channel_registry_test_ptr_ = new ChannelRegistry();

    // 测试用发布类型包装器，用于初始化
    publish_type_wrapper_test_ptr_->msg_type = "publish_msg_type_test";
    publish_type_wrapper_test_ptr_->pkg_path = "publish_pkg_path_test";
    publish_type_wrapper_test_ptr_->module_name = "publish_module_name_test";
    publish_type_wrapper_test_ptr_->topic_name = "publish_topic_name_test";

    // 测试用订阅包装器，用于初始化
    subscribe_wrapper_test_ptr_->msg_type = "subscribe_msg_type_test";
    subscribe_wrapper_test_ptr_->pkg_path = "subscribe_pkg_path_test";
    subscribe_wrapper_test_ptr_->module_name = "subscribe_module_name_test";
    subscribe_wrapper_test_ptr_->topic_name = "subscribe_topic_name_test";

    // 测试用发布包装器，用于程序运行
    publish_wrapper_test_ptr_->msg_type = "publish_msg_type_test";
    publish_wrapper_test_ptr_->pkg_path = "publish_pkg_path_test";
    publish_wrapper_test_ptr_->module_name = "publish_module_name_test";
    publish_wrapper_test_ptr_->topic_name = "publish_topic_name_test";
    publish_wrapper_test_ptr_->ctx_ref = aimrt::channel::ContextRef();

    // 测试GetState、RegisterChannelBackend、SetPubTopicsBackendsRules、SetSubTopicsBackendsRules、Initialize
    EXPECT_EQ(channel_backend_manager_.GetState(), ChannelBackendManager::State::PreInit);
    channel_backend_manager_.RegisterChannelBackend(mock_backend_test_ptr_);
    channel_backend_manager_.SetPubTopicsBackendsRules(pub_topics_backends_rules_test_);
    channel_backend_manager_.SetSubTopicsBackendsRules(pub_topics_backends_rules_test_);
    channel_backend_manager_.Initialize(channel_registry_test_ptr_);
    EXPECT_EQ(channel_backend_manager_.GetState(), ChannelBackendManager::State::Init);
  }
  // 测试用例结束后清理工作 测试Shutdown
  void TearDown() override {
    channel_backend_manager_.Shutdown();
    EXPECT_EQ(channel_backend_manager_.GetState(), ChannelBackendManager::State::Shutdown);
  }

  ChannelBackendManager channel_backend_manager_;
  MockChannelBackendBase* mock_backend_test_ptr_ = new MockChannelBackendBase();
  PublishTypeWrapper* publish_type_wrapper_test_ptr_ = new PublishTypeWrapper();
  SubscribeWrapper* subscribe_wrapper_test_ptr_ = new SubscribeWrapper();
  PublishWrapper* publish_wrapper_test_ptr_ = new PublishWrapper();
};

// 测试 RegisterChannelBackend 、Start
TEST_F(ChannelBackendManagerTest, RegisterChannelBackend_Start) {
  EXPECT_EQ(mock_backend_test_ptr_->is_statrted_, false);

  channel_backend_manager_.Start();
  EXPECT_EQ(mock_backend_test_ptr_->is_statrted_, true);
}

// 测试 RegisterPublishType
TEST_F(ChannelBackendManagerTest, RegisterPublishType) {
  EXPECT_EQ(mock_backend_test_ptr_->is_registered_publish_type_, false);
  EXPECT_TRUE(channel_backend_manager_.RegisterPublishType(std::move(*publish_type_wrapper_test_ptr_)));
  EXPECT_EQ(mock_backend_test_ptr_->is_registered_publish_type_, true);
}

// 测试 Subscribe
TEST_F(ChannelBackendManagerTest, Subscribe) {
  EXPECT_EQ(mock_backend_test_ptr_->is_subscribed, false);
  EXPECT_TRUE(channel_backend_manager_.Subscribe(std::move(*subscribe_wrapper_test_ptr_)));
  EXPECT_EQ(mock_backend_test_ptr_->is_subscribed, true);
}

// 测试 Publish
TEST_F(ChannelBackendManagerTest, Publish) {
  EXPECT_EQ(mock_backend_test_ptr_->is_published, false);
  EXPECT_TRUE(channel_backend_manager_.RegisterPublishType(std::move(*publish_type_wrapper_test_ptr_)));
  channel_backend_manager_.Publish(*publish_wrapper_test_ptr_);
  EXPECT_EQ(mock_backend_test_ptr_->is_published, true);
}

}  // namespace aimrt::runtime::core::channel