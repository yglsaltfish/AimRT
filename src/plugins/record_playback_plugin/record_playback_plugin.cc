#include "record_playback_plugin/record_playback_plugin.h"

#include "aimrt_module_cpp_interface/rpc/rpc_handle.h"
#include "aimrt_module_protobuf_interface/util/protobuf_tools.h"
#include "core/aimrt_core.h"
#include "record_playback_plugin/global.h"
#include "util/string_util.h"

namespace YAML {

template <>
struct convert<aimrt::plugins::record_playback_plugin::RecordPlaybackPlugin::Options::RecordAction> {
  using Options = aimrt::plugins::record_playback_plugin::RecordPlaybackPlugin::Options::RecordAction;

  static Node encode(const Options& rhs) {
    Node node;

    node["name"] = rhs.name;
    node["bag_path"] = rhs.bag_path;

    if (rhs.mode == Options::Mode::IMD) {
      node["mode"] = "imd";
    } else if (rhs.mode == Options::Mode::SIGNAL) {
      node["mode"] = "signal";
    }

    node["preparation_duration_s"] = rhs.preparation_duration_s;
    node["executor"] = rhs.executor;

    node["topic_meta_list"] = YAML::Node();
    for (const auto& topic_meta : rhs.topic_meta_list) {
      Node topic_meta_node;
      topic_meta_node["topic_name"] = topic_meta.topic_name;
      topic_meta_node["msg_type"] = topic_meta.msg_type;
      topic_meta_node["serialization_type"] = topic_meta.serialization_type;
      node["topic_meta_list"].push_back(topic_meta_node);
    }

    return node;
  }

  static bool decode(const Node& node, Options& rhs) {
    if (!node.IsMap()) return false;

    rhs.name = node["name"].as<std::string>();
    rhs.bag_path = node["bag_path"].as<std::string>();

    auto mode = aimrt::common::util::StrToLower(node["mode"].as<std::string>());
    if (mode == "imd") {
      rhs.mode = Options::Mode::IMD;
    } else if (mode == "signal") {
      rhs.mode = Options::Mode::SIGNAL;
    } else {
      throw aimrt::common::util::AimRTException("Invalid record mode: " + mode);
    }

    if (node["preparation_duration_s"])
      rhs.preparation_duration_s = node["preparation_duration_s"].as<uint32_t>();
    rhs.executor = node["executor"].as<std::string>();

    if (node["topic_meta_list"] && node["topic_meta_list"].IsSequence()) {
      for (auto& topic_meta_node : node["topic_meta_list"]) {
        auto topic_meta = Options::TopicMeta{
            .topic_name = topic_meta_node["topic_name"].as<std::string>(),
            .msg_type = topic_meta_node["msg_type"].as<std::string>()};

        if (topic_meta_node["serialization_type"])
          topic_meta.serialization_type = topic_meta_node["serialization_type"].as<std::string>();

        rhs.topic_meta_list.emplace_back(std::move(topic_meta));
      }
    }

    return true;
  }
};

template <>
struct convert<aimrt::plugins::record_playback_plugin::RecordPlaybackPlugin::Options::PlaybackAction> {
  using Options = aimrt::plugins::record_playback_plugin::RecordPlaybackPlugin::Options::PlaybackAction;

  static Node encode(const Options& rhs) {
    Node node;

    node["name"] = rhs.name;
    node["bag_path"] = rhs.bag_path;

    if (rhs.mode == Options::Mode::IMD) {
      node["mode"] = "imd";
    } else if (rhs.mode == Options::Mode::SIGNAL) {
      node["mode"] = "signal";
    }

    node["executor"] = rhs.executor;
    node["skip_duration_s"] = rhs.skip_duration_s;
    node["play_duration_s"] = rhs.play_duration_s;

    node["topic_meta_list"] = YAML::Node();
    for (const auto& topic_meta : rhs.topic_meta_list) {
      Node topic_meta_node;
      topic_meta_node["topic_name"] = topic_meta.topic_name;
      topic_meta_node["msg_type"] = topic_meta.msg_type;
      node["topic_meta_list"].push_back(topic_meta_node);
    }

    return node;
  }

  static bool decode(const Node& node, Options& rhs) {
    if (!node.IsMap()) return false;

    rhs.name = node["name"].as<std::string>();
    rhs.bag_path = node["bag_path"].as<std::string>();

    auto mode = aimrt::common::util::StrToLower(node["mode"].as<std::string>());
    if (mode == "imd") {
      rhs.mode = Options::Mode::IMD;
    } else if (mode == "signal") {
      rhs.mode = Options::Mode::SIGNAL;
    } else {
      throw aimrt::common::util::AimRTException("Invalid record mode: " + mode);
    }

    rhs.executor = node["executor"].as<std::string>();

    if (node["skip_duration_s"])
      rhs.skip_duration_s = node["skip_duration_s"].as<uint32_t>();
    if (node["play_duration_s"])
      rhs.play_duration_s = node["play_duration_s"].as<uint32_t>();

    if (node["topic_meta_list"] && node["topic_meta_list"].IsSequence()) {
      for (auto& topic_meta_node : node["topic_meta_list"]) {
        auto topic_meta = Options::TopicMeta{
            .topic_name = topic_meta_node["topic_name"].as<std::string>(),
            .msg_type = topic_meta_node["msg_type"].as<std::string>()};

        rhs.topic_meta_list.emplace_back(std::move(topic_meta));
      }
    }

    return true;
  }
};

template <>
struct convert<aimrt::plugins::record_playback_plugin::RecordPlaybackPlugin::Options> {
  using Options = aimrt::plugins::record_playback_plugin::RecordPlaybackPlugin::Options;

  static Node encode(const Options& rhs) {
    Node node;

    node["type_support_pkgs"] = YAML::Node();
    for (const auto& type_support_pkg : rhs.type_support_pkgs) {
      Node type_support_pkg_node;
      type_support_pkg_node["path"] = type_support_pkg.path;
      node["type_support_pkgs"].push_back(type_support_pkg_node);
    }

    node["record_actions"] = YAML::Node();
    for (const auto& record_action : rhs.record_actions) {
      node["record_actions"].push_back(record_action);
    }

    node["playback_actions"] = YAML::Node();
    for (const auto& playback_action : rhs.playback_actions) {
      node["playback_actions"].push_back(playback_action);
    }

    return node;
  }

  static bool decode(const Node& node, Options& rhs) {
    if (!node.IsMap()) return false;

    if (node["type_support_pkgs"] && node["type_support_pkgs"].IsSequence()) {
      for (auto& type_support_pkg_node : node["type_support_pkgs"]) {
        auto type_support_pkg = Options::TypeSupportPkg{
            .path = type_support_pkg_node["path"].as<std::string>()};

        rhs.type_support_pkgs.emplace_back(std::move(type_support_pkg));
      }
    }

    if (node["record_actions"] && node["record_actions"].IsSequence()) {
      for (auto& record_action_node : node["record_actions"]) {
        rhs.record_actions.emplace_back(record_action_node.as<Options::RecordAction>());
      }
    }

    if (node["playback_actions"] && node["playback_actions"].IsSequence()) {
      for (auto& playback_action_node : node["playback_actions"]) {
        rhs.playback_actions.emplace_back(playback_action_node.as<Options::PlaybackAction>());
      }
    }

    return true;
  }
};
}  // namespace YAML

namespace aimrt::plugins::record_playback_plugin {

co::Task<aimrt::rpc::Status> DebugLogServerFilter(
    aimrt::rpc::ContextRef ctx, const void* req_ptr, void* rsp_ptr,
    const aimrt::rpc::CoRpcHandle& next) {
  AIMRT_INFO("Svr get new rpc call. req: {}",
             aimrt::Pb2CompactJson(*static_cast<const google::protobuf::Message*>(req_ptr)));
  const auto& status = co_await next(ctx, req_ptr, rsp_ptr);
  AIMRT_INFO("Svr handle rpc completed, status: {}, rsp: {}",
             status.ToString(),
             aimrt::Pb2CompactJson(*static_cast<const google::protobuf::Message*>(rsp_ptr)));
  co_return status;
}

bool RecordPlaybackPlugin::Initialize(runtime::core::AimRTCore* core_ptr) noexcept {
  try {
    core_ptr_ = core_ptr;

    YAML::Node plugin_options_node = core_ptr_->GetPluginManager().GetPluginOptionsNode(Name());

    if (plugin_options_node && !plugin_options_node.IsNull()) {
      options_ = plugin_options_node.as<Options>();
    }

    init_flag_ = true;

    core_ptr_->RegisterHookFunc(runtime::core::AimRTCore::State::PostInitLog,
                                [this] { SetPluginLogger(); });

    core_ptr_->RegisterHookFunc(runtime::core::AimRTCore::State::PreInitModules,
                                [this] { RegisterRpcService(); });

    plugin_options_node = options_;
    return true;
  } catch (const std::exception& e) {
    AIMRT_ERROR("Initialize failed, {}", e.what());
  }

  return false;
}

void RecordPlaybackPlugin::Shutdown() noexcept {
  try {
    if (!init_flag_) return;

  } catch (const std::exception& e) {
    AIMRT_ERROR("Shutdown failed, {}", e.what());
  }
}

void RecordPlaybackPlugin::SetPluginLogger() {
  SetLogger(aimrt::logger::LoggerRef(
      core_ptr_->GetLoggerManager().GetLoggerProxy().NativeHandle()));
}

void RecordPlaybackPlugin::RegisterRpcService() {
  // 注册rpc服务
  service_ptr_ = std::make_unique<RecordPlaybackServiceImpl>();
  service_ptr_->RegisterFilter(DebugLogServerFilter);

  auto rpc_handle_ref = aimrt::rpc::RpcHandleRef(
      core_ptr_->GetRpcManager().GetRpcHandleProxy().NativeHandle());

  bool ret = rpc_handle_ref.RegisterService(service_ptr_.get());
  AIMRT_CHECK_ERROR(ret, "Register service failed.");
}

}  // namespace aimrt::plugins::record_playback_plugin
