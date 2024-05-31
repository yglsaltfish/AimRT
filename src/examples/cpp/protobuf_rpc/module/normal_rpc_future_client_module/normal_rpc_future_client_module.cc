#include "normal_rpc_future_client_module/normal_rpc_future_client_module.h"
#include "aimrt_module_protobuf_interface/util/protobuf_tools.h"

#include "yaml-cpp/yaml.h"

namespace aimrt::examples::cpp::protobuf_rpc::normal_rpc_future_client_module {

bool NormalRpcFutureClientModule::Initialize(aimrt::CoreRef core) {
  core_ = core;

  try {
    // Read cfg
    std::string file_path = std::string(core_.GetConfigurator().GetConfigFilePath());
    if (!file_path.empty()) {
      YAML::Node cfg_node = YAML::LoadFile(file_path);
      rpc_frq_ = cfg_node["rpc_frq"].as<double>();
    }

    // Get executor handle
    executor_ = core_.GetExecutorManager().GetExecutor("work_thread_pool");
    AIMRT_CHECK_ERROR_THROW(executor_, "Get executor 'work_thread_pool' failed.");

    // Get rpc handle
    auto rpc_handle = core_.GetRpcHandle();
    AIMRT_CHECK_ERROR_THROW(rpc_handle, "Get rpc handle failed.");

    // Register rpc client
    bool ret = aimrt::protocols::example::RegisterExampleServiceClientFunc(rpc_handle);
    AIMRT_CHECK_ERROR_THROW(ret, "Register client failed.");

  } catch (const std::exception& e) {
    AIMRT_ERROR("Init failed, {}", e.what());
    return false;
  }

  AIMRT_INFO("Init succeeded.");

  return true;
}

bool NormalRpcFutureClientModule::Start() {
  try {
    executor_.Execute(std::bind(&NormalRpcFutureClientModule::MainLoop, this));
  } catch (const std::exception& e) {
    AIMRT_ERROR("Start failed, {}", e.what());
    return false;
  }

  AIMRT_INFO("Start succeeded.");
  return true;
}

void NormalRpcFutureClientModule::Shutdown() {
  try {
    auto stop_future = stop_sig_.get_future();
    run_flag_ = false;
    stop_future.wait();
  } catch (const std::exception& e) {
    AIMRT_ERROR("Shutdown failed, {}", e.what());
    return;
  }

  AIMRT_INFO("Shutdown succeeded.");
}

// Main loop
void NormalRpcFutureClientModule::MainLoop() {
  try {
    AIMRT_INFO("Start MainLoop.");

    aimrt::protocols::example::ExampleServiceFutureProxy proxy(core_.GetRpcHandle());

    uint32_t count = 0;
    while (run_flag_) {
      std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<uint32_t>(1000 / rpc_frq_)));

      count++;
      AIMRT_INFO("Loop count : {} -------------------------", count);

      // call rpc 1
      {
        aimrt::protocols::example::GetFooDataReq req;
        aimrt::protocols::example::GetFooDataRsp rsp;
        req.set_msg("hello world foo, count " + std::to_string(count));

        auto ctx = proxy.NewContextRef();
        ctx.SetTimeout(std::chrono::seconds(3));

        AIMRT_INFO("Client start new rpc call. req: {}", aimrt::Pb2CompactJson(req));

        auto status_future = proxy.GetFooData(ctx, req, rsp);
        auto status = status_future.get();

        if (status.OK()) {
          AIMRT_INFO("Client get rpc ret, status: {}, rsp: {}", status.ToString(),
                     aimrt::Pb2CompactJson(rsp));
        } else {
          AIMRT_WARN("Client get rpc error ret, status: {}", status.ToString());
        }
      }

      // call rpc 2
      {
        aimrt::protocols::example::GetBarDataReq req;
        aimrt::protocols::example::GetBarDataRsp rsp;
        req.set_msg("hello world bar, count " + std::to_string(count));

        auto ctx = proxy.NewContextRef();
        ctx.SetTimeout(std::chrono::seconds(3));

        AIMRT_INFO("Client start new rpc call. req: {}", aimrt::Pb2CompactJson(req));

        auto status_future = proxy.GetBarData(ctx, req, rsp);
        auto status = status_future.get();

        if (status.OK()) {
          AIMRT_INFO("Client get rpc ret, status: {}, rsp: {}", status.ToString(),
                     aimrt::Pb2CompactJson(rsp));
        } else {
          AIMRT_WARN("Client get rpc error ret, status: {}", status.ToString());
        }
      }
    }

    AIMRT_INFO("Exit MainLoop.");
  } catch (const std::exception& e) {
    AIMRT_ERROR("Exit MainLoop with exception, {}", e.what());
  }

  stop_sig_.set_value();
}

}  // namespace aimrt::examples::cpp::protobuf_rpc::normal_rpc_future_client_module
