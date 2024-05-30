#include "core/rpc/rpc_manager.h"
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

    node["clients_options"] = YAML::Node();
    for (const auto& client_options : rhs.clients_options) {
      Node client_options_node;
      client_options_node["func_name"] = client_options.func_name;
      client_options_node["enable_backends"] = client_options.enable_backends;
      node["clients_options"].push_back(client_options_node);
    }

    node["servers_options"] = YAML::Node();
    for (const auto& server_options : rhs.servers_options) {
      Node server_options_node;
      server_options_node["func_name"] = server_options.func_name;
      server_options_node["enable_backends"] = server_options.enable_backends;
      node["servers_options"].push_back(server_options_node);
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

    if (node["clients_options"] && node["clients_options"].IsSequence()) {
      for (auto& client_options_node : node["clients_options"]) {
        auto client_options = Options::ClientOptions{
            .func_name = client_options_node["func_name"].as<std::string>(),
            .enable_backends = client_options_node["enable_backends"].as<std::vector<std::string>>()};

        rhs.clients_options.emplace_back(std::move(client_options));
      }
    }

    if (node["servers_options"] && node["servers_options"].IsSequence()) {
      for (auto& server_options_node : node["servers_options"]) {
        auto server_options = Options::ServerOptions{
            .func_name = server_options_node["func_name"].as<std::string>(),
            .enable_backends = server_options_node["enable_backends"].as<std::vector<std::string>>()};

        rhs.servers_options.emplace_back(std::move(server_options));
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
  rpc_registry_ptr_->SetLogger(logger_ptr_);

  context_manager_ptr_ = std::make_unique<ContextManager>();
  context_manager_ptr_->SetLogger(logger_ptr_);

  rpc_backend_manager_.SetLogger(logger_ptr_);

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

  // 设置backends rules
  std::vector<std::pair<std::string, std::vector<std::string>>> client_backends_rules;
  for (auto& item : options_.clients_options) {
    for (auto& backend_name : item.enable_backends) {
      AIMRT_CHECK_ERROR_THROW(
          std::find(rpc_backend_name_vec_.begin(), rpc_backend_name_vec_.end(), backend_name) != rpc_backend_name_vec_.end(),
          "Invalid rpc backend type '{}' for func '{}'",
          backend_name, item.func_name);
    }

    client_backends_rules.emplace_back(item.func_name, item.enable_backends);
  }
  rpc_backend_manager_.SetClientsBackendsRules(client_backends_rules);

  std::vector<std::pair<std::string, std::vector<std::string>>> server_backends_rules;
  for (auto& item : options_.servers_options) {
    for (auto& backend_name : item.enable_backends) {
      AIMRT_CHECK_ERROR_THROW(
          std::find(rpc_backend_name_vec_.begin(), rpc_backend_name_vec_.end(), backend_name) != rpc_backend_name_vec_.end(),
          "Invalid rpc backend type '{}' for func '{}'",
          backend_name, item.func_name);
    }

    server_backends_rules.emplace_back(item.func_name, item.enable_backends);
  }
  rpc_backend_manager_.SetServersBackendsRules(server_backends_rules);

  // 初始化backend manager
  rpc_backend_manager_.Initialize(rpc_registry_ptr_.get());

  options_node = options_;

  AIMRT_INFO(R"str(Rpc manager init complete. options:
----------------------------- aimrt.rpc ----------------------------------------
{}
----------------------------- aimrt.rpc ----------------------------------------

rpc backends list: {}
)str",
             YAML::Dump(options_node),
             aimrt::common::util::Vec2Str(rpc_backend_name_vec_));
}

void RpcManager::Start() {
  AIMRT_CHECK_ERROR_THROW(
      std::atomic_exchange(&state_, State::Start) == State::Init,
      "Function can only be called when state is 'Init'.");

  rpc_backend_manager_.Start();

  if (GetLogger().GetLogLevel() <= aimrt::common::util::kLogLevelInfo) {
    std::vector<std::vector<std::string>> client_info_table =
        {{"func", "module", "backends"}};

    const auto& client_backend_info = rpc_backend_manager_.GetClientsBackendInfo();
    const auto& client_index_map = rpc_registry_ptr_->GetClientIndexMap();

    for (const auto& client_index_itr : client_index_map) {
      auto client_backend_itr = client_backend_info.find(client_index_itr.first);
      AIMRT_CHECK_ERROR_THROW(client_backend_itr != client_backend_info.end(),
                              "Invalid rpc registry info.");

      for (const auto& item : client_index_itr.second) {
        std::vector<std::string> cur_client_info(3);
        cur_client_info[0] = client_index_itr.first;
        cur_client_info[1] = item->module_name;
        cur_client_info[2] = aimrt::common::util::JoinVec(client_backend_itr->second, ",");
        client_info_table.emplace_back(std::move(cur_client_info));
      }
    }

    std::vector<std::vector<std::string>> server_info_table =
        {{"func", "module", "backends"}};

    const auto& server_backend_info = rpc_backend_manager_.GetServersBackendInfo();
    const auto& server_index_map = rpc_registry_ptr_->GetServiceIndexMap();

    for (const auto& server_index_itr : server_index_map) {
      auto server_backend_itr = server_backend_info.find(server_index_itr.first);
      AIMRT_CHECK_ERROR_THROW(server_backend_itr != server_backend_info.end(),
                              "Invalid rpc registry info.");

      for (const auto& item : server_index_itr.second) {
        std::vector<std::string> cur_server_info(3);
        cur_server_info[0] = server_index_itr.first;
        cur_server_info[1] = item->module_name;
        cur_server_info[2] = aimrt::common::util::JoinVec(server_backend_itr->second, ",");
        server_info_table.emplace_back(std::move(cur_server_info));
      }
    }

    AIMRT_INFO(R"str(Rpc manager start complete.
client info table:{}
server info table:{}
)str",
               aimrt::common::util::DrawTable(client_info_table),
               aimrt::common::util::DrawTable(server_info_table));
  }
}

void RpcManager::Shutdown() {
  if (std::atomic_exchange(&state_, State::Shutdown) == State::Shutdown)
    return;

  AIMRT_INFO("Rpc manager Shutdown.");

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
  std::unique_ptr<RpcBackendBase> local_rpc_backend_ptr =
      std::make_unique<LocalRpcBackend>();

  static_cast<LocalRpcBackend*>(local_rpc_backend_ptr.get())
      ->SetLogger(logger_ptr_);

  RegisterRpcBackend(std::move(local_rpc_backend_ptr));
}
}  // namespace aimrt::runtime::core::rpc
