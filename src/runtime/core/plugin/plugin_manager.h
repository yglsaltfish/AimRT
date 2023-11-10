#pragma once

#include <atomic>
#include <functional>
#include <string>

#include "core/plugin/plugin_loader.h"

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

  using PluginInitFunc = std::function<void(AimRTCorePluginBase*)>;

 public:
  PluginManager() = default;
  ~PluginManager() = default;

  PluginManager(const PluginManager&) = delete;
  PluginManager& operator=(const PluginManager&) = delete;

  void Initialize(YAML::Node options_node);
  void Start();
  void Shutdown();

  void RegisterPluginInitFunc(PluginInitFunc&& plugin_init_func);

  YAML::Node GetPluginOptionsNode(std::string_view plugin_name) const;

 private:
  enum class Status : uint32_t {
    PreInit,
    Init,
    Start,
    Shutdown,
  };

  Options options_;
  std::atomic<Status> status_ = Status::PreInit;

  PluginInitFunc plugin_init_func_;

  std::vector<std::unique_ptr<PluginLoader> > plugin_loader_vec_;
};
}  // namespace aimrt::runtime::core::plugin
