#pragma once

#include "aimrt_module_c_interface/executor/executor_manager_base.h"
#include "aimrt_module_cpp_interface/util/function.h"
#include "aimrt_module_cpp_interface/util/string.h"

#include "yaml-cpp/yaml.h"

#include <boost/asio.hpp>

namespace aimrt::runtime::core::executor {

class MainThreadExecutor {
 public:
  struct Options {
    std::string thread_sched_policy;
    std::vector<uint32_t> thread_bind_cpu;
  };

  using Task = aimrt::util::Function<aimrt_function_executor_task_ops_t>;
  using SignalHandle = std::function<void(boost::system::error_code, int)>;

 public:
  MainThreadExecutor()
      : thread_id_(std::this_thread::get_id()),
        base_(GenBase(this)) {}
  ~MainThreadExecutor() = default;

  void Initialize(YAML::Node options_node);
  void Start();
  void Shutdown();

  void RegisterSignalHandle(const std::set<int>& signals,
                            SignalHandle&& signal_handle);

  std::string_view Type() const { return "asio_thread"; }
  std::string_view Name() const { return "main_thread"; }

  bool ThreadSafe() const { return true; }
  bool IsInCurrentExecutor() const {
    return std::this_thread::get_id() == thread_id_;
  }
  bool SupportTimerSchedule() const { return false; }

  void Execute(Task&& task);

  const aimrt_executor_base_t* NativeHandle() const { return &base_; }

 private:
  static aimrt_executor_base_t GenBase(void* impl) {
    return aimrt_executor_base_t{
        .type = [](void* impl) -> aimrt_string_view_t {
          return aimrt::util::ToAimRTStringView(
              static_cast<MainThreadExecutor*>(impl)->Type());
        },
        .name = [](void* impl) -> aimrt_string_view_t {
          return aimrt::util::ToAimRTStringView(
              static_cast<MainThreadExecutor*>(impl)->Name());
        },
        .is_thread_safe = [](void* impl) -> bool {
          return static_cast<MainThreadExecutor*>(impl)->ThreadSafe();
        },
        .is_in_current_executor = [](void* impl) -> bool {
          return static_cast<MainThreadExecutor*>(impl)->IsInCurrentExecutor();
        },
        .is_support_timer_schedule = [](void* impl) -> bool {
          return static_cast<MainThreadExecutor*>(impl)->SupportTimerSchedule();
        },
        .execute = [](void* impl, aimrt_function_base_t* task) {
          static_cast<MainThreadExecutor*>(impl)->Execute(Task(task));  //
        },
        .execute_after_ns = nullptr,
        .impl = impl};
  }

 private:
  enum class Status : uint32_t {
    PreInit,
    Init,
    Start,
    Shutdown,
  };

  Options options_;
  std::atomic<Status> status_ = Status::PreInit;

  const std::thread::id thread_id_;

  std::unique_ptr<boost::asio::io_context> io_ptr_;

  std::unique_ptr<boost::asio::signal_set> sig_ptr_;
  std::vector<std::pair<std::set<int>, SignalHandle> > signal_handle_vec_;

  const aimrt_executor_base_t base_;
};

}  // namespace aimrt::runtime::core::executor
