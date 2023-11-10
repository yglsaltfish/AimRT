#pragma once

#include <map>
#include <memory>
#include <string>

#include "core/channel/channel_backend_base.h"

#include "ros2_plugin/ros2_adapter_subscription.h"

#include "rclcpp/rclcpp.hpp"

#include "rcl/publisher.h"

namespace aimrt::plugins::ros2_plugin {

// todo：暂时只支持cdr序列化
class Ros2ChannelBackend : public runtime::core::channel::ChannelBackendBase {
 public:
  struct Options {};

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
  enum class Status : uint32_t {
    PreInit,
    Init,
    Start,
    Shutdown,
  };

  Options options_;
  std::atomic<Status> status_ = Status::PreInit;

  const runtime::core::channel::ChannelRegistry* channel_registry_ptr_ = nullptr;

  std::shared_ptr<rclcpp::Node> ros2_node_ptr_;

  struct ChannelTypeKey {
    ChannelTypeKey(std::string_view input_lib_path,
                   std::string_view input_module_name,
                   std::string_view input_topic_name,
                   std::string_view input_msg_type)
        : lib_path(input_lib_path),
          module_name(input_module_name),
          topic_name(input_topic_name),
          msg_type(input_msg_type) {}

    // 关系运算符重载
    bool operator<(const ChannelTypeKey& val) const {
      if (lib_path < val.lib_path) return true;
      if (lib_path > val.lib_path) return false;

      if (module_name < val.module_name) return true;
      if (module_name > val.module_name) return false;

      if (topic_name < val.topic_name) return true;
      if (topic_name > val.topic_name) return false;

      if (msg_type < val.msg_type) return true;
      return false;
    }

    bool operator>(const ChannelTypeKey& val) const {
      if (lib_path > val.lib_path) return true;
      if (lib_path < val.lib_path) return false;

      if (module_name > val.module_name) return true;
      if (module_name < val.module_name) return false;

      if (topic_name > val.topic_name) return true;
      if (topic_name < val.topic_name) return false;

      if (msg_type > val.msg_type) return true;
      return false;
    }

    bool operator==(const ChannelTypeKey& val) const {
      return ((lib_path == val.lib_path) && (module_name == val.module_name) &&
              (topic_name == val.topic_name) && (msg_type == val.msg_type));
    }

    bool operator<=(const ChannelTypeKey& val) const { return !(*this > val); }
    bool operator>=(const ChannelTypeKey& val) const { return !(*this < val); }
    bool operator!=(const ChannelTypeKey& val) const { return !(*this == val); }

    std::string_view lib_path;
    std::string_view module_name;
    std::string_view topic_name;
    std::string_view msg_type;
  };

  struct Ros2PublishWrapper {
    const runtime::core::channel::PublishTypeWrapper& type_wrapper;
    rcl_publisher_t publisher;
  };

  std::map<ChannelTypeKey, std::unique_ptr<Ros2PublishWrapper> > publish_type_wrapper_map_;

  std::map<ChannelTypeKey, std::shared_ptr<rclcpp::SubscriptionBase> > subscribe_wrapper_map_;

  struct PublisherGidView {
    explicit PublisherGidView(const rmw_gid_t& input_git) : git(input_git) {}

    bool operator<(const PublisherGidView& val) const {
      int identifier_ret = strcmp(git.implementation_identifier,
                                  val.git.implementation_identifier);
      if (identifier_ret != 0) return (identifier_ret < 0);

      for (size_t ii = 0; ii < RMW_GID_STORAGE_SIZE; ++ii) {
        if (git.data[ii] < val.git.data[ii]) return true;
        if (git.data[ii] > val.git.data[ii]) return false;
      }
      return false;
    }

    bool operator>(const PublisherGidView& val) const {
      int identifier_ret = strcmp(git.implementation_identifier,
                                  val.git.implementation_identifier);
      if (identifier_ret != 0) return (identifier_ret > 0);

      for (size_t ii = 0; ii < RMW_GID_STORAGE_SIZE; ++ii) {
        if (git.data[ii] > val.git.data[ii]) return true;
        if (git.data[ii] < val.git.data[ii]) return false;
      }
      return false;
    }

    bool operator==(const PublisherGidView& val) const {
      int identifier_ret = strcmp(git.implementation_identifier,
                                  val.git.implementation_identifier);
      if (identifier_ret != 0) return false;

      for (size_t ii = 0; ii < RMW_GID_STORAGE_SIZE; ++ii) {
        if (git.data[ii] != val.git.data[ii]) return false;
      }
      return true;
    }

    bool operator<=(const PublisherGidView& val) const {
      return !(*this > val);
    }
    bool operator>=(const PublisherGidView& val) const {
      return !(*this < val);
    }
    bool operator!=(const PublisherGidView& val) const {
      return !(*this == val);
    }

    const rmw_gid_t& git;
  };

  std::vector<std::unique_ptr<rmw_gid_t> > publisher_gid_vec_;
  std::set<PublisherGidView> publisher_gid_view_set_;
};

}  // namespace aimrt::plugins::ros2_plugin