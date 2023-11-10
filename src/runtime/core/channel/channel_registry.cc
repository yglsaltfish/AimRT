#include "core/channel/channel_registry.h"
#include "core/global.h"

namespace aimrt::runtime::core::channel {

bool ChannelRegistry::RegisterPublishType(
    std::unique_ptr<PublishTypeWrapper>&& publish_type_wrapper_ptr) {
  std::string_view msg_type = publish_type_wrapper_ptr->msg_type;
  std::string_view pkg_path = publish_type_wrapper_ptr->pkg_path;
  std::string_view module_name = publish_type_wrapper_ptr->module_name;
  std::string_view topic_name = publish_type_wrapper_ptr->topic_name;

  auto emplace_ret =
      publish_type_wrapper_map_[pkg_path][module_name][topic_name].emplace(
          msg_type, std::move(publish_type_wrapper_ptr));

  if (!emplace_ret.second) [[unlikely]] {
    AIMRT_WARN(
        "Publish msg type '{}' is registered repeatedly, topic '{}', module '{}', pkg path '{}'",
        msg_type, topic_name, module_name, pkg_path);
    return false;
  }

  return true;
}

bool ChannelRegistry::Subscribe(
    std::unique_ptr<SubscribeWrapper>&& subscribe_wrapper_ptr) {
  std::string_view msg_type = subscribe_wrapper_ptr->msg_type;
  std::string_view pkg_path = subscribe_wrapper_ptr->pkg_path;
  std::string_view module_name = subscribe_wrapper_ptr->module_name;
  std::string_view topic_name = subscribe_wrapper_ptr->topic_name;

  auto emplace_ret =
      subscribe_wrapper_map_[pkg_path][module_name][topic_name].emplace(
          msg_type, std::move(subscribe_wrapper_ptr));

  if (!emplace_ret.second) [[unlikely]] {
    AIMRT_WARN(
        "Msg type '{}' is subscribed repeatedly, topic '{}', module '{}', pkg path '{}'",
        msg_type, topic_name, module_name, pkg_path);
    return false;
  }

  return true;
}

const PublishTypeWrapper* ChannelRegistry::GetPublishTypeWrapper(
    std::string_view pkg_path,
    std::string_view module_name,
    std::string_view topic_name,
    std::string_view msg_type) const {
  auto find_pkg_itr = publish_type_wrapper_map_.find(pkg_path);
  if (find_pkg_itr == publish_type_wrapper_map_.end()) return nullptr;

  auto find_module_itr = find_pkg_itr->second.find(module_name);
  if (find_module_itr == find_pkg_itr->second.end()) return nullptr;

  auto find_topic_itr = find_module_itr->second.find(topic_name);
  if (find_topic_itr == find_module_itr->second.end()) return nullptr;

  auto find_msg_itr = find_topic_itr->second.find(msg_type);
  if (find_msg_itr == find_topic_itr->second.end()) return nullptr;

  return find_msg_itr->second.get();
}

}  // namespace aimrt::runtime::core::channel
