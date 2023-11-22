#include "ros2_rpc_server_module/global.h"

namespace aimrt::examples::example_ros2_rpc::ros2_rpc_server_module {

aimrt::logger::LoggerRef global_logger;
void SetLogger(aimrt::logger::LoggerRef logger) { global_logger = logger; }
aimrt::logger::LoggerRef GetLogger() { return global_logger; }

}  // namespace aimrt::examples::example_ros2_rpc::ros2_rpc_server_module
