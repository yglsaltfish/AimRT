#include "record_playback_plugin/playback_db_tool.h"
#include "record_playback_plugin/global.h"

namespace aimrt::plugins::record_playback_plugin {

void PlaybackDbTool::Initialize(const Options& options) {
  AIMRT_CHECK_ERROR_THROW(
      std::atomic_exchange(&state_, State::Init) == State::PreInit,
      "Can only be initialized once.");

  options_ = options;

  // 读取 metadata.yaml
  metadata_yaml_file_path_ = options_.bag_path / "metadata.yaml";

  AIMRT_CHECK_ERROR_THROW(
      std::filesystem::exists(metadata_yaml_file_path_) &&
          std::filesystem::is_regular_file(metadata_yaml_file_path_),
      "Can not find 'metadata.yaml' in bag path '{}'.", options_.bag_path.string());

  auto metadata_root_node = YAML::LoadFile(metadata_yaml_file_path_.string());
  metadata_ = metadata_root_node["aimrt_bagfile_information"].as<MetaData>();

  // 检查version
  AIMRT_CHECK_ERROR_THROW(metadata_.version == kVersion,
                          "Version inconsistency, cur plugin version: {}, bag version: {}",
                          kVersion, metadata_.version);

  // 检查 select_topic_meta_vec_
  if (!select_topic_meta_vec_.empty()) {
    std::vector<uint32_t> enable_topic_id_vec;

    std::vector<MetaData::TopicMeta> select_topics;
    for (auto item : select_topic_meta_vec_) {
      auto finditr = std::find_if(
          metadata_.topics.begin(), metadata_.topics.end(),
          [&item](const auto& topic_meta) {
            return (item.first == topic_meta.topic_name) && (item.second == topic_meta.msg_type);
          });

      AIMRT_CHECK_ERROR_THROW(finditr != metadata_.topics.end(),
                              "Can not find topic '{}' with msg type '{}' in bag '{}'.",
                              item.first, item.second, options_.bag_path.string());

      enable_topic_id_vec.emplace_back(finditr->id);

      select_topics.emplace_back(*finditr);
    }

    if (metadata_.topics.size() != select_topics.size()) {
      metadata_.topics = std::move(select_topics);

      for (size_t ii = 0; ii < enable_topic_id_vec.size(); ++ii) {
        select_msg_sql_topic_id_range_ += std::to_string(enable_topic_id_vec[ii]);
        if (ii != enable_topic_id_vec.size() - 1) {
          select_msg_sql_topic_id_range_ += ", ";
        }
      }
    }
  }

  AIMRT_CHECK_ERROR_THROW(!metadata_.files.empty(),
                          "Empty bag! bag path: {}", options_.bag_path.string());
}

void PlaybackDbTool::Shutdown() {
  if (std::atomic_exchange(&state_, State::Shutdown) == State::Shutdown)
    return;

  CloseDb();
}

void PlaybackDbTool::SelectTopicMeta(std::string_view topic_name, std::string_view msg_type) {
  AIMRT_CHECK_ERROR_THROW(
      state_.load() == State::PreInit,
      "Method can only be called when state is 'PreInit'.");

  auto finditr = std::find_if(
      select_topic_meta_vec_.begin(), select_topic_meta_vec_.end(),
      [&](const auto& item) {
        return (item.first == topic_name) && (item.second == msg_type);
      });

  AIMRT_CHECK_ERROR_THROW(
      finditr == select_topic_meta_vec_.end(),
      "Duplicate select topic meta, topic name: {}, msg type: {}.", topic_name, msg_type);

  select_topic_meta_vec_.emplace_back(topic_name, msg_type);
}

MetaData::TopicMeta PlaybackDbTool::GetTopicMeta(std::string_view topic_name, std::string_view msg_type) const {
  AIMRT_CHECK_ERROR_THROW(
      state_.load() == State::Init,
      "Method can only be called when state is 'Init'.");

  auto finditr = std::find_if(
      metadata_.topics.begin(), metadata_.topics.end(),
      [&](const auto& topic_meta) {
        return (topic_name == topic_meta.topic_name) && (msg_type == topic_meta.msg_type);
      });

  AIMRT_CHECK_ERROR_THROW(finditr != metadata_.topics.end(),
                          "Can not find topic '{}' with msg type '{}' in bag '{}'.",
                          topic_name, msg_type, options_.bag_path.string());

  return *finditr;
}

void PlaybackDbTool::Reset() {
  run_flag_.store(false);
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  CloseDb();
}

bool PlaybackDbTool::StartPlayback(uint64_t skip_duration_s, uint64_t play_duration_s) {
  AIMRT_CHECK_ERROR_THROW(
      state_.load() == State::Init,
      "Method can only be called when state is 'Init'.");

  if (std::atomic_exchange(&run_flag_, true) == true) [[unlikely]] {
    AIMRT_WARN("Playback is already running.");
    return false;
  }

  start_playback_timestamp_ = metadata_.files[0].start_timestamp + skip_duration_s * 1000000000;
  if (play_duration_s == 0) {
    stop_playback_timestamp_ = 0;
  } else {
    stop_playback_timestamp_ = start_playback_timestamp_ + play_duration_s * 1000000000;
  }

  size_t ii = 1;
  for (; ii < metadata_.files.size(); ++ii) {
    if (metadata_.files[ii].start_timestamp > start_playback_timestamp_)
      break;
  }
  cur_db_file_index_ = ii - 1;

  AIMRT_TRACE("Start a new playback, skip_duration_s: {}, play_duration_s: {}, start_playback_timestamp: {}, stop_playback_timestamp: {}, use db index: {}",
              skip_duration_s, play_duration_s,
              start_playback_timestamp_, stop_playback_timestamp_,
              cur_db_file_index_);

  return true;
}

std::vector<PlaybackDbTool::OneRecord> PlaybackDbTool::GetRecords() {
  AIMRT_CHECK_ERROR_THROW(
      state_.load() == State::Init,
      "Method can only be called when state is 'Init'.");

  if (!run_flag_.load()) [[unlikely]] {
    return {};
  }

  if (db_ == nullptr) [[unlikely]] {
    // first record
    if (!OpenNewDb()) return {};
  }

  // 一次性吐出至少1s的数据，最多xx条数据
  constexpr size_t max_record_size = 128;
  uint64_t cur_start_timestamp = 0;

  std::vector<OneRecord> result;
  result.reserve(max_record_size);

  while (true) {
    int ret = sqlite3_step(select_msg_stmt_);
    if (ret == SQLITE_ROW) {
      auto topic_id = sqlite3_column_int64(select_msg_stmt_, 0);
      auto timestamp = sqlite3_column_int64(select_msg_stmt_, 1);

      if (stop_playback_timestamp_ && timestamp >= stop_playback_timestamp_) [[unlikely]] {
        run_flag_.store(false);
        CloseDb();
        break;
      }

      uint32_t size = sqlite3_column_bytes(select_msg_stmt_, 2);
      const void* buf = sqlite3_column_blob(select_msg_stmt_, 2);

      auto data_ptr = new std::string(static_cast<const char*>(buf), size);

      aimrt_buffer_view_t buffer_view{
          .data = data_ptr->data(),
          .len = data_ptr->size()};

      aimrt_buffer_array_view_t buffer_array_view{
          .data = &buffer_view,
          .len = 1};

      OneRecord record;
      record.topic_id = topic_id;
      record.dt = timestamp - start_playback_timestamp_;
      record.buffer_view_ptr = std::shared_ptr<aimrt::util::BufferArrayView>(
          new aimrt::util::BufferArrayView(buffer_array_view),
          [data_ptr](const auto* ptr) { 
            delete data_ptr;
            delete ptr; });

      result.emplace_back(std::move(record));

      if (cur_start_timestamp == 0) [[unlikely]] {
        cur_start_timestamp = timestamp;
      } else if ((timestamp - cur_start_timestamp) >= 1000000000 || result.size() >= max_record_size) [[unlikely]] {
        break;
      }
    } else {
      CloseDb();
      if (!OpenNewDb()) break;
    }
  }

  AIMRT_TRACE("Get {} record.", result.size());

  return result;
}

bool PlaybackDbTool::OpenNewDb() {
  AIMRT_CHECK_ERROR_THROW(
      state_.load() == State::Init,
      "Method can only be called when state is 'Init'.");

  if (cur_db_file_index_ >= metadata_.files.size()) [[unlikely]] {
    run_flag_.store(false);
    return false;
  }

  const auto& file = metadata_.files[cur_db_file_index_];
  std::string db_file_path = options_.bag_path / file.path;
  ++cur_db_file_index_;

  uint64_t cur_db_start_timestamp = file.start_timestamp;

  if (stop_playback_timestamp_ && cur_db_start_timestamp > stop_playback_timestamp_) [[unlikely]] {
    return false;
  }

  // open db
  int ret = sqlite3_open(db_file_path.c_str(), &db_);
  AIMRT_CHECK_ERROR_THROW(ret == SQLITE_OK,
                          "Sqlite3 open db file failed, path: {}, ret: {}, error info: {}",
                          db_file_path, ret, sqlite3_errmsg(db_));

  AIMRT_TRACE("Open new db, path: {}", db_file_path);

  sqlite3_exec(db_, "PRAGMA synchronous = OFF; ", 0, 0, 0);

  // create select stmt
  std::string sql = "SELECT topic_id, timestamp, data FROM messages";

  std::vector<std::string> condition;

  if (cur_db_start_timestamp < start_playback_timestamp_)
    condition.emplace_back("timestamp >= " + std::to_string(start_playback_timestamp_));

  if (!select_msg_sql_topic_id_range_.empty())
    condition.emplace_back("topic_id IN ( " + select_msg_sql_topic_id_range_ + " )");

  for (size_t ii = 0; ii < condition.size(); ++ii) {
    if (ii == 0)
      sql += " WHERE ";
    else
      sql += " AND ";

    sql += condition[ii];
  }

  AIMRT_TRACE("Sql str: {}, db path: {}", sql, db_file_path);

  ret = sqlite3_prepare_v3(db_, sql.c_str(), sql.size(), 0, &select_msg_stmt_, nullptr);
  AIMRT_CHECK_ERROR_THROW(ret == SQLITE_OK,
                          "Sqlite3 prepare failed, sql: {}, ret: {}, error info: {}",
                          sql, ret, sqlite3_errmsg(db_));

  return true;
}

void PlaybackDbTool::CloseDb() {
  if (db_ != nullptr) {
    if (select_msg_stmt_ != nullptr) {
      sqlite3_finalize(select_msg_stmt_);
      select_msg_stmt_ = nullptr;
    }

    sqlite3_close_v2(db_);
    db_ = nullptr;
  }
}

}  // namespace aimrt::plugins::record_playback_plugin