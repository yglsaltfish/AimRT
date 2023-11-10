
#include "aimrt_core_plugin_interface/aimrt_core_plugin_main.h"
#include "lcm_plugin/lcm_plugin.h"

extern "C" {

aimrt::AimRTCorePluginBase* AimRTDynlibCreateCorePluginHandle() {
  return new aimrt::plugins::lcm_plugin::LcmPlugin();
}

void AimRTDynlibDestroyCorePluginHandle(const aimrt::AimRTCorePluginBase* plugin) {
  delete plugin;
}
}
