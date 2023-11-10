#include "normal_rpc_server_module/rpc_service.h"
#include "aimrt_module_protobuf_interface/util/protobuf_tools.h"
#include "normal_rpc_server_module/global.h"

namespace aimrt::examples::example_normal_rpc::normal_rpc_server_module {

aimrt::co::Task<aimrt::rpc::Status> HardwareServiceImpl::GetFooData(
    aimrt::rpc::ContextRef ctx,
    const ::aimrt::protocols::example::GetFooDataReq& req,
    ::aimrt::protocols::example::GetFooDataRsp& rsp) {
  rsp.set_msg("echo " + req.msg());

  AIMRT_INFO("Server handle new rpc call. req: {}, return rsp: {}",
             aimrt::Pb2CompactJson(req), aimrt::Pb2CompactJson(rsp));

  co_return aimrt::rpc::Status();
}

aimrt::co::Task<aimrt::rpc::Status> HardwareServiceImpl::GetBarData(
    aimrt::rpc::ContextRef ctx,
    const ::aimrt::protocols::example::GetBarDataReq& req,
    ::aimrt::protocols::example::GetBarDataRsp& rsp) {
  rsp.set_msg("echo " + req.msg());

  AIMRT_INFO("Server handle new rpc call. req: {}, return rsp: {}",
             aimrt::Pb2CompactJson(req), aimrt::Pb2CompactJson(rsp));

  co_return aimrt::rpc::Status();
}

}  // namespace aimrt::examples::example_normal_rpc::normal_rpc_server_module
