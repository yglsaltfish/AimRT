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
        core.GetModuleManager().CreateModule("NormalPublisherModule"));

    // Register publish type
    std::string topic_name = "test_topic";
    auto publisher = core_handle.GetChannelHandle().GetPublisher(topic_name);
    AIMRT_HL_CHECK_ERROR_THROW(core_handle.GetLogger(),
                               publisher, "Get publisher for topic '{}' failed.", topic_name);

    bool ret = aimrt::channel::RegisterPublishType<aimrt::protocols::example::ExampleEventMsg>(publisher);
    AIMRT_HL_CHECK_ERROR_THROW(core_handle.GetLogger(), ret, "Register publish type failed.");

    // Start
    auto fu = core.AsyncStart();

    // Publish event
    aimrt::protocols::example::ExampleEventMsg msg;
    msg.set_msg("example msg");
    msg.set_num(123456);

    AIMRT_HL_INFO(core_handle.GetLogger(), "Publish new pb event, data: {}", aimrt::Pb2CompactJson(msg));
    aimrt::channel::Publish(publisher, msg);

    // Wait
    fu.wait();

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
