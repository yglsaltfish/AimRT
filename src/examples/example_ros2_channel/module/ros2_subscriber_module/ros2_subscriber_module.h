#pragma once

#include "aimrt_module_cpp_interface/module_base.h"

namespace aimrt::examples::example_ros2_channel::ros2_subscriber_module {

class Ros2SubscriberModule : public aimrt::ModuleBase {
 public:
  Ros2SubscriberModule() = default;
  ~Ros2SubscriberModule() override = default;

  ModuleInfo Info() const noexcept override {
    return ModuleInfo{.name = "Ros2SubscriberModule"};
  }

  bool Initialize(aimrt::CoreRef core) noexcept override;

  bool Start() noexcept override;

  void Shutdown() noexcept override;

 private:
  aimrt::LoggerRef GetLogger() { return core_.GetLogger(); }

 private:
  aimrt::CoreRef core_;

  std::string topic_name_;
  aimrt::channel::SubscriberRef subscriber_;
};

}  // namespace aimrt::examples::example_ros2_channel::ros2_subscriber_module
