// Copyright (c) 2023, AgiBot Inc.
// All rights reserved

#pragma once

#include <condition_variable>
#include <mutex>
#include <queue>

#include "core/executor/executor_base.h"
#include "util/log_util.h"

namespace aimrt::runtime::core::executor {

class SimpleThreadExecutor : public ExecutorBase {
 public:
  struct Options {
    std::string thread_sched_policy;
    std::vector<uint32_t> thread_bind_cpu;
  };

  enum class State : uint32_t {
    PreInit,
    Init,
    Start,
    Shutdown,
  };

 public:
  SimpleThreadExecutor()
      : logger_ptr_(std::make_shared<aimrt::common::util::LoggerWrapper>()) {}
  ~SimpleThreadExecutor() override = default;

  void Initialize(std::string_view name, YAML::Node options_node) override;
  void Start() override;
  void Shutdown() override;

  std::string_view Type() const override { return "simple_thread"; }
  std::string_view Name() const override { return name_; }

  bool ThreadSafe() const override { return true; }
  bool IsInCurrentExecutor() const override {
    return std::this_thread::get_id() == thread_id_;
  }
  bool SupportTimerSchedule() const override { return false; }

  void Execute(aimrt::executor::Task&& task) override;

  std::chrono::system_clock::time_point Now() const override {
    return std::chrono::system_clock::now();
  }
  void ExecuteAt(std::chrono::system_clock::time_point tp, aimrt::executor::Task&& task) override;

  State GetState() const { return state_.load(); }

  void SetLogger(const std::shared_ptr<aimrt::common::util::LoggerWrapper>& logger_ptr) { logger_ptr_ = logger_ptr; }
  const aimrt::common::util::LoggerWrapper& GetLogger() const { return *logger_ptr_; }

 private:
  std::string name_;
  Options options_;
  std::atomic<State> state_ = State::PreInit;
  std::shared_ptr<aimrt::common::util::LoggerWrapper> logger_ptr_;

  std::thread::id thread_id_;

  std::mutex mutex_;
  std::condition_variable cond_;
  std::queue<aimrt::executor::Task> queue_;
  std::unique_ptr<std::thread> thread_ptr_;
};

}  // namespace aimrt::runtime::core::executor