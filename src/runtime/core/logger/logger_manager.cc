#include "core/logger/logger_manager.h"
#include "core/logger/console_logger_backend.h"
#include "core/logger/rotate_file_logger_backend.h"

namespace YAML {
template <>
struct convert<aimrt::runtime::core::logger::LoggerManager::Options> {
  using Options = aimrt::runtime::core::logger::LoggerManager::Options;

  static Node encode(const Options& rhs) {
    Node node;
    node["core_lvl"] = aimrt::runtime::core::logger::LogLevelTool::GetLogLevelName(rhs.core_lvl);
    node["default_module_lvl"] =
        aimrt::runtime::core::logger::LogLevelTool::GetLogLevelName(rhs.default_module_lvl);

    node["backends"] = YAML::Node();
    for (const auto& backend_options : rhs.backends_options) {
      Node backend_options_node;
      backend_options_node["type"] = backend_options.type;
      backend_options_node["options"] = backend_options.options;
      node["backends"].push_back(backend_options_node);
    }

    return node;
  }

  static bool decode(const Node& node, Options& rhs) {
    if (!node.IsMap()) return false;

    if (node["core_lvl"]) {
      rhs.core_lvl = aimrt::runtime::core::logger::LogLevelTool::GetLogLevelFromName(
          node["core_lvl"].as<std::string>());
    }

    if (node["default_module_lvl"]) {
      rhs.default_module_lvl = aimrt::runtime::core::logger::LogLevelTool::GetLogLevelFromName(
          node["default_module_lvl"].as<std::string>());
    }

    if (node["backends"] && node["backends"].IsSequence()) {
      for (auto& backend_options_node : node["backends"]) {
        auto backend_options = Options::BackendOptions{
            .type = backend_options_node["type"].as<std::string>()};

        if (backend_options_node["options"])
          backend_options.options = backend_options_node["options"];

        rhs.backends_options.emplace_back(std::move(backend_options));
      }
    }
    return true;
  }
};
}  // namespace YAML

namespace aimrt::runtime::core::logger {

void LoggerManager::Initialize(YAML::Node options_node) {
  RegisterConsoleLoggerBackend();
  RegisterRotateFileLoggerBackend();

  AIMRT_CHECK_ERROR_THROW(
      std::atomic_exchange(&state_, State::Init) == State::PreInit,
      "Logger manager can only be initialized once.");

  if (options_node && !options_node.IsNull())
    options_ = options_node.as<Options>();

  for (auto& backend_options : options_.backends_options) {
    auto finditr = std::find_if(
        logger_backend_ptr_vec_.begin(), logger_backend_ptr_vec_.end(),
        [&backend_options](const auto& ptr) {
          return backend_options.type == ptr->Name();
        });

    AIMRT_CHECK_ERROR_THROW(finditr != logger_backend_ptr_vec_.end(),
                            "Invalid logger backend type '{}'",
                            backend_options.type);

    (*finditr)->Initialize(backend_options.options);

    used_logger_backend_ptr_vec_.emplace_back(finditr->get());
  }

  options_node = options_;
}

void LoggerManager::Start() {
  AIMRT_CHECK_ERROR_THROW(
      std::atomic_exchange(&state_, State::Start) == State::Init,
      "Function can only be called when state is 'Init'.");
}

void LoggerManager::Shutdown() {
  if (std::atomic_exchange(&state_, State::Shutdown) == State::Shutdown)
    return;

  // logger_proxy_map_不能清，有些插件还会打日志
  // logger_proxy_map_.clear();

  for (auto& backend : used_logger_backend_ptr_vec_) {
    backend->Shutdown();
  }

  // logger_backend不能清，可能会有未完成的日志任务

  get_executor_func_ = std::function<executor::ExecutorRef(std::string_view)>();
}

void LoggerManager::RegisterGetExecutorFunc(
    const std::function<executor::ExecutorRef(std::string_view)>& get_executor_func) {
  AIMRT_CHECK_ERROR_THROW(
      state_.load() == State::PreInit,
      "Function can only be called when state is 'PreInit'.");

  get_executor_func_ = get_executor_func;
}

void LoggerManager::RegisterLoggerBackend(
    std::unique_ptr<LoggerBackendBase>&& logger_backend_ptr) {
  AIMRT_CHECK_ERROR_THROW(
      state_.load() == State::PreInit,
      "Function can only be called when state is 'PreInit'.");

  logger_backend_ptr_vec_.emplace_back(std::move(logger_backend_ptr));
}

LoggerProxy& LoggerManager::GetLoggerProxy(
    const util::ModuleDetailInfo& module_info) {
  AIMRT_CHECK_ERROR_THROW(
      state_.load() == State::Init || state_.load() == State::Start,
      "Function can only be called when state is 'Init' or 'Start'.");

  // module_name为空等效于aimrt节点
  const std::string& real_module_name =
      (module_info.name.empty()) ? "core" : module_info.name;

  auto itr = logger_proxy_map_.find(real_module_name);
  if (itr != logger_proxy_map_.end()) return *(itr->second);

  aimrt_log_level_t log_lvl =
      (real_module_name == "core")
          ? options_.core_lvl
          : (module_info.use_default_log_lvl ? options_.default_module_lvl
                                             : module_info.log_lvl);

  auto emplace_ret = logger_proxy_map_.emplace(
      real_module_name,
      std::make_unique<LoggerProxy>(real_module_name, log_lvl,
                                    used_logger_backend_ptr_vec_));

  return *(emplace_ret.first->second);
}

LoggerProxy& LoggerManager::GetLoggerProxy(std::string_view logger_name) {
  AIMRT_CHECK_ERROR_THROW(
      state_.load() == State::Init || state_.load() == State::Start,
      "Function can only be called when state is 'Init' or 'Start'.");

  // logger_name为空等效于aimrt节点
  const std::string& real_logger_name =
      (logger_name.empty()) ? "core" : std::string(logger_name);

  auto itr = logger_proxy_map_.find(real_logger_name);
  if (itr != logger_proxy_map_.end()) return *(itr->second);

  // 统一使用core_lvl
  auto emplace_ret = logger_proxy_map_.emplace(
      real_logger_name,
      std::make_unique<LoggerProxy>(real_logger_name, options_.core_lvl,
                                    used_logger_backend_ptr_vec_));

  return *(emplace_ret.first->second);
}

void LoggerManager::RegisterConsoleLoggerBackend() {
  std::unique_ptr<LoggerBackendBase> ptr =
      std::make_unique<ConsoleLoggerBackend>();

  static_cast<ConsoleLoggerBackend*>(ptr.get())->RegisterGetExecutorFunc(get_executor_func_);

  RegisterLoggerBackend(std::move(ptr));
}

void LoggerManager::RegisterRotateFileLoggerBackend() {
  std::unique_ptr<LoggerBackendBase> ptr =
      std::make_unique<RotateFileLoggerBackend>();

  static_cast<RotateFileLoggerBackend*>(ptr.get())->RegisterGetExecutorFunc(get_executor_func_);

  RegisterLoggerBackend(std::move(ptr));
}

}  // namespace aimrt::runtime::core::logger
