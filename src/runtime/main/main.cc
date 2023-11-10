#include <iostream>

#include "gflags/gflags.h"

#include "core/aimrt_core.h"

DEFINE_string(cfg_file_path, "", "config file path");
DEFINE_bool(dump_cfg_file, false, "dump config file");
DEFINE_string(dump_cfg_file_path, "", "dump config file path");

int32_t main(int32_t argc, char** argv) {
  gflags::ParseCommandLineFlags(&argc, &argv, true);

  std::cout << "AimRT start with cfg file: " << FLAGS_cfg_file_path
            << std::endl;

  aimrt::runtime::core::AimRTCore::Options options;
  options.cfg_file_path = FLAGS_cfg_file_path;
  options.dump_cfg_file = FLAGS_dump_cfg_file;
  options.dump_cfg_file_path = FLAGS_dump_cfg_file_path;

  try {
    aimrt::runtime::core::AimRTCore core;

    core.Initialize(options);

    core.Start();

    core.Shutdown();

  } catch (const std::exception& e) {
    std::cout << "AimRT run with exception and exit. " << e.what()
              << std::endl;
    return -1;
  }

  std::cout << "AimRT exit." << std::endl;

  return 0;
}
