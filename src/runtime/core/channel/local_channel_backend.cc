#include "core/channel/local_channel_backend.h"

#include <memory>

#define TO_AIMRT_BUFFER_ARRAY_VIEW(__aimrt_buffer_array__) \
  (static_cast<const aimrt_buffer_array_view_t*>(          \
      static_cast<const void*>(__aimrt_buffer_array__)))

namespace YAML {
template <>
struct convert<aimrt::runtime::core::channel::LocalChannelBackend::Options> {
  using Options = aimrt::runtime::core::channel::LocalChannelBackend::Options;

  static Node encode(const Options& rhs) {
    Node node;

    node["subscriber_use_inline_executor"] = rhs.subscriber_use_inline_executor;
    if (!rhs.subscriber_use_inline_executor)
      node["subscriber_executor"] = rhs.subscriber_executor;

    return node;
  }

  static bool decode(const Node& node, Options& rhs) {
    if (!node.IsMap()) return false;

    rhs.subscriber_use_inline_executor = node["subscriber_use_inline_executor"].as<bool>();
    if (!rhs.subscriber_use_inline_executor)
      rhs.subscriber_executor = node["subscriber_executor"].as<std::string>();

    return true;
  }
};
}  // namespace YAML

namespace aimrt::runtime::core::channel {

void LocalChannelBackend::Initialize(
    YAML::Node options_node,
    const ChannelRegistry* channel_registry_ptr) {
  AIMRT_CHECK_ERROR_THROW(
      std::atomic_exchange(&state_, State::Init) == State::PreInit,
      "Local channel backend can only be initialized once.");

  if (options_node && !options_node.IsNull())
    options_ = options_node.as<Options>();

  channel_registry_ptr_ = channel_registry_ptr;

  if (!options_.subscriber_use_inline_executor) {
    AIMRT_CHECK_ERROR_THROW(
        get_executor_func_,
        "Get executor function is not set before initialize.");

    subscribe_executor_ref_ = get_executor_func_(options_.subscriber_executor);

    AIMRT_CHECK_ERROR_THROW(
        subscribe_executor_ref_,
        "Invalid local subscriber executor '{}'.", options_.subscriber_executor);
  }

  options_node = options_;
}

void LocalChannelBackend::Start() {
  AIMRT_CHECK_ERROR_THROW(
      std::atomic_exchange(&state_, State::Start) == State::Init,
      "Method can only be called when state is 'Init'.");
}

void LocalChannelBackend::Shutdown() {
  if (std::atomic_exchange(&state_, State::Shutdown) == State::Shutdown)
    return;

  get_executor_func_ = std::function<executor::ExecutorRef(std::string_view)>();
  subscribe_index_map_.clear();
}

bool LocalChannelBackend::RegisterPublishType(
    const PublishTypeWrapper& publish_type_wrapper) noexcept {
  if (state_.load() != State::Init) {
    AIMRT_ERROR("Publish type can only be registered when state is 'Init'.");
    return false;
  }

  return true;
}

bool LocalChannelBackend::Subscribe(const SubscribeWrapper& subscribe_wrapper) noexcept {
  if (state_.load() != State::Init) {
    AIMRT_ERROR("Msg can only be subscribed when state is 'Init'.");
    return false;
  }

  std::string_view msg_type = subscribe_wrapper.msg_type;
  std::string_view pkg_path = subscribe_wrapper.pkg_path;
  std::string_view module_name = subscribe_wrapper.module_name;
  std::string_view topic_name = subscribe_wrapper.topic_name;

  subscribe_index_map_[msg_type][topic_name][pkg_path].emplace(module_name);

  return true;
}

void LocalChannelBackend::Publish(const PublishWrapper& publish_wrapper) noexcept {
  if (state_.load() != State::Start) [[unlikely]] {
    AIMRT_WARN("Method can only be called when state is 'Start'.");
    return;
  }

  std::string_view msg_type = publish_wrapper.msg_type;
  std::string_view pkg_path = publish_wrapper.pkg_path;
  std::string_view module_name = publish_wrapper.module_name;
  std::string_view topic_name = publish_wrapper.topic_name;

  const auto& subscribe_wrapper_map = channel_registry_ptr_->GetSubscribeWrapperMap();

  // 没有人订阅则直接返回
  auto subscribe_index_map_find_msg_itr = subscribe_index_map_.find(msg_type);
  if (subscribe_index_map_find_msg_itr == subscribe_index_map_.end()) return;

  auto subscribe_index_map_find_topic_itr = subscribe_index_map_find_msg_itr->second.find(topic_name);
  if (subscribe_index_map_find_topic_itr == subscribe_index_map_find_msg_itr->second.end()) return;

  // 遍历每个pkg
  for (const auto& subscribe_pkg_path_itr : subscribe_index_map_find_topic_itr->second) {
    std::string_view subscribe_pkg_path = subscribe_pkg_path_itr.first;

    // 同一个pkg下的各个模块，对同一个类型的创建/销毁方法应该是统一的
    // 随便选取一个模块的对该类型的创建/销毁方法作为pkg的全局选择
    const auto* tpl_subscribe_wrapper_ptr =
        GetTplSubscribeWrapper(subscribe_pkg_path, topic_name, msg_type);

    // 不可能选不出来
    // assert(tpl_subscribe_wrapper_ptr != nullptr);

    auto tpl_subscribe_type_support_ref = aimrt::util::TypeSupportRef(tpl_subscribe_wrapper_ptr->msg_type_support);

    // 该pkg下创建的结构体的智能指针，带删除器
    std::shared_ptr<void> msg_ptr = tpl_subscribe_type_support_ref.CreateSharedPtr();

    // context
    auto ctx_ptr = std::make_shared<aimrt::channel::Context>(aimrt_channel_context_type_t::AIMRT_RPC_SUBSCRIBER_CONTEXT);

    if (subscribe_pkg_path == pkg_path) {
      // 在同一个pkg中，直接复制
      tpl_subscribe_type_support_ref.Copy(publish_wrapper.msg_ptr, msg_ptr.get());

      ctx_ptr->SetSerializationType(publish_wrapper.ctx_ref.GetSerializationType());
    } else {
      // 在不同pkg中，需要进行序列化反序列化
      // 在同一个pkg中的不同模块中，对同一个类型结构可以复用，不管它是通过哪种序列化方法/反序列化方法从发布端原始结构转过来的

      auto publish_type_support_ref = aimrt::util::TypeSupportRef(publish_wrapper.msg_type_support);

      // 查询订阅端和发布端是否有同一种序列化方法
      auto [subscribe_wrapper_ptr, serialization_type] =
          GetSubscribeSerializationType(
              publish_type_support_ref,
              subscribe_pkg_path,
              topic_name,
              msg_type);

      // 本pkg中没有同一种序列化方法，应该不可能出现
      // assert(subscribe_wrapper_ptr != nullptr && !serialization_type.empty());

      ctx_ptr->SetSerializationType(serialization_type);

      // msg序列化
      std::shared_ptr<aimrt::util::BufferArray> buffer_array;

      auto find_serialization_cache_itr = publish_wrapper.serialization_cache.find(serialization_type);
      if (find_serialization_cache_itr == publish_wrapper.serialization_cache.end()) {
        // 没有缓存，序列化一次后放入缓存中
        buffer_array = std::make_shared<aimrt::util::BufferArray>();
        bool serialize_ret = publish_type_support_ref.Serialize(
            serialization_type, publish_wrapper.msg_ptr, buffer_array->AllocatorNativeHandle(), buffer_array->BufferArrayNativeHandle());

        if (!serialize_ret) {
          AIMRT_ERROR(
              "Msg serialization failed in local channel, serialization_type {}, pkg_path: {}, module_name: {}, topic_name: {}, msg_type: {}",
              serialization_type, pkg_path, module_name, topic_name, msg_type);
          return;
        }

        publish_wrapper.serialization_cache.emplace(serialization_type, buffer_array);
      } else {
        // 有缓存
        buffer_array = find_serialization_cache_itr->second;
      }

      // subscribe反序列化
      auto subscribe_type_support_ref = aimrt::util::TypeSupportRef(subscribe_wrapper_ptr->msg_type_support);
      bool deserialize_ret = subscribe_type_support_ref.Deserialize(
          serialization_type, *TO_AIMRT_BUFFER_ARRAY_VIEW(buffer_array->BufferArrayNativeHandle()), msg_ptr.get());
      if (!deserialize_ret) {
        // 反序列化失败
        AIMRT_FATAL(
            "Deserialization failed in local channel, serialization_type {}, pkg_path: {}, module_name: {}, topic_name: {}, msg_type: {}",
            serialization_type, pkg_path, module_name, topic_name, msg_type);
        continue;
      }
    }

    // 调用注册的subscribe方法
    for (const auto& subscribe_module_name : subscribe_pkg_path_itr.second) {
      auto get_subscribe_wrapper_ptr_func = [&]() -> const SubscribeWrapper* {
        auto find_pkg_itr = subscribe_wrapper_map.find(subscribe_pkg_path);
        if (find_pkg_itr == subscribe_wrapper_map.end()) return nullptr;

        auto find_module_itr = find_pkg_itr->second.find(subscribe_module_name);
        if (find_module_itr == find_pkg_itr->second.end()) return nullptr;

        auto find_topic_itr = find_module_itr->second.find(topic_name);
        if (find_topic_itr == find_module_itr->second.end()) return nullptr;

        auto find_msg_itr = find_topic_itr->second.find(msg_type);
        if (find_msg_itr == find_topic_itr->second.end()) return nullptr;

        return find_msg_itr->second.get();
      };

      const auto* subscribe_wrapper_ptr = get_subscribe_wrapper_ptr_func();

      // 不可能为空
      // assert(subscribe_wrapper_ptr != nullptr);

      if (subscribe_executor_ref_) {
        subscribe_executor_ref_.Execute(
            [subscribe_wrapper_ptr, msg_ptr, ctx_ptr]() {
              aimrt::channel::SubscriberReleaseCallback release_callback([msg_ptr, ctx_ptr]() {});
              subscribe_wrapper_ptr->callback(ctx_ptr->NativeHandle(), msg_ptr.get(), release_callback.NativeHandle());
            });
      } else {
        aimrt::channel::SubscriberReleaseCallback release_callback([msg_ptr, ctx_ptr]() {});
        subscribe_wrapper_ptr->callback(ctx_ptr->NativeHandle(), msg_ptr.get(), release_callback.NativeHandle());
      }
    }
  }
}

void LocalChannelBackend::RegisterGetExecutorFunc(
    const std::function<executor::ExecutorRef(std::string_view)>& get_executor_func) {
  AIMRT_CHECK_ERROR_THROW(
      state_.load() == State::PreInit,
      "Method can only be called when state is 'PreInit'.");

  get_executor_func_ = get_executor_func;
}

const SubscribeWrapper* LocalChannelBackend::GetTplSubscribeWrapper(
    std::string_view subscribe_pkg_path,
    std::string_view topic_name,
    std::string_view msg_type) const {
  const auto& subscribe_wrapper_map = channel_registry_ptr_->GetSubscribeWrapperMap();

  auto find_pkg_itr = subscribe_wrapper_map.find(subscribe_pkg_path);
  if (find_pkg_itr == subscribe_wrapper_map.end()) return nullptr;

  for (const auto& module_itr : find_pkg_itr->second) {
    auto find_topic_itr = module_itr.second.find(topic_name);
    if (find_topic_itr == module_itr.second.end()) continue;

    auto find_msg_itr = find_topic_itr->second.find(msg_type);
    if (find_msg_itr == find_topic_itr->second.end()) continue;

    return find_msg_itr->second.get();
  }
  return nullptr;
}

std::tuple<const SubscribeWrapper*, std::string_view>
LocalChannelBackend::GetSubscribeSerializationType(
    aimrt::util::TypeSupportRef publish_msg_type_support_ref,
    std::string_view subscribe_pkg_path,
    std::string_view topic_name,
    std::string_view msg_type) const {
  const auto& subscribe_wrapper_map = channel_registry_ptr_->GetSubscribeWrapperMap();

  auto find_pkg_itr = subscribe_wrapper_map.find(subscribe_pkg_path);
  if (find_pkg_itr == subscribe_wrapper_map.end()) return {nullptr, ""};

  size_t cur_pmsg_ser_type = publish_msg_type_support_ref.SerializationTypesSupportedNum();
  const auto* cur_pmsg_ser_list = publish_msg_type_support_ref.SerializationTypesSupportedList();

  for (const auto& module_itr : find_pkg_itr->second) {
    auto find_topic_itr = module_itr.second.find(topic_name);
    if (find_topic_itr == module_itr.second.end()) continue;

    auto find_msg_itr = find_topic_itr->second.find(msg_type);
    if (find_msg_itr == find_topic_itr->second.end()) continue;

    auto* cur_subscribe_wrapper_ptr = find_msg_itr->second.get();
    auto cur_subscribe_msg_type_support_ref = aimrt::util::TypeSupportRef(cur_subscribe_wrapper_ptr->msg_type_support);

    size_t cur_smsg_ser_num = cur_subscribe_msg_type_support_ref.SerializationTypesSupportedNum();
    const auto* cur_smsg_ser_list = cur_subscribe_msg_type_support_ref.SerializationTypesSupportedList();

    for (uint32_t ii = 0; ii < cur_smsg_ser_num; ++ii) {
      std::string_view cur_smsg_ser_type = aimrt::util::ToStdStringView(cur_smsg_ser_list[ii]);

      for (uint32_t jj = 0; jj < cur_pmsg_ser_type; ++jj) {
        std::string_view cur_pmsg_ser_type = aimrt::util::ToStdStringView(cur_pmsg_ser_list[jj]);

        if (cur_smsg_ser_type == cur_pmsg_ser_type) {
          return {cur_subscribe_wrapper_ptr, cur_pmsg_ser_type};
        }
      }
    }
  }
  return {nullptr, ""};
}

}  // namespace aimrt::runtime::core::channel