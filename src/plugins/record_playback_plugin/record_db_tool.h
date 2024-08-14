#pragma once

#include <filesystem>
#include <vector>

#include "aimrt_module_cpp_interface/util/buffer.h"
#include "record_playback_plugin/metadata_yaml.h"

#include "sqlite3.h"

namespace aimrt::plugins::record_playback_plugin {

class RecordDbTool {
 public:
  struct Options {
    uint32_t max_bag_size_m = 2048;

    std::filesystem::path bag_path;

    std::string bag_base_name;
  };

  struct OneRecord {
    uint64_t timestamp;
    uint32_t topic_index;
    std::shared_ptr<aimrt::util::BufferArrayView> buffer_view_ptr;
  };

 public:
  RecordDbTool() = default;
  ~RecordDbTool() { Shutdown(); }

  RecordDbTool(const RecordDbTool&) = delete;
  RecordDbTool& operator=(const RecordDbTool&) = delete;

  void Initialize(const Options& options);
  void Shutdown();

  void AddTopicMeta(
      std::string_view topic_name, std::string_view msg_type, std::string_view serialization_type);

  uint32_t GetTopicIndex(std::string_view topic_name, std::string_view msg_type) const;

  void AddRecord(const OneRecord& record);

 private:
  void OpenNewDb(uint64_t start_timestamp);
  void CloseDb();

 private:
  enum class State : uint32_t {
    PreInit,
    Init,
    Shutdown,
  };

  Options options_;
  std::atomic<State> state_ = State::PreInit;

  size_t max_bag_size_ = 0;

  uint32_t cur_db_file_index_ = 0;
  sqlite3* db_ = nullptr;
  sqlite3_stmt* insert_msg_stmt_ = nullptr;

  size_t cur_data_size_ = 0;

  struct Key {
    std::string topic_name;
    std::string msg_type;

    bool operator==(const Key& rhs) const {
      return topic_name == rhs.topic_name && msg_type == rhs.msg_type;
    }

    struct Hash {
      std::size_t operator()(const Key& k) const {
        return (std::hash<std::string>()(k.topic_name)) ^
               (std::hash<std::string>()(k.msg_type));
      }
    };
  };

  std::unordered_map<Key, int, Key::Hash> topic_id_map_;

  std::filesystem::path metadata_yaml_file_path_;
  MetaData metadata_;
};

}  // namespace aimrt::plugins::record_playback_plugin