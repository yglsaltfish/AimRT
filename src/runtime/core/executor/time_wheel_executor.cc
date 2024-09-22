// Copyright (c) 2023, AgiBot Inc.
// All rights reserved.

#include "core/executor/time_wheel_executor.h"
#include "core/util/thread_tools.h"
#include "util/time_util.h"

namespace YAML {
template <>
struct convert<aimrt::runtime::core::executor::TimeWheelExecutor::Options> {
  using Options = aimrt::runtime::core::executor::TimeWheelExecutor::Options;

  static Node encode(const Options& rhs) {
    Node node;

    node["bind_executor"] = rhs.bind_executor;
    node["dt_us"] = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(rhs.dt).count());
    node["wheel_size"] = rhs.wheel_size;
    node["thread_sched_policy"] = rhs.thread_sched_policy;
    node["thread_bind_cpu"] = rhs.thread_bind_cpu;

    return node;
  }

  static bool decode(const Node& node, Options& rhs) {
    if (!node.IsMap()) return false;

    if (node["bind_executor"])
      rhs.bind_executor = node["bind_executor"].as<std::string>();
    if (node["dt_us"])
      rhs.dt = std::chrono::microseconds(node["dt_us"].as<uint64_t>());
    if (node["wheel_size"])
      rhs.wheel_size = node["wheel_size"].as<std::vector<size_t>>();
    if (node["thread_sched_policy"])
      rhs.thread_sched_policy = node["thread_sched_policy"].as<std::string>();
    if (node["thread_bind_cpu"])
      rhs.thread_bind_cpu = node["thread_bind_cpu"].as<std::vector<uint32_t>>();

    return true;
  }
};
}  // namespace YAML

namespace aimrt::runtime::core::executor {

void TimeWheelExecutor::Initialize(std::string_view name,
                                   YAML::Node options_node) {
  AIMRT_CHECK_ERROR_THROW(
      get_executor_func_,
      "Get executor function is not set before initialize.");

  AIMRT_CHECK_ERROR_THROW(
      std::atomic_exchange(&state_, State::kInit) == State::kPreInit,
      "TimeWheelExecutor can only be initialized once.");

  name_ = std::string(name);

  if (options_node && !options_node.IsNull())
    options_ = options_node.as<Options>();

  if (!options_.bind_executor.empty()) {
    bind_executor_ref_ = get_executor_func_(options_.bind_executor);

    AIMRT_CHECK_ERROR_THROW(
        bind_executor_ref_,
        "Can not get executor {}.", options_.bind_executor);

    AIMRT_CHECK_ERROR_THROW(
        bind_executor_ref_.Name() != Name(),
        "Bind executor '{}' is self!", options_.bind_executor);

    thread_safe_ = bind_executor_ref_.ThreadSafe();
  }

  dt_count_ = static_cast<uint64_t>(
      std::chrono::duration_cast<std::chrono::nanoseconds>(options_.dt).count());

  uint64_t cur_scale = 1;
  for (size_t ii = 0; ii < options_.wheel_size.size(); ++ii) {
    uint64_t start_pos = (ii == 0) ? 0 : 1;
    timing_wheel_vec_.emplace_back(TimingWheelTool{
        .current_pos = start_pos,
        .scale = (cur_scale *= options_.wheel_size[ii]),
        .wheel = std::vector<TaskList>(options_.wheel_size[ii]),
        .borrow_func = [ii, this]() {
          TaskList task_list;
          if (ii < options_.wheel_size.size() - 1) {
            task_list = timing_wheel_vec_[ii + 1].Tick();
          } else {
            auto itr = timing_task_map_.find(timing_task_map_pos_);
            ++timing_task_map_pos_;
            if (itr != timing_task_map_.end()) {
              task_list = std::move(itr->second);
              timing_task_map_.erase(itr);
            }
          }

          while (!task_list.empty()) {
            auto itr = task_list.begin();
            auto& cur_list = timing_wheel_vec_[ii].wheel[itr->tick_count % timing_wheel_vec_[ii].scale];
            cur_list.splice(cur_list.end(), task_list, itr);
          }
        }});
  }

  timing_task_map_pos_ = 1;

  options_node = options_;
}

void TimeWheelExecutor::Start() {
  AIMRT_CHECK_ERROR_THROW(
      std::atomic_exchange(&state_, State::kStart) == State::kInit,
      "Method can only be called when state is 'Init'.");

  timer_thread_ptr_ = std::make_unique<std::thread>(std::bind(&TimeWheelExecutor::TimerLoop, this));

  start_flag_.wait(false);
}

void TimeWheelExecutor::Shutdown() {
  if (std::atomic_exchange(&state_, State::kShutdown) == State::kShutdown)
    return;

  if (timer_thread_ptr_ && timer_thread_ptr_->joinable())
    timer_thread_ptr_->join();

  timer_thread_ptr_.reset();
  timing_task_map_.clear();
  timing_wheel_vec_.clear();
  get_executor_func_ = std::function<aimrt::executor::ExecutorRef(std::string_view)>();
}

bool TimeWheelExecutor::IsInCurrentExecutor() const noexcept {
  try {
    return bind_executor_ref_
               ? bind_executor_ref_.IsInCurrentExecutor()
               : (tid_ == std::this_thread::get_id());
  } catch (const std::exception& e) {
    AIMRT_ERROR("{}", e.what());
  }
  return false;
}

void TimeWheelExecutor::Execute(aimrt::executor::Task&& task) noexcept {
  try {
    if (bind_executor_ref_) {
      bind_executor_ref_.Execute(std::move(task));
      return;
    }

    std::unique_lock<std::mutex> lck(imd_mutex_);
    imd_queue_.emplace(std::move(task));
  } catch (const std::exception& e) {
    AIMRT_ERROR("{}", e.what());
  }
}

std::chrono::system_clock::time_point TimeWheelExecutor::Now() const noexcept {
  std::shared_lock<std::shared_mutex> lck(tick_mutex_);

  return aimrt::common::util::GetTimePointFromTimestampNs(
      current_tick_count_ * dt_count_ + start_time_point_);
}

void TimeWheelExecutor::ExecuteAt(
    std::chrono::system_clock::time_point tp, aimrt::executor::Task&& task) noexcept {
  try {
    uint64_t virtual_tp = aimrt::common::util::GetTimestampNs(tp) - start_time_point_;

    std::unique_lock<std::shared_mutex> lck(tick_mutex_);

    if (virtual_tp < current_tick_count_ * dt_count_) {
      lck.unlock();
      Execute(std::move(task));
      return;
    }

    // 当前时间点 time_point_
    uint64_t temp_current_tick_count = current_tick_count_;
    uint64_t diff_tick_count = virtual_tp / dt_count_ - current_tick_count_;

    const size_t len = options_.wheel_size.size();
    for (size_t ii = 0; ii < len; ++ii) {
      if (diff_tick_count < options_.wheel_size[ii]) {
        auto pos = (diff_tick_count + temp_current_tick_count) % options_.wheel_size[ii];

        // TODO：基于时间将任务排序后插进去
        timing_wheel_vec_[ii].wheel[pos].emplace_back(
            TaskWithTimestamp{virtual_tp / dt_count_, std::move(task)});
        return;
      }
      diff_tick_count /= options_.wheel_size[ii];
      temp_current_tick_count /= options_.wheel_size[ii];
    }

    timing_task_map_[diff_tick_count + temp_current_tick_count].emplace_back(
        TaskWithTimestamp{virtual_tp / dt_count_, std::move(task)});
  } catch (const std::exception& e) {
    AIMRT_ERROR("{}", e.what());
  }
}

void TimeWheelExecutor::RegisterGetExecutorFunc(
    const std::function<aimrt::executor::ExecutorRef(std::string_view)>& get_executor_func) {
  AIMRT_CHECK_ERROR_THROW(
      state_.load() == State::kPreInit,
      "Method can only be called when state is 'PreInit'.");
  get_executor_func_ = get_executor_func;
}

void TimeWheelExecutor::TimerLoop() {
  tid_ = std::this_thread::get_id();

  std::string threadname = name_;

  try {
    aimrt::runtime::core::util::SetNameForCurrentThread(threadname);
    aimrt::runtime::core::util::BindCpuForCurrentThread(options_.thread_bind_cpu);
    aimrt::runtime::core::util::SetCpuSchedForCurrentThread(options_.thread_sched_policy);
  } catch (const std::exception& e) {
    AIMRT_WARN("Set thread policy for time manipulator executor '{}' get exception, {}",
               Name(), e.what());
  }

  auto last_loop_time_point = std::chrono::system_clock::now();

  // 记录初始时间
  start_time_point_ = aimrt::common::util::GetTimestampNs(last_loop_time_point);

  start_flag_.store(true);
  start_flag_.notify_all();

  while (state_.load() != State::kShutdown) {
    try {
      // sleep一个dt
      auto real_dt = options_.dt;
      do {
        // 最长sleep时间
        static constexpr auto kMaxSleepDt = std::chrono::seconds(1);

        auto sleep_time = (real_dt > kMaxSleepDt) ? kMaxSleepDt : real_dt;
        real_dt -= sleep_time;

        // 一个小优化，防止real_dt太小
        if (real_dt.count() && options_.dt < kMaxSleepDt && real_dt <= options_.dt) {
          sleep_time += real_dt;
          real_dt -= real_dt;
        }

        std::this_thread::sleep_until(
            last_loop_time_point +=
            std::chrono::duration_cast<std::chrono::system_clock::time_point::duration>(sleep_time));

      } while (state_.load() != State::kShutdown && real_dt.count());

      // 执行立即任务
      if (!bind_executor_ref_) {
        std::queue<aimrt::executor::Task> tmp_queue;

        imd_mutex_.lock();
        imd_queue_.swap(tmp_queue);
        imd_mutex_.unlock();

        while (!tmp_queue.empty()) {
          auto& task = tmp_queue.front();

          try {
            task();
          } catch (const std::exception& e) {
            AIMRT_FATAL("Time wheel executor run task get exception, {}", e.what());
          }

          tmp_queue.pop();
        }
      }

      // 走一个粒度的时间轮
      tick_mutex_.lock();

      // 取出task
      TaskList task_list = timing_wheel_vec_[0].Tick();

      // 执行任务
      if (!task_list.empty()) {
        tick_mutex_.unlock();

        for (auto& itr : task_list) {
          if (bind_executor_ref_) {
            bind_executor_ref_.Execute(std::move(itr.task));
          } else {
            try {
              itr.task();
            } catch (const std::exception& e) {
              AIMRT_FATAL("Time wheel executor run task get exception, {}", e.what());
            }
          }
        }

        tick_mutex_.lock();
      }

      // 更新time point
      ++current_tick_count_;

      tick_mutex_.unlock();
    } catch (const std::exception& e) {
      AIMRT_FATAL("Time manipulator executor '{}' run loop get exception, {}",
                  Name(), e.what());
    }
  }
}
}  // namespace aimrt::runtime::core::executor