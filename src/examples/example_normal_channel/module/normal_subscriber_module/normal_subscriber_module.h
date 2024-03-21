#pragma once

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

  co::Task<void> EventHandle(
      const aimrt::protocols::example::ExampleEventMsg& data);

 private:
  aimrt::CoreRef core_;

  std::string topic_name_ = "test_topic";
  aimrt::channel::SubscriberRef subscriber_;
};

}  // namespace aimrt::examples::example_normal_channel::normal_subscriber_module
