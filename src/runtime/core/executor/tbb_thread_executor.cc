// Copyright (c) 2023, AgiBot Inc.
// All rights reserved

#include "core/executor/tbb_thread_executor.h"
#include "aimrt_module_cpp_interface/util/string.h"
#include "core/util/thread_tools.h"

namespace YAML {
template <>
struct convert<aimrt::runtime::core::executor::TBBThreadExecutor::Options> {
  using Options = aimrt::runtime::core::executor::TBBThreadExecutor::Options;

  static Node encode(const Options& rhs) {
    Node node;

    node["thread_num"] = rhs.thread_num;
    node["thread_sched_policy"] = rhs.thread_sched_policy;
    node["thread_bind_cpu"] = rhs.thread_bind_cpu;
    node["timeout_alarm_threshold_us"] = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(
            rhs.timeout_alarm_threshold_us)
            .count());

    return node;
  }

  static bool decode(const Node& node, Options& rhs) {
    if (!node.IsMap()) return false;

    if (node["thread_num"]) rhs.thread_num = node["thread_num"].as<uint32_t>();
    if (node["thread_sched_policy"])
      rhs.thread_sched_policy = node["thread_sched_policy"].as<std::string>();
    if (node["thread_bind_cpu"])
      rhs.thread_bind_cpu = node["thread_bind_cpu"].as<std::vector<uint32_t>>();
    if (node["timeout_alarm_threshold_us"])
      rhs.timeout_alarm_threshold_us = std::chrono::microseconds(
          node["timeout_alarm_threshold_us"].as<uint64_t>());

    return true;
  }
};

}  // namespace YAML

namespace aimrt::runtime::core::executor {

void TBBThreadExecutor::Initialize(std::string_view name, YAML::Node options_node) {
  AIMRT_CHECK_ERROR_THROW(
      std::atomic_exchange(&state_, State::Init) == State::PreInit,
      "TBBThreadExecutor can only be initialized once.");

  name_ = std::string(name);
  if (options_node && !options_node.IsNull())
    options_ = options_node.as<Options>();

  AIMRT_CHECK_ERROR_THROW(
      options_.thread_num > 0,
      "Invalide tbb thread executor options, thread num is zero.");

  thread_id_vec_.resize(options_.thread_num);

  for (uint32_t ii = 0; ii < options_.thread_num; ++ii) {
    threads_.emplace_back([this, ii] {
      ++work_thread_num;

      thread_id_vec_[ii] = std::this_thread::get_id();

      std::string threadname = name_;
      if (options_.thread_num > 1)
        threadname = threadname + "." + std::to_string(ii);

      try {
        util::SetNameForCurrentThread(threadname);
        util::BindCpuForCurrentThread(options_.thread_bind_cpu);
        util::SetCpuSchedForCurrentThread(options_.thread_sched_policy);
      } catch (const std::exception& e) {
        AIMRT_WARN("Set thread policy for tbb thread executor '{}' get exception, {}",
                   Name(), e.what());
      }

      aimrt::executor::Task task;
      while (true) {
        try {
          while (qu_.try_pop(task)) task();
        } catch (const std::exception& e) {
          AIMRT_FATAL("Tbb thread executor '{}' run loop get exception, {}",
                      Name(), e.what());
        }

        if (state_.load() == State::Shutdown) break;

        try {
          qu_.pop(task);
          task();
        } catch (const tbb::user_abort& e) {
        } catch (const std::exception& e) {
          AIMRT_FATAL("Tbb thread executor '{}' run loop get exception, {}",
                      Name(), e.what());
        }
      }

      thread_id_vec_[ii] = std::thread::id();

      --work_thread_num;
    });
  }

  options_node = options_;
}

void TBBThreadExecutor::Start() {
  AIMRT_CHECK_ERROR_THROW(
      std::atomic_exchange(&state_, State::Start) == State::Init,
      "Method can only be called when state is 'Init'.");
}

void TBBThreadExecutor::Shutdown() {
  if (std::atomic_exchange(&state_, State::Shutdown) == State::Shutdown)
    return;

  while (work_thread_num.load()) {
    qu_.abort();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }

  for (auto itr = threads_.begin(); itr != threads_.end();) {
    if (itr->joinable()) itr->join();
    threads_.erase(itr++);
  }
}

bool TBBThreadExecutor::IsInCurrentExecutor() const {
  return (std::find(thread_id_vec_.begin(), thread_id_vec_.end(),
                    std::this_thread::get_id()) != thread_id_vec_.end());
}

void TBBThreadExecutor::Execute(aimrt::executor::Task&& task) {
  try {
    qu_.emplace(std::move(task));
  } catch (const std::exception& e) {
    AIMRT_FATAL("Tbb thread executor '{}' execute task get exception, {}",
                Name(), e.what());
  }
}

void TBBThreadExecutor::ExecuteAt(std::chrono::system_clock::time_point tp, aimrt::executor::Task&& task) {
  AIMRT_ERROR_THROW("Tbb thread executor '{}' does not support timer schedule.", Name());
}

}  // namespace aimrt::runtime::core::executor
