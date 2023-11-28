
#pragma once

#include <memory>
#include <string>

#include "aimrt_core_plugin_interface/aimrt_core_plugin_base.h"

namespace aimrt::plugins::sm_plugin {

class SmPlugin : public AimRTCorePluginBase {
 public:
  struct Options {
  };

 public:
  SmPlugin() = default;
  ~SmPlugin() override = default;

  std::string_view Name() const noexcept override { return "sm_plugin"; }

  bool Initialize(runtime::core::AimRTCore* core_ptr) noexcept override;
  void Shutdown() noexcept override;

 private:
  void SetPluginLogger();
  void RegisterSmRpcBackend();
  void RegisterSmChannelBackend();

 private:
  runtime::core::AimRTCore* core_ptr_ = nullptr;

  Options options_;

  bool init_flag_ = false;
};

}  // namespace aimrt::plugins::sm_plugin