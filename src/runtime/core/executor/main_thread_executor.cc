#include "core/executor/main_thread_executor.h"
#include "core/util/thread_tools.h"

namespace YAML {
template <>
struct convert<aimrt::runtime::core::executor::MainThreadExecutor::Options> {
  using Options = aimrt::runtime::core::executor::MainThreadExecutor::Options;

  static Node encode(const Options& rhs) {
    Node node;

    node["name"] = rhs.name;
    node["thread_sched_policy"] = rhs.thread_sched_policy;
    node["thread_bind_cpu"] = rhs.thread_bind_cpu;

    return node;
  }

  static bool decode(const Node& node, Options& rhs) {
    if (!node.IsMap()) return false;

    if (node["name"])
      rhs.name = node["name"].as<std::string>();

    if (node["thread_sched_policy"])
      rhs.thread_sched_policy = node["thread_sched_policy"].as<std::string>();

    if (node["thread_bind_cpu"])
      rhs.thread_bind_cpu = node["thread_bind_cpu"].as<std::vector<uint32_t>>();

    return true;
  }
};
}  // namespace YAML

namespace aimrt::runtime::core::executor {

void MainThreadExecutor::Initialize(YAML::Node options_node) {
  AIMRT_CHECK_ERROR_THROW(
      std::atomic_exchange(&state_, State::Init) == State::PreInit,
      "Executor can only be initialized once.");

  if (options_node && !options_node.IsNull())
    options_ = options_node.as<Options>();

  name_ = options_.name;

  try {
    util::SetNameForCurrentThread(Name());
    util::BindCpuForCurrentThread(options_.thread_bind_cpu);
    util::SetCpuSchedForCurrentThread(options_.thread_sched_policy);
  } catch (const std::exception& e) {
    AIMRT_WARN("Set thread policy for main thread get exception, {}", e.what());
  }

  options_node = options_;

  AIMRT_INFO("Main thread executor init complete");
}

void MainThreadExecutor::Start() {
  AIMRT_CHECK_ERROR_THROW(
      std::atomic_exchange(&state_, State::Start) == State::Init,
      "Main thread executor can only run when state is 'Init'.");

  AIMRT_INFO("Main thread executor start complete, will blocks current thread until shutdown.");

  while (state_.load() != State::Shutdown) {
    // 多生产-单消费优化
    std::queue<Task> tmp_queue;

    {
      std::unique_lock<std::mutex> lck(mutex_);
      cond_.wait(lck, [this] { return !queue_.empty(); });
      queue_.swap(tmp_queue);
    }

    while (!tmp_queue.empty()) {
      auto& task = tmp_queue.front();

      try {
        task();
      } catch (const std::exception& e) {
        AIMRT_FATAL("Main thread executor run task get exception, {}", e.what());
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
      AIMRT_FATAL("Main thread executor run task get exception, {}", e.what());
    }

    queue_.pop();
  }
}

void MainThreadExecutor::Shutdown() {
  if (std::atomic_exchange(&state_, State::Shutdown) == State::Shutdown)
    return;

  AIMRT_INFO("Main thread executor shutdown.");

  // 对主线程来说，shutdown一定跑在task内
  // 并不是真正的shutdown，任务队列还要跑，不能全清了
}

void MainThreadExecutor::Execute(Task&& task) {
  assert(state_.load() == State::Init || state_.load() == State::Start);

  std::unique_lock<std::mutex> lck(mutex_);
  queue_.emplace(std::move(task));
  cond_.notify_one();
}

std::vector<std::pair<std::string, std::string>>
MainThreadExecutor::GenInitializationReport() const {
  return {};
}

}  // namespace aimrt::runtime::core::executor
