#include "core/executor/executor_manager.h"
#include "aimrt_module_cpp_interface/util/string.h"
#include "core/executor/asio_strand_executor.h"
#include "core/executor/asio_thread_executor.h"
#include "core/executor/tbb_thread_executor.h"
#include "core/global.h"
#include "util/string_util.h"

namespace YAML {
template <>
struct convert<aimrt::runtime::core::executor::ExecutorManager::Options> {
  using Options = aimrt::runtime::core::executor::ExecutorManager::Options;

  static Node encode(const Options& rhs) {
    Node node;

    node["executors"] = YAML::Node();
    for (const auto& executor : rhs.executors_options) {
      Node executor_node;
      executor_node["name"] = executor.name;
      executor_node["type"] = executor.type;
      executor_node["options"] = executor.options;
      node["executors"].push_back(executor_node);
    }

    return node;
  }

  static bool decode(const Node& node, Options& rhs) {
    if (!node.IsMap()) return false;

    if (node["executors"] && node["executors"].IsSequence()) {
      for (auto& executor_node : node["executors"]) {
        auto executor_options = Options::ExecutorOptions{
            .name = executor_node["name"].as<std::string>(),
            .type = executor_node["type"].as<std::string>()};

        if (executor_node["options"])
          executor_options.options = executor_node["options"];

        rhs.executors_options.emplace_back(std::move(executor_options));
      }
    }

    return true;
  }
};
}  // namespace YAML

namespace aimrt::runtime::core::executor {

void ExecutorManager::Initialize(YAML::Node options_node) {
  RegisterAsioExecutorGenFunc();
  RegisterTBBExecutorGenFunc();

  AIMRT_CHECK_ERROR_THROW(
      std::atomic_exchange(&state_, State::Init) == State::PreInit,
      "Executor manager can only be initialized once.");

  if (options_node && !options_node.IsNull())
    options_ = options_node.as<Options>();

  // 生成executor
  for (auto& executor_options : options_.executors_options) {
    AIMRT_CHECK_ERROR_THROW(
        executor_proxy_map_.find(executor_options.name) == executor_proxy_map_.end(),
        "Duplicate executor name '{}'.", executor_options.name);

    auto finditr = executor_gen_func_map_.find(executor_options.type);
    AIMRT_CHECK_ERROR_THROW(finditr != executor_gen_func_map_.end(),
                            "Invalid executor type '{}'.",
                            executor_options.type);

    auto executor_ptr = finditr->second();
    AIMRT_CHECK_ERROR_THROW(
        executor_ptr,
        "Gen executor failed, executor name '{}', executor type '{}'.",
        executor_options.name, executor_options.type);

    executor_ptr->Initialize(executor_options.name, executor_options.options);

    AIMRT_TRACE("Gen executor '{}' success.", executor_ptr->Name());

    executor_proxy_map_.emplace(
        executor_options.name,
        std::make_unique<ExecutorProxy>(executor_ptr.get()));

    executor_vec_.emplace_back(std::move(executor_ptr));
  }

  options_node = options_;
}

void ExecutorManager::Start() {
  AIMRT_CHECK_ERROR_THROW(
      std::atomic_exchange(&state_, State::Start) == State::Init,
      "Function can only be called when state is 'Init'.");

  for (auto& itr : executor_vec_) {
    itr->Start();
  }
}

void ExecutorManager::Shutdown() {
  if (std::atomic_exchange(&state_, State::Shutdown) == State::Shutdown)
    return;

  executor_manager_proxy_map_.clear();
  executor_proxy_map_.clear();

  for (auto& itr : executor_vec_) {
    itr->Shutdown();
  }

  executor_vec_.clear();
  executor_gen_func_map_.clear();
}

void ExecutorManager::RegisterExecutorGenFunc(
    std::string_view type, ExecutorGenFunc&& executor_gen_func) {
  AIMRT_CHECK_ERROR_THROW(
      state_.load() == State::PreInit,
      "Function can only be called when state is 'PreInit'.");

  executor_gen_func_map_.emplace(type, std::move(executor_gen_func));
}

ExecutorManagerProxy& ExecutorManager::GetExecutorManagerProxy(
    const util::ModuleDetailInfo& module_info) {
  AIMRT_CHECK_ERROR_THROW(
      state_.load() == State::Init,
      "Function can only be called when state is 'Init'.");

  auto itr = executor_manager_proxy_map_.find(module_info.name);
  if (itr != executor_manager_proxy_map_.end()) return *(itr->second);

  auto emplace_ret = executor_manager_proxy_map_.emplace(
      module_info.name,
      std::make_unique<ExecutorManagerProxy>(executor_proxy_map_));

  return *(emplace_ret.first->second);
}

void ExecutorManager::RegisterAsioExecutorGenFunc() {
  RegisterExecutorGenFunc("asio_thread", []() -> std::unique_ptr<ExecutorBase> {
    return std::make_unique<AsioThreadExecutor>();
  });

  RegisterExecutorGenFunc("asio_strand", [this]() -> std::unique_ptr<ExecutorBase> {
    auto ptr = std::make_unique<AsioStrandExecutor>();
    ptr->RegisterGetAsioHandle(
        [this](std::string_view name) -> boost::asio::io_context* {
          auto itr = std::find_if(
              executor_vec_.begin(),
              executor_vec_.end(),
              [name](const std::unique_ptr<ExecutorBase>& executor) -> bool {
                return (executor->Type() == "asio_thread") &&
                       (executor->Name() == name);
              });

          if (itr != executor_vec_.end())
            return dynamic_cast<AsioThreadExecutor*>(itr->get())->IOCTX();

          return nullptr;
        });
    return ptr;
  });
}

void ExecutorManager::RegisterTBBExecutorGenFunc() {
  RegisterExecutorGenFunc("tbb_thread", []() -> std::unique_ptr<ExecutorBase> {
    return std::make_unique<TBBThreadExecutor>();
  });
}

}  // namespace aimrt::runtime::core::executor
