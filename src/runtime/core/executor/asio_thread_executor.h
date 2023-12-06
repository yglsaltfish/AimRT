#pragma once

#include <atomic>
#include <list>
#include <memory>
#include <set>
#include <string>
#include <thread>

#include "core/executor/executor_base.h"

#include "yaml-cpp/yaml.h"

#include <boost/asio.hpp>

namespace aimrt::runtime::core::executor {

class AsioThreadExecutor : public ExecutorBase {
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
  AsioThreadExecutor() = default;
  ~AsioThreadExecutor() override = default;

  void Initialize(std::string_view name, YAML::Node options_node) override;
  void Start() override;
  void Shutdown() override;

  std::string_view Type() const override { return "asio_thread"; }
  std::string_view Name() const override { return name_; }

  bool ThreadSafe() const override { return (options_.thread_num == 1); }
  bool IsInCurrentExecutor() const override;
  bool SupportTimerSchedule() const override { return true; }

  void Execute(Task&& task) override;

  std::chrono::steady_clock::time_point Now() const override {
    return std::chrono::steady_clock::now();
  }
  void ExecuteAt(std::chrono::steady_clock::time_point tp, Task&& task) override;

  State GetState() const { return state_.load(); }

  boost::asio::io_context* IOCTX() { return io_ptr_.get(); }

 private:
  std::string name_;
  Options options_;
  std::atomic<State> state_ = State::PreInit;

  std::unique_ptr<boost::asio::io_context> io_ptr_;
  std::unique_ptr<
      boost::asio::executor_work_guard<boost::asio::io_context::executor_type>>
      work_guard_ptr_;

  std::vector<std::thread::id> thread_id_vec_;
  std::list<std::thread> threads_;
};

}  // namespace aimrt::runtime::core::executor
