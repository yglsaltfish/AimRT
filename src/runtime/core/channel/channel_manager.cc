#include "core/channel/channel_manager.h"
#include "core/channel/local_channel_backend.h"
#include "core/global.h"
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

    return true;
  }
};
}  // namespace YAML

namespace aimrt::runtime::core::channel {

void ChannelManager::Initialize(YAML::Node options_node) {
  RegisterLocalChannelBackend();

  AIMRT_CHECK_ERROR_THROW(
      std::atomic_exchange(&status_, Status::Init) == Status::PreInit,
      "Channel manager can only be initialized once.");

  if (options_node && !options_node.IsNull())
    options_ = options_node.as<Options>();

  channel_registry_ptr_ = std::make_unique<ChannelRegistry>();
  context_manager_ptr_ = std::make_unique<ContextManager>();

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

  channel_backend_manager_.Initialize(channel_registry_ptr_.get());

  AIMRT_TRACE("Channel manager init success, backends list: {}",
              common::util::Vec2Str(channel_backend_name_vec_));

  options_node = options_;
}

void ChannelManager::Start() {
  AIMRT_CHECK_ERROR_THROW(
      std::atomic_exchange(&status_, Status::Start) == Status::Init,
      "Function can only be called when status is 'Init'.");

  channel_backend_manager_.Start();
}

void ChannelManager::Shutdown() {
  if (std::atomic_exchange(&status_, Status::Shutdown) == Status::Shutdown)
    return;

  channel_proxy_map_.clear();

  channel_backend_manager_.Shutdown();

  channel_backend_vec_.clear();

  context_manager_ptr_.reset();

  channel_registry_ptr_.reset();

  get_executor_func_ = std::function<executor::ExecutorRef(std::string_view)>();
}

void ChannelManager::RegisterChannelBackend(
    std::unique_ptr<ChannelBackendBase>&& channel_backend_ptr) {
  AIMRT_CHECK_ERROR_THROW(
      status_.load() == Status::PreInit,
      "Function can only be called when status is 'PreInit'.");

  channel_backend_vec_.emplace_back(std::move(channel_backend_ptr));
}

void ChannelManager::RegisterGetExecutorFunc(
    const std::function<executor::ExecutorRef(std::string_view)>& get_executor_func) {
  AIMRT_CHECK_ERROR_THROW(
      status_.load() == Status::PreInit,
      "Function can only be called when status is 'PreInit'.");

  get_executor_func_ = get_executor_func;
}

ChannelProxy& ChannelManager::GetChannelProxy(
    const util::ModuleDetailInfo& module_info) {
  AIMRT_CHECK_ERROR_THROW(
      status_.load() == Status::Init,
      "Function can only be called when status is 'Init'.");

  auto itr = channel_proxy_map_.find(module_info.name);
  if (itr != channel_proxy_map_.end()) return *(itr->second);

  auto emplace_ret = channel_proxy_map_.emplace(
      module_info.name,
      std::make_unique<ChannelProxy>(module_info.pkg_path, module_info.name,
                                     channel_backend_manager_, *context_manager_ptr_));

  return *(emplace_ret.first->second);
}

const ChannelRegistry* ChannelManager::GetChannelRegistry() const {
  AIMRT_CHECK_ERROR_THROW(
      status_.load() == Status::Init,
      "Function can only be called when status is 'Init'.");

  return channel_registry_ptr_.get();
}

const std::vector<std::string>& ChannelManager::GetChannelBackendNameList() const {
  AIMRT_CHECK_ERROR_THROW(
      status_.load() == Status::Init,
      "Function can only be called when status is 'Init'.");

  return channel_backend_name_vec_;
}

void ChannelManager::RegisterLocalChannelBackend() {
  std::unique_ptr<ChannelBackendBase> local_channel_backend_ptr =
      std::make_unique<LocalChannelBackend>();

  static_cast<LocalChannelBackend*>(local_channel_backend_ptr.get())
      ->RegisterGetExecutorFunc(get_executor_func_);

  RegisterChannelBackend(std::move(local_channel_backend_ptr));
}

}  // namespace aimrt::runtime::core::channel
