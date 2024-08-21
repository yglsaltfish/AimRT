// Copyright (c) 2023, AgiBot Inc.
// All rights reserved

#pragma once

#include <atomic>
#include <future>

#include "aimrt_module_cpp_interface/module_base.h"

namespace aimrt::examples::cpp::protobuf_channel::benchmark_publisher_module {

class BenchmarkPublisherModule : public aimrt::ModuleBase {
 public:
  BenchmarkPublisherModule() = default;
  ~BenchmarkPublisherModule() override = default;

  ModuleInfo Info() const override {
    return ModuleInfo{.name = "BenchmarkPublisherModule"};
  }

  bool Initialize(aimrt::CoreRef core) override;

  bool Start() override;

  void Shutdown() override;

 private:
  auto GetLogger() { return core_.GetLogger(); }

  void MainLoop();

 private:
  aimrt::CoreRef core_;
  aimrt::executor::ExecutorRef executor_;

  std::atomic_bool run_flag_ = true;
  std::promise<void> stop_sig_;

  aimrt::channel::PublisherRef signal_publisher_;

  uint32_t channel_frq_ = 1;
  uint32_t msg_size_ = 1024;
  uint32_t msg_count_ = 1000;
  uint32_t topic_number_ = 1;
  std::string topic_name_prefix_ = "test_topic";

  std::vector<aimrt::channel::PublisherRef> publishers_;
  std::vector<std::future<void>> futures_;
};

}  // namespace aimrt::examples::cpp::protobuf_channel::benchmark_publisher_module
