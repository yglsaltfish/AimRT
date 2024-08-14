#include "record_playback_plugin/service.h"
#include "aimrt_module_protobuf_interface/util/protobuf_tools.h"
#include "record_playback_plugin/global.h"
#include "record_playback_plugin/record_playback_plugin.h"

namespace aimrt::plugins::record_playback_plugin {

aimrt::co::Task<aimrt::rpc::Status> RecordPlaybackServiceImpl::StartRecord(
    aimrt::rpc::ContextRef ctx_ref,
    const ::aimrt::protocols::record_playback_plugin::StartRecordReq& req,
    ::aimrt::protocols::record_playback_plugin::CommonRsp& rsp) {
  auto action_wrapper_ptr = plugin_ptr_->GetRecordActionWrapper(req.action_name());

  if (action_wrapper_ptr == nullptr) {
    SetErrorCode(ErrorCode::INVALID_ACTION_NAME, rsp);
    co_return aimrt::rpc::Status();
  }

  if (action_wrapper_ptr->options.mode != RecordPlaybackPlugin::Options::RecordAction::Mode::SIGNAL) {
    SetErrorCode(ErrorCode::INVALID_ACTION_MODE, rsp);
    co_return aimrt::rpc::Status();
  }

  if (action_wrapper_ptr->record_flag) {
    SetErrorCode(ErrorCode::RECORD_IS_ALREADY_IN_PROGRESS, rsp);
    co_return aimrt::rpc::Status();
  }

  uint64_t preparation_duration_s = req.preparation_duration_s();
  uint64_t record_duration_s = req.record_duration_s();

  action_wrapper_ptr->executor.Execute(
      [action_wrapper_ptr, preparation_duration_s, record_duration_s]() {
        if (action_wrapper_ptr->record_flag) {
          AIMRT_WARN("Recording is already in progress");
          return;
        }

        action_wrapper_ptr->record_flag = true;

        auto now = aimrt::common::util::GetCurTimestampNs();

        action_wrapper_ptr->stop_record_timestamp = now + record_duration_s * 1000000000;

        uint64_t start_record_timestamp = now - preparation_duration_s * 1000000000;

        // 二分查找到缓存中需要开始记录的地方
        size_t last_cache_size = action_wrapper_ptr->last_cache.size();
        size_t cur_cache_size = action_wrapper_ptr->cur_cache.size();
        size_t all_cache_size = last_cache_size + cur_cache_size;

        if (all_cache_size == 0) return;

        auto get_cahce_timestamp = [&](size_t idx) {
          if (idx < last_cache_size)
            return action_wrapper_ptr->last_cache[idx].timestamp;
          return action_wrapper_ptr->cur_cache[idx - last_cache_size].timestamp;
        };

        size_t low = 0, high = all_cache_size - 1;

        while (low < high) {
          size_t mid = low + (high - low) / 2;
          if (get_cahce_timestamp(mid) < start_record_timestamp)
            low = mid + 1;
          else
            high = mid;
        }

        // 将缓存写入db
        if (low < last_cache_size) {
          for (size_t ii = low; ii < last_cache_size; ++ii) {
            action_wrapper_ptr->db_tool.AddRecord(action_wrapper_ptr->last_cache[ii]);
          }

          for (size_t ii = 0; ii < cur_cache_size; ++ii) {
            action_wrapper_ptr->db_tool.AddRecord(action_wrapper_ptr->cur_cache[ii]);
          }
        } else {
          for (size_t ii = low - last_cache_size; ii < cur_cache_size; ++ii) {
            action_wrapper_ptr->db_tool.AddRecord(action_wrapper_ptr->cur_cache[ii]);
          }
        }

        // 清空缓存
        action_wrapper_ptr->last_cache.clear();
        action_wrapper_ptr->cur_cache.clear();
      });

  co_return aimrt::rpc::Status();
}

aimrt::co::Task<aimrt::rpc::Status> RecordPlaybackServiceImpl::StopRecord(
    aimrt::rpc::ContextRef ctx_ref,
    const ::aimrt::protocols::record_playback_plugin::StopRecordReq& req,
    ::aimrt::protocols::record_playback_plugin::CommonRsp& rsp) {
  auto action_wrapper_ptr = plugin_ptr_->GetRecordActionWrapper(req.action_name());

  if (action_wrapper_ptr == nullptr) {
    SetErrorCode(ErrorCode::INVALID_ACTION_NAME, rsp);
    co_return aimrt::rpc::Status();
  }

  if (action_wrapper_ptr->options.mode != RecordPlaybackPlugin::Options::RecordAction::Mode::SIGNAL) {
    SetErrorCode(ErrorCode::INVALID_ACTION_MODE, rsp);
    co_return aimrt::rpc::Status();
  }

  action_wrapper_ptr->executor.Execute(
      [action_wrapper_ptr]() {
        action_wrapper_ptr->record_flag = false;
      });

  co_return aimrt::rpc::Status();
}

aimrt::co::Task<aimrt::rpc::Status> RecordPlaybackServiceImpl::StartPlayback(
    aimrt::rpc::ContextRef ctx_ref,
    const ::aimrt::protocols::record_playback_plugin::StartPlaybackReq& req,
    ::aimrt::protocols::record_playback_plugin::CommonRsp& rsp) {
  auto action_wrapper_ptr = plugin_ptr_->GetPlaybackActionWrapper(req.action_name());

  if (action_wrapper_ptr == nullptr) {
    SetErrorCode(ErrorCode::INVALID_ACTION_NAME, rsp);
    co_return aimrt::rpc::Status();
  }

  if (action_wrapper_ptr->options.mode != RecordPlaybackPlugin::Options::PlaybackAction::Mode::SIGNAL) {
    SetErrorCode(ErrorCode::INVALID_ACTION_MODE, rsp);
    co_return aimrt::rpc::Status();
  }

  bool ret = action_wrapper_ptr->db_tool.StartPlayback(req.skip_duration_s(), req.play_duration_s());
  if (!ret) {
    SetErrorCode(ErrorCode::PLAYBACK_IS_ALREADY_IN_PROGRESS, rsp);
    co_return aimrt::rpc::Status();
  }

  action_wrapper_ptr->run_flag.store(true);
  action_wrapper_ptr->start_timestamp = aimrt::common::util::GetCurTimestampNs();

  // 开始两个task包
  plugin_ptr_->AddPlaybackTasks(*action_wrapper_ptr);
  plugin_ptr_->AddPlaybackTasks(*action_wrapper_ptr);

  co_return aimrt::rpc::Status();
}

aimrt::co::Task<aimrt::rpc::Status> RecordPlaybackServiceImpl::StopPlayback(
    aimrt::rpc::ContextRef ctx_ref,
    const ::aimrt::protocols::record_playback_plugin::StopPlaybackReq& req,
    ::aimrt::protocols::record_playback_plugin::CommonRsp& rsp) {
  auto action_wrapper_ptr = plugin_ptr_->GetPlaybackActionWrapper(req.action_name());

  if (action_wrapper_ptr == nullptr) {
    SetErrorCode(ErrorCode::INVALID_ACTION_NAME, rsp);
    co_return aimrt::rpc::Status();
  }

  if (action_wrapper_ptr->options.mode != RecordPlaybackPlugin::Options::PlaybackAction::Mode::SIGNAL) {
    SetErrorCode(ErrorCode::INVALID_ACTION_MODE, rsp);
    co_return aimrt::rpc::Status();
  }

  action_wrapper_ptr->run_flag.store(false);
  action_wrapper_ptr->db_tool.Reset();

  co_return aimrt::rpc::Status();
}

}  // namespace aimrt::plugins::record_playback_plugin
