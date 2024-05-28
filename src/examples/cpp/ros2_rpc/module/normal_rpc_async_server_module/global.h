#pragma once

#include "aimrt_module_cpp_interface/logger/logger.h"

namespace aimrt::examples::cpp::ros2_rpc::normal_rpc_async_server_module {

void SetLogger(aimrt::logger::LoggerRef);
aimrt::logger::LoggerRef GetLogger();

}  // namespace aimrt::examples::cpp::ros2_rpc::normal_rpc_async_server_module
