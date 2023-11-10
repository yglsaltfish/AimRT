#pragma once

#include <map>
#include <memory>
#include <string_view>

#include "aimrt_module_cpp_interface/channel/channel_context.h"
#include "aimrt_module_cpp_interface/util/buffer.h"
#include "core/channel/channel_registry.h"
#include "core/channel/context_manager.h"

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
  mutable std::map<std::string_view, std::shared_ptr<BufferArray> > serialization_cache;
};

class ChannelBackendBase {
 public:
  ChannelBackendBase() = default;
  virtual ~ChannelBackendBase() = default;

  ChannelBackendBase(const ChannelBackendBase&) = delete;
  ChannelBackendBase& operator=(const ChannelBackendBase&) = delete;

  virtual std::string_view Name() const = 0;

  virtual void Initialize(YAML::Node options_node,
                          const ChannelRegistry* channel_registry_ptr,
                          ContextManager* context_manager_ptr) = 0;
  virtual void Start() = 0;
  virtual void Shutdown() = 0;

  virtual bool RegisterPublishType(
      const PublishTypeWrapper& publish_type_wrapper) noexcept = 0;
  virtual bool Subscribe(const SubscribeWrapper& subscribe_wrapper) noexcept = 0;
  virtual void Publish(const PublishWrapper& publish_wrapper) noexcept = 0;
};

}  // namespace aimrt::runtime::core::channel
