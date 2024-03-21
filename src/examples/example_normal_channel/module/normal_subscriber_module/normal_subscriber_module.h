#pragma once

#include <atomic>

#include "aimrt_module_cpp_interface/co/async_scope.h"
#include "aimrt_module_cpp_interface/co/task.h"
#include "aimrt_module_cpp_interface/module_base.h"

#include "event.pb.h"

namespace aimrt::examples::example_normal_channel::normal_subscriber_module {

class NormalSubscriberModule : public aimrt::ModuleBase {
 public:
  NormalSubscriberModule() = default;
  ~NormalSubscriberModule() override = default;

  ModuleInfo Info() const noexcept override {
    return ModuleInfo{.name = "NormalSubscriberModule"};
  }

  bool Initialize(aimrt::CoreRef core) noexcept override;

  bool Start() noexcept override;

  void Shutdown() noexcept override;

 private:
  aimrt::logger::LoggerRef GetLogger() { return core_.GetLogger(); }

  co::Task<void> MainLoop();

  co::Task<void> EventHandle(
      const aimrt::protocols::example::ExampleEventMsg& data);

 private:
  aimrt::CoreRef core_;
  aimrt::executor::ExecutorRef executor_;

  co::AsyncScope scope_;
  std::atomic_bool run_flag_ = true;

  std::string publish_topic_name_ = "subscribe_test_topic";
  std::string subscribe_topic_name_ = "publish_test_topic";
  double channel_frq_ = 0.5;
  aimrt::channel::PublisherRef publisher_;
  aimrt::channel::SubscriberRef subscriber_;
};

}  // namespace aimrt::examples::example_normal_channel::normal_subscriber_module
