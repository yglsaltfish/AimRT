#include "core/aimrt_core.h"

#include <fstream>
#include <iostream>

namespace aimrt::runtime::core {

AimRTCore::AimRTCore()
    : logger_ptr_(std::make_shared<aimrt::common::util::LoggerWrapper>()) {}

AimRTCore::~AimRTCore() {
  try {
    Shutdown();
  } catch (const std::exception& e) {
    AIMRT_INFO("AimRTCore destruct get exception, {}", e.what());
  }

  for (uint32_t ii = 0; ii < static_cast<uint32_t>(State::MaxStateNum); ++ii) {
    hook_task_vec_array_[ii].clear();
  }
}

void AimRTCore::Initialize(const Options& options) {
  EnterState(State::PreInit);

  options_ = options;

  // Init configurator
  EnterState(State::PreInitConfigurator);
  configurator_manager_.SetLogger(logger_ptr_);
  configurator_manager_.Initialize(options_.cfg_file_path);
  EnterState(State::PostInitConfigurator);

  // Init plugin
  EnterState(State::PreInitPlugin);
  plugin_manager_.SetLogger(logger_ptr_);
  plugin_manager_.RegisterPluginInitFunc(
      [this](AimRTCorePluginBase* plugin_ptr) { return plugin_ptr->Initialize(this); });
  plugin_manager_.Initialize(configurator_manager_.GetAimRTOptionsNode("plugin"));
  EnterState(State::PostInitPlugin);

  // Init main thread executor
  EnterState(State::PreInitMainThread);
  main_thread_executor_.SetLogger(logger_ptr_);
  main_thread_executor_.Initialize(configurator_manager_.GetAimRTOptionsNode("main_thread"));
  EnterState(State::PostInitMainThread);

  // Init allocator
  EnterState(State::PreInitAllocator);
  allocator_manager_.SetLogger(logger_ptr_);
  allocator_manager_.Initialize(configurator_manager_.GetAimRTOptionsNode("allocator"));
  EnterState(State::PostInitAllocator);

  // Init executor
  EnterState(State::PreInitExecutor);
  executor_manager_.SetLogger(logger_ptr_);
  executor_manager_.SetUsedExecutorName(main_thread_executor_.Name());
  executor_manager_.Initialize(configurator_manager_.GetAimRTOptionsNode("executor"));
  EnterState(State::PostInitExecutor);

  // Init log
  EnterState(State::PreInitLog);
  logger_manager_.SetLogger(logger_ptr_);
  logger_manager_.RegisterGetExecutorFunc(
      std::bind(&AimRTCore::GetExecutor, this, std::placeholders::_1));
  logger_manager_.Initialize(configurator_manager_.GetAimRTOptionsNode("log"));
  SetCoreLogger();
  EnterState(State::PostInitLog);

  // Init rpc
  EnterState(State::PreInitRpc);
  rpc_manager_.SetLogger(logger_ptr_);
  rpc_manager_.RegisterGetExecutorFunc(
      std::bind(&AimRTCore::GetExecutor, this, std::placeholders::_1));
  rpc_manager_.Initialize(configurator_manager_.GetAimRTOptionsNode("rpc"));
  EnterState(State::PostInitRpc);

  // Init channel
  EnterState(State::PreInitChannel);
  channel_manager_.SetLogger(logger_ptr_);
  channel_manager_.RegisterGetExecutorFunc(
      std::bind(&AimRTCore::GetExecutor, this, std::placeholders::_1));
  channel_manager_.Initialize(configurator_manager_.GetAimRTOptionsNode("channel"));
  EnterState(State::PostInitChannel);

  // Init parameter
  EnterState(State::PreInitParameter);
  parameter_manager_.SetLogger(logger_ptr_);
  parameter_manager_.Initialize(configurator_manager_.GetAimRTOptionsNode("parameter"));
  EnterState(State::PostInitParameter);

  // Init modules
  EnterState(State::PreInitModules);
  module_manager_.SetLogger(logger_ptr_);
  module_manager_.RegisterCoreProxyConfigurator(
      std::bind(&AimRTCore::InitCoreProxy, this, std::placeholders::_1, std::placeholders::_2));
  module_manager_.Initialize(configurator_manager_.GetAimRTOptionsNode("module"));
  EnterState(State::PostInitModules);

  // Dump cfg file
  DumpCfgFile();

  EnterState(State::PostInit);
}

void AimRTCore::Start() {
  AIMRT_INFO("Gen initialization report:\n{}", GenInitializationReport());

  EnterState(State::PreStart);
  configurator_manager_.Start();
  plugin_manager_.Start();
  allocator_manager_.Start();
  executor_manager_.Start();
  logger_manager_.Start();
  rpc_manager_.Start();
  channel_manager_.Start();
  parameter_manager_.Start();
  module_manager_.Start();
  EnterState(State::PostStart);

  main_thread_executor_.Start();
}

std::future<void> AimRTCore::AsyncStart() {
  AIMRT_INFO("Gen initialization report:\n{}", GenInitializationReport());

  EnterState(State::PreStart);
  configurator_manager_.Start();
  plugin_manager_.Start();
  allocator_manager_.Start();
  executor_manager_.Start();
  logger_manager_.Start();
  rpc_manager_.Start();
  channel_manager_.Start();
  parameter_manager_.Start();
  module_manager_.Start();
  EnterState(State::PostStart);

  std::promise<void> thread_exit_promise;
  auto fu = thread_exit_promise.get_future();
  std::thread(
      [thread_exit_promise{std::move(thread_exit_promise)}, this]() mutable {
        try {
          main_thread_executor_.Start();
          thread_exit_promise.set_value_at_thread_exit();
        } catch (std::exception_ptr p) {
          thread_exit_promise.set_exception_at_thread_exit(p);
        }
      })
      .detach();

  return fu;
}

void AimRTCore::Shutdown() {
  if (std::atomic_exchange(&stop_flag_, true)) return;

  auto stop_work = [this]() {
    EnterState(State::PreShutdown);

    module_manager_.Shutdown();

    parameter_manager_.Shutdown();

    channel_manager_.Shutdown();

    rpc_manager_.Shutdown();

    ResetCoreLogger();
    logger_manager_.Shutdown();

    executor_manager_.Shutdown();

    allocator_manager_.Shutdown();

    plugin_manager_.Shutdown();

    configurator_manager_.Shutdown();

    main_thread_executor_.Shutdown();

    EnterState(State::PostShutdown);
  };

  if (main_thread_executor_.GetState() == executor::MainThreadExecutor::State::Start) {
    main_thread_executor_.Execute(std::move(stop_work));
  } else {
    stop_work();
  }
}

void AimRTCore::EnterState(State state) {
  state_ = state;
  for (const auto& func : hook_task_vec_array_[static_cast<uint32_t>(state)]) {
    func();
  }
}

void AimRTCore::SetCoreLogger() {
  const auto* core_logger_ptr = logger_manager_.GetLoggerProxy("core").NativeHandle();
  const auto* core_allocator_ptr = allocator_manager_.GetAllocatorProxy("core").NativeHandle();

  logger_ptr_->get_log_level_func = [core_logger_ptr]() -> uint32_t {
    return core_logger_ptr->get_log_level(core_logger_ptr->impl);
  };

  logger_ptr_->log_func =
      [core_logger_ptr](
          uint32_t lvl,
          uint32_t line,
          uint32_t column,
          const char* file_name,
          const char* function_name,
          const char* log_data,
          size_t log_data_size) {
        core_logger_ptr->log(
            core_logger_ptr->impl,
            static_cast<aimrt_log_level_t>(lvl),
            line, column, file_name, function_name,
            log_data, log_data_size);  //
      };

  logger_ptr_->get_log_buf_func =
      [core_allocator_ptr]() -> std::tuple<char*, size_t> {
    constexpr size_t kMaxLogBufSize = 1024 * 1024;
    void* buf = core_allocator_ptr->get_thread_local_buf(core_allocator_ptr->impl, kMaxLogBufSize);
    return {static_cast<char*>(buf), kMaxLogBufSize};
  };
}

void AimRTCore::ResetCoreLogger() {
  logger_ptr_->get_log_level_func = aimrt::common::util::SimpleLogger::GetLogLevel;
  logger_ptr_->log_func = aimrt::common::util::SimpleLogger::Log;
  logger_ptr_->get_log_buf_func = []() -> std::tuple<char*, size_t> { return {nullptr, 0}; };
}

aimrt::executor::ExecutorRef AimRTCore::GetExecutor(std::string_view executor_name) {
  if (executor_name.empty() || executor_name == main_thread_executor_.Name())
    return aimrt::executor::ExecutorRef(main_thread_executor_.NativeHandle());
  return GetExecutorManager().GetExecutor(executor_name);
}

void AimRTCore::InitCoreProxy(const util::ModuleDetailInfo& info, module::CoreProxy& proxy) {
  proxy.SetConfigurator(configurator_manager_.GetConfiguratorProxy(info).NativeHandle());
  proxy.SetAllocator(allocator_manager_.GetAllocatorProxy(info).NativeHandle());
  proxy.SetExecutorManager(executor_manager_.GetExecutorManagerProxy(info).NativeHandle());
  proxy.SetLogger(logger_manager_.GetLoggerProxy(info).NativeHandle());
  proxy.SetRpcHandle(rpc_manager_.GetRpcHandleProxy(info).NativeHandle());
  proxy.SetChannelHandle(channel_manager_.GetChannelHandleProxy(info).NativeHandle());
  proxy.SetParameterHandle(parameter_manager_.GetParameterHandleProxy(info).NativeHandle());
}

void AimRTCore::DumpCfgFile() const {
  if (!options_.dump_cfg_file) return;

  std::filesystem::path path =
      options_.dump_cfg_file_path.empty() ? "./dump_cfg.yaml" : options_.dump_cfg_file_path;

  std::ofstream ofs;
  ofs.open(path, std::ios::trunc);

  YAML::Node dump_node = configurator_manager_.DumpRootOptionsNode();
  ofs << dump_node;
  ofs.flush();
  ofs.clear();
  ofs.close();

  AIMRT_INFO("Dump cfg to file {}", path.string());
}

std::string AimRTCore::GenInitializationReport() const {
  std::list<std::pair<std::string, std::string>> report;

  auto configurator_manager_report = configurator_manager_.GenInitializationReport();
  report.splice(report.end(), configurator_manager_report);

  auto plugin_manager_report = plugin_manager_.GenInitializationReport();
  report.splice(report.end(), plugin_manager_report);

  auto main_thread_executor_report = main_thread_executor_.GenInitializationReport();
  report.splice(report.end(), main_thread_executor_report);

  auto allocator_manager_report = allocator_manager_.GenInitializationReport();
  report.splice(report.end(), allocator_manager_report);

  auto executor_manager_report = executor_manager_.GenInitializationReport();
  report.splice(report.end(), executor_manager_report);

  auto logger_manager_report = logger_manager_.GenInitializationReport();
  report.splice(report.end(), logger_manager_report);

  auto rpc_manager_report = rpc_manager_.GenInitializationReport();
  report.splice(report.end(), rpc_manager_report);

  auto channel_manager_report = channel_manager_.GenInitializationReport();
  report.splice(report.end(), channel_manager_report);

  auto parameter_manager_report = parameter_manager_.GenInitializationReport();
  report.splice(report.end(), parameter_manager_report);

  auto module_manager_report = module_manager_.GenInitializationReport();
  report.splice(report.end(), module_manager_report);

  std::stringstream result;
  result << "\n----------------------- AimRT Initialization Report Begin ----------------------\n\n";

  size_t count = 0;
  for (auto& itr : report) {
    ++count;
    result << "[" << count << "]. " << itr.first << "\n"
           << itr.second << "\n\n";
  }

  result << "\n----------------------- AimRT Initialization Report End ------------------------\n\n";

  return result.str();
}

}  // namespace aimrt::runtime::core
