#pragma once

#include <string>

#include "aimrt_module_c_interface/parameter/parameter_handle_base.h"
#include "core/parameter/parameter_handle.h"
#include "core/parameter/parameter_handle_proxy.h"
#include "core/util/module_detail_info.h"
#include "util/string_util.h"

#include "yaml-cpp/yaml.h"

namespace aimrt::runtime::core::parameter {

class ParameterManager {
 public:
  struct Options {};

  enum class State : uint32_t {
    PreInit,
    Init,
    Start,
    Shutdown,
  };

 public:
  ParameterManager() = default;
  ~ParameterManager() = default;

  ParameterManager(const ParameterManager&) = delete;
  ParameterManager& operator=(const ParameterManager&) = delete;

  void Initialize(YAML::Node options_node);
  void Start();
  void Shutdown();

  const ParameterHandleProxy& GetParameterHandleProxy(
      const util::ModuleDetailInfo& module_info);

  State GetState() const { return state_.load(); }

  ParameterHandle* GetParameterHandle(std::string_view module_name) const;

 private:
  Options options_;
  std::atomic<State> state_ = State::PreInit;

  class ParameterHandleProxyWrap {
   public:
    ParameterHandleProxyWrap()
        : parameter_handle_proxy(parameter_handle) {}

    ParameterHandle parameter_handle;
    ParameterHandleProxy parameter_handle_proxy;
  };

  std::unordered_map<
      std::string,
      std::unique_ptr<ParameterHandleProxyWrap>,
      aimrt::common::util::StringHash,
      std::equal_to<>>
      parameter_handle_proxy_wrap_map_;
};

}  // namespace aimrt::runtime::core::parameter
