#include "core/channel/channel_manager.h"
#include "core/channel/local_channel_backend.h"

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

    node["pub_hooks"] = rhs.pub_hooks;
    node["sub_hooks"] = rhs.sub_hooks;

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

    if (node["pub_hooks"])
      rhs.pub_hooks = node["pub_hooks"].as<std::vector<std::string>>();

    if (node["sub_hooks"])
      rhs.sub_hooks = node["sub_hooks"].as<std::vector<std::string>>();

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

  // 根据配置初始化hook
  for (const auto& item : options_.pub_hooks) {
    auto finditr = publish_hook_map_.find(item);
    AIMRT_CHECK_ERROR_THROW(finditr != publish_hook_map_.end(),
                            "Invalid publish hook '{}'", item);

    publish_hook_vec_.emplace_back(std::move(finditr->second));
  }

  for (const auto& item : options_.sub_hooks) {
    auto finditr = subscribe_hook_map_.find(item);
    AIMRT_CHECK_ERROR_THROW(finditr != subscribe_hook_map_.end(),
                            "Invalid subscribe hook '{}'", item);

    subscribe_hook_vec_.emplace_back(std::move(finditr->second));
  }

  channel_registry_ptr_ = std::make_unique<ChannelRegistry>();
  channel_registry_ptr_->SetLogger(logger_ptr_);

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

    (*finditr)->Initialize(backend_options.options, channel_registry_ptr_.get());

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

  options_node = options_;

  AIMRT_INFO("Channel manager init complete");
}

void ChannelManager::Start() {
  AIMRT_CHECK_ERROR_THROW(
      std::atomic_exchange(&state_, State::Start) == State::Init,
      "Method can only be called when state is 'Init'.");

  channel_backend_manager_.Start();
  channel_handle_proxy_start_flag_.store(true);

  AIMRT_INFO("Channel manager start completed.");
}

void ChannelManager::Shutdown() {
  if (std::atomic_exchange(&state_, State::Shutdown) == State::Shutdown)
    return;

  AIMRT_INFO("Channel manager shutdown.");

  channel_handle_proxy_wrap_map_.clear();

  channel_backend_manager_.Shutdown();

  channel_backend_vec_.clear();

  channel_registry_ptr_.reset();

  subscribe_hook_vec_.clear();
  publish_hook_vec_.clear();

  subscribe_hook_map_.clear();
  publish_hook_map_.clear();

  get_executor_func_ = std::function<executor::ExecutorRef(std::string_view)>();
}

void ChannelManager::RegisterChannelBackend(
    std::unique_ptr<ChannelBackendBase>&& channel_backend_ptr) {
  AIMRT_CHECK_ERROR_THROW(
      state_.load() == State::PreInit,
      "Method can only be called when state is 'PreInit'.");

  channel_backend_vec_.emplace_back(std::move(channel_backend_ptr));
}

void ChannelManager::RegisterGetExecutorFunc(
    const std::function<executor::ExecutorRef(std::string_view)>& get_executor_func) {
  AIMRT_CHECK_ERROR_THROW(
      state_.load() == State::PreInit,
      "Method can only be called when state is 'PreInit'.");

  get_executor_func_ = get_executor_func;
}

const ChannelHandleProxy& ChannelManager::GetChannelHandleProxy(
    const util::ModuleDetailInfo& module_info) {
  AIMRT_CHECK_ERROR_THROW(
      state_.load() == State::Init,
      "Method can only be called when state is 'Init'.");

  auto itr = channel_handle_proxy_wrap_map_.find(module_info.name);
  if (itr != channel_handle_proxy_wrap_map_.end()) return itr->second->channel_handle_proxy;

  auto emplace_ret = channel_handle_proxy_wrap_map_.emplace(
      module_info.name,
      std::make_unique<ChannelHandleProxyWrap>(
          module_info.pkg_path,
          module_info.name,
          *logger_ptr_,
          channel_backend_manager_,
          passed_context_meta_keys_,
          publish_hook_vec_,
          subscribe_hook_vec_,
          channel_handle_proxy_start_flag_));

  return emplace_ret.first->second->channel_handle_proxy;
}

void ChannelManager::SetPassedContextMetaKeys(const std::unordered_set<std::string>& keys) {
  AIMRT_CHECK_ERROR_THROW(
      state_.load() == State::PreInit,
      "Method can only be called when state is 'PreInit'.");
  passed_context_meta_keys_.insert(keys.begin(), keys.end());
}

const ChannelRegistry* ChannelManager::GetChannelRegistry() const {
  AIMRT_CHECK_ERROR_THROW(
      state_.load() == State::Init,
      "Method can only be called when state is 'Init'.");

  return channel_registry_ptr_.get();
}

const std::vector<std::string>& ChannelManager::GetChannelBackendNameList() const {
  AIMRT_CHECK_ERROR_THROW(
      state_.load() == State::Init,
      "Method can only be called when state is 'Init'.");

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

std::list<std::pair<std::string, std::string>> ChannelManager::GenInitializationReport() const {
  AIMRT_CHECK_ERROR_THROW(
      state_.load() == State::Init,
      "Method can only be called when state is 'Init'.");

  std::vector<std::vector<std::string>> pub_topic_info_table =
      {{"topic", "msg type", "module", "backends"}};

  const auto& pub_topic_backend_info = channel_backend_manager_.GetPubTopicBackendInfo();
  const auto& pub_topic_index_map = channel_registry_ptr_->GetPubTopicIndexMap();

  for (const auto& pub_topic_index_itr : pub_topic_index_map) {
    auto pub_topic_backend_itr = pub_topic_backend_info.find(pub_topic_index_itr.first);
    AIMRT_CHECK_ERROR_THROW(pub_topic_backend_itr != pub_topic_backend_info.end(),
                            "Invalid channel registry info.");

    for (const auto& item : pub_topic_index_itr.second) {
      std::vector<std::string> cur_topic_info(4);
      cur_topic_info[0] = pub_topic_index_itr.first;
      cur_topic_info[1] = item->msg_type;
      cur_topic_info[2] = item->module_name;
      cur_topic_info[3] = aimrt::common::util::JoinVec(pub_topic_backend_itr->second, ",");
      pub_topic_info_table.emplace_back(std::move(cur_topic_info));
    }
  }

  std::vector<std::vector<std::string>> sub_topic_info_table =
      {{"topic", "msg type", "module", "backends"}};

  const auto& sub_topic_backend_info = channel_backend_manager_.GetSubTopicBackendInfo();
  const auto& sub_topic_index_map = channel_registry_ptr_->GetSubTopicIndexMap();

  for (const auto& sub_topic_index_itr : sub_topic_index_map) {
    auto sub_topic_backend_itr = sub_topic_backend_info.find(sub_topic_index_itr.first);
    AIMRT_CHECK_ERROR_THROW(sub_topic_backend_itr != sub_topic_backend_info.end(),
                            "Invalid channel registry info.");

    for (const auto& item : sub_topic_index_itr.second) {
      std::vector<std::string> cur_topic_info(4);
      cur_topic_info[0] = sub_topic_index_itr.first;
      cur_topic_info[1] = item->msg_type;
      cur_topic_info[2] = item->module_name;
      cur_topic_info[3] = aimrt::common::util::JoinVec(sub_topic_backend_itr->second, ",");
      sub_topic_info_table.emplace_back(std::move(cur_topic_info));
    }
  }

  std::string channel_backend_name_list;
  if (channel_backend_name_vec_.empty()) {
    channel_backend_name_list = "<empty>";
  } else {
    channel_backend_name_list = "[ " + aimrt::common::util::JoinVec(channel_backend_name_vec_, " , ") + " ]";
  }

  return {{"Channel Backend List", channel_backend_name_list},
          {"Channel Pub Topic Info", aimrt::common::util::DrawTable(pub_topic_info_table)},
          {"Channel Sub Topic Info", aimrt::common::util::DrawTable(sub_topic_info_table)}};
}

}  // namespace aimrt::runtime::core::channel
