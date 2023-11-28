
#pragma once

#include <memory>
#include <string>

#include "aimrt_core_plugin_interface/aimrt_core_plugin_base.h"

namespace aimrt::plugins::lcm_plugin {

class LcmPlugin : public AimRTCorePluginBase {
 public:
  struct Options {
  };

 public:
  LcmPlugin() = default;
  ~LcmPlugin() override = default;

  std::string_view Name() const noexcept override { return "lcm_plugin"; }

  bool Initialize(runtime::core::AimRTCore* core_ptr) noexcept override;
  void Shutdown() noexcept override;

 private:
  void SetPluginLogger();
  void RegisterLcmRpcBackend();
  void RegisterLcmChannelBackend();

 private:
  runtime::core::AimRTCore* core_ptr_ = nullptr;

  Options options_;

  bool init_flag_ = false;
};

}  // namespace aimrt::plugins::lcm_plugin
