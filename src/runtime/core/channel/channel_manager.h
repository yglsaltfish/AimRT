#pragma once

#include <atomic>
#include <memory>
#include <string>
#include <vector>

#include "aimrt_module_cpp_interface/executor/executor.h"
#include "core/channel/channel_backend_base.h"
#include "core/channel/channel_handle_proxy.h"
#include "core/channel/context_manager.h"
#include "core/util/module_detail_info.h"
#include "util/log_util.h"

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
  ChannelManager()
      : logger_ptr_(std::make_shared<common::util::LoggerWrapper>()) {}
  ~ChannelManager() = default;

  ChannelManager(const ChannelManager&) = delete;
  ChannelManager& operator=(const ChannelManager&) = delete;

  void Initialize(YAML::Node options_node);
  void Start();
  void Shutdown();

  void RegisterChannelBackend(
      std::unique_ptr<ChannelBackendBase>&& channel_backend_ptr);

  void RegisterGetExecutorFunc(
      const std::function<aimrt::executor::ExecutorRef(std::string_view)>& get_executor_func);

  ChannelHandleProxy& GetChannelHandleProxy(const util::ModuleDetailInfo& module_info);

  // 信息查询类接口
  const ChannelRegistry* GetChannelRegistry() const;
  const std::vector<std::string>& GetChannelBackendNameList() const;

  State GetState() const { return state_.load(); }

  void SetLogger(const std::shared_ptr<common::util::LoggerWrapper>& logger_ptr) { logger_ptr_ = logger_ptr; }
  const common::util::LoggerWrapper& GetLogger() const { return *logger_ptr_; }

 private:
  void RegisterLocalChannelBackend();

 private:
  Options options_;
  std::atomic<State> state_ = State::PreInit;
  std::shared_ptr<common::util::LoggerWrapper> logger_ptr_;

  std::function<aimrt::executor::ExecutorRef(std::string_view)> get_executor_func_;

  std::unique_ptr<ChannelRegistry> channel_registry_ptr_;

  std::unique_ptr<ContextManager> context_manager_ptr_;

  std::vector<std::unique_ptr<ChannelBackendBase>> channel_backend_vec_;

  ChannelBackendManager channel_backend_manager_;

  class ChannelHandleProxyWrap {
   public:
    ChannelHandleProxyWrap(
        std::string_view input_pkg_path,
        std::string_view input_module_name,
        ChannelBackendManager& channel_backend_manager,
        ContextManager& context_manager)
        : pkg_path(input_pkg_path),
          module_name(input_module_name),
          channel_handle_proxy(
              pkg_path,
              module_name,
              channel_backend_manager,
              context_manager,
              publisher_proxy_map,
              subscriber_proxy_map) {}

    const std::string pkg_path;
    const std::string module_name;

    ChannelHandleProxy::PublisherProxyMap publisher_proxy_map;
    ChannelHandleProxy::SubscriberProxyMap subscriber_proxy_map;
    ChannelHandleProxy channel_handle_proxy;
  };

  std::unordered_map<std::string, std::unique_ptr<ChannelHandleProxyWrap>> channel_handle_proxy_wrap_map_;

  // 信息查询类变量
  std::vector<std::string> channel_backend_name_vec_;
};

}  // namespace aimrt::runtime::core::channel