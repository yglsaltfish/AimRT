#pragma once

#include <atomic>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <thread>

#include "core/executor/executor_base.h"

#include "yaml-cpp/yaml.h"

#include <boost/asio.hpp>

namespace aimrt::runtime::core::executor {

class ThreadExecutor : public ExecutorBase {
 public:
  struct Options {
    uint32_t thread_num = 1;
    std::string thread_sched_policy;
    std::vector<uint32_t> thread_bind_cpu;
    std::chrono::microseconds timeout_alarm_threshold_us =
        std::chrono::microseconds(1000 * 1000);
  };

 public:
  ThreadExecutor() = default;
  ~ThreadExecutor() override = default;

  void Initialize(std::string_view name, YAML::Node options_node) override;
  void Start() override;
  void Shutdown() override;

  std::string_view Type() const override { return "thread"; }
  std::string_view Name() const override { return name_; }

  bool ThreadSafe() const override { return (options_.thread_num == 1); }
  bool IsInCurrentExecutor() const override;

  void Execute(Function<aimrt_function_executor_task_ops_t>&& task) override;
  void ExecuteAfterNs(
      uint64_t dt,
      Function<aimrt_function_executor_task_ops_t>&& task) override;
  void ExecuteAtNs(
      uint64_t tp,
      Function<aimrt_function_executor_task_ops_t>&& task) override;

 private:
  enum class Status : uint32_t {
    PreInit,
    Init,
    Start,
    Shutdown,
  };

  std::string name_;
  Options options_;
  std::atomic<Status> status_ = Status::PreInit;

  std::unique_ptr<boost::asio::io_context> io_ptr_;
  std::unique_ptr<
      boost::asio::executor_work_guard<boost::asio::io_context::executor_type> >
      work_guard_ptr_;

  std::vector<std::thread::id> thread_id_vec_;
  std::list<std::thread> threads_;
};

}  // namespace aimrt::runtime::core::executor
