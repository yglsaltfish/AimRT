#pragma once

#include <memory>
#include <numeric>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "core/channel/channel_backend_base.h"

#include "ros2_plugin/ros2_adapter_subscription.h"

#include "rclcpp/rclcpp.hpp"

#include "rcl/publisher.h"

namespace aimrt::plugins::ros2_plugin {

// todo：暂时只支持cdr序列化
class Ros2ChannelBackend : public runtime::core::channel::ChannelBackendBase {
 public:
  struct Options {
    struct PubTopicOptions {
      std::string topic_name;
      bool enable = false;
    };

    std::vector<PubTopicOptions> pub_topics_options;

    struct SubTopicOptions {
      std::string topic_name;
    };

    std::vector<SubTopicOptions> sub_topics_options;
  };

 public:
  Ros2ChannelBackend() = default;
  ~Ros2ChannelBackend() override = default;

  std::string_view Name() const override { return "ros2"; }

  void Initialize(YAML::Node options_node,
                  const runtime::core::channel::ChannelRegistry* channel_registry_ptr,
                  runtime::core::channel::ContextManager* context_manager_ptr) override;
  void Start() override;
  void Shutdown() override;

  bool RegisterPublishType(
      const runtime::core::channel::PublishTypeWrapper& publish_type_wrapper) noexcept override;
  bool Subscribe(const runtime::core::channel::SubscribeWrapper& subscribe_wrapper) noexcept override;
  void Publish(const runtime::core::channel::PublishWrapper& publish_wrapper) noexcept override;

  void SetNodePtr(const std::shared_ptr<rclcpp::Node>& ros2_node_ptr) {
    ros2_node_ptr_ = ros2_node_ptr;
  }

 private:
  static bool CheckRosMsg(std::string_view msg_type) {
    return (msg_type.substr(0, 5) == "ros2:");
  }

 private:
  enum class State : uint32_t {
    PreInit,
    Init,
    Start,
    Shutdown,
  };

  Options options_;
  std::atomic<State> state_ = State::PreInit;

  const runtime::core::channel::ChannelRegistry* channel_registry_ptr_ = nullptr;
  runtime::core::channel::ContextManager* context_manager_ptr_ = nullptr;

  std::shared_ptr<rclcpp::Node> ros2_node_ptr_;

  struct ChannelTypeKey {
    std::string_view lib_path;
    std::string_view module_name;
    std::string_view topic_name;
    std::string_view msg_type;

    bool operator==(const ChannelTypeKey& val) const {
      return ((lib_path == val.lib_path) &&
              (module_name == val.module_name) &&
              (topic_name == val.topic_name) &&
              (msg_type == val.msg_type));
    }
  };

  struct ChannelTypeKeyHash {
    std::size_t operator()(const ChannelTypeKey& key) const {
      return std::hash<std::string_view>{}(key.lib_path) +
             std::hash<std::string_view>{}(key.module_name) +
             std::hash<std::string_view>{}(key.topic_name) +
             std::hash<std::string_view>{}(key.msg_type);
    }
  };

  struct Ros2PublishWrapper {
    const runtime::core::channel::PublishTypeWrapper& type_wrapper;
    rcl_publisher_t publisher;
  };

  std::unordered_map<
      ChannelTypeKey,
      std::unique_ptr<Ros2PublishWrapper>,
      ChannelTypeKeyHash,
      std::equal_to<>>
      publish_type_wrapper_map_;

  std::unordered_map<
      ChannelTypeKey,
      std::shared_ptr<rclcpp::SubscriptionBase>,
      ChannelTypeKeyHash,
      std::equal_to<>>
      subscribe_wrapper_map_;

  struct PublisherGidView {
    const rmw_gid_t& git;

    bool operator==(const PublisherGidView& val) const {
      int identifier_ret = strcmp(git.implementation_identifier,
                                  val.git.implementation_identifier);
      if (identifier_ret != 0) return false;

      for (size_t ii = 0; ii < RMW_GID_STORAGE_SIZE; ++ii) {
        if (git.data[ii] != val.git.data[ii]) return false;
      }
      return true;
    }
  };

  struct PublisherGidViewHash {
    std::size_t operator()(const PublisherGidView& key) const {
      return std::accumulate(
          std::begin(key.git.data),
          std::end(key.git.data),
          std::hash<std::string_view>{}(key.git.implementation_identifier));
    }
  };

  std::vector<std::unique_ptr<rmw_gid_t>> publisher_gid_vec_;
  std::unordered_set<PublisherGidView, PublisherGidViewHash, std::equal_to<>>
      publisher_gid_view_set_;
};

}  // namespace aimrt::plugins::ros2_plugin