// Copyright (c) 2023, AgiBot Inc.
// All rights reserved.

#pragma once

#include "core/logger/logger_manager.h"
#include "record_playback_plugin/playback_action.h"
#include "record_playback_plugin/record_action.h"

#include "record_playback.aimrt_rpc.pb.h"

namespace aimrt::plugins::record_playback_plugin {

class RecordPlaybackServiceImpl : public aimrt::protocols::record_playback_plugin::RecordPlaybackServiceCoService {
 public:
  RecordPlaybackServiceImpl() = default;
  ~RecordPlaybackServiceImpl() override = default;

  void SetRecordActionMap(
      std::unordered_map<std::string_view, std::unique_ptr<RecordAction>>* record_action_map_ptr) {
    record_action_map_ptr_ = record_action_map_ptr;
  }

  void SetPlaybackActionMap(
      std::unordered_map<std::string_view, std::unique_ptr<PlaybackAction>>* playback_action_map_ptr) {
    playback_action_map_ptr_ = playback_action_map_ptr;
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
    START_RECORD_FAILED = 3,
    START_PLAYBACK_FAILED = 4,
  };

  static constexpr std::string_view error_info_array[] = {
      "",
      "INVALID_ACTION_NAME",
      "INVALID_ACTION_MODE",
      "START_RECORD_FAILED",
      "START_PLAYBACK_FAILED"};

  template <typename T>
  void SetErrorCode(ErrorCode code, T& rsp) {
    rsp.set_code(static_cast<uint32_t>(code));
    rsp.set_msg(std::string(error_info_array[static_cast<uint32_t>(code)]));
  }

  std::unordered_map<std::string_view, std::unique_ptr<RecordAction>>* record_action_map_ptr_ = nullptr;
  std::unordered_map<std::string_view, std::unique_ptr<PlaybackAction>>* playback_action_map_ptr_ = nullptr;
};

}  // namespace aimrt::plugins::record_playback_plugin
