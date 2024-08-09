#pragma once

#include "core/logger/logger_manager.h"

#include "record_playback.aimrt_rpc.pb.h"

namespace aimrt::plugins::record_playback_plugin {

class RecordPlaybackServiceImpl : public aimrt::protocols::record_playback_plugin::RecordPlaybackServiceCoService {
 public:
  RecordPlaybackServiceImpl() = default;
  ~RecordPlaybackServiceImpl() override = default;

  aimrt::co::Task<aimrt::rpc::Status> StartRecord(
      aimrt::rpc::ContextRef ctx_ref,
      const ::aimrt::protocols::record_playback_plugin::StartRecordReq& req,
      ::aimrt::protocols::record_playback_plugin::CommonRsp& rsp) override;

  aimrt::co::Task<aimrt::rpc::Status> StartPlayback(
      aimrt::rpc::ContextRef ctx_ref,
      const ::aimrt::protocols::record_playback_plugin::StartPlaybackReq& req,
      ::aimrt::protocols::record_playback_plugin::CommonRsp& rsp) override;

  aimrt::co::Task<aimrt::rpc::Status> StopAction(
      aimrt::rpc::ContextRef ctx_ref,
      const ::aimrt::protocols::record_playback_plugin::StopActionReq& req,
      ::aimrt::protocols::record_playback_plugin::CommonRsp& rsp) override;

  aimrt::co::Task<aimrt::rpc::Status> StartActions(
      aimrt::rpc::ContextRef ctx_ref,
      const ::aimrt::protocols::record_playback_plugin::StartActionsReq& req,
      ::aimrt::protocols::record_playback_plugin::CommonRsp& rsp) override;

  aimrt::co::Task<aimrt::rpc::Status> StopActions(
      aimrt::rpc::ContextRef ctx_ref,
      const ::aimrt::protocols::record_playback_plugin::StopActionsReq& req,
      ::aimrt::protocols::record_playback_plugin::CommonRsp& rsp) override;

 private:
  enum class ErrorCode : uint32_t {
    SUC = 0,
    INVALID_MODULE_NAME = 1,
  };

  static constexpr std::string_view error_info_array[] = {
      "",
      "INVALID_MODULE_NAME"};

  template <typename T>
  void SetErrorCode(ErrorCode code, T& rsp) {
    rsp.set_code(static_cast<uint32_t>(code));
    rsp.set_msg(std::string(error_info_array[static_cast<uint32_t>(code)]));
  }
};

}  // namespace aimrt::plugins::record_playback_plugin
