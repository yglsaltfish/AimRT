#include "core/configurator/configurator_manager.h"

#include <cstdlib>
#include <fstream>

#include "util/string_util.h"

namespace YAML {
template <>
struct convert<aimrt::runtime::core::configurator::ConfiguratorManager::Options> {
  using Options = aimrt::runtime::core::configurator::ConfiguratorManager::Options;

  static Node encode(const Options& rhs) {
    Node node;

    node["temp_cfg_path"] = rhs.temp_cfg_path.string();

    return node;
  }

  static bool decode(const Node& node, Options& rhs) {
    if (!node.IsMap()) return false;

    if (node["temp_cfg_path"])
      rhs.temp_cfg_path = node["temp_cfg_path"].as<std::string>();

    return true;
  }
};
}  // namespace YAML

namespace aimrt::runtime::core::configurator {

void ConfiguratorManager::Initialize(
    const std::filesystem::path& cfg_file_path) {
  AIMRT_CHECK_ERROR_THROW(
      std::atomic_exchange(&state_, State::Init) == State::PreInit,
      "Configurator manager can only be initialized once.");

  cfg_file_path_ = cfg_file_path;

  // TODO: yaml-cpp和插件一起使用时无法在结束时正常析构。这里先不析构
  ori_root_options_node_ptr_ = new YAML::Node();
  root_options_node_ptr_ = new YAML::Node();
  auto& ori_root_options_node_ = *ori_root_options_node_ptr_;
  auto& root_options_node_ = *root_options_node_ptr_;

  if (!cfg_file_path_.empty()) {
    std::ifstream file_stream(cfg_file_path_);
    AIMRT_CHECK_ERROR_THROW(file_stream, "Can not open cfg file '{}'.", cfg_file_path_.string());

    std::stringstream file_data;
    file_data << file_stream.rdbuf();
    ori_root_options_node_ = YAML::Load(aimrt::common::util::ReplaceEnvVars(file_data.str()));
  }

  if (!ori_root_options_node_["aimrt"]) {
    ori_root_options_node_["aimrt"] = YAML::Node();
  }

  root_options_node_["aimrt"] = YAML::Node();

  YAML::Node configurator_options_node = GetAimRTOptionsNode("configurator");
  if (configurator_options_node && !configurator_options_node.IsNull())
    options_ = configurator_options_node.as<Options>();

  if (!(std::filesystem::exists(options_.temp_cfg_path) &&
        std::filesystem::is_directory(options_.temp_cfg_path))) {
    std::filesystem::create_directories(options_.temp_cfg_path);
  }

  configurator_options_node = options_;

  AIMRT_INFO("Configurator manager init complete");
}

void ConfiguratorManager::Start() {
  AIMRT_CHECK_ERROR_THROW(
      std::atomic_exchange(&state_, State::Start) == State::Init,
      "Function can only be called when state is 'Init'.");

  AIMRT_INFO("Configurator manager start complete.");
}

void ConfiguratorManager::Shutdown() {
  if (std::atomic_exchange(&state_, State::Shutdown) == State::Shutdown)
    return;

  AIMRT_INFO("Configurator manager shutdown.");

  cfg_proxy_map_.clear();
}

YAML::Node ConfiguratorManager::GetOriRootOptionsNode() const {
  AIMRT_CHECK_ERROR_THROW(
      state_.load() == State::Init,
      "Function can only be called when state is 'Init'.");

  return *ori_root_options_node_ptr_;
}

YAML::Node ConfiguratorManager::DumpRootOptionsNode() const {
  AIMRT_CHECK_ERROR_THROW(
      state_.load() == State::Init,
      "Function can only be called when state is 'Init'.");

  return *root_options_node_ptr_;
}

const ConfiguratorProxy& ConfiguratorManager::GetConfiguratorProxy(
    const util::ModuleDetailInfo& module_info) {
  AIMRT_CHECK_ERROR_THROW(
      state_.load() == State::Init,
      "Function can only be called when state is 'Init'.");

  AIMRT_TRACE("Get configurator proxy for module '{}'.", module_info.name);

  auto itr = cfg_proxy_map_.find(module_info.name);
  if (itr != cfg_proxy_map_.end()) return *(itr->second);

  // 如果直接指定了配置文件路径，则使用指定的
  if (!module_info.cfg_file_path.empty()) {
    auto emplace_ret = cfg_proxy_map_.emplace(
        module_info.name,
        std::make_unique<ConfiguratorProxy>(module_info.cfg_file_path));
    return *(emplace_ret.first->second);
  }

  auto& ori_root_options_node_ = *ori_root_options_node_ptr_;
  auto& root_options_node_ = *root_options_node_ptr_;

  // 如果根配置文件中有这个模块节点，则将内容生成到临时配置文件中
  if (ori_root_options_node_[module_info.name] &&
      !ori_root_options_node_[module_info.name].IsNull()) {
    root_options_node_[module_info.name] =
        ori_root_options_node_[module_info.name];

    std::filesystem::path temp_cfg_file_path =
        options_.temp_cfg_path /
        ("temp_cfg_file_for_" + module_info.name + ".yaml");
    std::ofstream ofs;
    ofs.open(temp_cfg_file_path, std::ios::trunc);
    ofs << root_options_node_[module_info.name];
    ofs.flush();
    ofs.clear();
    ofs.close();

    auto emplace_ret = cfg_proxy_map_.emplace(
        module_info.name,
        std::make_unique<ConfiguratorProxy>(temp_cfg_file_path.string()));
    return *(emplace_ret.first->second);
  }

  return default_cfg_proxy;
}

YAML::Node ConfiguratorManager::GetAimRTOptionsNode(std::string_view key) {
  AIMRT_CHECK_ERROR_THROW(
      state_.load() == State::Init,
      "Function can only be called when state is 'Init'.");

  auto& ori_root_options_node_ = *ori_root_options_node_ptr_;
  auto& root_options_node_ = *root_options_node_ptr_;

  return root_options_node_["aimrt"][key] = ori_root_options_node_["aimrt"][key];
}

std::list<std::pair<std::string, std::string>> ConfiguratorManager::GenInitializationReport() const {
  AIMRT_CHECK_ERROR_THROW(
      state_.load() == State::Init,
      "Function can only be called when state is 'Init'.");

  return {{"AimRT Core Option", YAML::Dump((*root_options_node_ptr_)["aimrt"])}};
}

}  // namespace aimrt::runtime::core::configurator