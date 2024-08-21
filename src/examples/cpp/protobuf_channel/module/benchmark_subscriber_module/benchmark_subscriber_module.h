// Copyright (c) 2023, AgiBot Inc.
// All rights reserved.

#pragma once

#include "aimrt_module_cpp_interface/module_base.h"

#include "benchmark.pb.h"

namespace aimrt::examples::cpp::protobuf_channel::benchmark_subscriber_module {

class BenchmarkSubscriberModule : public aimrt::ModuleBase {
 public:
  BenchmarkSubscriberModule() = default;
  ~BenchmarkSubscriberModule() override = default;

  ModuleInfo Info() const override {
    return ModuleInfo{.name = "BenchmarkSubscriberModule"};
  }

  bool Initialize(aimrt::CoreRef core) override;

  bool Start() override;

  void Shutdown() override;

 private:
  auto GetLogger() const { return core_.GetLogger(); }

  void BenchmarkSignalHandle(
      const std::shared_ptr<const aimrt::protocols::example::BenchmarkSignal>& data);
  void BenchmarkMessageHandle(
      uint32_t topic_index,
      const std::shared_ptr<const aimrt::protocols::example::BenchmarkMessage>& data);

  void CheckAndEvaluate() const;

 private:
  aimrt::CoreRef core_;

  aimrt::channel::SubscriberRef signal_subscriber_;

  uint32_t topic_number_ = 1;
  std::string topic_name_prefix_ = "test_topic";
  std::vector<aimrt::channel::SubscriberRef> subscribers_;

  struct TopicRecord {
    std::string topic_name;

    uint32_t expect_send_num = 0;
    uint32_t real_send_num = 0;

    uint32_t message_size = 0;
    uint32_t send_frequency = 0;

    bool is_finished = false;

    struct MsgRecord {
      bool recv = false;
      uint32_t data_size = 0;
      uint64_t send_timestamp = 0;
      uint64_t recv_timestamp = 0;
    };
    std::vector<MsgRecord> msg_record_vec;
  };

  std::vector<TopicRecord> topic_record_vec_;
};

}  // namespace aimrt::examples::cpp::protobuf_channel::benchmark_subscriber_module