#include "ros2_plugin/ros2_channel_backend.h"

#include "aimrt_module_cpp_interface/util/string.h"
#include "aimrt_module_cpp_interface/util/type_support.h"
#include "ros2_plugin/global.h"

#include "rcl/error_handling.h"

namespace YAML {
template <>
struct convert<aimrt::plugins::ros2_plugin::Ros2ChannelBackend::Options> {
  using Options = aimrt::plugins::ros2_plugin::Ros2ChannelBackend::Options;

  static Node encode(const Options& rhs) {
    Node node;

    return node;
  }

  static bool decode(const Node& node, Options& rhs) {
    return true;
  }
};
}  // namespace YAML

namespace aimrt::plugins::ros2_plugin {

void Ros2ChannelBackend::Initialize(
    YAML::Node options_node,
    const runtime::core::channel::ChannelRegistry* channel_registry_ptr,
    runtime::core::channel::ContextManager* context_manager_ptr) {
  AIMRT_CHECK_ERROR_THROW(
      std::atomic_exchange(&state_, State::Init) == State::PreInit,
      "Ros2 channel backend can only be initialized once.");

  if (options_node && !options_node.IsNull())
    options_ = options_node.as<Options>();

  channel_registry_ptr_ = channel_registry_ptr;

  options_node = options_;
}

void Ros2ChannelBackend::Start() {
  AIMRT_CHECK_ERROR_THROW(
      std::atomic_exchange(&state_, State::Start) == State::Init,
      "Function can only be called when state is 'Init'.");

  for (auto& itr : subscribe_wrapper_map_) {
    static_cast<Ros2AdapterSubscription*>(itr.second.get())->Start();
  }
}

void Ros2ChannelBackend::Shutdown() {
  if (std::atomic_exchange(&state_, State::Shutdown) == State::Shutdown)
    return;

  for (auto& itr : publish_type_wrapper_map_) {
    rcl_publisher_t& publisher = itr.second->publisher;
    rcl_ret_t ret =
        rcl_publisher_fini(&publisher, ros2_node_ptr_->get_node_base_interface()
                                           ->get_shared_rcl_node_handle()
                                           .get());
    if (ret != RMW_RET_OK) {
      AIMRT_WARN(
          "Publisher fini failed in ros2 channel backend, type '{}', error info: {}",
          itr.first.msg_type, rcl_get_error_string().str);
      rcl_reset_error();
    }
  }

  for (auto& itr : subscribe_wrapper_map_) {
    static_cast<Ros2AdapterSubscription*>(itr.second.get())->Shutdown();
  }

  ros2_node_ptr_.reset();
}

bool Ros2ChannelBackend::RegisterPublishType(
    const runtime::core::channel::PublishTypeWrapper& publish_type_wrapper) noexcept {
  if (state_.load() != State::Init) {
    AIMRT_ERROR("Publish type can only be registered when state is 'Init'.");
    return false;
  }

  // 只管前缀是ros2类型的消息
  if (!CheckRosMsg(aimrt::util::TypeSupportRef(publish_type_wrapper.msg_type_support).TypeName()))
    return true;

  ChannelTypeKey type_key{
      publish_type_wrapper.pkg_path,
      publish_type_wrapper.module_name,
      publish_type_wrapper.topic_name,
      publish_type_wrapper.msg_type};

  auto emplace_ret = publish_type_wrapper_map_.emplace(
      type_key, std::make_unique<Ros2PublishWrapper>(Ros2PublishWrapper{
                    .type_wrapper = publish_type_wrapper,
                    .publisher = rcl_get_zero_initialized_publisher()}));

  if (!emplace_ret.second) {
    // 重复注册
    AIMRT_WARN(
        "Publish msg type '{}' is registered repeatedly in ros2 channel backend, topic '{}', module '{}', lib path '{}'",
        publish_type_wrapper.msg_type, publish_type_wrapper.topic_name,
        publish_type_wrapper.module_name, publish_type_wrapper.pkg_path);
    return false;
  }

  std::string ros2_topic_name = rclcpp::extend_name_with_sub_namespace(
      std::string(publish_type_wrapper.topic_name),
      ros2_node_ptr_->get_sub_namespace());

  rcl_publisher_t& publisher = emplace_ret.first->second->publisher;
  rcl_publisher_options_t publisher_options =
      rcl_publisher_get_default_options();

  rcl_ret_t ret = rcl_publisher_init(
      &publisher,
      ros2_node_ptr_->get_node_base_interface()
          ->get_shared_rcl_node_handle()
          .get(),
      static_cast<const rosidl_message_type_support_t*>(
          aimrt::util::TypeSupportRef(publish_type_wrapper.msg_type_support).CustomTypeSupportPtr()),
      ros2_topic_name.c_str(), &publisher_options);

  if (ret != RMW_RET_OK) {
    AIMRT_WARN("Ros2 publisher init failed, type '{}', error info: {}",
               publish_type_wrapper.msg_type, rcl_get_error_string().str);
    rcl_reset_error();
    return false;
  }

  rmw_publisher_t* rmw_handle_ptr = rcl_publisher_get_rmw_handle(&publisher);
  if (rmw_handle_ptr == nullptr) {
    AIMRT_WARN(
        "Ros2 publisher get rmw handle failed, type '{}', error info: {}",
        publish_type_wrapper.msg_type, rcl_get_error_string().str);
    rcl_reset_error();
    return false;
  }

  rmw_gid_t rmw_gid;
  ret = rmw_get_gid_for_publisher(rmw_handle_ptr, &rmw_gid);
  if (ret != RMW_RET_OK) {
    AIMRT_WARN("Ros2 publisher get rmw gid failed, type '{}', error info: {}",
               publish_type_wrapper.msg_type, rcl_get_error_string().str);
    rcl_reset_error();
    return false;
  }

  const auto& publisher_gid_ref = *(publisher_gid_vec_.emplace_back(
      std::make_unique<rmw_gid_t>(std::move(rmw_gid))));
  publisher_gid_view_set_.emplace(PublisherGidView{publisher_gid_ref});

  return true;
}

bool Ros2ChannelBackend::Subscribe(
    const runtime::core::channel::SubscribeWrapper& subscribe_wrapper) noexcept {
  if (state_.load() != State::Init) {
    AIMRT_ERROR("Msg can only be subscribed when state is 'Init'.");
    return false;
  }

  // 只管前缀是ros2类型的消息
  if (!CheckRosMsg(aimrt::util::TypeSupportRef(subscribe_wrapper.msg_type_support).TypeName()))
    return true;

  ChannelTypeKey type_key{
      subscribe_wrapper.pkg_path,
      subscribe_wrapper.module_name,
      subscribe_wrapper.topic_name,
      subscribe_wrapper.msg_type};

  if (subscribe_wrapper_map_.find(type_key) != subscribe_wrapper_map_.end()) {
    // 重复注册
    AIMRT_WARN(
        "Msg type '{}' is subscribed repeatedly in ros2 channel backend, topic '{}', module '{}', lib path '{}'",
        subscribe_wrapper.msg_type, subscribe_wrapper.topic_name,
        subscribe_wrapper.module_name, subscribe_wrapper.pkg_path);
    return false;
  }

  std::string ros2_topic_name = rclcpp::extend_name_with_sub_namespace(
      std::string(subscribe_wrapper.topic_name),
      ros2_node_ptr_->get_sub_namespace());
  rclcpp::QoS qos(10);  // todo，配置化

  auto node_topics_interface =
      rclcpp::node_interfaces::get_node_topics_interface(*ros2_node_ptr_);

  rclcpp::SubscriptionFactory factory{
      [&subscribe_wrapper,
       publisher_gid_view_set_ptr{&publisher_gid_view_set_}](
          rclcpp::node_interfaces::NodeBaseInterface* node_base,
          const std::string& topic_name,
          const rclcpp::QoS& qos) -> rclcpp::SubscriptionBase::SharedPtr {
        const rclcpp::SubscriptionOptionsWithAllocator<
            std::allocator<void>>& options =
            rclcpp::SubscriptionOptionsWithAllocator<std::allocator<void>>();

        std::shared_ptr<Ros2AdapterSubscription> subscriber =
            std::make_shared<Ros2AdapterSubscription>(
                node_base,
                *static_cast<const rosidl_message_type_support_t*>(
                    aimrt::util::TypeSupportRef(subscribe_wrapper.msg_type_support).CustomTypeSupportPtr()),
                topic_name,
                // todo: ros2的bug，新版本修复后去掉模板参数
                options.to_rcl_subscription_options<void>(qos),
                subscribe_wrapper,
                [publisher_gid_view_set_ptr](const rmw_gid_t& gid) -> bool {
                  return (publisher_gid_view_set_ptr->find(PublisherGidView{gid}) !=
                          publisher_gid_view_set_ptr->end());
                });
        return std::dynamic_pointer_cast<rclcpp::SubscriptionBase>(subscriber);
      }};

  auto subscriber =
      node_topics_interface->create_subscription(ros2_topic_name, factory, qos);
  node_topics_interface->add_subscription(subscriber, nullptr);

  auto emplace_ret = subscribe_wrapper_map_.emplace(type_key, subscriber);

  return true;
}

void Ros2ChannelBackend::Publish(
    const runtime::core::channel::PublishWrapper& publish_wrapper) noexcept {
  assert(state_.load() == State::Start);

  // 只管前缀是ros2类型的消息
  if (!CheckRosMsg(publish_wrapper.msg_type)) return;

  ChannelTypeKey type_key{
      publish_wrapper.pkg_path,
      publish_wrapper.module_name,
      publish_wrapper.topic_name,
      publish_wrapper.msg_type};

  auto finditr = publish_type_wrapper_map_.find(type_key);
  if (finditr == publish_type_wrapper_map_.end()) {
    AIMRT_WARN(
        "Publish msg type '{}' unregistered in ros2 channel backend, topic '{}', module '{}', lib path '{}'",
        publish_wrapper.msg_type, publish_wrapper.topic_name,
        publish_wrapper.module_name, publish_wrapper.pkg_path);
    return;
  }

  rcl_publisher_t& publisher = finditr->second->publisher;

  rcl_ret_t ret = rcl_publish(&publisher, publish_wrapper.msg_ptr, nullptr);
  if (ret != RMW_RET_OK) {
    AIMRT_WARN(
        "Publish msg type '{}' failed in ros2 channel backend, topic '{}', module '{}', lib path '{}', error info: {}",
        publish_wrapper.msg_type, publish_wrapper.topic_name,
        publish_wrapper.module_name, publish_wrapper.pkg_path,
        rcl_get_error_string().str);
    rcl_reset_error();
  }
}

}  // namespace aimrt::plugins::ros2_plugin