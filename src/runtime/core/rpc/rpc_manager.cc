#include "core/rpc/rpc_manager.h"
#include "core/global.h"
#include "core/rpc/local_rpc_backend.h"
#include "util/stl_tool.h"

namespace YAML {
template <>
struct convert<aimrt::runtime::core::rpc::RpcManager::Options> {
  using Options = aimrt::runtime::core::rpc::RpcManager::Options;

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

namespace aimrt::runtime::core::rpc {

void RpcManager::Initialize(YAML::Node options_node) {
  RegisterLocalRpcBackend();

  AIMRT_CHECK_ERROR_THROW(
      std::atomic_exchange(&state_, State::Init) == State::PreInit,
      "Rpc manager can only be initialized once.");

  if (options_node && !options_node.IsNull())
    options_ = options_node.as<Options>();

  rpc_registry_ptr_ = std::make_unique<RpcRegistry>();
  context_manager_ptr_ = std::make_unique<ContextManager>();

  // 根据配置初始化指定的backend
  for (auto& backend_options : options_.backends_options) {
    auto finditr =
        std::find_if(rpc_backend_vec_.begin(), rpc_backend_vec_.end(),
                     [&backend_options](const auto& ptr) {
                       return ptr->Name() == backend_options.type;
                     });

    AIMRT_CHECK_ERROR_THROW(finditr != rpc_backend_vec_.end(),
                            "Invalid rpc backend type '{}'",
                            backend_options.type);

    (*finditr)->Initialize(backend_options.options,
                           rpc_registry_ptr_.get(),
                           context_manager_ptr_.get());

    rpc_backend_manager_.RegisterRpcBackend(finditr->get());

    rpc_backend_name_vec_.emplace_back((*finditr)->Name());
  }

  rpc_backend_manager_.Initialize(rpc_registry_ptr_.get());

  AIMRT_TRACE("Rpc manager init success, backends list: {}",
              common::util::Vec2Str(rpc_backend_name_vec_));

  options_node = options_;
}

void RpcManager::Start() {
  AIMRT_CHECK_ERROR_THROW(
      std::atomic_exchange(&state_, State::Start) == State::Init,
      "Function can only be called when state is 'Init'.");

  rpc_backend_manager_.Start();
}

void RpcManager::Shutdown() {
  if (std::atomic_exchange(&state_, State::Shutdown) == State::Shutdown)
    return;

  rpc_handle_proxy_map_.clear();

  rpc_backend_manager_.Shutdown();

  rpc_backend_vec_.clear();

  context_manager_ptr_.reset();

  rpc_registry_ptr_.reset();

  get_executor_func_ = std::function<executor::ExecutorRef(std::string_view)>();
}

void RpcManager::RegisterRpcBackend(
    std::unique_ptr<RpcBackendBase>&& rpc_backend_ptr) {
  AIMRT_CHECK_ERROR_THROW(
      state_.load() == State::PreInit,
      "Function can only be called when state is 'PreInit'.");

  rpc_backend_vec_.emplace_back(std::move(rpc_backend_ptr));
}

void RpcManager::RegisterGetExecutorFunc(
    const std::function<executor::ExecutorRef(std::string_view)>& get_executor_func) {
  AIMRT_CHECK_ERROR_THROW(
      state_.load() == State::PreInit,
      "Function can only be called when state is 'PreInit'.");

  get_executor_func_ = get_executor_func;
}

RpcHandleProxy& RpcManager::GetRpcHandleProxy(
    const util::ModuleDetailInfo& module_info) {
  AIMRT_CHECK_ERROR_THROW(
      state_.load() == State::Init,
      "Function can only be called when state is 'Init'.");

  auto itr = rpc_handle_proxy_map_.find(module_info.name);
  if (itr != rpc_handle_proxy_map_.end()) return *(itr->second);

  auto emplace_ret = rpc_handle_proxy_map_.emplace(
      module_info.name, std::make_unique<RpcHandleProxy>(
                            module_info.pkg_path, module_info.name,
                            rpc_backend_manager_, *context_manager_ptr_));
  return *(emplace_ret.first->second);
}

const RpcRegistry* RpcManager::GetRpcRegistry() const {
  AIMRT_CHECK_ERROR_THROW(
      state_.load() == State::Init,
      "Function can only be called when state is 'Init'.");

  return rpc_registry_ptr_.get();
}

const std::vector<std::string>& RpcManager::GetRpcBackendNameList() const {
  AIMRT_CHECK_ERROR_THROW(
      state_.load() == State::Init,
      "Function can only be called when state is 'Init'.");

  return rpc_backend_name_vec_;
}

void RpcManager::RegisterLocalRpcBackend() {
  std::unique_ptr<RpcBackendBase> local_rpc_backend_ptr = std::make_unique<LocalRpcBackend>();
  RegisterRpcBackend(std::move(local_rpc_backend_ptr));
}
}  // namespace aimrt::runtime::core::rpc
