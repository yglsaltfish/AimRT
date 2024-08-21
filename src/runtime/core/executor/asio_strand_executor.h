// Copyright (c) 2023, AgiBot Inc.
// All rights reserved.

#pragma once

#include "core/executor/executor_base.h"
#include "util/log_util.h"

#include "yaml-cpp/yaml.h"

#include <boost/asio.hpp>

namespace aimrt::runtime::core::executor {

class AsioStrandExecutor : public ExecutorBase {
 public:
  struct Options {
    std::string bind_asio_thread_executor_name;
    std::chrono::nanoseconds timeout_alarm_threshold_us = std::chrono::seconds(1);
  };

  enum class State : uint32_t {
    PreInit,
    Init,
    Start,
    Shutdown,
  };

  using GetAsioHandle = std::function<boost::asio::io_context*(std::string_view)>;

 public:
  AsioStrandExecutor()
      : logger_ptr_(std::make_shared<aimrt::common::util::LoggerWrapper>()) {}
  ~AsioStrandExecutor() override = default;

  void Initialize(std::string_view name, YAML::Node options_node) override;
  void Start() override;
  void Shutdown() override;

  std::string_view Type() const override { return "asio_strand"; }
  std::string_view Name() const override { return name_; }

  bool ThreadSafe() const override { return true; }
  bool IsInCurrentExecutor() const override { return false; }
  bool SupportTimerSchedule() const override { return true; }

  void Execute(aimrt::executor::Task&& task) override;

  std::chrono::system_clock::time_point Now() const override {
    return std::chrono::system_clock::now();
  }
  void ExecuteAt(std::chrono::system_clock::time_point tp, aimrt::executor::Task&& task) override;

  void RegisterGetAsioHandle(GetAsioHandle&& handle);

  State GetState() const { return state_.load(); }

  void SetLogger(const std::shared_ptr<aimrt::common::util::LoggerWrapper>& logger_ptr) { logger_ptr_ = logger_ptr; }
  const aimrt::common::util::LoggerWrapper& GetLogger() const { return *logger_ptr_; }

 private:
  std::string name_;
  Options options_;
  std::atomic<State> state_ = State::PreInit;
  std::shared_ptr<aimrt::common::util::LoggerWrapper> logger_ptr_;

  GetAsioHandle get_asio_handle_;

  using Strand = boost::asio::strand<boost::asio::io_context::executor_type>;
  std::unique_ptr<Strand> strand_ptr_;
};

}  // namespace aimrt::runtime::core::executor
