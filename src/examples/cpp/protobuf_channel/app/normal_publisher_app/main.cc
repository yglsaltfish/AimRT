#include <csignal>
#include <iostream>

#include "core/aimrt_core.h"

#include "aimrt_module_cpp_interface/core.h"
#include "aimrt_module_protobuf_interface/channel/protobuf_channel.h"
#include "aimrt_module_protobuf_interface/util/protobuf_tools.h"

#include "event.pb.h"

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
  signal(SIGINT, SignalHandler);
  signal(SIGTERM, SignalHandler);

  std::cout << "AimRT start." << std::endl;

  try {
    AimRTCore core;
    global_core_ptr_ = &core;

    // Initialize
    AimRTCore::Options options;
    if (argc > 1) options.cfg_file_path = argv[1];

    core.Initialize(options);

    // Create Module
    aimrt::CoreRef module_handle(
        core.GetModuleManager().CreateModule("NormalPublisherModule"));

    // Register publish type
    std::string topic_name = "test_topic";
    auto publisher = module_handle.GetChannelHandle().GetPublisher(topic_name);
    AIMRT_HL_CHECK_ERROR_THROW(module_handle.GetLogger(),
                               publisher, "Get publisher for topic '{}' failed.", topic_name);

    bool ret = aimrt::channel::RegisterPublishType<aimrt::protocols::example::ExampleEventMsg>(publisher);
    AIMRT_HL_CHECK_ERROR_THROW(module_handle.GetLogger(), ret, "Register publish type failed.");

    // Start
    auto fu = core.AsyncStart();

    // Publish event
    aimrt::protocols::example::ExampleEventMsg msg;
    msg.set_msg("example msg");
    msg.set_num(123456);

    AIMRT_HL_INFO(module_handle.GetLogger(), "Publish new pb event, data: {}", aimrt::Pb2CompactJson(msg));
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
