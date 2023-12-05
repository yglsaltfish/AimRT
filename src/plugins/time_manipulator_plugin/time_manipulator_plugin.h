#pragma once

#include <chrono>
#include <vector>

#include "aimrt_core_plugin_interface/aimrt_core_plugin_base.h"

namespace aimrt::plugins::time_manipulator_plugin {

class TimeManipulatorPlugin : public AimRTCorePluginBase {
 public:
  struct Options {
    bool enable_rpc = true;
  };

 public:
  TimeManipulatorPlugin() = default;
  ~TimeManipulatorPlugin() override = default;

  std::string_view Name() const noexcept override { return "time_manipulator_plugin"; }

  bool Initialize(runtime::core::AimRTCore* core_ptr) noexcept override;
  void Shutdown() noexcept override;

 private:
  void SetPluginLogger();
  void RegisterTimeManipulatorExecutor();

 private:
  runtime::core::AimRTCore* core_ptr_ = nullptr;

  Options options_;

  bool init_flag_ = false;
};

}  // namespace aimrt::plugins::time_manipulator_plugin
