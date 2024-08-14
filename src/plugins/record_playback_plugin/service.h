#pragma once

#include "core/logger/logger_manager.h"

#include "record_playback.aimrt_rpc.pb.h"

namespace aimrt::plugins::record_playback_plugin {

class RecordPlaybackPlugin;

class RecordPlaybackServiceImpl : public aimrt::protocols::record_playback_plugin::RecordPlaybackServiceCoService {
 public:
  RecordPlaybackServiceImpl() = default;
  ~RecordPlaybackServiceImpl() override = default;

  void SetPluginPtr(RecordPlaybackPlugin* ptr) {
    plugin_ptr_ = ptr;
  }

  aimrt::co::Task<aimrt::rpc::Status> StartRecord(
      aimrt::rpc::ContextRef ctx_ref,
      const ::aimrt::protocols::record_playback_plugin::StartRecordReq& req,
      ::aimrt::protocols::record_playback_plugin::CommonRsp& rsp) override;

  aimrt::co::Task<aimrt::rpc::Status> StopRecord(
      aimrt::rpc::ContextRef ctx_ref,
      const ::aimrt::protocols::record_playback_plugin::StopRecordReq& req,
      ::aimrt::protocols::record_playback_plugin::CommonRsp& rsp) override;

  aimrt::co::Task<aimrt::rpc::Status> StartPlayback(
      aimrt::rpc::ContextRef ctx_ref,
      const ::aimrt::protocols::record_playback_plugin::StartPlaybackReq& req,
      ::aimrt::protocols::record_playback_plugin::CommonRsp& rsp) override;

  aimrt::co::Task<aimrt::rpc::Status> StopPlayback(
      aimrt::rpc::ContextRef ctx_ref,
      const ::aimrt::protocols::record_playback_plugin::StopPlaybackReq& req,
      ::aimrt::protocols::record_playback_plugin::CommonRsp& rsp) override;

 private:
  enum class ErrorCode : uint32_t {
    SUC = 0,
    INVALID_ACTION_NAME = 1,
    INVALID_ACTION_MODE = 2,
    RECORD_IS_ALREADY_IN_PROGRESS = 3,
    PLAYBACK_IS_ALREADY_IN_PROGRESS = 4,
  };

  static constexpr std::string_view error_info_array[] = {
      "",
      "INVALID_ACTION_NAME",
      "INVALID_ACTION_MODE",
      "RECORD_IS_ALREADY_IN_PROGRESS",
      "PLAYBACK_IS_ALREADY_IN_PROGRESS"};

  template <typename T>
  void SetErrorCode(ErrorCode code, T& rsp) {
    rsp.set_code(static_cast<uint32_t>(code));
    rsp.set_msg(std::string(error_info_array[static_cast<uint32_t>(code)]));
  }

  RecordPlaybackPlugin* plugin_ptr_ = nullptr;
};

}  // namespace aimrt::plugins::record_playback_plugin
