#include "record_playback_plugin/service.h"
#include "aimrt_module_protobuf_interface/util/protobuf_tools.h"
#include "record_playback_plugin/global.h"

namespace aimrt::plugins::record_playback_plugin {

aimrt::co::Task<aimrt::rpc::Status> RecordPlaybackServiceImpl::StartRecord(
    aimrt::rpc::ContextRef ctx_ref,
    const ::aimrt::protocols::record_playback_plugin::StartRecordReq& req,
    ::aimrt::protocols::record_playback_plugin::CommonRsp& rsp) {
  co_return aimrt::rpc::Status();
}

aimrt::co::Task<aimrt::rpc::Status> RecordPlaybackServiceImpl::StartPlayback(
    aimrt::rpc::ContextRef ctx_ref,
    const ::aimrt::protocols::record_playback_plugin::StartPlaybackReq& req,
    ::aimrt::protocols::record_playback_plugin::CommonRsp& rsp) {
  co_return aimrt::rpc::Status();
}

aimrt::co::Task<aimrt::rpc::Status> RecordPlaybackServiceImpl::StopAction(
    aimrt::rpc::ContextRef ctx_ref,
    const ::aimrt::protocols::record_playback_plugin::StopActionReq& req,
    ::aimrt::protocols::record_playback_plugin::CommonRsp& rsp) {
  co_return aimrt::rpc::Status();
}

aimrt::co::Task<aimrt::rpc::Status> RecordPlaybackServiceImpl::StartActions(
    aimrt::rpc::ContextRef ctx_ref,
    const ::aimrt::protocols::record_playback_plugin::StartActionsReq& req,
    ::aimrt::protocols::record_playback_plugin::CommonRsp& rsp) {
  co_return aimrt::rpc::Status();
}

aimrt::co::Task<aimrt::rpc::Status> RecordPlaybackServiceImpl::StopActions(
    aimrt::rpc::ContextRef ctx_ref,
    const ::aimrt::protocols::record_playback_plugin::StopActionsReq& req,
    ::aimrt::protocols::record_playback_plugin::CommonRsp& rsp) {
  co_return aimrt::rpc::Status();
}

}  // namespace aimrt::plugins::record_playback_plugin
