
#pragma once

#include "aimrt_module_cpp_interface/co/task.h"
#include "aimrt_module_cpp_interface/module_base.h"

#include "benchmark.pb.h"

namespace aimrt::examples::example_normal_channel::benchmark_subscriber_module {

class BenchmarkSubscriberModule : public aimrt::ModuleBase {
 public:
  BenchmarkSubscriberModule() = default;
  ~BenchmarkSubscriberModule() override = default;

  ModuleInfo Info() const noexcept override {
    return ModuleInfo{.name = "BenchmarkSubscriberModule"};
  }

  bool Initialize(aimrt::CoreRef core) noexcept override;

  bool Start() noexcept override;

  void Shutdown() noexcept override;

 private:
  aimrt::logger::LoggerRef GetLogger() { return core_.GetLogger(); }

  aimrt::co::Task<void> BenchmarkSignalHandle(const aimrt::protocols::example::BenchmarkSignal& data);
  aimrt::co::Task<void> BenchmarkMessageHandle(const aimrt::protocols::example::BenchmarkMessage& data);

 private:
  aimrt::CoreRef core_;

  aimrt::channel::SubscriberRef signal_subscriber_;
  aimrt::channel::SubscriberRef message_subscriber_;

  uint32_t topic_number_ = 1;
  std::string topic_name_prefix_ = "test_topic";
  std::vector<aimrt::channel::SubscriberRef> subscribers_;
};

}  // namespace aimrt::examples::example_normal_channel::benchmark_subscriber_module