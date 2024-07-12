#include <csignal>
#include <iostream>

#include "gflags/gflags.h"

#include "core/aimrt_core.h"

DEFINE_string(cfg_file_path, "", "config file path");

DEFINE_bool(dump_cfg_file, false, "dump config file");
DEFINE_string(dump_cfg_file_path, "", "dump config file path");

DEFINE_bool(register_signal, true, "register handle for sigint and sigterm");
DEFINE_int32(running_duration, 0, "running duration, seconds");

using namespace aimrt::runtime::core;

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

    AimRTCore::Options options;
    options.cfg_file_path = FLAGS_cfg_file_path;
    options.dump_cfg_file = FLAGS_dump_cfg_file;
    options.dump_cfg_file_path = FLAGS_dump_cfg_file_path;
    core.Initialize(options);

    if (FLAGS_running_duration == 0) {
      core.Start();

      core.Shutdown();
    } else {
      auto fu = core.AsyncStart();

      std::this_thread::sleep_for(std::chrono::seconds(FLAGS_running_duration));

      core.Shutdown();

      fu.wait();
    }

    global_core_ptr_ = nullptr;
  } catch (const std::exception& e) {
    std::cout << "AimRT run with exception and exit. " << e.what()
              << std::endl;
    return -1;
  }

  std::cout << "AimRT exit." << std::endl;

  return 0;
}
