#pragma once

#include <memory>
#include <vector>

#include "aimrt_core_plugin_interface/aimrt_core_plugin_base.h"
#include "aimrt_module_cpp_interface/util/type_support.h"
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

      enum class Mode {
        IMD,
        SIGNAL,
      };
      Mode mode = Mode::IMD;

      uint32_t preparation_duration_s = 0;
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

 public:
  RecordPlaybackPlugin() = default;
  ~RecordPlaybackPlugin() override = default;

  std::string_view Name() const noexcept override { return "record_playback_plugin"; }

  bool Initialize(runtime::core::AimRTCore* core_ptr) noexcept override;
  void Shutdown() noexcept override;

 private:
  void SetPluginLogger();
  void RegisterRpcService();

 private:
  runtime::core::AimRTCore* core_ptr_ = nullptr;

  Options options_;

  bool init_flag_ = false;

  std::unique_ptr<RecordPlaybackServiceImpl> service_ptr_;

  std::vector<std::unique_ptr<TypeSupportPkgLoader>> type_support_pkg_loader_vec_;

  struct Wrapper {
    aimrt::util::TypeSupportRef type_support_ref;
    TypeSupportPkgLoader* loader_ptr;
  };
  std::unordered_map<std::string_view, Wrapper> type_support_map_;
};

}  // namespace aimrt::plugins::record_playback_plugin
