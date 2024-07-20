#pragma once

#include <memory>
#include <string_view>
#include <unordered_map>

#include "aimrt_module_cpp_interface/channel/channel_context.h"
#include "aimrt_module_cpp_interface/util/buffer.h"
#include "core/channel/channel_registry.h"

#include "yaml-cpp/yaml.h"

namespace aimrt::runtime::core::channel {

struct PublishWrapper {
  std::string_view msg_type;
  std::string_view pkg_path;
  std::string_view module_name;
  std::string_view topic_name;

  aimrt::channel::ContextRef ctx_ref;
  const void* msg_ptr = nullptr;

  // cache
  mutable const aimrt_type_support_base_t* msg_type_support = nullptr;
  mutable std::unordered_map<std::string_view, std::shared_ptr<aimrt::util::BufferArray>> serialization_cache;
};

class ChannelBackendBase {
 public:
  ChannelBackendBase() = default;
  virtual ~ChannelBackendBase() = default;

  ChannelBackendBase(const ChannelBackendBase&) = delete;
  ChannelBackendBase& operator=(const ChannelBackendBase&) = delete;

  virtual std::string_view Name() const = 0;  // It should always return the same value

  virtual void Initialize(YAML::Node options_node,
                          const ChannelRegistry* channel_registry_ptr) = 0;
  virtual void Start() = 0;
  virtual void Shutdown() = 0;

  /**
   * @brief Register publish type
   * @note
   * 1. This method will only be called after 'Initialize' and before 'Start'.
   *
   * @param publish_type_wrapper
   * @return Register result
   */
  virtual bool RegisterPublishType(
      const PublishTypeWrapper& publish_type_wrapper) noexcept = 0;

  /**
   * @brief Subscribe
   * @note
   * 1. This method will only be called after 'Initialize' and before 'Start'.
   *
   * @param subscribe_wrapper
   * @return Subscribe result
   */
  virtual bool Subscribe(const SubscribeWrapper& subscribe_wrapper) noexcept = 0;

  /**
   * @brief Publish
   * @note
   * 1. This method will only be called after 'Start' and before 'Shutdown'.
   *
   * @param publish_wrapper
   */
  virtual void Publish(const PublishWrapper& publish_wrapper) noexcept = 0;
};

}  // namespace aimrt::runtime::core::channel
