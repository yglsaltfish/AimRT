#pragma once

#include "core/allocator/allocator_proxy.h"
#include "core/util/module_detail_info.h"

#include "yaml-cpp/yaml.h"

namespace aimrt::runtime::core::allocator {

class AllocatorManager {
 public:
  struct Options {};

  enum class State : uint32_t {
    PreInit,
    Init,
    Start,
    Shutdown,
  };

 public:
  AllocatorManager() = default;
  ~AllocatorManager() = default;

  AllocatorManager(const AllocatorManager&) = delete;
  AllocatorManager& operator=(const AllocatorManager&) = delete;

  void Initialize(YAML::Node options_node);
  void Start();
  void Shutdown();

  const AllocatorProxy& GetAllocatorProxy(
      const util::ModuleDetailInfo& module_info);

  State GetState() const { return state_.load(); }

 private:
  Options options_;
  std::atomic<State> state_ = State::PreInit;

  AllocatorProxy allocator_proxy_;
};

}  // namespace aimrt::runtime::core::allocator
