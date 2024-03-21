

#include <iostream>
#include <regex>
#include <thread>

#include "aimrt_module_cpp_interface/util/type_support.h"
#include "lcm_channel_backend.h"

#include "lcm_plugin/global.h"

namespace YAML {
template <>
struct convert<aimrt::plugins::lcm_plugin::LcmChannelBackend::Options> {
  using Options = aimrt::plugins::lcm_plugin::LcmChannelBackend::Options;

  static Node encode(const Options& rhs) {
    Node node;

    if (!rhs.sub_default_executor.empty())
      node["sub_default_executor"] = rhs.sub_default_executor;

    if (!rhs.sub_topic_options.empty()) {
      for (auto sub_topic_option : rhs.sub_topic_options) {
        Node topic_options_node;
        topic_options_node["topic_name"] = sub_topic_option.topic_name;
        topic_options_node["executor"] = sub_topic_option.executor;
        topic_options_node["priority"] = sub_topic_option.priority;
        topic_options_node["lcm_url"] = sub_topic_option.lcm_url;
        topic_options_node["lcm_dispatcher_executor"] = sub_topic_option.lcm_dispatcher_executor;
        node["sub_topic_options"].push_back(topic_options_node);
      }
    }

    for (auto& passable_pub_topic : rhs.passable_pub_topics) {
      node["passable_pub_topics"].push_back(passable_pub_topic);
    }

    for (auto& unpassable_pub_topic : rhs.unpassable_pub_topics) {
      node["unpassable_pub_topics"].push_back(unpassable_pub_topic);
    }

    return node;
  }

  static bool decode(const Node& node, Options& rhs) {
    // 如果存在 sub_default_executor 则赋值
    if (node["sub_default_executor"]) {
      rhs.sub_default_executor = node["sub_default_executor"].as<std::string>();
    }

    // 如果存在 sub_topic_options 则赋值
    if (node["sub_topic_options"]) {
      for (auto sub_topic_option : node["sub_topic_options"]) {
        aimrt::plugins::lcm_plugin::LcmChannelBackend::SubTopicOptions options;
        if (!sub_topic_option["topic_name"]) {
          std::cout << "sub_topic_options must have topic_name" << std::endl;
          return false;
        }

        options.topic_name = sub_topic_option["topic_name"].as<std::string>();

        if (sub_topic_option["executor"]) {
          options.executor = sub_topic_option["executor"].as<std::string>();  // exist, get it
        }

        if (sub_topic_option["lcm_url"]) {
          options.lcm_url = sub_topic_option["lcm_url"].as<std::string>();  // exist, get it
        } else {
          options.lcm_url = "";  // not exist, use default lcm url
        }

        // 如果存在 lcm_dispatcher_executor 则赋值
        if (sub_topic_option["lcm_dispatcher_executor"]) {
          options.lcm_dispatcher_executor = sub_topic_option["lcm_dispatcher_executor"].as<std::string>();
        }

        // 如果存在 priority 则赋值
        if (sub_topic_option["priority"]) {
          options.priority = sub_topic_option["priority"].as<int32_t>();
        }

        rhs.sub_topic_options.push_back(options);
      }
    }

    // 如果存在 pub_topic_options 则赋值
    if (node["pub_topic_options"]) {
      for (auto pub_topic_option : node["pub_topic_options"]) {
        aimrt::plugins::lcm_plugin::LcmChannelBackend::PubTopicOptions options;
        if (!pub_topic_option["topic_name"]) {
          std::cout << "pub_topic_options must have topic_name" << std::endl;
          return false;
        }

        options.topic_name = pub_topic_option["topic_name"].as<std::string>();

        if (pub_topic_option["lcm_url"]) {
          options.lcm_url = pub_topic_option["lcm_url"].as<std::string>();  // exist, get it
        } else {
          options.lcm_url = "";  // not exist, use default lcm url
        }

        // 如果存在 priority 则赋值
        if (pub_topic_option["priority"]) {
          options.priority = pub_topic_option["priority"].as<int32_t>();
        }

        rhs.pub_topic_options.push_back(options);
      }
    }

    // 如果存在 passable_pub_topics 则赋值
    if (node["passable_pub_topics"]) {
      rhs.passable_pub_topics = node["passable_pub_topics"].as<std::list<std::string>>();
    }

    // 如果存在 unpassable_pub_topics 则赋值
    if (node["unpassable_pub_topics"]) {
      rhs.unpassable_pub_topics = node["unpassable_pub_topics"].as<std::list<std::string>>();
    }

    return true;
  }
};
}  // namespace YAML

namespace aimrt::plugins::lcm_plugin {

void LcmChannelBackend::Initialize(
    YAML::Node options_node,
    const runtime::core::channel::ChannelRegistry* channel_registry_ptr,
    runtime::core::channel::ContextManager* context_manager_ptr) {
  AIMRT_CHECK_ERROR_THROW(
      std::atomic_exchange(&state_, State::Init) == State::PreInit,
      "share memory channel backend can only be initialized once.");

  if (options_node && !options_node.IsNull()) {
    options_ = options_node.as<Options>();
  }

  // subscriber options
  const auto default_lcm_url = LcmManager::GetInstance().GetDefaultLcmUrl();

  for (auto& pub_topic_option : options_.pub_topic_options) {
    if (pub_topic_option.lcm_url.empty()) {
      pub_topic_option.lcm_url = default_lcm_url;
    }
  }

  // 记录 lcm_url:lcm_dispatcher_executor
  std::map<std::string, std::string> lcm_info_map;
  for (auto& sub_topic_option : options_.sub_topic_options) {
    // 如果 sub_topic_option.lcm_url 为空，则使用默认值
    if (sub_topic_option.lcm_url.empty()) {
      sub_topic_option.lcm_url = default_lcm_url;
    }

    // 如果用户配置了 executor
    if (!sub_topic_option.executor.empty()) {
      // 校验 executor 是否存在，如果不存在，则报错
      AIMRT_CHECK_ERROR_THROW((get_executor_func_(sub_topic_option.executor)),
                              "Executor '{}' is not registered.", sub_topic_option.executor);
      // 如果用户没有配置 lcm_dispatcher_executor，则尝试使用 executor 作为lcm dispatcher 的 executor
      if (sub_topic_option.lcm_dispatcher_executor.empty()) {
        sub_topic_option.lcm_dispatcher_executor = sub_topic_option.executor;
      }
    } else {
      // 如果 订阅者没有指定 executor，需要校验默认的 executor 是否存在
      AIMRT_CHECK_ERROR_THROW((!options_.sub_default_executor.empty()),
                              "Subscribed topic '{}' has no executor set and no default executor set.", sub_topic_option.topic_name);
      if (!sub_default_executor_ref_) {
        sub_default_executor_ref_ = get_executor_func_(options_.sub_default_executor);
        AIMRT_CHECK_ERROR_THROW(sub_default_executor_ref_, "Default Executor '{}' is not registered.", options_.sub_default_executor);
      }

      // 如果用户没有配置 lcm_dispatcher_executor，则尝试使用默认的 executor 作为lcm dispatcher 的 executor
      if (sub_topic_option.lcm_dispatcher_executor.empty()) {
        sub_topic_option.lcm_dispatcher_executor = options_.sub_default_executor;
      }
    }

    // lcm url 去除 '?ttl=' 与后面字符串
    std::string lcm_url = sub_topic_option.lcm_url;
    auto pos = lcm_url.find("?");
    if (pos != std::string::npos) {
      lcm_url = lcm_url.substr(0, pos);
    }

    if (lcm_info_map.find(lcm_url) == lcm_info_map.end()) {
      lcm_info_map[lcm_url] = sub_topic_option.lcm_dispatcher_executor;
      AIMRT_CHECK_ERROR_THROW((get_executor_func_(sub_topic_option.lcm_dispatcher_executor)),
                              "Lcm dispatcher executor '{}' is not registered.", sub_topic_option.lcm_dispatcher_executor);
    } else {
      // 如果出现同样的 lcm url，则需要保证 lcm_dispatcher_executor 一致
      if (lcm_info_map[lcm_url] != sub_topic_option.lcm_dispatcher_executor) {
        AIMRT_ERROR("topic '{}', lcm url '{}' has different lcm_dispatcher_executor, '{}' and '{}'.",
                    sub_topic_option.topic_name, lcm_url, lcm_info_map[lcm_url], sub_topic_option.lcm_dispatcher_executor);
        return;
      }
    }
  }

  context_manager_ptr_ = context_manager_ptr;

  options_node = options_;  // for dump options
}

void LcmChannelBackend::Start() {
  AIMRT_CHECK_ERROR_THROW(
      std::atomic_exchange(&state_, State::Start) == State::Init,
      "Function can only be called when state is 'Init'.");

  for (auto& [id, pair] : dispatcher_map_) {
    (void)id;
    auto dispatcher_ptr = pair.second;
    pair.first.Execute([this, dispatcher_ptr]() {
      dispatcher_ptr->Run();  // run lcm dispatcher
    });
  }
}

void LcmChannelBackend::Shutdown() {
  if (std::atomic_exchange(&state_, State::Shutdown) == State::Shutdown)
    return;

  for (auto& [id, lcm] : publisher_map_) {
    (void)id;
    lcm = nullptr;
  }

  for (auto& [id, pair] : dispatcher_map_) {
    (void)id;
    pair.second->Shutdown();
  }

  for (auto& [id, subscriber_info] : subscriber_info_map_) {
    (void)id;
    subscriber_info = nullptr;
  }

  publisher_map_.clear();
  dispatcher_map_.clear();
  subscriber_info_map_.clear();
}

bool LcmChannelBackend::RegisterPublishType(const runtime::core::channel::PublishTypeWrapper& publish_type_wrapper) noexcept {
  if (state_.load() != State::Init) {
    AIMRT_ERROR("Publish type can only be registered when state is 'Init'.");
    return false;
  }

  LcmPtr lcm = nullptr;
  std::string lcm_url = "";

  try {
    // 如果 options_.passable_pub_topics 没有配置，则默认为不通行
    if (options_.passable_pub_topics.empty()) {
      AIMRT_INFO("publish topic '{}' is unpassable publish by lcm plugin.", publish_type_wrapper.topic_name);
      return true;
    }

    bool passable = false;
    for (auto& passable_pub_topic : options_.passable_pub_topics) {
      std::regex pattern(passable_pub_topic);
      if ((passable_pub_topic.empty()) || (std::regex_match(std::string(publish_type_wrapper.topic_name), pattern))) {
        passable = true;
        break;
      }
    }

    // topic需要符合规则，需要允许通行的topic才能注册，如果不允许通行，则不允许注册，属于预期内配置，返回true
    for (auto& unpassable_pub_topic : options_.unpassable_pub_topics) {
      std::regex pattern(unpassable_pub_topic);
      if (std::regex_match(std::string(publish_type_wrapper.topic_name), pattern)) {
        passable = false;
        break;
      }
    }

    if (!passable) {
      AIMRT_INFO("publish topic '{}' is unpassable publish by lcm plugin.", publish_type_wrapper.topic_name);
      return true;
    }

    auto& ops = options_.pub_topic_options;
    int32_t last_priority = std::numeric_limits<int32_t>::min();
    // 倒序遍历 options_ 中的 pub_topic_options, 优先级高的 lcm_url 优先级高
    for (auto op = ops.rbegin(); op != ops.rend(); op++) {
      // 使用正则表达式进行匹配
      std::regex pattern(op->topic_name);
      if (std::regex_match(std::string(publish_type_wrapper.topic_name), pattern)) {
        // 如果优先级比当前优先级高，则更新优先级，优先级必须大于等于0才有效
        if (last_priority == std::numeric_limits<int32_t>::min() ||
            last_priority > op->priority) {
          last_priority = op->priority;
          lcm_url = op->lcm_url;
        }
      }
    }

    lcm = LcmManager::GetInstance().GetLcm(lcm_url);

    if (lcm == nullptr) {
      AIMRT_ERROR("Lcm url '{}' is not registered.", lcm_url);
      return false;
    }

  } catch (const std::exception& e) {
    AIMRT_ERROR("check lcm plugin publish topic regex error: {}", e.what());
    return false;
  }

  uint64_t msg_hash = std::hash<std::string>{}(
      std::string(publish_type_wrapper.topic_name) + std::string(publish_type_wrapper.msg_type));

  if (publisher_map_.count(msg_hash) > 0) {
    AIMRT_ERROR("Publish topic '{}' type '{}' has been registered.",
                publish_type_wrapper.topic_name, publish_type_wrapper.msg_type);
    return false;
  }

  publisher_map_.emplace(msg_hash, lcm);

  AIMRT_INFO("lcm backend register publish type for topic '{}' success. lcm url: '{}'",
             publish_type_wrapper.topic_name, lcm_url);

  return true;
}

bool LcmChannelBackend::Subscribe(const runtime::core::channel::SubscribeWrapper& subscribe_wrapper) noexcept {
  if (state_.load() != State::Init) {
    AIMRT_ERROR("Subscribe can only be called when state is 'Init'.");
    return false;
  }

  bool is_config = false;
  // 遍历是否配置了订阅列表
  for (auto& sub_topic_option : options_.sub_topic_options) {
    if (sub_topic_option.topic_name == subscribe_wrapper.topic_name) {
      is_config = true;
    }
  }

  if (!is_config) {
    AIMRT_INFO("subscribe topic '{}' is not configured in lcm plugin.", subscribe_wrapper.topic_name);
    return true;
  }

  LcmDispatcherPtr dispatcher_ptr = nullptr;
  SubscriberInfoPtr subscriber_info = nullptr;
  uint64_t msg_hash = std::hash<std::string>{}(std::string(subscribe_wrapper.topic_name) + std::string(subscribe_wrapper.msg_type));

  if (subscriber_info_map_.count(msg_hash) > 0) {
    // 遍历 subscriber_info_map_ 的 module_info_list，如果已经存在相同的内容，则不再添加
    if (std::any_of(
            subscriber_info_map_[msg_hash]->module_info_list.begin(),
            subscriber_info_map_[msg_hash]->module_info_list.end(),
            [&](const auto& module_info) {
              return module_info->module_name == subscribe_wrapper.module_name &&
                     module_info->pkg_path == subscribe_wrapper.pkg_path;
            })) {
      AIMRT_ERROR("Subscribe topic '{}', type '{}' has been registered by lcm plugin.",
                  subscribe_wrapper.topic_name, subscribe_wrapper.msg_type);
      return false;
    }
    subscriber_info = subscriber_info_map_[msg_hash];  // exist, get it
  } else {
    subscriber_info = std::make_shared<SubscriberInfo>();  // not exist, create it
    subscriber_info->msg_type = subscribe_wrapper.msg_type;
    subscriber_info->topic_name = subscribe_wrapper.topic_name;
    subscriber_info_map_.emplace(msg_hash, subscriber_info);  // add it to subscriber_info_map_
  }

  // set module info and pkg path and callback
  std::shared_ptr<ModuleInfo> module_info = std::make_shared<ModuleInfo>(subscribe_wrapper);
  subscriber_info->module_info_list.push_back(module_info);

  // 如果没有配置 sub_topic_options，则默认为不订阅共享内存的 topic
  if (options_.sub_topic_options.empty()) {
    AIMRT_INFO("subscribe topic '{}' is unpassable subscribe by lcm plugin.", subscribe_wrapper.topic_name);
    return true;
  }

  try {
    auto& ops = options_.sub_topic_options;
    // 倒序遍历 options_ 中的 sub_topic_options, 优先级高的 executor、lcm_url 优先级高
    for (auto op = ops.rbegin(); op != ops.rend(); op++) {
      // 使用正则表达式进行匹配
      std::regex pattern(op->topic_name);
      if (std::regex_match(std::string(subscribe_wrapper.topic_name), pattern)) {
        // 如果优先级比当前优先级高，则更新优先级，优先级必须大于等于0才有效
        if (subscriber_info->priority == std::numeric_limits<int32_t>::min() ||
            subscriber_info->priority > op->priority) {
          subscriber_info->lcm_url = op->lcm_url;
          subscriber_info->priority = op->priority;
          subscriber_info->lcm_dispatcher_executor = op->lcm_dispatcher_executor;
          if (op->executor.empty()) {
            subscriber_info->executor = sub_default_executor_ref_;
          } else {
            subscriber_info->executor = get_executor_func_(op->executor);
          }
        }
      }
    }
  } catch (const std::exception& e) {
    AIMRT_ERROR("check lcm plugin subscribe topic regex error: {}", e.what());
    return false;
  }

  std::string dispatcher_executor = subscriber_info->lcm_dispatcher_executor;
  uint64_t dispatcher_key = std::hash<std::string>{}(dispatcher_executor);
  if (dispatcher_map_.count(dispatcher_key) != 0) {
    auto& pair = dispatcher_map_[dispatcher_key];
    dispatcher_ptr = pair.second;
  } else {
    dispatcher_ptr = std::make_shared<LcmDispatcher>();
    dispatcher_map_.emplace(dispatcher_key, std::make_pair(get_executor_func_(dispatcher_executor), dispatcher_ptr));
  }

  if (dispatcher_ptr == nullptr) {
    AIMRT_ERROR("Lcm url '{}' is not registered.", subscriber_info->lcm_url);
    return false;
  }

  std::string key = subscribe_wrapper.topic_name.data();
  key += subscribe_wrapper.msg_type.data();
  std::string channel = std::to_string(std::hash<std::string>{}(key));

  DisPatcherAttribute dispatcher_attribute;
  dispatcher_attribute.url = subscriber_info->lcm_url;
  dispatcher_attribute.channel_name = channel;

  dispatcher_ptr->AddListener(
      dispatcher_attribute,
      [subscriber_info, context_manager_ptr{context_manager_ptr_}](const void* data, size_t size) {
        for (auto module_info : subscriber_info->module_info_list) {
          const runtime::core::channel::SubscribeWrapper& wrapper = module_info->subscribe_wrapper;

          AIMRT_TRACE("lcm channel backend receive data, topic name: {}, msg type: {}, size: {}",
                      wrapper.topic_name, wrapper.msg_type, size);

          auto get_serialization_type_func =
              [](const runtime::core::channel::SubscribeWrapper& wrapper) -> std::string_view {
            auto type_support_ref = aimrt::util::TypeSupportRef(wrapper.msg_type_support);
            if (type_support_ref.SerializationTypesSupportedNum()) {
              return aimrt::util::ToStdStringView(type_support_ref.SerializationTypesSupportedList()[0]);
            }
            return "";
          };

          auto serialization_type = get_serialization_type_func(wrapper);
          aimrt_buffer_view_t buffer_view;
          buffer_view.data = (const void*)data;
          buffer_view.len = size;

          aimrt_buffer_array_view_t buffer_array_view;
          buffer_array_view.data = &buffer_view;
          buffer_array_view.len = 1;

          auto type_support_ref = aimrt::util::TypeSupportRef(wrapper.msg_type_support);

          std::shared_ptr<void> msg_ptr = type_support_ref.CreateSharedPtr();

          // context
          auto ctx_ptr = context_manager_ptr->NewContextSharedPtr();
          auto ctx_ref = aimrt::channel::ContextRef(ctx_ptr->NativeHandle());
          ctx_ref.SetSerializationType(serialization_type);

          bool deserialize_ret = type_support_ref.Deserialize(
              serialization_type, buffer_array_view, msg_ptr.get());

          if (!deserialize_ret) {
            AIMRT_ERROR("msg deserialization failed in lcm channel,serialization_type {}", serialization_type);
            return;
          }

          if (subscriber_info->executor.ThreadSafe()) {
            // 直接执行
            aimrt::util::Function<aimrt_function_subscriber_release_callback_ops_t> release_callback(
                [msg_ptr, ctx_ptr]() {});
            wrapper.callback(ctx_ptr->NativeHandle(),
                             msg_ptr.get(),
                             release_callback.NativeHandle());
          } else {
            // 放入线程池执行
            subscriber_info->executor.Execute([&wrapper, msg_ptr, ctx_ptr]() {
              aimrt::util::Function<aimrt_function_subscriber_release_callback_ops_t> release_callback(
                  [msg_ptr, ctx_ptr]() {});
              wrapper.callback(ctx_ptr->NativeHandle(),
                               msg_ptr.get(),
                               release_callback.NativeHandle());
            });
          }
        }
      });

  AIMRT_INFO("lcm plugin backend add listener for topic '{}' success. lcm url: '{}', lcm dispatcher executor: '{}', channel executor: '{}'",
             subscribe_wrapper.topic_name, (subscriber_info->lcm_url.empty() ? "default" : subscriber_info->lcm_url),
             dispatcher_executor, subscriber_info->executor.Name());

  return true;
}

void LcmChannelBackend::Publish(const runtime::core::channel::PublishWrapper& publish_wrapper) noexcept {
  static int count = 0;

  uint64_t msg_hash = std::hash<std::string>{}(
      std::string(publish_wrapper.topic_name) + std::string(publish_wrapper.msg_type));

  if (publisher_map_.count(msg_hash) == 0) {
    return;
  }

  auto& lcm = publisher_map_[msg_hash];

  // publisher序列化
  std::shared_ptr<aimrt::util::BufferArray> buffer_array;

  auto get_serialization_type_func = [&publish_wrapper]() -> std::string_view {
    auto type_support_ref = aimrt::util::TypeSupportRef(publish_wrapper.msg_type_support);
    if (type_support_ref.SerializationTypesSupportedNum()) {
      return aimrt::util::ToStdStringView(type_support_ref.SerializationTypesSupportedList()[0]);
    }
    return "";
  };

  auto serialization_type = get_serialization_type_func();

  auto find_serialization_cache_itr = publish_wrapper.serialization_cache.find(serialization_type);  // ros2

  if (find_serialization_cache_itr != publish_wrapper.serialization_cache.end()) {
    buffer_array = find_serialization_cache_itr->second;  // 有缓存
  } else {
    // 没有缓存，序列化一次后放入缓存中
    buffer_array = std::make_shared<aimrt::util::BufferArray>();
    auto publish_type_support_ref = aimrt::util::TypeSupportRef(publish_wrapper.msg_type_support);
    bool serialize_ret = publish_type_support_ref.Serialize(
        serialization_type, publish_wrapper.msg_ptr, buffer_array->NativeHandle());

    if (!serialize_ret) {
      AIMRT_ERROR("msg serialization failed in lcm channel,serialization_type {}", serialization_type);
      return;
    }

    publish_wrapper.serialization_cache.emplace(serialization_type, buffer_array);
  }

  // 将 buffer_array 的分段的数据整合到 std::string
  std::string data;
  for (size_t i = 0; i < buffer_array->Size(); i++) {
    auto buffer_data = buffer_array->Data();
    data.append((char*)buffer_data[i].data, buffer_data[i].len);
  }

  // publish channel=hash<topic::msg_type>
  std::string key = publish_wrapper.topic_name.data();
  key += publish_wrapper.msg_type.data();
  std::string channel = std::to_string(std::hash<std::string>{}(key));
  lcm->publish(channel, data.data(), data.size());
}

void LcmChannelBackend::RegisterGetExecutorFunc(
    const std::function<aimrt::executor::ExecutorRef(std::string_view)>& get_executor_func) {
  AIMRT_CHECK_ERROR_THROW(
      state_.load() == State::PreInit,
      "Function can only be called when state is 'PreInit'.");
  get_executor_func_ = get_executor_func;
}

}  // namespace aimrt::plugins::lcm_plugin
