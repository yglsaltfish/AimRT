#include "core/executor/asio_thread_executor.h"
#include "aimrt_module_cpp_interface/util/string.h"
#include "core/global.h"
#include "core/util/thread_tools.h"

namespace YAML {
template <>
struct convert<aimrt::runtime::core::executor::AsioThreadExecutor::Options> {
  using Options = aimrt::runtime::core::executor::AsioThreadExecutor::Options;

  static Node encode(const Options& rhs) {
    Node node;

    node["thread_num"] = rhs.thread_num;
    node["thread_sched_policy"] = rhs.thread_sched_policy;
    node["thread_bind_cpu"] = rhs.thread_bind_cpu;
    node["timeout_alarm_threshold_us"] = rhs.timeout_alarm_threshold_us.count();

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
          node["timeout_alarm_threshold_us"].as<uint32_t>());

    return true;
  }
};
}  // namespace YAML

namespace aimrt::runtime::core::executor {

void AsioThreadExecutor::Initialize(std::string_view name,
                                    YAML::Node options_node) {
  AIMRT_CHECK_ERROR_THROW(
      std::atomic_exchange(&state_, State::Init) == State::PreInit,
      "AsioThreadExecutor can only be initialized once.");

  name_ = std::string(name);
  if (options_node && !options_node.IsNull())
    options_ = options_node.as<Options>();

  AIMRT_CHECK_ERROR_THROW(
      options_.thread_num > 0,
      "Invalide asio thread executor options, thread num is zero.");

  io_ptr_ = std::make_unique<boost::asio::io_context>(options_.thread_num);
  work_guard_ptr_ = std::make_unique<
      boost::asio::executor_work_guard<boost::asio::io_context::executor_type>>(
      io_ptr_->get_executor());

  thread_id_vec_.resize(options_.thread_num);

  for (uint32_t ii = 0; ii < options_.thread_num; ++ii) {
    threads_.emplace_back([this, ii] {
      thread_id_vec_[ii] = std::this_thread::get_id();

      std::string threadname = name_;
      if (options_.thread_num > 1)
        threadname = threadname + "." + std::to_string(ii);

      try {
        util::SetNameForCurrentThread(threadname);
        util::BindCpuForCurrentThread(options_.thread_bind_cpu);
        util::SetCpuSchedForCurrentThread(options_.thread_sched_policy);
      } catch (const std::exception& e) {
        AIMRT_WARN("Set thread policy for asio thread executor '{}' get exception, {}",
                   Name(), e.what());
      }

      while (state_.load() != State::Shutdown) {
        try {
          io_ptr_->run();
        } catch (const std::exception& e) {
          AIMRT_FATAL("Asio thread executor '{}' run loop get exception, {}",
                      Name(), e.what());
        }
      }

      thread_id_vec_[ii] = std::thread::id();
    });
  }

  options_node = options_;
}

void AsioThreadExecutor::Start() {
  AIMRT_CHECK_ERROR_THROW(
      std::atomic_exchange(&state_, State::Start) == State::Init,
      "Function can only be called when state is 'Init'.");
}

void AsioThreadExecutor::Shutdown() {
  if (std::atomic_exchange(&state_, State::Shutdown) == State::Shutdown)
    return;

  if (work_guard_ptr_) work_guard_ptr_->reset();

  for (auto itr = threads_.begin(); itr != threads_.end();) {
    if (itr->joinable()) itr->join();
    threads_.erase(itr++);
  }
}

bool AsioThreadExecutor::IsInCurrentExecutor() const {
  assert(state_ == State::Start);
  return (std::find(thread_id_vec_.begin(), thread_id_vec_.end(),
                    std::this_thread::get_id()) != thread_id_vec_.end());
}

void AsioThreadExecutor::Execute(Task&& task) {
  assert(state_ == State::Start);
  boost::asio::post(*io_ptr_, std::move(task));
}

void AsioThreadExecutor::ExecuteAfterNs(uint64_t dt, Task&& task) {
  assert(state_ == State::Start);
  auto timer_ptr_ = std::make_shared<boost::asio::steady_timer>(*io_ptr_);
  timer_ptr_->expires_after(std::chrono::nanoseconds(dt));
  timer_ptr_->async_wait([this, timer_ptr_,
                          task{std::move(task)}](boost::system::error_code ec) {
    if (ec) [[unlikely]] {
      AIMRT_ERROR("Asio thread executor '{}' timer get err, code '{}', msg: {}",
                  Name(), ec.value(), ec.message());
      return;
    }

    auto dif_time = std::chrono::steady_clock::now() - timer_ptr_->expiry();

    task();

    AIMRT_CHECK_WARN(
        dif_time <= options_.timeout_alarm_threshold_us,
        "Asio thread executor '{}' timer delay too much, error time value '{}', require '{}'. "
        "Perhaps the CPU load is too high",
        Name(), std::chrono::duration_cast<std::chrono::microseconds>(dif_time),
        options_.timeout_alarm_threshold_us);
  });
}

}  // namespace aimrt::runtime::core::executor
