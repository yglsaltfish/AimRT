#include "record_playback_plugin/record_playback_plugin.h"

#include "aimrt_module_cpp_interface/rpc/rpc_handle.h"
#include "aimrt_module_protobuf_interface/util/protobuf_tools.h"
#include "core/aimrt_core.h"
#include "record_playback_plugin/global.h"
#include "util/string_util.h"
#include "util/time_util.h"

namespace YAML {

template <>
struct convert<aimrt::plugins::record_playback_plugin::RecordPlaybackPlugin::Options::RecordAction> {
  using Options = aimrt::plugins::record_playback_plugin::RecordPlaybackPlugin::Options::RecordAction;

  static Node encode(const Options& rhs) {
    Node node;

    node["name"] = rhs.name;
    node["bag_path"] = rhs.bag_path;
    node["max_bag_size_m"] = rhs.max_bag_size_m;

    if (rhs.mode == Options::Mode::IMD) {
      node["mode"] = "imd";
    } else if (rhs.mode == Options::Mode::SIGNAL) {
      node["mode"] = "signal";
    }

    node["max_preparation_duration_s"] = rhs.max_preparation_duration_s;
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

    if (node["max_bag_size_m"])
      rhs.max_bag_size_m = node["max_bag_size_m"].as<uint32_t>();

    auto mode = aimrt::common::util::StrToLower(node["mode"].as<std::string>());
    if (mode == "imd") {
      rhs.mode = Options::Mode::IMD;
    } else if (mode == "signal") {
      rhs.mode = Options::Mode::SIGNAL;
    } else {
      throw aimrt::common::util::AimRTException("Invalid record mode: " + mode);
    }

    if (node["max_preparation_duration_s"])
      rhs.max_preparation_duration_s = node["max_preparation_duration_s"].as<uint32_t>();

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
  AIMRT_TRACE("Svr get new rpc call. req: {}",
              aimrt::Pb2CompactJson(*static_cast<const google::protobuf::Message*>(req_ptr)));
  const auto& status = co_await next(ctx, req_ptr, rsp_ptr);
  AIMRT_TRACE("Svr handle rpc completed, status: {}, rsp: {}",
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

    // type support
    for (auto& type_support_pkg : options_.type_support_pkgs) {
      // 检查重复pkg
      auto finditr = std::find_if(
          options_.type_support_pkgs.begin(), options_.type_support_pkgs.end(),
          [&type_support_pkg](const auto& op) {
            if (&type_support_pkg == &op) return false;
            return op.path == type_support_pkg.path;
          });
      AIMRT_CHECK_ERROR_THROW(finditr == options_.type_support_pkgs.end(),
                              "Duplicate pkg path {}", type_support_pkg.path);

      InitTypeSupport(type_support_pkg);
    }

    AIMRT_TRACE("Load {} pkg and {} type.",
                type_support_pkg_loader_vec_.size(), type_support_map_.size());

    // record
    for (auto& record_action_options : options_.record_actions) {
      // 检查重复 record
      auto finditr = std::find_if(
          options_.record_actions.begin(), options_.record_actions.end(),
          [&record_action_options](const auto& op) {
            if (&record_action_options == &op) return false;
            return op.name == record_action_options.name;
          });
      AIMRT_CHECK_ERROR_THROW(finditr == options_.record_actions.end(),
                              "Duplicate record action {}", record_action_options.name);

      InitRecordAction(record_action_options);
    }

    // playback
    for (auto& playback_action_options : options_.playback_actions) {
      // 检查重复 playback
      auto finditr = std::find_if(
          options_.playback_actions.begin(), options_.playback_actions.end(),
          [&playback_action_options](const auto& op) {
            if (&playback_action_options == &op) return false;
            return op.name == playback_action_options.name;
          });
      AIMRT_CHECK_ERROR_THROW(finditr == options_.playback_actions.end(),
                              "Duplicate playback action {}", playback_action_options.name);

      InitPlaybackAction(playback_action_options);
    }

    core_ptr_->RegisterHookFunc(runtime::core::AimRTCore::State::PostInitLog,
                                [this] { SetPluginLogger(); });

    core_ptr_->RegisterHookFunc(runtime::core::AimRTCore::State::PreInitModules,
                                [this] {
                                  RegisterRpcService();
                                  RegisterRecordChannel();
                                  RegisterPlaybackChannel();
                                });

    core_ptr_->RegisterHookFunc(runtime::core::AimRTCore::State::PostStart,
                                [this] { StartPlayback(); });

    core_ptr_->RegisterHookFunc(runtime::core::AimRTCore::State::PreShutdown,
                                [this] { StopPlayback(); });

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

RecordPlaybackPlugin::RecordActionWrapper* RecordPlaybackPlugin::GetRecordActionWrapper(
    std::string_view action_name) const {
  auto finditr = record_action_map_.find(action_name);
  if (finditr != record_action_map_.end())
    return finditr->second.get();

  return nullptr;
}

RecordPlaybackPlugin::PlaybackActionWrapper* RecordPlaybackPlugin::GetPlaybackActionWrapper(
    std::string_view action_name) const {
  auto finditr = playback_action_map_.find(action_name);
  if (finditr != playback_action_map_.end())
    return finditr->second.get();

  return nullptr;
}

void RecordPlaybackPlugin::InitTypeSupport(Options::TypeSupportPkg& options) {
  auto loader_ptr = std::make_unique<TypeSupportPkgLoader>();
  loader_ptr->LoadTypeSupportPkg(options.path);

  options.path = loader_ptr->GetDynamicLib().GetLibFullPath();

  auto type_support_array = loader_ptr->GetTypeSupportArray();

  for (auto item : type_support_array) {
    aimrt::util::TypeSupportRef type_support_ref(item);
    auto type_name = type_support_ref.TypeName();

    // 检查重复 type
    auto finditr = type_support_map_.find(type_name);
    if (finditr != type_support_map_.end()) {
      AIMRT_WARN("Duplicate msg type '{}' in {} and {}.",
                 type_name,
                 options.path,
                 finditr->second.options.path);
      continue;
    }

    type_support_map_.emplace(
        type_name,
        TypeSupportWrapper{
            .options = options,
            .type_support_ref = type_support_ref,
            .loader_ptr = loader_ptr.get()});
  }

  type_support_pkg_loader_vec_.emplace_back(std::move(loader_ptr));
}

void RecordPlaybackPlugin::InitRecordAction(Options::RecordAction& options) {
  auto wrapper_ptr = std::make_unique<RecordActionWrapper>(options);

  // 检查 topic meta
  for (auto& topic_meta : options.topic_meta_list) {
    // 检查消息类型
    auto finditr = type_support_map_.find(topic_meta.msg_type);
    AIMRT_CHECK_ERROR_THROW(finditr != type_support_map_.end(),
                            "Can not find type '{}' in any type support pkg!", topic_meta.msg_type);

    const auto& type_support_wrapper = finditr->second;
    auto type_support_ref = type_support_wrapper.type_support_ref;

    // 检查序列化类型
    if (!topic_meta.serialization_type.empty()) {
      bool check_ret = type_support_ref.CheckSerializationTypeSupported(topic_meta.serialization_type);
      AIMRT_CHECK_ERROR_THROW(check_ret,
                              "Msg type '{}' does not support serialization type '{}'.",
                              topic_meta.msg_type, topic_meta.serialization_type);
    } else {
      topic_meta.serialization_type = type_support_ref.DefaultSerializationType();
    }
  }

  // 初始化 db
  auto tm = aimrt::common::util::GetCurTm();
  char buf[17];  // _YYYYMMDD_hhmmss
  snprintf(buf, sizeof(buf), "_%04d%02d%02d_%02d%02d%02d",
           (tm.tm_year + 1900) % 10000u, (tm.tm_mon + 1) % 100u, (tm.tm_mday) % 100u,
           (tm.tm_hour) % 100u, (tm.tm_min) % 100u, (tm.tm_sec) % 100u);
  auto bag_base_name = "aimrtbag_" + options.name + std::string(buf);

  std::filesystem::path parent_bag_path = std::filesystem::absolute(options.bag_path);
  if (!(std::filesystem::exists(parent_bag_path) && std::filesystem::is_directory(parent_bag_path))) {
    std::filesystem::create_directories(parent_bag_path);
  }
  options.bag_path = std::filesystem::canonical(parent_bag_path).string();

  std::filesystem::path bag_path = parent_bag_path / bag_base_name;
  AIMRT_CHECK_ERROR_THROW(!std::filesystem::exists(bag_path),
                          "Bag path '{}' is exist!", bag_path.string());

  std::filesystem::create_directories(bag_path);

  for (auto& topic_meta : options.topic_meta_list) {
    wrapper_ptr->db_tool.AddTopicMeta(
        topic_meta.topic_name, topic_meta.msg_type, topic_meta.serialization_type);
  }

  RecordDbTool::Options db_tool_op{
      .max_bag_size_m = options.max_bag_size_m,
      .bag_path = bag_path,
      .bag_base_name = bag_base_name};
  wrapper_ptr->db_tool.Initialize(db_tool_op);

  record_action_map_.emplace(options.name, std::move(wrapper_ptr));
}

void RecordPlaybackPlugin::InitPlaybackAction(Options::PlaybackAction& options) {
  auto wrapper_ptr = std::make_unique<PlaybackActionWrapper>(options);

  // 检查 topic meta
  for (auto& topic_meta : options.topic_meta_list) {
    // 检查消息类型
    auto finditr = type_support_map_.find(topic_meta.msg_type);
    AIMRT_CHECK_ERROR_THROW(finditr != type_support_map_.end(),
                            "Can not find type '{}' in any type support pkg!", topic_meta.msg_type);

    const auto& type_support_wrapper = finditr->second;
    auto type_support_ref = type_support_wrapper.type_support_ref;
  }

  options.bag_path = std::filesystem::canonical(std::filesystem::absolute(options.bag_path)).string();

  // 初始化 sqlite
  for (auto& topic_meta : options.topic_meta_list) {
    wrapper_ptr->db_tool.SelectTopicMeta(topic_meta.topic_name, topic_meta.msg_type);
  }

  PlaybackDbTool::Options db_tool_op{
      .bag_path = options.bag_path};
  wrapper_ptr->db_tool.Initialize(db_tool_op);

  playback_action_map_.emplace(options.name, std::move(wrapper_ptr));
}

void RecordPlaybackPlugin::SetPluginLogger() {
  SetLogger(aimrt::logger::LoggerRef(
      core_ptr_->GetLoggerManager().GetLoggerProxy().NativeHandle()));
}

void RecordPlaybackPlugin::RegisterRpcService() {
  // 注册rpc服务
  service_ptr_ = std::make_unique<RecordPlaybackServiceImpl>();
  service_ptr_->SetPluginPtr(this);
  service_ptr_->RegisterFilter(DebugLogServerFilter);

  auto rpc_handle_ref = aimrt::rpc::RpcHandleRef(
      core_ptr_->GetRpcManager().GetRpcHandleProxy().NativeHandle());

  bool ret = rpc_handle_ref.RegisterService(service_ptr_.get());
  AIMRT_CHECK_ERROR(ret, "Register service failed.");
}

void RecordPlaybackPlugin::RegisterRecordChannel() {
  using namespace aimrt::runtime::core::channel;

  struct Key {
    std::string_view topic_name;
    std::string_view msg_type;

    bool operator==(const Key& rhs) const {
      return topic_name == rhs.topic_name && msg_type == rhs.msg_type;
    }

    struct Hash {
      std::size_t operator()(const Key& k) const {
        return (std::hash<std::string_view>()(k.topic_name)) ^
               (std::hash<std::string_view>()(k.msg_type));
      }
    };
  };

  using RecordFunc = std::function<void(uint64_t, MsgWrapper&)>;
  std::unordered_map<Key, std::vector<RecordFunc>, Key::Hash> recore_func_map;

  for (auto& record_action_itr : record_action_map_) {
    auto& record_action_wrapper = *(record_action_itr.second);
    const auto& options = record_action_wrapper.options;

    // 获取executor
    auto executor = core_ptr_->GetExecutorManager().GetExecutor(options.executor);
    AIMRT_CHECK_ERROR_THROW(executor, "Get record executor {} failed!", options.executor);
    AIMRT_CHECK_ERROR_THROW(executor.ThreadSafe(),
                            "Record executor {} is not thread safe!", options.executor);

    record_action_wrapper.executor = executor;

    // RecordFunc
    for (const auto& topic_meta : options.topic_meta_list) {
      RecordFunc recore_func;

      uint32_t topic_id = record_action_wrapper.db_tool.GetTopicIndex(topic_meta.topic_name, topic_meta.msg_type);

      if (options.mode == Options::RecordAction::Mode::IMD) {
        // IMD模式
        recore_func =
            [&record_action_wrapper, topic_id, serialization_type{topic_meta.serialization_type}](
                uint64_t cur_timestamp, MsgWrapper& msg_wrapper) {
              auto buffer_view_ptr = msg_wrapper.SerializeMsgWithCache(serialization_type);
              if (!buffer_view_ptr) [[unlikely]] {
                AIMRT_WARN("Can not serialize msg type '{}' with serialization type '{}'.",
                           msg_wrapper.info.msg_type, serialization_type);
                return;
              }

              record_action_wrapper.executor.Execute(
                  [&record_action_wrapper, topic_id, cur_timestamp, buffer_view_ptr]() {
                    record_action_wrapper.db_tool.AddRecord(
                        RecordDbTool::OneRecord{
                            .timestamp = cur_timestamp,
                            .topic_index = topic_id,
                            .buffer_view_ptr = buffer_view_ptr});
                  });
            };
      } else if (options.mode == Options::RecordAction::Mode::SIGNAL) {
        // Signal模式
        uint64_t max_preparation_duration_ns = options.max_preparation_duration_s * 1000000000;
        recore_func =
            [&record_action_wrapper, topic_id, max_preparation_duration_ns, serialization_type{topic_meta.serialization_type}](
                uint64_t cur_timestamp, MsgWrapper& msg_wrapper) {
              auto buffer_view_ptr = msg_wrapper.SerializeMsgWithCache(serialization_type);
              if (!buffer_view_ptr) [[unlikely]] {
                AIMRT_WARN("Can not serialize msg type '{}' with serialization type '{}'.",
                           msg_wrapper.info.msg_type, serialization_type);
                return;
              }

              record_action_wrapper.executor.Execute(
                  [&record_action_wrapper, topic_id, max_preparation_duration_ns, cur_timestamp, buffer_view_ptr]() {
                    RecordDbTool::OneRecord record{
                        .timestamp = cur_timestamp,
                        .topic_index = topic_id,
                        .buffer_view_ptr = buffer_view_ptr};

                    if (record_action_wrapper.record_flag) {
                      if (record_action_wrapper.stop_record_timestamp != 0 &&
                          cur_timestamp > record_action_wrapper.stop_record_timestamp) {
                        record_action_wrapper.record_flag = false;
                      }
                    }

                    if (record_action_wrapper.record_flag) {
                      record_action_wrapper.db_tool.AddRecord(record);
                    } else {
                      record_action_wrapper.cur_cache.emplace_back(std::move(record));

                      auto cur_cache_begin_timestamp = record_action_wrapper.cur_cache.begin()->timestamp;

                      if ((cur_timestamp - cur_cache_begin_timestamp) > max_preparation_duration_ns) {
                        record_action_wrapper.cur_cache.swap(record_action_wrapper.last_cache);
                        record_action_wrapper.cur_cache.clear();
                      }
                    }
                  });
            };
      }

      recore_func_map[Key{topic_meta.topic_name, topic_meta.msg_type}].emplace_back(std::move(recore_func));
    }
  }

  // Subscribe
  for (auto& recore_func_itr : recore_func_map) {
    const auto& key = recore_func_itr.first;

    auto finditr = type_support_map_.find(key.msg_type);

    const auto& type_support_wrapper = finditr->second;
    auto type_support_ref = type_support_wrapper.type_support_ref;

    SubscribeWrapper sub_wrapper;
    sub_wrapper.info = TopicInfo{
        .msg_type = key.msg_type,
        .topic_name = key.topic_name,
        .pkg_path = type_support_wrapper.options.path,
        .module_name = "core",
        .msg_type_support_ref = type_support_ref};

    // 小优化
    auto& recore_func_vec = recore_func_itr.second;
    if (recore_func_vec.size() == 1) {
      sub_wrapper.callback =
          [recore_func{std::move(recore_func_vec[0])}](
              MsgWrapper& msg_wrapper, std::function<void()>&& release_callback) {
            auto cur_timestamp = aimrt::common::util::GetCurTimestampNs();

            recore_func(cur_timestamp, msg_wrapper);

            release_callback();
          };
    } else {
      sub_wrapper.callback =
          [recore_func_vec{std::move(recore_func_vec)}](
              MsgWrapper& msg_wrapper, std::function<void()>&& release_callback) {
            auto cur_timestamp = aimrt::common::util::GetCurTimestampNs();

            for (const auto& recore_func : recore_func_vec)
              recore_func(cur_timestamp, msg_wrapper);

            release_callback();
          };
    }

    core_ptr_->GetChannelManager().Subscribe(std::move(sub_wrapper));
  }
}

void RecordPlaybackPlugin::RegisterPlaybackChannel() {
  using namespace aimrt::runtime::core::channel;

  struct Key {
    std::string_view topic_name;
    std::string_view msg_type;

    bool operator==(const Key& rhs) const {
      return topic_name == rhs.topic_name && msg_type == rhs.msg_type;
    }

    struct Hash {
      std::size_t operator()(const Key& k) const {
        return (std::hash<std::string_view>()(k.topic_name)) ^
               (std::hash<std::string_view>()(k.msg_type));
      }
    };
  };

  std::unordered_map<Key, std::vector<PlaybackActionWrapper*>, Key::Hash> playback_topic_meta_map;

  // 处理 playback action
  for (auto& playback_action_itr : playback_action_map_) {
    auto& playback_action_wrapper = *(playback_action_itr.second);
    const auto& options = playback_action_wrapper.options;

    // 获取executor
    auto executor = core_ptr_->GetExecutorManager().GetExecutor(options.executor);
    AIMRT_CHECK_ERROR_THROW(executor, "Get record executor {} failed!", options.executor);
    AIMRT_CHECK_ERROR_THROW(executor.SupportTimerSchedule(),
                            "Record executor {} do not support time schedule!", options.executor);

    playback_action_wrapper.executor = executor;

    for (const auto& topic_meta : options.topic_meta_list) {
      playback_topic_meta_map[Key{topic_meta.topic_name, topic_meta.msg_type}]
          .emplace_back(&playback_action_wrapper);
    }
  }

  // RegisterPublishType
  for (auto& item : playback_topic_meta_map) {
    auto& key = item.first;
    auto finditr = type_support_map_.find(key.msg_type);

    const auto& type_support_wrapper = finditr->second;
    auto type_support_ref = type_support_wrapper.type_support_ref;

    PublishTypeWrapper pub_type_wrapper;
    pub_type_wrapper.info = TopicInfo{
        .msg_type = key.msg_type,
        .topic_name = key.topic_name,
        .pkg_path = type_support_wrapper.options.path,
        .module_name = "core",
        .msg_type_support_ref = type_support_ref};

    bool ret = core_ptr_->GetChannelManager().RegisterPublishType(std::move(pub_type_wrapper));
    AIMRT_CHECK_ERROR_THROW(ret, "Register publish type failed!");

    auto pub_type_wrapper_ptr = core_ptr_->GetChannelManager().GetChannelRegistry()->GetPublishTypeWrapperPtr(
        key.msg_type, key.topic_name, type_support_wrapper.options.path, "core");
    AIMRT_CHECK_ERROR_THROW(pub_type_wrapper_ptr, "Get publish type failed!");

    for (auto playback_action_wrapper_ptr : item.second) {
      auto topic_meta = playback_action_wrapper_ptr->db_tool.GetTopicMeta(key.topic_name, key.msg_type);
      playback_action_wrapper_ptr->publish_meta_map.emplace(
          topic_meta.id,
          PlaybackActionWrapper::PublishMeta{pub_type_wrapper_ptr, topic_meta.serialization_type});
    }
  }
}

void RecordPlaybackPlugin::StartPlayback() {
  for (auto& playback_action_itr : playback_action_map_) {
    auto& playback_action_wrapper = *(playback_action_itr.second);
    const auto& options = playback_action_wrapper.options;

    // 只开始imd模式的action
    if (options.mode != Options::PlaybackAction::Mode::IMD) continue;

    playback_action_wrapper.db_tool.StartPlayback(options.skip_duration_s, options.play_duration_s);

    playback_action_wrapper.run_flag.store(true);
    playback_action_wrapper.start_timestamp = aimrt::common::util::GetCurTimestampNs();

    // 开始两个task包
    AddPlaybackTasks(playback_action_wrapper);
    AddPlaybackTasks(playback_action_wrapper);
  }
}

void RecordPlaybackPlugin::StopPlayback() {
  for (auto& playback_action_itr : playback_action_map_) {
    auto& playback_action_wrapper = *(playback_action_itr.second);

    playback_action_wrapper.run_flag.store(false);
  }
}

void RecordPlaybackPlugin::AddPlaybackTasks(PlaybackActionWrapper& wrapper) {
  using namespace aimrt::runtime::core::channel;

  // get records
  auto records = wrapper.db_tool.GetRecords();

  if (records.empty()) return;

  size_t len = records.size();
  for (size_t ii = 0; ii < len; ++ii) {
    // add publish task
    bool last_record_flag = (ii == len - 1);
    auto& record = records[ii];
    auto tp = aimrt::common::util::GetTimePointFromTimestampNs(wrapper.start_timestamp + record.dt);
    wrapper.executor.ExecuteAt(
        tp,
        [this, &wrapper, record{std::move(record)}, last_record_flag]() {
          if (!(wrapper.run_flag.load())) [[unlikely]]
            return;

          auto finditr = wrapper.publish_meta_map.find(record.topic_id);
          if (finditr == wrapper.publish_meta_map.end()) [[unlikely]] {
            AIMRT_WARN("Invalid topic id: {}.", record.topic_id);
            return;
          }

          const auto* pub_type_wrapper_ptr = finditr->second.pub_type_wrapper_ptr;
          const auto& serialization_type = finditr->second.serialization_type;
          auto msg_type_support_ref = pub_type_wrapper_ptr->info.msg_type_support_ref;

          aimrt::channel::Context ctx;
          // TODO: 记录ctx

          MsgWrapper msg_wrapper{
              .info = pub_type_wrapper_ptr->info,
              .msg_ptr = nullptr,
              .ctx_ref = ctx};

          msg_wrapper.serialization_cache.emplace(serialization_type, record.buffer_view_ptr);

          core_ptr_->GetChannelManager().Publish(std::move(msg_wrapper));

          // add new playback tasks to last publish task
          if (last_record_flag) [[unlikely]]
            AddPlaybackTasks(wrapper);
        });
  }
}

}  // namespace aimrt::plugins::record_playback_plugin
