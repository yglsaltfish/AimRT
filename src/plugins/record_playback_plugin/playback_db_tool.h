#pragma once

#include <filesystem>

#include "aimrt_module_cpp_interface/util/buffer.h"
#include "record_playback_plugin/metadata_yaml.h"

#include "yaml-cpp/yaml.h"

#include "sqlite3.h"

namespace aimrt::plugins::record_playback_plugin {

class PlaybackDbTool {
 public:
  struct Options {
    std::filesystem::path bag_path;
  };

  struct OneRecord {
    uint32_t topic_id;
    uint64_t dt;
    std::shared_ptr<aimrt::util::BufferArrayView> buffer_view_ptr;
  };

 public:
  PlaybackDbTool() = default;
  ~PlaybackDbTool() { Shutdown(); }

  PlaybackDbTool(const PlaybackDbTool&) = delete;
  PlaybackDbTool& operator=(const PlaybackDbTool&) = delete;

  void Initialize(const Options& options);
  void Shutdown();

  void SelectTopicMeta(std::string_view topic_name, std::string_view msg_type);

  MetaData::TopicMeta GetTopicMeta(
      std::string_view topic_name, std::string_view msg_type) const;

  void Reset();
  bool StartPlayback(uint64_t skip_duration_s, uint64_t play_duration_s);
  std::vector<OneRecord> GetRecords();

 private:
  void CloseDb();
  bool OpenNewDb();

 private:
  enum class State : uint32_t {
    PreInit,
    Init,
    Shutdown,
  };

  Options options_;
  std::atomic<State> state_ = State::PreInit;

  std::filesystem::path metadata_yaml_file_path_;
  MetaData metadata_;

  std::vector<std::pair<std::string, std::string>> select_topic_meta_vec_;

  uint32_t cur_db_file_index_ = 0;
  std::atomic_bool run_flag_ = false;
  uint64_t start_playback_timestamp_ = 0;
  uint64_t stop_playback_timestamp_ = 0;

  sqlite3* db_ = nullptr;
  sqlite3_stmt* select_msg_stmt_ = nullptr;
  std::string select_msg_sql_topic_id_range_;
};

}  // namespace aimrt::plugins::record_playback_plugin