#pragma once

#include "aimrt_module_cpp_interface/logger/logger.h"

namespace aimrt::examples::example_normal_rpc::normal_rpc_server_module {

void SetLogger(aimrt::logger::LoggerRef);
aimrt::logger::LoggerRef GetLogger();

}  // namespace aimrt::examples::example_normal_rpc::normal_rpc_server_module
