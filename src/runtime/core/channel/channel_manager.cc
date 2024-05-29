#include "core/channel/channel_manager.h"
#include "core/channel/local_channel_backend.h"
#include "util/stl_tool.h"

namespace YAML {
template <>
struct convert<aimrt::runtime::core::channel::ChannelManager::Options> {
  using Options = aimrt::runtime::core::channel::ChannelManager::Options;

  static Node encode(const Options& rhs) {
    Node node;

    node["backends"] = YAML::Node();
    for (const auto& backend : rhs.backends_options) {
      Node backend_options_node;
      backend_options_node["type"] = backend.type;
      backend_options_node["options"] = backend.options;
      node["backends"].push_back(backend_options_node);
    }

    node["pub_topics_options"] = YAML::Node();
    for (const auto& pub_topic_options : rhs.pub_topics_options) {
      Node pub_topic_options_node;
      pub_topic_options_node["topic_name"] = pub_topic_options.topic_name;
      pub_topic_options_node["enable_backends"] = pub_topic_options.enable_backends;
      node["pub_topics_options"].push_back(pub_topic_options_node);
    }

    node["sub_topics_options"] = YAML::Node();
    for (const auto& sub_topic_options : rhs.sub_topics_options) {
      Node sub_topic_options_node;
      sub_topic_options_node["topic_name"] = sub_topic_options.topic_name;
      sub_topic_options_node["enable_backends"] = sub_topic_options.enable_backends;
      node["sub_topics_options"].push_back(sub_topic_options_node);
    }

    return node;
  }

  static bool decode(const Node& node, Options& rhs) {
    if (!node.IsMap()) return false;

    if (node["backends"] && node["backends"].IsSequence()) {
      for (auto& backend_options_node : node["backends"]) {
        auto backend_options = Options::BackendOptions{
            .type = backend_options_node["type"].as<std::string>()};

        if (backend_options_node["options"])
          backend_options.options = backend_options_node["options"];

        rhs.backends_options.emplace_back(std::move(backend_options));
      }
    }

    if (node["pub_topics_options"] && node["pub_topics_options"].IsSequence()) {
      for (auto& pub_topic_options_node : node["pub_topics_options"]) {
        auto pub_topic_options = Options::PubTopicOptions{
            .topic_name = pub_topic_options_node["topic_name"].as<std::string>(),
            .enable_backends = pub_topic_options_node["enable_backends"].as<std::vector<std::string>>()};

        rhs.pub_topics_options.emplace_back(std::move(pub_topic_options));
      }
    }

    if (node["sub_topics_options"] && node["sub_topics_options"].IsSequence()) {
      for (auto& sub_topic_options_node : node["sub_topics_options"]) {
        auto sub_topic_options = Options::SubTopicOptions{
            .topic_name = sub_topic_options_node["topic_name"].as<std::string>(),
            .enable_backends = sub_topic_options_node["enable_backends"].as<std::vector<std::string>>()};

        rhs.sub_topics_options.emplace_back(std::move(sub_topic_options));
      }
    }

    return true;
  }
};
}  // namespace YAML

namespace aimrt::runtime::core::channel {

void ChannelManager::Initialize(YAML::Node options_node) {
  RegisterLocalChannelBackend();

  AIMRT_CHECK_ERROR_THROW(
      std::atomic_exchange(&state_, State::Init) == State::PreInit,
      "Channel manager can only be initialized once.");

  if (options_node && !options_node.IsNull())
    options_ = options_node.as<Options>();

  channel_registry_ptr_ = std::make_unique<ChannelRegistry>();
  channel_registry_ptr_->SetLogger(logger_ptr_);

  context_manager_ptr_ = std::make_unique<ContextManager>();
  context_manager_ptr_->SetLogger(logger_ptr_);

  channel_backend_manager_.SetLogger(logger_ptr_);

  // 根据配置初始化指定的backend
  for (auto& backend_options : options_.backends_options) {
    auto finditr =
        std::find_if(channel_backend_vec_.begin(), channel_backend_vec_.end(),
                     [&backend_options](const auto& ptr) {
                       return ptr->Name() == backend_options.type;
                     });

    AIMRT_CHECK_ERROR_THROW(finditr != channel_backend_vec_.end(),
                            "Invalid channel backend type '{}'",
                            backend_options.type);

    (*finditr)->Initialize(backend_options.options,
                           channel_registry_ptr_.get(),
                           context_manager_ptr_.get());

    channel_backend_manager_.RegisterChannelBackend(finditr->get());

    channel_backend_name_vec_.emplace_back((*finditr)->Name());
  }

  // 设置backends rules
  std::vector<std::pair<std::string, std::vector<std::string>>> pub_topics_backends_rules;
  for (auto& item : options_.pub_topics_options) {
    for (auto& backend_name : item.enable_backends) {
      AIMRT_CHECK_ERROR_THROW(
          std::find(channel_backend_name_vec_.begin(), channel_backend_name_vec_.end(), backend_name) != channel_backend_name_vec_.end(),
          "Invalid channel backend type '{}' for pub topic '{}'",
          backend_name, item.topic_name);
    }

    pub_topics_backends_rules.emplace_back(item.topic_name, item.enable_backends);
  }
  channel_backend_manager_.SetPubTopicsBackendsRules(pub_topics_backends_rules);

  std::vector<std::pair<std::string, std::vector<std::string>>> sub_topics_backends_rules;
  for (auto& item : options_.sub_topics_options) {
    for (auto& backend_name : item.enable_backends) {
      AIMRT_CHECK_ERROR_THROW(
          std::find(channel_backend_name_vec_.begin(), channel_backend_name_vec_.end(), backend_name) != channel_backend_name_vec_.end(),
          "Invalid channel backend type '{}' for sub topic '{}'",
          backend_name, item.topic_name);
    }

    sub_topics_backends_rules.emplace_back(item.topic_name, item.enable_backends);
  }
  channel_backend_manager_.SetSubTopicsBackendsRules(sub_topics_backends_rules);

  // 初始化backend manager
  channel_backend_manager_.Initialize(channel_registry_ptr_.get());

  AIMRT_TRACE("Channel manager init success, backends list: {}",
              aimrt::common::util::Vec2Str(channel_backend_name_vec_));

  options_node = options_;

  AIMRT_INFO("Channel init complete.");
}

void ChannelManager::Start() {
  AIMRT_CHECK_ERROR_THROW(
      std::atomic_exchange(&state_, State::Start) == State::Init,
      "Function can only be called when state is 'Init'.");

  channel_backend_manager_.Start();
  channel_handle_proxy_start_flag_.store(true);
}

void ChannelManager::Shutdown() {
  if (std::atomic_exchange(&state_, State::Shutdown) == State::Shutdown)
    return;

  AIMRT_INFO("Shutdown channel.");

  channel_handle_proxy_wrap_map_.clear();

  channel_backend_manager_.Shutdown();

  channel_backend_vec_.clear();

  context_manager_ptr_.reset();

  channel_registry_ptr_.reset();

  get_executor_func_ = std::function<executor::ExecutorRef(std::string_view)>();
}

void ChannelManager::RegisterChannelBackend(
    std::unique_ptr<ChannelBackendBase>&& channel_backend_ptr) {
  AIMRT_CHECK_ERROR_THROW(
      state_.load() == State::PreInit,
      "Function can only be called when state is 'PreInit'.");

  channel_backend_vec_.emplace_back(std::move(channel_backend_ptr));
}

void ChannelManager::RegisterGetExecutorFunc(
    const std::function<executor::ExecutorRef(std::string_view)>& get_executor_func) {
  AIMRT_CHECK_ERROR_THROW(
      state_.load() == State::PreInit,
      "Function can only be called when state is 'PreInit'.");

  get_executor_func_ = get_executor_func;
}

ChannelHandleProxy& ChannelManager::GetChannelHandleProxy(
    const util::ModuleDetailInfo& module_info) {
  AIMRT_CHECK_ERROR_THROW(
      state_.load() == State::Init,
      "Function can only be called when state is 'Init'.");

  auto itr = channel_handle_proxy_wrap_map_.find(module_info.name);
  if (itr != channel_handle_proxy_wrap_map_.end()) return itr->second->channel_handle_proxy;

  auto emplace_ret = channel_handle_proxy_wrap_map_.emplace(
      module_info.name,
      std::make_unique<ChannelHandleProxyWrap>(
          module_info.pkg_path,
          module_info.name,
          *logger_ptr_,
          channel_backend_manager_,
          *context_manager_ptr_,
          channel_handle_proxy_start_flag_));

  return emplace_ret.first->second->channel_handle_proxy;
}

const ChannelRegistry* ChannelManager::GetChannelRegistry() const {
  AIMRT_CHECK_ERROR_THROW(
      state_.load() == State::Init,
      "Function can only be called when state is 'Init'.");

  return channel_registry_ptr_.get();
}

const std::vector<std::string>& ChannelManager::GetChannelBackendNameList() const {
  AIMRT_CHECK_ERROR_THROW(
      state_.load() == State::Init,
      "Function can only be called when state is 'Init'.");

  return channel_backend_name_vec_;
}

void ChannelManager::RegisterLocalChannelBackend() {
  std::unique_ptr<ChannelBackendBase> local_channel_backend_ptr =
      std::make_unique<LocalChannelBackend>();

  static_cast<LocalChannelBackend*>(local_channel_backend_ptr.get())
      ->RegisterGetExecutorFunc(get_executor_func_);

  static_cast<LocalChannelBackend*>(local_channel_backend_ptr.get())
      ->SetLogger(logger_ptr_);

  RegisterChannelBackend(std::move(local_channel_backend_ptr));
}

}  // namespace aimrt::runtime::core::channel
