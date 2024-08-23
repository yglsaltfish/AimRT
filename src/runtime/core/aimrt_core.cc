// Copyright (c) 2023, AgiBot Inc.
// All rights reserved.

#include "core/aimrt_core.h"

#include <fstream>
#include <iostream>

#include "core/util/yaml_tools.h"

namespace aimrt::runtime::core {

AimRTCore::AimRTCore()
    : logger_ptr_(std::make_shared<aimrt::common::util::LoggerWrapper>()) {}

AimRTCore::~AimRTCore() {
  try {
    ShutdownImpl();
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
  plugin_manager_.RegisterCorePtr(this);
  plugin_manager_.Initialize(configurator_manager_.GetAimRTOptionsNode("plugin"));
  EnterState(State::PostInitPlugin);

  // Init main thread executor
  EnterState(State::PreInitMainThread);
  main_thread_executor_.SetLogger(logger_ptr_);
  main_thread_executor_.Initialize(configurator_manager_.GetAimRTOptionsNode("main_thread"));
  EnterState(State::PostInitMainThread);

  // Init guard thread executor
  EnterState(State::PreInitGuardThread);
  guard_thread_executor_.SetLogger(logger_ptr_);
  guard_thread_executor_.Initialize(configurator_manager_.GetAimRTOptionsNode("guard_thread"));
  EnterState(State::PostInitGuardThread);

  // Init executor
  EnterState(State::PreInitExecutor);
  executor_manager_.SetLogger(logger_ptr_);
  executor_manager_.SetUsedExecutorName(main_thread_executor_.Name());
  executor_manager_.SetUsedExecutorName(guard_thread_executor_.Name());
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

  // Init allocator
  EnterState(State::PreInitAllocator);
  allocator_manager_.SetLogger(logger_ptr_);
  allocator_manager_.Initialize(configurator_manager_.GetAimRTOptionsNode("allocator"));
  EnterState(State::PostInitAllocator);

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

  // Check cfg file
  CheckCfgFile();

  EnterState(State::PostInit);
}

void AimRTCore::StartImpl() {
  AIMRT_INFO("Gen initialization report:\n{}", GenInitializationReport());

  EnterState(State::PreStart);
  configurator_manager_.Start();
  plugin_manager_.Start();
  main_thread_executor_.Start();
  guard_thread_executor_.Start();
  executor_manager_.Start();
  logger_manager_.Start();
  allocator_manager_.Start();
  rpc_manager_.Start();
  channel_manager_.Start();
  parameter_manager_.Start();
  module_manager_.Start();
  EnterState(State::PostStart);
}

void AimRTCore::ShutdownImpl() {
  if (std::atomic_exchange(&shutdown_impl_flag_, true)) return;

  EnterState(State::PreShutdown);
  module_manager_.Shutdown();
  parameter_manager_.Shutdown();
  channel_manager_.Shutdown();
  rpc_manager_.Shutdown();
  allocator_manager_.Shutdown();

  ResetCoreLogger();
  std::this_thread::sleep_for(std::chrono::milliseconds(200));
  logger_manager_.Shutdown();

  executor_manager_.Shutdown();
  guard_thread_executor_.Shutdown();
  main_thread_executor_.Shutdown();
  plugin_manager_.Shutdown();
  configurator_manager_.Shutdown();
  EnterState(State::PostShutdown);
}

void AimRTCore::Start() {
  StartImpl();

  AIMRT_INFO("AimRT start completed, will waiting for shutdown.");

  shutdown_promise_.get_future().wait();
  ShutdownImpl();
}

std::future<void> AimRTCore::AsyncStart() {
  StartImpl();

  AIMRT_INFO("AimRT start completed, will waiting for async shutdown.");

  std::promise<void> end_running_promise;
  auto fu = end_running_promise.get_future();

  std::thread([this, end_running_promise{std::move(end_running_promise)}]() mutable {
    shutdown_promise_.get_future().wait();
    ShutdownImpl();
    end_running_promise.set_value();
  }).detach();

  return fu;
}

void AimRTCore::Shutdown() {
  if (std::atomic_exchange(&shutdown_flag_, true)) return;

  shutdown_promise_.set_value();
}

void AimRTCore::EnterState(State state) {
  state_ = state;
  for (const auto& func : hook_task_vec_array_[static_cast<uint32_t>(state)])
    func();
}

void AimRTCore::SetCoreLogger() {
  const auto* core_logger_ptr = logger_manager_.GetLoggerProxy("core").NativeHandle();

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
}

void AimRTCore::ResetCoreLogger() {
  logger_ptr_->get_log_level_func = aimrt::common::util::SimpleLogger::GetLogLevel;
  logger_ptr_->log_func = aimrt::common::util::SimpleLogger::Log;
}

aimrt::executor::ExecutorRef AimRTCore::GetExecutor(std::string_view executor_name) {
  if (executor_name.empty() || executor_name == guard_thread_executor_.Name())
    return aimrt::executor::ExecutorRef(guard_thread_executor_.NativeHandle());
  return GetExecutorManager().GetExecutor(executor_name);
}

void AimRTCore::InitCoreProxy(const util::ModuleDetailInfo& info, module::CoreProxy& proxy) {
  proxy.SetConfigurator(configurator_manager_.GetConfiguratorProxy(info).NativeHandle());
  proxy.SetExecutorManager(executor_manager_.GetExecutorManagerProxy(info).NativeHandle());
  proxy.SetLogger(logger_manager_.GetLoggerProxy(info).NativeHandle());
  proxy.SetAllocator(allocator_manager_.GetAllocatorProxy(info).NativeHandle());
  proxy.SetRpcHandle(rpc_manager_.GetRpcHandleProxy(info).NativeHandle());
  proxy.SetChannelHandle(channel_manager_.GetChannelHandleProxy(info).NativeHandle());
  proxy.SetParameterHandle(parameter_manager_.GetParameterHandleProxy(info).NativeHandle());
}

void AimRTCore::CheckCfgFile() const {
  std::string check_msg = util::CheckYamlNodes(
      configurator_manager_.GetRootOptionsNode()["aimrt"],
      configurator_manager_.GetUserRootOptionsNode()["aimrt"],
      "aimrt");

  if (!check_msg.empty())
    AIMRT_WARN("Configuration Name Warning in \"{}\":\n{}",
               configurator_manager_.GetConfigureFilePath(), check_msg);
}

std::string AimRTCore::GenInitializationReport() const {
  AIMRT_CHECK_ERROR_THROW(state_ == State::PostInit,
                          "Initialization report can only be generated after initialization is complete.");

  std::list<std::pair<std::string, std::string>> report;

  report.splice(report.end(), configurator_manager_.GenInitializationReport());
  report.splice(report.end(), plugin_manager_.GenInitializationReport());
  report.splice(report.end(), executor_manager_.GenInitializationReport());
  report.splice(report.end(), logger_manager_.GenInitializationReport());
  report.splice(report.end(), allocator_manager_.GenInitializationReport());
  report.splice(report.end(), rpc_manager_.GenInitializationReport());
  report.splice(report.end(), channel_manager_.GenInitializationReport());
  report.splice(report.end(), parameter_manager_.GenInitializationReport());
  report.splice(report.end(), module_manager_.GenInitializationReport());

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
