#include "normal_rpc_server_module/global.h"

namespace aimrt::examples::example_normal_rpc::normal_rpc_server_module {

LoggerRef global_logger;
void SetLogger(LoggerRef logger) { global_logger = logger; }
LoggerRef GetLogger() { return global_logger; }

}  // namespace aimrt::examples::example_normal_rpc::normal_rpc_server_module
