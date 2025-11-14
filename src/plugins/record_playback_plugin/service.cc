// Copyright (c) 2023, AgiBot Inc.
// All rights reserved.

#include "record_playback_plugin/service.h"
#include "co/task.h"
#include "record_playback_plugin/topic_meta.h"

namespace aimrt::plugins::record_playback_plugin {

aimrt::co::Task<aimrt::rpc::Status> RecordPlaybackServiceImpl::StartRecord(
    aimrt::rpc::ContextRef ctx_ref,
    const ::aimrt::protocols::record_playback_plugin::StartRecordReq& req,
    ::aimrt::protocols::record_playback_plugin::StartRecordRsp& rsp) {
  auto finditr = record_action_map_ptr_->find(req.action_name());
  auto common_rsp = rsp.mutable_common_rsp();

  if (finditr == record_action_map_ptr_->end()) {
    SetErrorCode(ErrorCode::kInvalidActionName, *common_rsp);
    co_return aimrt::rpc::Status();
  }

  auto& action_wrapper = *(finditr->second);

  if (action_wrapper.GetOptions().mode != RecordAction::Options::Mode::kSignal) {
    SetErrorCode(ErrorCode::kInvalidActionMode, *common_rsp);
    co_return aimrt::rpc::Status();
  }

  uint64_t preparation_duration_s = req.preparation_duration_s();
  uint64_t record_duration_s = req.record_duration_s();

  std::string filefolder;
  bool ret = action_wrapper.StartSignalRecord(preparation_duration_s, record_duration_s, filefolder);
  if (!ret) {
    SetErrorCode(ErrorCode::kStartRecordFailed, *common_rsp);
    co_return aimrt::rpc::Status();
  }

  rsp.set_filefolder(filefolder);

  co_return aimrt::rpc::Status();
}

aimrt::co::Task<aimrt::rpc::Status> RecordPlaybackServiceImpl::StopRecord(
    aimrt::rpc::ContextRef ctx_ref,
    const ::aimrt::protocols::record_playback_plugin::StopRecordReq& req,
    ::aimrt::protocols::record_playback_plugin::CommonRsp& rsp) {
  auto finditr = record_action_map_ptr_->find(req.action_name());
  if (finditr == record_action_map_ptr_->end()) {
    SetErrorCode(ErrorCode::kInvalidActionName, rsp);
    co_return aimrt::rpc::Status();
  }

  auto& action_wrapper = *(finditr->second);

  if (action_wrapper.GetOptions().mode != RecordAction::Options::Mode::kSignal) {
    SetErrorCode(ErrorCode::kInvalidActionMode, rsp);
    co_return aimrt::rpc::Status();
  }

  bool ret = action_wrapper.StopSignalRecord();
  if (!ret) {
    SetErrorCode(ErrorCode::kStopRecordFailed, rsp);
    co_return aimrt::rpc::Status();
  }

  co_return aimrt::rpc::Status();
}

aimrt::co::Task<aimrt::rpc::Status> RecordPlaybackServiceImpl::StartPlayback(
    aimrt::rpc::ContextRef ctx_ref,
    const ::aimrt::protocols::record_playback_plugin::StartPlaybackReq& req,
    ::aimrt::protocols::record_playback_plugin::CommonRsp& rsp) {
  auto finditr = playback_action_map_ptr_->find(req.action_name());
  if (finditr == playback_action_map_ptr_->end()) {
    SetErrorCode(ErrorCode::kInvalidActionName, rsp);
    co_return aimrt::rpc::Status();
  }

  auto& action_wrapper = *(finditr->second);

  if (action_wrapper.GetOptions().mode != PlaybackAction::Options::Mode::kSignal) {
    SetErrorCode(ErrorCode::kInvalidActionMode, rsp);
    co_return aimrt::rpc::Status();
  }

  uint64_t skip_duration_s = req.skip_duration_s();
  uint64_t play_duration_s = req.play_duration_s();

  bool ret = action_wrapper.StartSignalPlayback(skip_duration_s, play_duration_s);
  if (!ret) {
    SetErrorCode(ErrorCode::kStartPlaybackFailed, rsp);
    co_return aimrt::rpc::Status();
  }

  co_return aimrt::rpc::Status();
}

aimrt::co::Task<aimrt::rpc::Status> RecordPlaybackServiceImpl::StopPlayback(
    aimrt::rpc::ContextRef ctx_ref,
    const ::aimrt::protocols::record_playback_plugin::StopPlaybackReq& req,
    ::aimrt::protocols::record_playback_plugin::CommonRsp& rsp) {
  auto finditr = playback_action_map_ptr_->find(req.action_name());
  if (finditr == playback_action_map_ptr_->end()) {
    SetErrorCode(ErrorCode::kInvalidActionName, rsp);
    co_return aimrt::rpc::Status();
  }

  auto& action_wrapper = *(finditr->second);

  if (action_wrapper.GetOptions().mode != PlaybackAction::Options::Mode::kSignal) {
    SetErrorCode(ErrorCode::kInvalidActionMode, rsp);
    co_return aimrt::rpc::Status();
  }

  action_wrapper.StopSignalPlayback();

  co_return aimrt::rpc::Status();
}

aimrt::co::Task<aimrt::rpc::Status> RecordPlaybackServiceImpl::UpdateMetadata(
    aimrt::rpc::ContextRef ctx_ref,
    const ::aimrt::protocols::record_playback_plugin::UpdateMetadataReq& req,
    ::aimrt::protocols::record_playback_plugin::CommonRsp& rsp) {
  auto finditr = record_action_map_ptr_->find(req.action_name());
  if (finditr == record_action_map_ptr_->end()) {
    SetErrorCode(ErrorCode::kInvalidActionName, rsp);
    co_return aimrt::rpc::Status();
  }
  std::unordered_map<std::string, std::string> kv_pairs;
  for (const auto& [key, value] : req.kv_pairs()) {
    if (key.empty()) [[unlikely]] {
      AIMRT_WARN("Received metadata update with empty key. Skipping.");
      continue;
    }
    kv_pairs[key] = value;
  }

  auto& action_wrapper = *(finditr->second);

  action_wrapper.UpdateMetadata(std::move(kv_pairs));
  co_return aimrt::rpc::Status();
}

aimrt::co::Task<aimrt::rpc::Status> RecordPlaybackServiceImpl::UpdateRecordAction(
    aimrt::rpc::ContextRef ctx_ref,
    const ::aimrt::protocols::record_playback_plugin::UpdateRecordActionReq& req,
    ::aimrt::protocols::record_playback_plugin::UpdateRecordActionRsp& rsp) {
  bool all_success = true;
  for (const auto& action_meta : req.action_metas()) {
    auto* action_rsp = rsp.add_action_rsps();
    action_rsp->set_action_name(action_meta.action_name());
    auto* action_common_rsp = action_rsp->mutable_common_rsp();

    auto finditr = record_action_map_ptr_->find(action_meta.action_name());
    if (finditr == record_action_map_ptr_->end()) {
      SetErrorCode(ErrorCode::kInvalidActionName, *action_common_rsp);
      all_success = false;
      continue;
    }

    auto& action_wrapper = *(finditr->second);

    // 更新动作级别的 record_enabled
    action_wrapper.UpdateRecordEnabled(action_meta.record_enabled());

    // 更新主题级别的 record_enabled
    std::vector<TopicMeta> topic_metas;
    topic_metas.reserve(static_cast<size_t>(action_meta.topic_metas_size()));
    for (const auto& topic_meta : action_meta.topic_metas()) {
      topic_metas.emplace_back(TopicMeta{
          .topic_name = topic_meta.topic_name(),
          .msg_type = topic_meta.msg_type(),
          .record_enabled = topic_meta.record_enabled()});
    }
    if (!topic_metas.empty()) {
      action_wrapper.UpdateTopicMetaRecord(std::move(topic_metas));
    }

    // 填充已应用的元信息
    bool applied_action_record_enabled = false;
    std::vector<TopicMeta> applied_topic_metas;
    action_wrapper.GetAppliedActionMeta(applied_action_record_enabled, applied_topic_metas);

    auto* applied_meta = action_rsp->mutable_applied_action_meta();
    applied_meta->set_action_name(std::string(action_meta.action_name()));
    applied_meta->set_record_enabled(applied_action_record_enabled);
    for (const auto& meta : applied_topic_metas) {
      auto* out_meta = applied_meta->add_topic_metas();
      out_meta->set_topic_name(meta.topic_name);
      out_meta->set_msg_type(meta.msg_type);
      out_meta->set_record_enabled(meta.record_enabled);
    }

    SetErrorCode(ErrorCode::kSuc, *action_common_rsp);
  }

  auto common_rsp = rsp.mutable_common_rsp();
  SetErrorCode(all_success ? ErrorCode::kSuc : ErrorCode::kInvalidActionName, *common_rsp);
  co_return aimrt::rpc::Status();
}

}  // namespace aimrt::plugins::record_playback_plugin
