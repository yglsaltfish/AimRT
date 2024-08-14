#include "record_playback_plugin/record_db_tool.h"
#include "record_playback_plugin/global.h"

namespace aimrt::plugins::record_playback_plugin {

void RecordDbTool::Initialize(const Options& options) {
  AIMRT_CHECK_ERROR_THROW(
      std::atomic_exchange(&state_, State::Init) == State::PreInit,
      "Can only be initialized once.");

  options_ = options;

  max_bag_size_ = options_.max_bag_size_m * 1024 * 1024;

  if (!(std::filesystem::exists(options_.bag_path) && std::filesystem::is_directory(options_.bag_path))) {
    std::filesystem::create_directories(options_.bag_path);
  }

  // 初始化 metadata.yaml
  metadata_yaml_file_path_ = options_.bag_path / "metadata.yaml";

  metadata_.version = kVersion;
}

void RecordDbTool::Shutdown() {
  if (std::atomic_exchange(&state_, State::Shutdown) == State::Shutdown)
    return;

  CloseDb();
}

void RecordDbTool::AddTopicMeta(
    std::string_view topic_name, std::string_view msg_type, std::string_view serialization_type) {
  AIMRT_CHECK_ERROR_THROW(
      state_.load() == State::PreInit,
      "Method can only be called when state is 'PreInit'.");

  Key key{std::string(topic_name), std::string(msg_type)};

  AIMRT_CHECK_ERROR_THROW(
      topic_id_map_.find(key) == topic_id_map_.end(),
      "Duplicate topic meta, topic name: {}, msg type: {}.", topic_name, msg_type);

  uint32_t topic_id = metadata_.topics.size();

  metadata_.topics.emplace_back(
      MetaData::TopicMeta{
          .id = topic_id,
          .topic_name = std::string(topic_name),
          .msg_type = std::string(msg_type),
          .serialization_type = std::string(serialization_type)});

  topic_id_map_.emplace(key, topic_id);
}

uint32_t RecordDbTool::GetTopicIndex(std::string_view topic_name, std::string_view msg_type) const {
  AIMRT_CHECK_ERROR_THROW(
      state_.load() == State::Init,
      "Method can only be called when state is 'Init'.");

  auto finditr = topic_id_map_.find(Key{std::string(topic_name), std::string(msg_type)});

  AIMRT_CHECK_ERROR_THROW(
      finditr != topic_id_map_.end(),
      "Can not find topic index, topic name: {}, msg type: {}.", topic_name, msg_type);

  return finditr->second;
}

void RecordDbTool::AddRecord(const OneRecord& record) {
  AIMRT_CHECK_ERROR_THROW(
      state_.load() == State::Init,
      "Method can only be called when state is 'Init'.");

  if (db_ == nullptr) [[unlikely]] {
    // first record
    OpenNewDb(record.timestamp);
  } else if (cur_data_size_ > max_bag_size_) [[unlikely]] {
    // check db size
    cur_data_size_ = 0;
    CloseDb();
    OpenNewDb(record.timestamp);
  }

  // insert data
  sqlite3_bind_int64(insert_msg_stmt_, 1, record.topic_index);
  sqlite3_bind_int64(insert_msg_stmt_, 2, record.timestamp);

  auto& buffer_array_view = *record.buffer_view_ptr;

  if (buffer_array_view.Size() == 1) {
    auto data = buffer_array_view.Data()[0];
    sqlite3_bind_blob(insert_msg_stmt_, 3, data.data, data.len, SQLITE_STATIC);
    cur_data_size_ += data.len;
  } else {
    const std::string& data_str = buffer_array_view.JoinToString();
    sqlite3_bind_blob(insert_msg_stmt_, 3, data_str.c_str(), data_str.size(), SQLITE_STATIC);
    cur_data_size_ += data_str.size();
  }

  sqlite3_step(insert_msg_stmt_);
  sqlite3_reset(insert_msg_stmt_);

  cur_data_size_ += 24;  // id + topic_id + timestamp
}

void RecordDbTool::CloseDb() {
  if (db_ != nullptr) {
    if (insert_msg_stmt_ != nullptr) {
      sqlite3_finalize(insert_msg_stmt_);
      insert_msg_stmt_ = nullptr;
    }

    sqlite3_close_v2(db_);
    db_ = nullptr;
  }
}

void RecordDbTool::OpenNewDb(uint64_t start_timestamp) {
  AIMRT_CHECK_ERROR_THROW(
      state_.load() == State::Init,
      "Method can only be called when state is 'Init'.");

  std::string cur_db_file_name = options_.bag_base_name + "_" + std::to_string(cur_db_file_index_) + ".db3";
  std::string db_file_path = options_.bag_path / cur_db_file_name;

  ++cur_db_file_index_;

  // open db
  int ret = sqlite3_open(db_file_path.c_str(), &db_);
  AIMRT_CHECK_ERROR_THROW(ret == SQLITE_OK,
                          "Sqlite3 open db file failed, path: {}, ret: {}, error info: {}",
                          db_file_path, ret, sqlite3_errmsg(db_));

  AIMRT_TRACE("Open new db, path: {}", db_file_path);

  sqlite3_exec(db_, "PRAGMA synchronous = OFF; ", 0, 0, 0);

  // create table
  std::string sql = R"str(
CREATE TABLE messages(
id          INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
topic_id    INTEGER NOT NULL,
timestamp   INTEGER NOT NULL,
data        BLOB NOT NULL);
)str";

  ret = sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, nullptr);
  AIMRT_CHECK_ERROR_THROW(ret == SQLITE_OK,
                          "Sqlite3 create table failed, sql: {}, ret: {}, error info: {}",
                          sql, ret, sqlite3_errmsg(db_));

  // create stmt
  sql = "INSERT INTO messages(topic_id, timestamp, data) VALUES(?, ?, ?)";

  ret = sqlite3_prepare_v3(db_, sql.c_str(), sql.size(), 0, &insert_msg_stmt_, nullptr);
  AIMRT_CHECK_ERROR_THROW(ret == SQLITE_OK,
                          "Sqlite3 prepare failed, sql: {}, ret: {}, error info: {}",
                          sql, ret, sqlite3_errmsg(db_));

  // update metadatat.yaml
  metadata_.files.emplace_back(MetaData::FileMeta{
      .path = cur_db_file_name,
      .start_timestamp = start_timestamp});

  YAML::Node node;
  node["aimrt_bagfile_information"] = metadata_;

  std::ofstream ofs(metadata_yaml_file_path_);
  ofs << node;
  ofs.close();
}

}  // namespace aimrt::plugins::record_playback_plugin