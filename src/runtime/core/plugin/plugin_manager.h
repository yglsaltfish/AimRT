#pragma once

#include <atomic>
#include <functional>
#include <string>

#include "core/plugin/plugin_loader.h"
#include "util/log_util.h"

#include "yaml-cpp/yaml.h"

namespace aimrt::runtime::core::plugin {

class PluginManager {
 public:
  struct Options {
    struct PluginOptions {
      std::string name;
      std::string path;
      YAML::Node options;
    };
    std::vector<PluginOptions> plugins_options;
  };

  enum class State : uint32_t {
    PreInit,
    Init,
    Start,
    Shutdown,
  };

  using PluginInitFunc = std::function<bool(AimRTCorePluginBase*)>;

 public:
  PluginManager()
      : logger_ptr_(std::make_shared<aimrt::common::util::LoggerWrapper>()) {}
  ~PluginManager() = default;

  PluginManager(const PluginManager&) = delete;
  PluginManager& operator=(const PluginManager&) = delete;

  void Initialize(YAML::Node options_node);
  void Start();
  void Shutdown();

  void RegisterPlugin(AimRTCorePluginBase* plugin);

  void RegisterPluginInitFunc(PluginInitFunc&& plugin_init_func);

  YAML::Node GetPluginOptionsNode(std::string_view plugin_name) const;

  State GetState() const { return state_.load(); }

  std::list<std::pair<std::string, std::string>> GenInitializationReport() const;

  void SetLogger(const std::shared_ptr<aimrt::common::util::LoggerWrapper>& logger_ptr) { logger_ptr_ = logger_ptr; }
  const aimrt::common::util::LoggerWrapper& GetLogger() const { return *logger_ptr_; }

 private:
  Options options_;
  std::atomic<State> state_ = State::PreInit;
  std::shared_ptr<aimrt::common::util::LoggerWrapper> logger_ptr_;

  PluginInitFunc plugin_init_func_;

  // 直接注册的插件
  std::vector<AimRTCorePluginBase*> registered_plugin_vec_;

  // 通过动态库加载的插件
  std::vector<std::unique_ptr<PluginLoader>> plugin_loader_vec_;
};
}  // namespace aimrt::runtime::core::plugin
