#include "core/aimrt_core.h"

#include <fstream>
#include <iostream>

#include "core/global.h"

namespace aimrt::runtime::core {

AimRTCore* AimRTCore::global_core_ptr_ = nullptr;

AimRTCore::AimRTCore() {}

AimRTCore::~AimRTCore() {
  try {
    Shutdown();
  } catch (const std::exception& e) {
    AIMRT_INFO("AimRTCore destruct get exception, {}", e.what());
  }

  UnSetGlobal();
}

void AimRTCore::Initialize(const Options& options) {
  EnterState(State::PreInit);

  options_ = options;

  if (options_.auto_set_to_global) SetGlobal();

  // init configurator
  EnterState(State::PreInitConfigurator);
  configurator_manager_.Initialize(options_.cfg_file_path);
  AIMRT_INFO("Configurator init complete.");
  EnterState(State::PostInitConfigurator);

  // init plugin
  EnterState(State::PreInitPlugin);
  plugin_manager_.RegisterPluginInitFunc(
      [this](AimRTCorePluginBase* plugin_ptr) { plugin_ptr->Initialize(this); });
  plugin_manager_.Initialize(configurator_manager_.GetAimRTOptionsNode("plugin"));
  AIMRT_INFO("Plugin init complete.");
  EnterState(State::PostInitPlugin);

  // init main thread executor
  EnterState(State::PreInitMainThread);
  main_thread_executor_.Initialize(configurator_manager_.GetAimRTOptionsNode("main_thread"));
  AIMRT_INFO("Main thread executor init complete.");
  EnterState(State::PostInitMainThread);

  // init log
  EnterState(State::PreInitLog);
  logger_manager_.SetLogExecutor(
      aimrt::executor::ExecutorRef(main_thread_executor_.NativeHandle()));
  logger_manager_.Initialize(configurator_manager_.GetAimRTOptionsNode("log"));
  AIMRT_INFO("Logger init complete.");
  EnterState(State::PostInitLog);

  // init executor
  EnterState(State::PreInitExecutor);
  executor_manager_.Initialize(configurator_manager_.GetAimRTOptionsNode("executor"));
  AIMRT_INFO("Executor init complete.");
  EnterState(State::PostInitExecutor);

  // init rpc
  EnterState(State::PreInitRpc);
  rpc_manager_.RegisterGetExecutorFunc(
      std::bind(&AimRTCore::GetExecutor, this, std::placeholders::_1));
  rpc_manager_.Initialize(configurator_manager_.GetAimRTOptionsNode("rpc"));
  AIMRT_INFO("Rpc init complete.");
  EnterState(State::PostInitRpc);

  // init channel
  EnterState(State::PreInitChannel);
  channel_manager_.RegisterGetExecutorFunc(
      std::bind(&AimRTCore::GetExecutor, this, std::placeholders::_1));
  channel_manager_.Initialize(configurator_manager_.GetAimRTOptionsNode("channel"));
  AIMRT_INFO("Channel init complete.");
  EnterState(State::PostInitChannel);

  // init modules
  EnterState(State::PreInitModules);
  module_manager_.RegisterCoreProxyConfigurator(
      std::bind(&AimRTCore::InitCoreProxy, this, std::placeholders::_1, std::placeholders::_2));
  module_manager_.Initialize(configurator_manager_.GetAimRTOptionsNode("module"));
  AIMRT_INFO("All modules init complete.");
  EnterState(State::PostInitModules);

  // dump cfg file
  DumpCfgFile();

  // register signal handle
  RegisterSignalToSystem();

  EnterState(State::PostInit);
}

void AimRTCore::Start() {
  // start modules
  EnterState(State::PreStart);
  configurator_manager_.Start();
  plugin_manager_.Start();
  logger_manager_.Start();
  executor_manager_.Start();
  rpc_manager_.Start();
  channel_manager_.Start();
  module_manager_.Start();
  AIMRT_INFO("All modules start complete.");
  EnterState(State::PostStart);

  main_thread_executor_.Start();
}

void AimRTCore::Shutdown() {
  if (std::atomic_exchange(&stop_flag_, true)) return;

  auto stop_work = [this]() {
    EnterState(State::PreShutdown);

    module_manager_.Shutdown();
    channel_manager_.Shutdown();
    rpc_manager_.Shutdown();
    executor_manager_.Shutdown();
    logger_manager_.Shutdown();
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

void AimRTCore::InitCoreProxy(const util::ModuleDetailInfo& info, module::CoreProxy& proxy) {
  proxy.SetConfigurator(configurator_manager_.GetConfiguratorProxy(info).NativeHandle());
  proxy.SetLogger(logger_manager_.GetLoggerProxy(info).NativeHandle());
  proxy.SetExecutorManager(executor_manager_.GetExecutorManagerProxy(info).NativeHandle());
  proxy.SetRpcHandle(rpc_manager_.GetRpcHandleProxy(info).NativeHandle());
  proxy.SetChannel(channel_manager_.GetChannelProxy(info).NativeHandle());
  proxy.SetAllocator(nullptr);
}

void AimRTCore::DumpCfgFile() {
  if (!options_.dump_cfg_file) return;

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

void AimRTCore::RegisterSignalToSystem() {
  if (!options_.register_signal) return;

  RegisterSignalHandle(std::set<int>{SIGINT, SIGTERM}, [this](auto) { Shutdown(); });

  std::set<int> signals;
  for (auto& itr : signal_handle_vec_) {
    signals.insert(itr.first.begin(), itr.first.end());
  }

  for (auto sig : signals) {
    signal(sig, SignalHandler);
  }
}

void AimRTCore::SignalHandler(int sig) {
  AimRTCore* core_ptr = GetGlobalAimRTCore();

  if (core_ptr) {
    const auto& signal_handle_vec = core_ptr->signal_handle_vec_;
    for (const auto& itr : signal_handle_vec) {
      if (itr.first.find(sig) != itr.first.end()) itr.second(sig);
    }

    return;
  }

  AIMRT_WARN("Global AimRTCore pointer is null when handle sig '{}'", sig);

  raise(sig);
}

}  // namespace aimrt::runtime::core
