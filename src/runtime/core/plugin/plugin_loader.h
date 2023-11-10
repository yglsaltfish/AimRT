#pragma once

#include <string>

#include "aimrt_core_plugin_interface/aimrt_core_plugin_base.h"
#include "util/dynamic_lib.h"

namespace aimrt::runtime::core::plugin {

class PluginLoader {
 public:
  PluginLoader() = default;
  ~PluginLoader() { UnLoadPlugin(); }

  PluginLoader(const PluginLoader&) = delete;
  PluginLoader& operator=(const PluginLoader&) = delete;

  void LoadPlugin(std::string_view plugin_path);

  void UnLoadPlugin();

  AimRTCorePluginBase* GetPlugin() { return plugin_ptr_; }

 private:
  std::string plugin_path_;
  common::util::DynamicLib dynamic_lib_;

  common::util::DynamicLib::SymbolType destroy_func_ = nullptr;

  AimRTCorePluginBase* plugin_ptr_ = nullptr;
};

}  // namespace aimrt::runtime::core::plugin
