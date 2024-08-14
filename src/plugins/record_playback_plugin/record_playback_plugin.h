#pragma once

#include <memory>
#include <vector>

#include "aimrt_core_plugin_interface/aimrt_core_plugin_base.h"
#include "aimrt_module_cpp_interface/util/type_support.h"
#include "core/channel/channel_registry.h"
#include "record_playback_plugin/playback_db_tool.h"
#include "record_playback_plugin/record_db_tool.h"
#include "record_playback_plugin/service.h"
#include "record_playback_plugin/type_support_pkg_loader.h"

namespace aimrt::plugins::record_playback_plugin {

class RecordPlaybackPlugin : public AimRTCorePluginBase {
 public:
  struct Options {
    struct TypeSupportPkg {
      std::string path;
    };
    std::vector<TypeSupportPkg> type_support_pkgs;

    struct RecordAction {
      std::string name;
      std::string bag_path;
      uint32_t max_bag_size_m = 2048;

      enum class Mode {
        IMD,
        SIGNAL,
      };
      Mode mode = Mode::IMD;

      uint32_t max_preparation_duration_s = 0;
      std::string executor;

      struct TopicMeta {
        std::string topic_name;
        std::string msg_type;
        std::string serialization_type;
      };
      std::vector<TopicMeta> topic_meta_list;
    };
    std::vector<RecordAction> record_actions;

    struct PlaybackAction {
      std::string name;
      std::string bag_path;

      enum class Mode {
        IMD,
        SIGNAL,
      };
      Mode mode = Mode::IMD;

      std::string executor;

      uint32_t skip_duration_s = 0;
      uint32_t play_duration_s = 0;

      struct TopicMeta {
        std::string topic_name;
        std::string msg_type;
      };
      std::vector<TopicMeta> topic_meta_list;
    };
    std::vector<PlaybackAction> playback_actions;
  };

  struct RecordActionWrapper {
    RecordActionWrapper(const Options::RecordAction& input_options)
        : options(input_options) {}

    const Options::RecordAction& options;
    aimrt::executor::ExecutorRef executor;

    RecordDbTool db_tool;

    // only for signal mode
    bool record_flag = false;
    uint64_t stop_record_timestamp = 0;

    std::deque<RecordDbTool::OneRecord> last_cache;
    std::deque<RecordDbTool::OneRecord> cur_cache;
  };

  struct PlaybackActionWrapper {
    PlaybackActionWrapper(const Options::PlaybackAction& input_options)
        : options(input_options) {}

    const Options::PlaybackAction& options;
    aimrt::executor::ExecutorRef executor;

    PlaybackDbTool db_tool;

    uint64_t start_timestamp = 0;

    std::atomic_bool run_flag = false;

    struct PublishMeta {
      const aimrt::runtime::core::channel::PublishTypeWrapper* pub_type_wrapper_ptr;
      std::string serialization_type;
    };

    std::unordered_map<uint32_t, PublishMeta> publish_meta_map;
  };

 public:
  RecordPlaybackPlugin() = default;
  ~RecordPlaybackPlugin() override = default;

  std::string_view Name() const noexcept override { return "record_playback_plugin"; }

  bool Initialize(runtime::core::AimRTCore* core_ptr) noexcept override;
  void Shutdown() noexcept override;

  RecordActionWrapper* GetRecordActionWrapper(std::string_view action_name) const;
  PlaybackActionWrapper* GetPlaybackActionWrapper(std::string_view action_name) const;

  void AddPlaybackTasks(PlaybackActionWrapper& wrapper);

 private:
  void InitTypeSupport(Options::TypeSupportPkg& options);
  void InitRecordAction(Options::RecordAction& options);
  void InitPlaybackAction(Options::PlaybackAction& options);

  void SetPluginLogger();
  void RegisterRpcService();
  void RegisterRecordChannel();
  void RegisterPlaybackChannel();
  void StartPlayback();
  void StopPlayback();

 private:
  runtime::core::AimRTCore* core_ptr_ = nullptr;

  Options options_;

  bool init_flag_ = false;

  std::unique_ptr<RecordPlaybackServiceImpl> service_ptr_;

  std::vector<std::unique_ptr<TypeSupportPkgLoader>> type_support_pkg_loader_vec_;

  struct TypeSupportWrapper {
    const Options::TypeSupportPkg& options;
    aimrt::util::TypeSupportRef type_support_ref;
    TypeSupportPkgLoader* loader_ptr;
  };
  std::unordered_map<std::string_view, TypeSupportWrapper> type_support_map_;

  std::unordered_map<std::string_view, std::unique_ptr<RecordActionWrapper>> record_action_map_;
  std::unordered_map<std::string_view, std::unique_ptr<PlaybackActionWrapper>> playback_action_map_;
};

}  // namespace aimrt::plugins::record_playback_plugin
