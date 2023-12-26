#pragma once

#include <string>

#include "aimrt_core_plugin_interface/aimrt_core_plugin_base.h"
#include "util/dynamic_lib.h"
#include "util/log_util.h"

namespace aimrt::runtime::core::plugin {

class PluginLoader {
 public:
  PluginLoader()
      : logger_ptr_(std::make_shared<common::util::LoggerWrapper>()) {}
  ~PluginLoader() { UnLoadPlugin(); }

  PluginLoader(const PluginLoader&) = delete;
  PluginLoader& operator=(const PluginLoader&) = delete;

  void SetLogger(const std::shared_ptr<common::util::LoggerWrapper>& logger_ptr) { logger_ptr_ = logger_ptr; }
  const common::util::LoggerWrapper& GetLogger() const { return *logger_ptr_; }

  void LoadPlugin(std::string_view plugin_path);

  void UnLoadPlugin();

  AimRTCorePluginBase* GetPlugin() { return plugin_ptr_; }

 private:
  std::shared_ptr<common::util::LoggerWrapper> logger_ptr_;

  std::string plugin_path_;
  common::util::DynamicLib dynamic_lib_;

  common::util::DynamicLib::SymbolType destroy_func_ = nullptr;

  AimRTCorePluginBase* plugin_ptr_ = nullptr;
};

}  // namespace aimrt::runtime::core::plugin
