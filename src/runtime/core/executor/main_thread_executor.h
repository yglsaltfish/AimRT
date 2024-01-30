#pragma once

#include "aimrt_module_c_interface/executor/executor_manager_base.h"
#include "aimrt_module_cpp_interface/util/function.h"
#include "aimrt_module_cpp_interface/util/string.h"
#include "util/log_util.h"

#include "tbb/concurrent_queue.h"
#include "yaml-cpp/yaml.h"

namespace aimrt::runtime::core::executor {

class MainThreadExecutor {
 public:
  struct Options {
    std::string name = "aimrt_main";
    std::string thread_sched_policy;
    std::vector<uint32_t> thread_bind_cpu;
  };

  enum class State : uint32_t {
    PreInit,
    Init,
    Start,
    Shutdown,
  };

  using Task = aimrt::util::Function<aimrt_function_executor_task_ops_t>;

 public:
  MainThreadExecutor()
      : logger_ptr_(std::make_shared<common::util::LoggerWrapper>()),
        thread_id_(std::this_thread::get_id()),
        base_(GenBase(this)) {}
  ~MainThreadExecutor() = default;

  void Initialize(YAML::Node options_node);
  void Start();
  void Shutdown();

  std::string_view Type() const { return "tbb_thread"; }
  std::string_view Name() const { return name_; }

  bool ThreadSafe() const { return true; }
  bool IsInCurrentExecutor() const {
    return std::this_thread::get_id() == thread_id_;
  }
  bool SupportTimerSchedule() const { return false; }

  void Execute(Task&& task);

  State GetState() const { return state_.load(); }

  void SetLogger(const std::shared_ptr<common::util::LoggerWrapper>& logger_ptr) { logger_ptr_ = logger_ptr; }
  const common::util::LoggerWrapper& GetLogger() const { return *logger_ptr_; }

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
        .execute_at_ns = nullptr,
        .impl = impl};
  }

 private:
  Options options_;
  std::atomic<State> state_ = State::PreInit;
  std::shared_ptr<common::util::LoggerWrapper> logger_ptr_;

  std::string name_;
  const std::thread::id thread_id_;

  tbb::concurrent_bounded_queue<Task> qu_;

  const aimrt_executor_base_t base_;
};

}  // namespace aimrt::runtime::core::executor
