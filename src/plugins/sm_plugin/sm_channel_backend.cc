

#include <iostream>
#include <regex>
#include <thread>

#include "aimrt_module_cpp_interface/util/type_support.h"
#include "dispatcher/shm_dispatcher.h"
#include "sm_channel_backend.h"
#include "transmitter/shm_transmitter.h"

#include "sm_plugin/global.h"

namespace YAML {
template <>
struct convert<aimrt::plugins::sm_plugin::SmChannelBackend::Options> {
  using Options = aimrt::plugins::sm_plugin::SmChannelBackend::Options;

  static Node encode(const Options& rhs) {
    Node node;

    if (!rhs.sub_default_executor.empty())
      node["sub_default_executor"] = rhs.sub_default_executor;

    for (auto& sub_topic_option : rhs.sub_topic_options) {
      Node options_node;
      options_node["topic_name"] = sub_topic_option.topic_name;
      options_node["executor"] = sub_topic_option.executor;
      options_node["priority"] = sub_topic_option.priority;
      node["sub_topic_options"].push_back(options_node);
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
      for (auto& sub_topic_option : node["sub_topic_options"]) {
        aimrt::plugins::sm_plugin::SmChannelBackend::TopicOptions options;
        if (!sub_topic_option["topic_name"]) {
          std::cout << "sub_topic_options must have topic_name" << std::endl;
          return false;
        }

        options.topic_name = sub_topic_option["topic_name"].as<std::string>();

        if (sub_topic_option["executor"]) {
          options.executor = sub_topic_option["executor"].as<std::string>();  // exist, get it
        }

        // 如果存在 priority 则赋值
        if (sub_topic_option["priority"]) {
          options.priority = sub_topic_option["priority"].as<int32_t>();
        }

        rhs.sub_topic_options.push_back(options);
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

namespace aimrt::plugins::sm_plugin {

void SmChannelBackend::Initialize(
    YAML::Node options_node,
    const runtime::core::channel::ChannelRegistry* channel_registry_ptr,
    runtime::core::channel::ContextManager* context_manager_ptr) {
  AIMRT_CHECK_ERROR_THROW(
      std::atomic_exchange(&state_, State::Init) == State::PreInit,
      "share memory channel backend can only be initialized once.");

  if (options_node && !options_node.IsNull())
    options_ = options_node.as<Options>();

  // subscriber options
  if (!options_.sub_topic_options.empty()) {
    // 如果存在 sub_topic_options 配置，但是没有 sub_default_executor 配置，则报错
    AIMRT_CHECK_ERROR_THROW((!options_.sub_default_executor.empty()),
                            "sub_topic_options is not empty, but sub_default_executor is empty.");

    sub_default_executor_ref_ = get_executor_func_(options_.sub_default_executor);
    if (!sub_default_executor_ref_) {
      AIMRT_ERROR("Default executor '{}' is not registered.", options_.sub_default_executor);
    }

    for (auto& sub_topic_option : options_.sub_topic_options) {
      if (!sub_topic_option.executor.empty()) {
        // 校验 executor 是否存在，如果不存在，则报错
        AIMRT_CHECK_ERROR_THROW((get_executor_func_(sub_topic_option.executor)),
                                "Executor '{}' is not registered.", sub_topic_option.executor);
      }
    }
  }

  options_node = options_;  // for dump options
}

void SmChannelBackend::Start() {
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

void SmChannelBackend::Shutdown() {
  if (std::atomic_exchange(&state_, State::Shutdown) == State::Shutdown)
    return;

  for (auto& [id, transmitter] : publisher_map_) {
    (void)id;
    transmitter = nullptr;
  }

  for (auto& [id, pair] : dispatcher_map_) {
    (void)id;
    pair.second->Shutdown();
    pair.second = nullptr;
  }

  for (auto& [id, subscriber_info] : subscriber_info_map_) {
    (void)id;
    subscriber_info = nullptr;
  }

  publisher_map_.clear();
  dispatcher_map_.clear();
  subscriber_info_map_.clear();
}

bool SmChannelBackend::RegisterPublishType(const runtime::core::channel::PublishTypeWrapper& publish_type_wrapper) noexcept {
  if (state_.load() != State::Init) {
    AIMRT_ERROR("Publish type can only be registered when state is 'Init'.");
    return false;
  }

  try {
    // topic需要符合规则，需要允许通行的topic才能注册，如果不允许通行，则不允许注册，属于预期内配置，返回true
    for (auto& unpassable_pub_topic : options_.unpassable_pub_topics) {
      std::regex pattern(unpassable_pub_topic);
      if (std::regex_match(std::string(publish_type_wrapper.topic_name), pattern)) {
        AIMRT_TRACE("topic '{}' is unpassable publish by sm plugin.", publish_type_wrapper.topic_name);
        return true;
      }
    }

    bool passable = (options_.passable_pub_topics.empty() ? true : false);
    for (auto& passable_pub_topic : options_.passable_pub_topics) {
      // 如果正则表达式是空的，则默认为通行
      std::regex pattern(passable_pub_topic);
      if ((passable_pub_topic.empty()) || (std::regex_match(std::string(publish_type_wrapper.topic_name), pattern))) {
        passable = true;
        break;
      }
    }

    if (!passable) {
      AIMRT_TRACE("topic '{}' is unpassable publish by sm plugin.", publish_type_wrapper.topic_name);
      return true;
    }
  } catch (std::exception& e) {
    AIMRT_ERROR("check sm plugin publish topic regex error: {}", e.what());
    return false;
  }

  uint64_t msg_hash = std::hash<std::string>{}(
      std::string(publish_type_wrapper.topic_name) + std::string(publish_type_wrapper.msg_type));

  if (publisher_map_.count(msg_hash) > 0) {
    AIMRT_ERROR("Publish topic '{}' type '{}' has been registered.",
                publish_type_wrapper.topic_name, publish_type_wrapper.msg_type);
    return false;
  }

  TransmitterAttribute transmitter_attribute;
  transmitter_attribute.channel_id = msg_hash;
  transmitter_attribute.pkg_path = publish_type_wrapper.pkg_path;
  transmitter_attribute.msg_type = publish_type_wrapper.msg_type;
  transmitter_attribute.module_name = publish_type_wrapper.module_name;
  transmitter_attribute.channel_name = publish_type_wrapper.topic_name;

  auto transmitter_ptr = std::make_shared<ShmTransmitter>(transmitter_attribute);

  publisher_map_.emplace(msg_hash, transmitter_ptr);

  return true;
}

bool SmChannelBackend::Subscribe(const runtime::core::channel::SubscribeWrapper& subscribe_wrapper) noexcept {
  if (state_.load() != State::Init) {
    AIMRT_ERROR("Subscribe can only be called when state is 'Init'.");
    return false;
  }

  DisPatcherBasePtr dispatcher_ptr = nullptr;
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
      std::cerr << "Subscribe topic '" << subscribe_wrapper.topic_name << "' type '"
                << subscribe_wrapper.msg_type << "' has been registered." << std::endl;
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

  try {
    auto& ops = options_.sub_topic_options;
    // 倒序遍历 options_ 中的 sub_topic_options, 优先级高的 executor 优先级高
    for (auto op = ops.rbegin(); op != ops.rend(); op++) {
      // 使用正则表达式进行匹配
      std::regex pattern(op->topic_name);
      if (std::regex_match(std::string(subscribe_wrapper.topic_name), pattern)) {
        // 如果优先级比当前优先级高，则更新优先级，优先级必须大于等于0才有效
        if (subscriber_info->priority == std::numeric_limits<int32_t>::min() ||
            subscriber_info->priority > op->priority) {
          subscriber_info->priority = op->priority;
          if (op->executor.empty()) {
            subscriber_info->executor = get_executor_func_(options_.sub_default_executor);
          } else {
            subscriber_info->executor = get_executor_func_(op->executor);
          }
        }
      }
    }

    if (ops.size() == 0) {
      if (options_.sub_default_executor.empty()) {
        AIMRT_ERROR("sm plugin must config default executor in Subscribe");
        return false;
      }
      subscriber_info->executor = get_executor_func_(options_.sub_default_executor);
    }

  } catch (std::exception& e) {
    AIMRT_ERROR("check sm plugin subscribe topic regex error: {}", e.what());
    return false;
  }

  std::string dispatcher_executor = subscriber_info->executor.Name().data();
  uint64_t dispatcher_key = std::hash<std::string>{}(dispatcher_executor);
  if (dispatcher_map_.count(dispatcher_key) != 0) {
    auto& pair = dispatcher_map_[dispatcher_key];
    dispatcher_ptr = pair.second;
  } else {
    dispatcher_ptr = std::make_shared<ShmDisPatcher>();
    dispatcher_map_.emplace(dispatcher_key, std::make_pair(get_executor_func_(dispatcher_executor), dispatcher_ptr));
    AIMRT_TRACE("sm channel backend create sm dispatcher, executor: {}", dispatcher_executor);
  }

  DisPatcherAttribute dispatcher_attribute;
  dispatcher_attribute.channel_id = msg_hash;
  dispatcher_attribute.pkg_path = subscribe_wrapper.pkg_path;
  dispatcher_attribute.msg_type = subscribe_wrapper.msg_type;
  dispatcher_attribute.module_name = subscribe_wrapper.module_name;
  dispatcher_attribute.channel_name = subscribe_wrapper.topic_name;

  dispatcher_ptr->AddListener(
      dispatcher_attribute,
      [subscriber_info](const void* data, size_t size, Context* context) {
        for (auto module_info : subscriber_info->module_info_list) {
          const runtime::core::channel::SubscribeWrapper& wrapper = module_info->subscribe_wrapper;

          // AIMRT_TRACE("sm channel backend receive data, topic name: {}, msg type: {}, size: {}",
          //              wrapper.topic_name, wrapper.msg_type, size);

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

          bool deserialize_ret = type_support_ref.Deserialize(
              serialization_type, buffer_array_view, msg_ptr.get());

          if (!deserialize_ret) {
            AIMRT_ERROR("msg deserialization failed in sm channel,serialization_type {}", serialization_type);
            return;
          }

          if (subscriber_info->executor.ThreadSafe()) {
            // 直接执行
            aimrt::util::Function<aimrt_function_subscriber_release_callback_ops_t> release_callback([msg_ptr]() {});
            wrapper.callback(nullptr,
                             msg_ptr.get(),
                             release_callback.NativeHandle());
          } else {
            // 放入线程池执行
            subscriber_info->executor.Execute([&wrapper, msg_ptr]() {
              aimrt::util::Function<aimrt_function_subscriber_release_callback_ops_t> release_callback([msg_ptr]() {});
              wrapper.callback(nullptr,
                               msg_ptr.get(),
                               release_callback.NativeHandle());
            });
          }
        }
      });

  return true;
}

void SmChannelBackend::Publish(const runtime::core::channel::PublishWrapper& publish_wrapper) noexcept {
  static int count = 0;

  uint64_t msg_hash = std::hash<std::string>{}(
      std::string(publish_wrapper.topic_name) + std::string(publish_wrapper.msg_type));

  if (publisher_map_.count(msg_hash) == 0) {
    return;
  }

  auto& transmitter_ptr = publisher_map_[msg_hash];

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
    auto type_support_ref = aimrt::util::TypeSupportRef(publish_wrapper.msg_type_support);
    bool serialize_ret = type_support_ref.Serialize(
        serialization_type, publish_wrapper.msg_ptr, buffer_array->NativeHandle());

    if (!serialize_ret) {
      AIMRT_ERROR("msg serialization failed in sm channel,serialization_type {}", serialization_type);
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

  transmitter_ptr->Transmit(data.c_str(), data.size());
}

void SmChannelBackend::RegisterGetExecutorFunc(
    const std::function<aimrt::executor::ExecutorRef(std::string_view)>& get_executor_func) {
  AIMRT_CHECK_ERROR_THROW(
      state_.load() == State::PreInit,
      "Function can only be called when state is 'PreInit'.");
  get_executor_func_ = get_executor_func;
}

}  // namespace aimrt::plugins::sm_plugin
