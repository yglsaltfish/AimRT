#include "core/executor/main_thread_executor.h"
#include "core/global.h"
#include "core/util/thread_tools.h"

namespace YAML {
template <>
struct convert<aimrt::runtime::core::executor::MainThreadExecutor::Options> {
  using Options = aimrt::runtime::core::executor::MainThreadExecutor::Options;

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

void MainThreadExecutor::Initialize(YAML::Node options_node) {
  AIMRT_CHECK_ERROR_THROW(
      std::atomic_exchange(&status_, Status::Init) == Status::PreInit,
      "Executor can only be initialized once.");

  if (options_node && !options_node.IsNull())
    options_ = options_node.as<Options>();

  io_ptr_ = std::make_unique<boost::asio::io_context>(1);
  sig_ptr_ = std::make_unique<boost::asio::signal_set>(*io_ptr_);

  std::set<int> signals;

  for (auto& itr : signal_handle_vec_)
    signals.insert(itr.first.begin(), itr.first.end());

  for (auto sig : signals) sig_ptr_->add(sig);

  sig_ptr_->async_wait([signal_handle_vec{std::move(signal_handle_vec_)}](
                           boost::system::error_code err, int sig) {
    for (auto& itr : signal_handle_vec) {
      if (itr.first.find(sig) != itr.first.end()) itr.second(err, sig);
    }
  });

  try {
    util::SetNameForCurrentThread(Name());
    util::BindCpuForCurrentThread(options_.thread_bind_cpu);
    util::SetCpuSchedForCurrentThread(options_.thread_sched_policy);
  } catch (const std::exception& e) {
    AIMRT_WARN("Set thread policy for main thread get exception, {}",
               e.what());
  }

  options_node = options_;
}

void MainThreadExecutor::Start() {
  AIMRT_CHECK_ERROR_THROW(
      std::atomic_exchange(&status_, Status::Start) == Status::Init,
      "Main thread executor can only run when status is 'Init'.");

  io_ptr_->run();
}

void MainThreadExecutor::Shutdown() {
  if (std::atomic_exchange(&status_, Status::Shutdown) == Status::Shutdown)
    return;

  if (sig_ptr_) {
    sig_ptr_->cancel();
    sig_ptr_->clear();
  }

  // 并不是真正的shutdown，io_ctx还要跑，不能全清了
  signal_handle_vec_.clear();
}

void MainThreadExecutor::RegisterSignalHandle(const std::set<int>& signals,
                                              SignalHandle&& signal_handle) {
  AIMRT_CHECK_ERROR_THROW(
      status_.load() == Status::PreInit,
      "Function can only be called when status is 'PreInit'.");

  signal_handle_vec_.emplace_back(signals, std::move(signal_handle));
}

void MainThreadExecutor::Execute(Task&& task) {
  assert(status_ == Status::Init || status_ == Status::Start);
  boost::asio::post(*io_ptr_, std::move(task));
}

}  // namespace aimrt::runtime::core::executor
