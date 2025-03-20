// Copyright (c) 2023, AgiBot Inc.
// All rights reserved.

#pragma once

#include <cstdint>
#include <deque>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include "google/protobuf/descriptor.h"
#include "google/protobuf/descriptor.pb.h"
#include "sqlite3.h"
#include "yaml-cpp/yaml.h"

#include "aimrt_module_cpp_interface/executor/executor.h"
#include "aimrt_module_cpp_interface/executor/timer.h"
#include "aimrt_module_cpp_interface/util/buffer.h"
#include "aimrt_module_cpp_interface/util/type_support.h"
#include "core/util/topic_meta_key.h"
#include "record_playback_plugin/metadata_yaml.h"
#include "record_playback_plugin/topic_meta.h"
#include "storage/mcap_storage.h"
#include "storage/sqlite_storage.h"
#include "topic_meta.h"

namespace aimrt::plugins::record_playback_plugin {

class RecordAction {
 public:
  struct Options {
    std::string bag_path;
    enum class Mode {
      kImd,
      kSignal,
    };
    Mode mode = Mode::kImd;

    struct StoragePolicy {
      std::string storage_format = "mcap";
      uint32_t max_bag_size_m = 2048;
      uint32_t max_bag_num = 0;
      uint32_t msg_write_interval = 1000;
      uint32_t msg_write_interval_time = 1000;
      std::string synchronous_mode = "full";
      std::string journal_mode = "memory";
      std::string compression_mode = "zstd";
      std::string compression_level = "default";
    };

    StoragePolicy storage_policy;

    struct ExtData {
      std::string key;
      std::string value;
    };
    std::vector<ExtData> ext_data;

    uint64_t max_preparation_duration_s = 0;
    std::string executor;

    struct TopicMeta {
      std::string topic_name;
      std::string msg_type;
      std::string serialization_type;
    };
    std::vector<TopicMeta> topic_meta_list;
  };

  struct OneRecord {
    uint64_t timestamp;
    uint64_t topic_index;
    std::shared_ptr<aimrt::util::BufferArrayView> buffer_view_ptr;
  };

 public:
  RecordAction() = default;
  ~RecordAction() = default;

  RecordAction(const RecordAction&) = delete;
  RecordAction& operator=(const RecordAction&) = delete;

  void Initialize(YAML::Node options_node);
  void Start();
  void Shutdown();

  void InitExecutor(aimrt::executor::ExecutorRef);

  const Options& GetOptions() const { return options_; }

  void RegisterGetExecutorFunc(
      const std::function<executor::ExecutorRef(std::string_view)>& get_executor_func);

  void RegisterGetTypeSupportFunc(
      const std::function<aimrt::util::TypeSupportRef(std::string_view)>& get_type_support_func);

  const auto& GetTopicMetaMap() const { return topic_meta_map_; }
  void AddRecord(OneRecord&& record);

  bool StartSignalRecord(uint64_t preparation_duration_s, uint64_t record_duration_s);
  void StopSignalRecord();

 private:
  void AddRecordImpl(OneRecord&& record);

  void OpenNewDb(uint64_t start_timestamp);
  void CloseDb();
  size_t GetDbFileSize() const;

  enum class State : uint32_t {
    kPreInit,
    kInit,
    kStart,
    kShutdown,
  };

 private:
  Options options_;
  std::atomic<State> state_ = State::kPreInit;

  std::function<executor::ExecutorRef(std::string_view)> get_executor_func_;
  aimrt::executor::ExecutorRef executor_;

  std::unique_ptr<StorageInterface> storage_;

  std::function<aimrt::util::TypeSupportRef(std::string_view)> get_type_support_func_;
  std::unordered_map<aimrt::runtime::core::util::TopicMetaKey, TopicMeta,
                     aimrt::runtime::core::util::TopicMetaKey::Hash>
      topic_meta_map_;

  std::shared_ptr<aimrt::executor::TimerBase> sync_timer_;

  size_t max_bag_size_ = 0;

  size_t cur_exec_count_ = 0;

  std::filesystem::path metadata_yaml_file_path_;
  MetaData metadata_;

  // only for signal mode
  bool recording_flag_ = false;
  uint64_t max_preparation_duration_ns_ = 0;
  uint64_t stop_record_timestamp_ = 0;

  std::deque<OneRecord> last_cache_;
  std::deque<OneRecord> cur_cache_;
};

}  // namespace aimrt::plugins::record_playback_plugin
