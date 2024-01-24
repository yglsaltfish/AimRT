#include "core/aimrt_core.h"

#include <fstream>
#include <iostream>

namespace aimrt::runtime::core {

AimRTCore::AimRTCore()
    : logger_ptr_(std::make_shared<common::util::LoggerWrapper>()) {}

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

  // init configurator
  EnterState(State::PreInitConfigurator);
  configurator_manager_.SetLogger(logger_ptr_);
  configurator_manager_.Initialize(options_.cfg_file_path);
  AIMRT_INFO("Configurator init complete.");
  EnterState(State::PostInitConfigurator);

  // init plugin
  EnterState(State::PreInitPlugin);
  plugin_manager_.SetLogger(logger_ptr_);
  plugin_manager_.RegisterPluginInitFunc(
      [this](AimRTCorePluginBase* plugin_ptr) { plugin_ptr->Initialize(this); });
  plugin_manager_.Initialize(configurator_manager_.GetAimRTOptionsNode("plugin"));
  AIMRT_INFO("Plugin init complete.");
  EnterState(State::PostInitPlugin);

  // init main thread executor
  EnterState(State::PreInitMainThread);
  main_thread_executor_.SetLogger(logger_ptr_);
  main_thread_executor_.Initialize(configurator_manager_.GetAimRTOptionsNode("main_thread"));
  AIMRT_INFO("Main thread executor init complete.");
  EnterState(State::PostInitMainThread);

  // init allocator
  EnterState(State::PreInitAllocator);
  allocator_manager_.SetLogger(logger_ptr_);
  allocator_manager_.Initialize(configurator_manager_.GetAimRTOptionsNode("allocator"));
  AIMRT_INFO("Allocator init complete.");
  EnterState(State::PostInitAllocator);

  // init log
  EnterState(State::PreInitLog);
  logger_manager_.SetLogger(logger_ptr_);
  logger_manager_.SetLogExecutor(
      aimrt::executor::ExecutorRef(main_thread_executor_.NativeHandle()));
  logger_manager_.Initialize(configurator_manager_.GetAimRTOptionsNode("log"));
  SetCoreLogger();
  AIMRT_INFO("Logger init complete.");
  EnterState(State::PostInitLog);

  // init executor
  EnterState(State::PreInitExecutor);
  executor_manager_.SetLogger(logger_ptr_);
  executor_manager_.Initialize(configurator_manager_.GetAimRTOptionsNode("executor"));
  AIMRT_INFO("Executor init complete.");
  EnterState(State::PostInitExecutor);

  // init rpc
  EnterState(State::PreInitRpc);
  rpc_manager_.SetLogger(logger_ptr_);
  rpc_manager_.RegisterGetExecutorFunc(
      std::bind(&AimRTCore::GetExecutor, this, std::placeholders::_1));
  rpc_manager_.Initialize(configurator_manager_.GetAimRTOptionsNode("rpc"));
  AIMRT_INFO("Rpc init complete.");
  EnterState(State::PostInitRpc);

  // init channel
  EnterState(State::PreInitChannel);
  channel_manager_.SetLogger(logger_ptr_);
  channel_manager_.RegisterGetExecutorFunc(
      std::bind(&AimRTCore::GetExecutor, this, std::placeholders::_1));
  channel_manager_.Initialize(configurator_manager_.GetAimRTOptionsNode("channel"));
  AIMRT_INFO("Channel init complete.");
  EnterState(State::PostInitChannel);

  // init parameter
  EnterState(State::PreInitParameter);
  parameter_manager_.SetLogger(logger_ptr_);
  parameter_manager_.Initialize(configurator_manager_.GetAimRTOptionsNode("parameter"));
  AIMRT_INFO("Parameter init complete.");
  EnterState(State::PostInitParameter);

  // init modules
  EnterState(State::PreInitModules);
  module_manager_.SetLogger(logger_ptr_);
  module_manager_.RegisterCoreProxyConfigurator(
      std::bind(&AimRTCore::InitCoreProxy, this, std::placeholders::_1, std::placeholders::_2));
  module_manager_.Initialize(configurator_manager_.GetAimRTOptionsNode("module"));
  AIMRT_INFO("All modules init complete.");
  EnterState(State::PostInitModules);

  // dump cfg file
  if (options_.dump_cfg_file)
    DumpCfgFile(options_.dump_cfg_file_path);

  EnterState(State::PostInit);
}

void AimRTCore::Start() {
  // start modules
  EnterState(State::PreStart);
  configurator_manager_.Start();
  plugin_manager_.Start();
  allocator_manager_.Start();
  logger_manager_.Start();
  executor_manager_.Start();
  rpc_manager_.Start();
  channel_manager_.Start();
  parameter_manager_.Start();
  module_manager_.Start();
  AIMRT_INFO("All modules start complete.");
  EnterState(State::PostStart);

  main_thread_executor_.Start();
}

void AimRTCore::Shutdown() {
  if (std::atomic_exchange(&stop_flag_, true)) return;

  auto stop_work = [this]() {
    EnterState(State::PreShutdown);

    AIMRT_INFO("Shutdown.");

    module_manager_.Shutdown();
    parameter_manager_.Shutdown();
    channel_manager_.Shutdown();
    rpc_manager_.Shutdown();
    executor_manager_.Shutdown();
    logger_manager_.Shutdown();
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

void AimRTCore::SetCoreLogger() {
  const auto* core_logger_ptr = logger_manager_.GetLoggerProxy("core").NativeHandle();
  const auto* core_allocator_ptr = allocator_manager_.GetAllocatorProxy().NativeHandle();

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
    constexpr size_t kMaxLogBufSize = 1024 * 4;
    void* buf = core_allocator_ptr->get_thread_local_buf(core_allocator_ptr->impl, kMaxLogBufSize);
    return {static_cast<char*>(buf), kMaxLogBufSize};
  };
}

void AimRTCore::InitCoreProxy(const util::ModuleDetailInfo& info, module::CoreProxy& proxy) {
  proxy.SetConfigurator(configurator_manager_.GetConfiguratorProxy(info).NativeHandle());
  proxy.SetAllocator(allocator_manager_.GetAllocatorProxy(info).NativeHandle());
  proxy.SetLogger(logger_manager_.GetLoggerProxy(info).NativeHandle());
  proxy.SetExecutorManager(executor_manager_.GetExecutorManagerProxy(info).NativeHandle());
  proxy.SetRpcHandle(rpc_manager_.GetRpcHandleProxy(info).NativeHandle());
  proxy.SetChannelHandle(channel_manager_.GetChannelHandleProxy(info).NativeHandle());
  proxy.SetParameterHandle(parameter_manager_.GetParameterHandleProxy(info).NativeHandle());
}

void AimRTCore::DumpCfgFile(const std::string& path) {
  YAML::Node dump_node = configurator_manager_.DumpRootOptionsNode();

  if (!path.empty()) {
    std::ofstream ofs;
    ofs.open(path, std::ios::trunc);
    ofs << dump_node;
    ofs.flush();
    ofs.clear();
    ofs.close();
  } else {
    std::cout << "dump cfg file:\n\n"
              << dump_node << "\n\n"
              << std::endl;
  }
}

}  // namespace aimrt::runtime::core
