#include <csignal>
#include <iostream>

#include "gflags/gflags.h"

#include "core/aimrt_core.h"
#include "helloworld_module/helloworld_module.h"

DEFINE_string(cfg_file_path, "", "config file path");

DEFINE_bool(dump_cfg_file, false, "dump config file");
DEFINE_string(dump_cfg_file_path, "", "dump config file path");

DEFINE_bool(register_signal, true, "register handle for sigint and sigterm");

using namespace aimrt::runtime::core;
using namespace aimrt::examples::cpp::helloworld::helloworld_module;

AimRTCore* global_core_ptr_ = nullptr;

void SignalHandler(int sig) {
  if (global_core_ptr_ && (sig == SIGINT || sig == SIGTERM)) {
    global_core_ptr_->Shutdown();
    return;
  }

  raise(sig);
};

int32_t main(int32_t argc, char** argv) {
  gflags::ParseCommandLineFlags(&argc, &argv, true);

  if (FLAGS_register_signal) {
    signal(SIGINT, SignalHandler);
    signal(SIGTERM, SignalHandler);
  }

  std::cout << "AimRT start." << std::endl;

  try {
    AimRTCore core;
    global_core_ptr_ = &core;

    // register module
    HelloWorldModule helloworld_module;
    core.GetModuleManager().RegisterModule(helloworld_module.NativeHandle());

    AimRTCore::Options options;
    options.cfg_file_path = FLAGS_cfg_file_path;
    options.dump_cfg_file = FLAGS_dump_cfg_file;
    options.dump_cfg_file_path = FLAGS_dump_cfg_file_path;
    core.Initialize(options);

    core.Start();

    core.Shutdown();

    global_core_ptr_ = nullptr;
  } catch (const std::exception& e) {
    std::cout << "AimRT run with exception and exit. " << e.what()
              << std::endl;
    return -1;
  }

  std::cout << "AimRT exit." << std::endl;

  return 0;
}
