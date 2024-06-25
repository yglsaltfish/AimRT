#include "core/allocator/allocator_manager.h"

namespace YAML {
template <>
struct convert<aimrt::runtime::core::allocator::AllocatorManager::Options> {
  using Options = aimrt::runtime::core::allocator::AllocatorManager::Options;

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

namespace aimrt::runtime::core::allocator {

void AllocatorManager::Initialize(YAML::Node options_node) {
  AIMRT_CHECK_ERROR_THROW(
      std::atomic_exchange(&state_, State::Init) == State::PreInit,
      "AllocatorManager manager can only be initialized once.");

  if (options_node && !options_node.IsNull())
    options_ = options_node.as<Options>();

  options_node = options_;

  AIMRT_INFO("Allocator manager init complete");
}

void AllocatorManager::Start() {
  AIMRT_CHECK_ERROR_THROW(
      std::atomic_exchange(&state_, State::Start) == State::Init,
      "Function can only be called when state is 'Init'.");

  AIMRT_INFO("Allocator manager start complete.");
}

void AllocatorManager::Shutdown() {
  if (std::atomic_exchange(&state_, State::Shutdown) == State::Shutdown)
    return;

  AIMRT_INFO("Allocator manager shutdown.");
}

const AllocatorProxy& AllocatorManager::GetAllocatorProxy(const util::ModuleDetailInfo& module_info) {
  AIMRT_CHECK_ERROR_THROW(
      state_.load() == State::Init,
      "Function can only be called when state is 'Init'.");

  return allocator_proxy_;
}

std::list<std::pair<std::string, std::string>> AllocatorManager::GenInitializationReport() const {
  AIMRT_CHECK_ERROR_THROW(
      state_.load() == State::Init,
      "Function can only be called when state is 'Init'.");

  return {};
}

}  // namespace aimrt::runtime::core::allocator
