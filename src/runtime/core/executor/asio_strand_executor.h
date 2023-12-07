#pragma once

#include "core/executor/executor_base.h"

#include "yaml-cpp/yaml.h"

#include <boost/asio.hpp>

namespace aimrt::runtime::core::executor {

class AsioStrandExecutor : public ExecutorBase {
 public:
  struct Options {
    std::string bind_asio_thread_executor_name;
    std::chrono::steady_clock::duration timeout_alarm_threshold_us =
        std::chrono::microseconds(1000 * 1000);
  };

  enum class State : uint32_t {
    PreInit,
    Init,
    Start,
    Shutdown,
  };

  using GetAsioHandle = std::function<boost::asio::io_context*(std::string_view)>;

 public:
  AsioStrandExecutor() = default;
  ~AsioStrandExecutor() override = default;

  void Initialize(std::string_view name, YAML::Node options_node) override;
  void Start() override;
  void Shutdown() override;

  std::string_view Type() const override { return "asio_strand"; }
  std::string_view Name() const override { return name_; }

  bool ThreadSafe() const override { return true; }
  bool IsInCurrentExecutor() const override { return false; }
  bool SupportTimerSchedule() const override { return true; }

  void Execute(Task&& task) override;

  std::chrono::steady_clock::time_point Now() const override {
    return std::chrono::steady_clock::now();
  }
  void ExecuteAt(std::chrono::steady_clock::time_point tp, Task&& task) override;

  void RegisterGetAsioHandle(GetAsioHandle&& handle);

  State GetState() const { return state_.load(); }

 private:
  std::string name_;
  Options options_;
  std::atomic<State> state_ = State::PreInit;

  GetAsioHandle get_asio_handle_;

  using Strand = boost::asio::strand<boost::asio::io_context::executor_type>;
  std::unique_ptr<Strand> strand_ptr_;
};

}  // namespace aimrt::runtime::core::executor
