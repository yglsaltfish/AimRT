#include "core/aimrt_core.h"

#include <fstream>
#include <iostream>

#include "core/global.h"

namespace aimrt::runtime::core {

AimRTCore::~AimRTCore() {
  try {
    Shutdown();
  } catch (const std::exception& e) {
    AIMRT_INFO("AimRTCore destruct get exception, {}", e.what());
  }
}

void AimRTCore::Initialize(const Options& options) {
  options_ = options;

  // init configurator
  configurator_manager_.Initialize(options_.cfg_file_path);
  AIMRT_INFO("Configurator init complete.");

  // init plugin
  plugin_manager_.RegisterPluginInitFunc(
      [this](AimRTCorePluginBase* plugin_ptr) { plugin_ptr->Initialize(this); });
  plugin_manager_.Initialize(configurator_manager_.GetAimRTOptionsNode("plugin"));
  AIMRT_INFO("Plugin init complete.");

  // init main thread executor
  ExecuteHook(HookPoint::PreInitMainThread);
  main_thread_executor_.Initialize(configurator_manager_.GetAimRTOptionsNode("main_thread"));
  AIMRT_INFO("Main thread executor init complete.");
  ExecuteHook(HookPoint::PostInitMainThread);

  // init log
  ExecuteHook(HookPoint::PreInitLog);
  logger_manager_.SetLogExecutor(
      aimrt::executor::ExecutorRef(main_thread_executor_.NativeHandle()));
  logger_manager_.Initialize(configurator_manager_.GetAimRTOptionsNode("log"));
  AIMRT_INFO("Logger init complete.");
  ExecuteHook(HookPoint::PostInitLog);

  // init executor
  ExecuteHook(HookPoint::PreInitExecutor);
  executor_manager_.Initialize(configurator_manager_.GetAimRTOptionsNode("executor"));
  AIMRT_INFO("Executor init complete.");
  ExecuteHook(HookPoint::PostInitExecutor);

  // init rpc
  ExecuteHook(HookPoint::PreInitRpc);
  rpc_manager_.RegisterGetExecutorFunc(
      std::bind(&AimRTCore::GetExecutor, this, std::placeholders::_1));
  rpc_manager_.Initialize(configurator_manager_.GetAimRTOptionsNode("rpc"));
  AIMRT_INFO("Rpc init complete.");
  ExecuteHook(HookPoint::PostInitRpc);

  // init channel
  ExecuteHook(HookPoint::PreInitChannel);
  channel_manager_.RegisterGetExecutorFunc(
      std::bind(&AimRTCore::GetExecutor, this, std::placeholders::_1));
  channel_manager_.Initialize(configurator_manager_.GetAimRTOptionsNode("channel"));
  AIMRT_INFO("Channel init complete.");
  ExecuteHook(HookPoint::PostInitChannel);

  // init modules
  ExecuteHook(HookPoint::PreInitModules);
  module_manager_.RegisterCoreProxyConfigurator(
      std::bind(&AimRTCore::InitCoreProxy, this, std::placeholders::_1, std::placeholders::_2));
  module_manager_.Initialize(configurator_manager_.GetAimRTOptionsNode("module"));
  AIMRT_INFO("All modules init complete.");
  ExecuteHook(HookPoint::PostInitModules);

  // dump cfg file
  if (options_.dump_cfg_file) {
    YAML::Node dump_node = configurator_manager_.DumpRootOptionsNode();

    if (!options_.dump_cfg_file_path.empty()) {
      std::ofstream ofs;
      ofs.open(options_.dump_cfg_file_path, std::ios::trunc);
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
}

void AimRTCore::Start() {
  // start modules
  ExecuteHook(HookPoint::PreStart);
  configurator_manager_.Start();
  plugin_manager_.Start();
  logger_manager_.Start();
  executor_manager_.Start();
  rpc_manager_.Start();
  channel_manager_.Start();
  module_manager_.Start();
  AIMRT_INFO("All modules start complete.");
  ExecuteHook(HookPoint::PostStart);

  RegisterSignalHandle(
      std::set<int>{SIGINT, SIGTERM}, [this](auto) { Shutdown(); });

  main_thread_executor_.Start();
}

void AimRTCore::Shutdown() {
  if (std::atomic_exchange(&stop_flag_, true)) return;

  auto stop_work = [this]() {
    ExecuteHook(HookPoint::PreShutdown);

    module_manager_.Shutdown();
    channel_manager_.Shutdown();
    rpc_manager_.Shutdown();
    executor_manager_.Shutdown();
    logger_manager_.Shutdown();
    plugin_manager_.Shutdown();
    configurator_manager_.Shutdown();

    main_thread_executor_.Shutdown();

    ExecuteHook(HookPoint::PostShutdown);
  };

  if (main_thread_executor_.IsStart()) {
    main_thread_executor_.Execute(std::move(stop_work));
  } else {
    stop_work();
  }
}

void AimRTCore::InitCoreProxy(const util::ModuleDetailInfo& info, module::CoreProxy& proxy) {
  proxy.SetConfigurator(configurator_manager_.GetConfiguratorProxy(info).NativeHandle());
  proxy.SetLogger(logger_manager_.GetLoggerProxy(info).NativeHandle());
  proxy.SetExecutorManager(executor_manager_.GetExecutorManagerProxy(info).NativeHandle());
  proxy.SetRpcHandle(rpc_manager_.GetRpcHandleProxy(info).NativeHandle());
  proxy.SetChannel(channel_manager_.GetChannelProxy(info).NativeHandle());
}

}  // namespace aimrt::runtime::core
