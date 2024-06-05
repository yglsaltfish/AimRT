#include <csignal>
#include <iostream>

#include "gflags/gflags.h"

#include "core/aimrt_core.h"

#include "aimrt_module_cpp_interface/core.h"
#include "aimrt_module_protobuf_interface/channel/protobuf_channel.h"
#include "aimrt_module_protobuf_interface/util/protobuf_tools.h"

#include "event.pb.h"

DEFINE_string(cfg_file_path, "", "config file path");

DEFINE_bool(dump_cfg_file, false, "dump config file");
DEFINE_string(dump_cfg_file_path, "", "dump config file path");

DEFINE_bool(register_signal, true, "register handle for sigint and sigterm");

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

    // Initialize
    AimRTCore::Options options;
    options.cfg_file_path = FLAGS_cfg_file_path;
    options.dump_cfg_file = FLAGS_dump_cfg_file;
    options.dump_cfg_file_path = FLAGS_dump_cfg_file_path;
    core.Initialize(options);

    // Create Module
    aimrt::CoreRef core_handle(
        core.GetModuleManager().CreateModule("NormalSubscriberModule"));

    // Subscribe event
    std::string topic_name = "test_topic";
    auto subscriber = core_handle.GetChannelHandle().GetSubscriber(topic_name);
    AIMRT_HL_CHECK_ERROR_THROW(core_handle.GetLogger(),
                               subscriber, "Get subscriber for topic '{}' failed.", topic_name);

    bool ret = aimrt::channel::Subscribe<aimrt::protocols::example::ExampleEventMsg>(
        subscriber,
        [core_handle](const std::shared_ptr<const aimrt::protocols::example::ExampleEventMsg>& data) {
          AIMRT_HL_INFO(core_handle.GetLogger(),
                        "Receive new pb event, data: {}", aimrt::Pb2CompactJson(*data));
        });
    AIMRT_HL_CHECK_ERROR_THROW(core_handle.GetLogger(), ret, "Subscribe failed.");

    // Start
    core.Start();

    // Shutdown
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
