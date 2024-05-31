#include "core/parameter/parameter_manager.h"

namespace YAML {
template <>
struct convert<aimrt::runtime::core::parameter::ParameterManager::Options> {
  using Options = aimrt::runtime::core::parameter::ParameterManager::Options;

  static Node encode(const Options& rhs) {
    Node node;

    return node;
  }

  static bool decode(const Node& node, Options& rhs) {
    if (!node.IsMap()) return false;

    return true;
  }
};
}  // namespace YAML

namespace aimrt::runtime::core::parameter {

void ParameterManager::Initialize(YAML::Node options_node) {
  AIMRT_CHECK_ERROR_THROW(
      std::atomic_exchange(&state_, State::Init) == State::PreInit,
      "ParameterManager manager can only be initialized once.");

  if (options_node && !options_node.IsNull())
    options_ = options_node.as<Options>();

  options_node = options_;

  AIMRT_INFO("Parameter manager init complete");
}

void ParameterManager::Start() {
  AIMRT_CHECK_ERROR_THROW(
      std::atomic_exchange(&state_, State::Start) == State::Init,
      "Function can only be called when state is 'Init'.");

  AIMRT_INFO("Parameter manager start complete.");
}

void ParameterManager::Shutdown() {
  if (std::atomic_exchange(&state_, State::Shutdown) == State::Shutdown)
    return;

  AIMRT_INFO("Parameter manager shutdown.");

  parameter_handle_proxy_wrap_map_.clear();
}

const ParameterHandleProxy& ParameterManager::GetParameterHandleProxy(
    const util::ModuleDetailInfo& module_info) {
  AIMRT_CHECK_ERROR_THROW(
      state_.load() == State::Init,
      "Function can only be called when state is 'Init'.");

  auto itr = parameter_handle_proxy_wrap_map_.find(module_info.name);
  if (itr != parameter_handle_proxy_wrap_map_.end()) return itr->second->parameter_handle_proxy;

  auto ptr = std::make_unique<ParameterHandleProxyWrap>();
  ptr->parameter_handle.SetLogger(logger_ptr_);

  auto emplace_ret = parameter_handle_proxy_wrap_map_.emplace(
      module_info.name,
      std::move(ptr));

  return emplace_ret.first->second->parameter_handle_proxy;
}

ParameterHandle* ParameterManager::GetParameterHandle(std::string_view module_name) const {
  AIMRT_CHECK_ERROR_THROW(
      state_.load() == State::Start,
      "Function can only be called when state is 'Start'.");

  auto itr = parameter_handle_proxy_wrap_map_.find(module_name);
  if (itr != parameter_handle_proxy_wrap_map_.end()) return &(itr->second->parameter_handle);

  return nullptr;
}

std::vector<std::pair<std::string, std::string>>
ParameterManager::GenInitializationReport() const {
  return {};
}

}  // namespace aimrt::runtime::core::parameter