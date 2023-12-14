#pragma once

#include <atomic>

#include "aimrt_module_cpp_interface/co/async_scope.h"
#include "aimrt_module_cpp_interface/co/task.h"
#include "aimrt_module_cpp_interface/module_base.h"

namespace aimrt::examples::example_ros2_channel::ros2_publisher_module {

class Ros2PublisherModule : public aimrt::ModuleBase {
 public:
  Ros2PublisherModule() = default;
  ~Ros2PublisherModule() override = default;

  ModuleInfo Info() const noexcept override {
    return ModuleInfo{.name = "Ros2PublisherModule"};
  }

  bool Initialize(aimrt::CoreRef core) noexcept override;

  bool Start() noexcept override;

  void Shutdown() noexcept override;

 private:
  aimrt::logger::LoggerRef GetLogger() { return core_.GetLogger(); }

  co::Task<void> MainLoop();

 private:
  aimrt::CoreRef core_;
  aimrt::executor::ExecutorRef executor_;

  co::AsyncScope scope_;
  std::atomic_bool run_flag_ = true;

  std::string topic_name_;
  double channel_frq_;
  aimrt::channel::PublisherRef publisher_;
};

}  // namespace aimrt::examples::example_ros2_channel::ros2_publisher_module
