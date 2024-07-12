#include "core/executor/simple_thread_executor.h"
#include "core/util/thread_tools.h"

namespace YAML {
template <>
struct convert<aimrt::runtime::core::executor::SimpleThreadExecutor::Options> {
  using Options = aimrt::runtime::core::executor::SimpleThreadExecutor::Options;

  static Node encode(const Options& rhs) {
    Node node;

    node["thread_sched_policy"] = rhs.thread_sched_policy;
    node["thread_bind_cpu"] = rhs.thread_bind_cpu;

    return node;
  }

  static bool decode(const Node& node, Options& rhs) {
    if (!node.IsMap()) return false;

    if (node["thread_sched_policy"])
      rhs.thread_sched_policy = node["thread_sched_policy"].as<std::string>();

    if (node["thread_bind_cpu"])
      rhs.thread_bind_cpu = node["thread_bind_cpu"].as<std::vector<uint32_t>>();

    return true;
  }
};
}  // namespace YAML

namespace aimrt::runtime::core::executor {

void SimpleThreadExecutor::Initialize(std::string_view name, YAML::Node options_node) {
  AIMRT_CHECK_ERROR_THROW(
      std::atomic_exchange(&state_, State::Init) == State::PreInit,
      "SimpleThreadExecutor can only be initialized once.");

  name_ = std::string(name);
  if (options_node && !options_node.IsNull())
    options_ = options_node.as<Options>();

  thread_ptr_ = std::make_unique<std::thread>([this]() {
    thread_id_ = std::this_thread::get_id();

    try {
      util::SetNameForCurrentThread(name_);
      util::BindCpuForCurrentThread(options_.thread_bind_cpu);
      util::SetCpuSchedForCurrentThread(options_.thread_sched_policy);
    } catch (const std::exception& e) {
      AIMRT_WARN("Set thread policy for simple thread executor '{}' get exception, {}",
                 Name(), e.what());
    }

    while (state_.load() != State::Shutdown) {
      // 多生产-单消费优化
      std::queue<aimrt::executor::Task> tmp_queue;

      {
        std::unique_lock<std::mutex> lck(mutex_);
        cond_.wait(lck, [this] { return !queue_.empty() || state_.load() == State::Shutdown; });
        queue_.swap(tmp_queue);
      }

      while (!tmp_queue.empty()) {
        auto& task = tmp_queue.front();

        try {
          task();
        } catch (const std::exception& e) {
          AIMRT_FATAL("Simple thread executor run task get exception, {}", e.what());
        }

        tmp_queue.pop();
      }
    }

    // Shutdown之后再运行一次，不用加锁了，因为不会再有task进入队列了
    while (!queue_.empty()) {
      auto& task = queue_.front();

      try {
        task();
      } catch (const std::exception& e) {
        AIMRT_FATAL("Simple thread executor run task get exception, {}", e.what());
      }

      queue_.pop();
    }

    thread_id_ = std::thread::id();
  });

  options_node = options_;
}

void SimpleThreadExecutor::Start() {
  AIMRT_CHECK_ERROR_THROW(
      std::atomic_exchange(&state_, State::Start) == State::Init,
      "Function can only be called when state is 'Init'.");
}

void SimpleThreadExecutor::Shutdown() {
  if (std::atomic_exchange(&state_, State::Shutdown) == State::Shutdown)
    return;

  {
    std::unique_lock<std::mutex> lck(mutex_);
    cond_.notify_one();
  }

  if (thread_ptr_->joinable())
    thread_ptr_->join();

  thread_ptr_.reset();
}

void SimpleThreadExecutor::Execute(aimrt::executor::Task&& task) {
  assert(state_.load() == State::Init || state_.load() == State::Start);

  std::unique_lock<std::mutex> lck(mutex_);
  queue_.emplace(std::move(task));
  cond_.notify_one();
}

void SimpleThreadExecutor::ExecuteAt(std::chrono::system_clock::time_point tp, aimrt::executor::Task&& task) {
  AIMRT_ERROR_THROW("Simple thread executor '{}' does not support timer schedule.", Name());
}

}  // namespace aimrt::runtime::core::executor
