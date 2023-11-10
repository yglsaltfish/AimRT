#pragma once

#include <atomic>
#include <filesystem>
#include <map>
#include <memory>
#include <string>

#include "yaml-cpp/yaml.h"

#include "core/configurator/configurator_proxy.h"
#include "core/util/module_detail_info.h"

namespace aimrt::runtime::core::configurator {

class ConfiguratorManager {
 public:
  struct Options {
    std::filesystem::path temp_cfg_path = "./cfg/tmp";
  };

 public:
  ConfiguratorManager() = default;
  ~ConfiguratorManager() = default;

  ConfiguratorManager(const ConfiguratorManager&) = delete;
  ConfiguratorManager& operator=(const ConfiguratorManager&) = delete;

  void Initialize(const std::filesystem::path& cfg_file_path);
  void Start();
  void Shutdown();

  YAML::Node GetOriRootOptionsNode() const;
  YAML::Node DumpRootOptionsNode() const;

  const ConfiguratorProxy& GetConfiguratorProxy(
      const util::ModuleDetailInfo& module_info);

  YAML::Node GetAimRTOptionsNode(std::string_view key);

 private:
  enum class Status : uint32_t {
    PreInit,
    Init,
    Start,
    Shutdown,
  };

  std::filesystem::path cfg_file_path_;
  Options options_;
  std::atomic<Status> status_ = Status::PreInit;

  YAML::Node ori_root_options_node_;
  YAML::Node root_options_node_;

  std::map<std::string, std::unique_ptr<ConfiguratorProxy> > cfg_proxy_map_;
};

}  // namespace aimrt::runtime::core::configurator
