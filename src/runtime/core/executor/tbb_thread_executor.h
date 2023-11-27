#pragma once

#include <thread>

#include "core/executor/executor_base.h"
#include "tbb/concurrent_queue.h"

namespace aimrt::runtime::core::executor {

class TBBThreadExecutor : public ExecutorBase {
 public:
  struct Options {
    uint32_t thread_num = 1;
    std::string thread_sched_policy;
    std::vector<uint32_t> thread_bind_cpu;
    std::chrono::microseconds timeout_alarm_threshold_us =
        std::chrono::microseconds(1000 * 1000);
  };

  enum class State : uint32_t {
    PreInit,
    Init,
    Start,
    Shutdown,
  };

 public:
  TBBThreadExecutor() = default;
  ~TBBThreadExecutor() override = default;

  void Initialize(std::string_view name, YAML::Node options_node) override;
  void Start() override;
  void Shutdown() override;

  std::string_view Type() const override { return "tbb_thread"; }
  std::string_view Name() const override { return name_; }

  bool ThreadSafe() const override { return (options_.thread_num == 1); }
  bool IsInCurrentExecutor() const override;
  bool SupportTimerSchedule() const override { return false; }

  void Execute(Task&& task) override;
  void ExecuteAfterNs(uint64_t dt, Task&& task) override;

  State GetState() const { return state_.load(); }

 private:
  std::string name_;
  Options options_;
  std::atomic<State> state_ = State::PreInit;

  tbb::concurrent_queue<Task> qu_;
  std::atomic_bool sig_flag_ = false;

  std::vector<std::thread::id> thread_id_vec_;
  std::list<std::thread> threads_;
};

}  // namespace aimrt::runtime::core::executor
