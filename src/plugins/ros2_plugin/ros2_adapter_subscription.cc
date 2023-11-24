#include "ros2_plugin/ros2_adapter_subscription.h"
#include "ros2_plugin/global.h"

namespace aimrt::plugins::ros2_plugin {
std::shared_ptr<void> Ros2AdapterSubscription::create_message() {
  return std::shared_ptr<void>(
      subscribe_wrapper_.msg_type_support->create(),
      [destory_func(subscribe_wrapper_.msg_type_support->destory)](void* ptr) {
        destory_func(ptr);
      });
}

std::shared_ptr<rclcpp::SerializedMessage>
Ros2AdapterSubscription::create_serialized_message() {
  return std::make_shared<rclcpp::SerializedMessage>();
}

void Ros2AdapterSubscription::handle_message(
    std::shared_ptr<void>& message, const rclcpp::MessageInfo& message_info) {
  if (!run_flag.load()) return;

  const rmw_gid_t& gid = message_info.get_rmw_message_info().publisher_gid;
  if (!check_local_publisher_func_(gid)) {
    aimrt::util::Function<aimrt_function_subscriber_release_callback_ops_t>
        release_callback([message]() {});
    // TODO: context
    subscribe_wrapper_.callback(nullptr, message.get(), release_callback.NativeHandle());
  }
}

void Ros2AdapterSubscription::handle_serialized_message(
    const std::shared_ptr<rclcpp::SerializedMessage>& serialized_message,
    const rclcpp::MessageInfo& message_info) {
  AIMRT_WARN("not support ros2 serialized message");
}

void Ros2AdapterSubscription::handle_loaned_message(
    void* loaned_message, const rclcpp::MessageInfo& message_info) {
  AIMRT_WARN("not support ros2 loaned message");
}

void Ros2AdapterSubscription::return_message(std::shared_ptr<void>& message) {
  message.reset();
}

void Ros2AdapterSubscription::return_serialized_message(
    std::shared_ptr<rclcpp::SerializedMessage>& message) {
  message.reset();
}

}  // namespace aimrt::plugins::ros2_plugin