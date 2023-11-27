#pragma once

#include <atomic>
#include <memory>
#include <string>
#include <vector>

#include "aimrt_module_cpp_interface/executor/executor.h"
#include "core/channel/channel_backend_base.h"
#include "core/channel/channel_proxy.h"
#include "core/channel/context_manager.h"
#include "core/util/module_detail_info.h"

namespace aimrt::runtime::core::channel {

class ChannelManager {
 public:
  struct Options {
    struct BackendOptions {
      std::string type;
      YAML::Node options;
    };
    std::vector<BackendOptions> backends_options;
  };

  enum class State : uint32_t {
    PreInit,
    Init,
    Start,
    Shutdown,
  };

 public:
  ChannelManager() = default;
  ~ChannelManager() = default;

  ChannelManager(const ChannelManager&) = delete;
  ChannelManager& operator=(const ChannelManager&) = delete;

  void Initialize(YAML::Node options_node);
  void Start();
  void Shutdown();

  void RegisterChannelBackend(
      std::unique_ptr<ChannelBackendBase>&& channel_backend_ptr);

  void RegisterGetExecutorFunc(
      const std::function<executor::ExecutorRef(std::string_view)>& get_executor_func);

  ChannelProxy& GetChannelProxy(const util::ModuleDetailInfo& module_info);

  // 信息查询类接口
  const ChannelRegistry* GetChannelRegistry() const;
  const std::vector<std::string>& GetChannelBackendNameList() const;

  State GetState() const { return state_.load(); }

 private:
  void RegisterLocalChannelBackend();

 private:
  Options options_;
  std::atomic<State> state_ = State::PreInit;

  std::function<executor::ExecutorRef(std::string_view)> get_executor_func_;

  std::unique_ptr<ChannelRegistry> channel_registry_ptr_;

  std::unique_ptr<ContextManager> context_manager_ptr_;

  std::vector<std::unique_ptr<ChannelBackendBase> > channel_backend_vec_;

  ChannelBackendManager channel_backend_manager_;

  std::map<std::string, std::unique_ptr<ChannelProxy> > channel_proxy_map_;

  // 信息查询类变量
  std::vector<std::string> channel_backend_name_vec_;
};

}  // namespace aimrt::runtime::core::channel