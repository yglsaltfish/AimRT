#include "core/module/module_manager.h"

#include <algorithm>

#include "aimrt_module_cpp_interface/util/string.h"
#include "core/logger/log_level_tool.h"
#include "util/stl_tool.h"

namespace YAML {
template <>
struct convert<aimrt::runtime::core::module::ModuleManager::Options> {
  using Options = aimrt::runtime::core::module::ModuleManager::Options;

  static Node encode(const Options& rhs) {
    Node node;

    node["pkgs"] = YAML::Node();
    for (const auto& pkg_options : rhs.pkgs_options) {
      Node pkg_options_node;
      pkg_options_node["path"] = pkg_options.path;
      pkg_options_node["disable_modules"] = pkg_options.disable_modules;
      node["pkgs"].push_back(pkg_options_node);
    }

    node["modules"] = YAML::Node();
    for (const auto& module_options : rhs.modules_options) {
      Node module_options_node;
      module_options_node["name"] = module_options.name;
      if (!module_options.use_default_log_lvl)
        module_options_node["log_lvl"] =
            aimrt::runtime::core::logger::LogLevelTool::GetLogLevelName(module_options.log_lvl);

      if (!module_options.cfg_file_path.empty())
        module_options_node["cfg_file_path"] = module_options.cfg_file_path;

      node["modules"].push_back(module_options_node);
    }

    return node;
  }

  static bool decode(const Node& node, Options& rhs) {
    if (!node.IsMap()) return false;

    if (node["pkgs"] && node["pkgs"].IsSequence()) {
      for (auto& pkg_options_node : node["pkgs"]) {
        Options::PkgLoaderOptions pkg_options{
            .path = pkg_options_node["path"].as<std::string>()};

        if (pkg_options_node["disable_module"])
          pkg_options.disable_modules =
              pkg_options_node["disable_module"].as<std::vector<std::string>>();

        rhs.pkgs_options.emplace_back(std::move(pkg_options));
      }
    }

    if (node["modules"] && node["modules"].IsSequence()) {
      for (auto& module_options_node : node["modules"]) {
        Options::ModuleOptions module_options{
            .name = module_options_node["name"].as<std::string>()};

        if (module_options_node["log_lvl"]) {
          module_options.use_default_log_lvl = false;
          module_options.log_lvl = aimrt::runtime::core::logger::LogLevelTool::GetLogLevelFromName(
              module_options_node["log_lvl"].as<std::string>());
        }

        if (module_options_node["cfg_file_path"]) {
          module_options.cfg_file_path =
              module_options_node["cfg_file_path"].as<std::string>();
        }

        rhs.modules_options.emplace_back(std::move(module_options));
      }
    }

    return true;
  }
};
}  // namespace YAML

namespace aimrt::runtime::core::module {

void ModuleManager::Initialize(YAML::Node options_node) {
  AIMRT_CHECK_ERROR_THROW(
      module_proxy_configurator_,
      "Module proxy configurator is not set before initialize.");

  AIMRT_CHECK_ERROR_THROW(
      std::atomic_exchange(&state_, State::Init) == State::PreInit,
      "Module manager can only be initialized once.");

  if (options_node && !options_node.IsNull())
    options_ = options_node.as<Options>();

  // 加载所有动态库
  for (const auto& pkg_options : options_.pkgs_options) {
    // 检查重复动态库
    auto finditr =
        std::find_if(options_.pkgs_options.begin(), options_.pkgs_options.end(),
                     [&pkg_options](const auto& op) {
                       if (&pkg_options == &op) return false;
                       return op.path == pkg_options.path;
                     });
    AIMRT_CHECK_ERROR_THROW(finditr == options_.pkgs_options.end(),
                            "Duplicate load pkg lib {}", pkg_options.path);

    auto module_loader_ptr = std::make_unique<ModuleLoader>();
    module_loader_ptr->SetLogger(logger_ptr_);
    module_loader_ptr->LoadPkg(pkg_options.path, pkg_options.disable_modules);

    AIMRT_INFO("Load pkg '{}' succeeded, found {} module, load {} module:\n{}",
               pkg_options.path, module_loader_ptr->GetModuleNameList().size(),
               module_loader_ptr->GetModuleNameList().size(),
               common::util::Vec2Str(module_loader_ptr->GetModuleNameList()));

    module_loader_map_.emplace(pkg_options.path, std::move(module_loader_ptr));
  }

  // 初始化直接注册的模块
  for (const auto& module_ptr : registered_module_vec_) {
    assert(module_ptr != nullptr);
    auto info = module_ptr->info(module_ptr->impl);
    const std::string& module_name = aimrt::util::ToStdString(info.name);

    // 检查重复模块
    auto finditr = module_wrapper_map_.find(module_name);
    AIMRT_CHECK_ERROR_THROW(finditr == module_wrapper_map_.end(),
                            "Duplicate module '{}' in core and lib {}.",
                            module_name, finditr->second->info.pkg_path);

    // 初始化module wrapper
    auto module_wrapper_ptr = std::make_unique<ModuleWrapper>(
        ModuleWrapper{
            .info = util::ModuleDetailInfo{
                .name = module_name,
                .pkg_path = "core",
                .major_version = info.major_version,
                .minor_version = info.minor_version,
                .patch_version = info.patch_version,
                .build_version = info.build_version,
                .author = aimrt::util::ToStdString(info.author),
                .description = aimrt::util::ToStdString(info.description)},
            .loader_ptr = nullptr,
            .module_ptr = module_ptr,
            .core_proxy_ptr = std::make_unique<CoreProxy>()});

    InitModule(module_wrapper_ptr.get());

    module_init_order_.emplace_back(module_name);
    module_detail_info_vec_.emplace_back(&(module_wrapper_ptr->info));
    module_wrapper_map_.emplace(module_name, std::move(module_wrapper_ptr));
  }

  // 初始化动态库加载的模块
  for (const auto& module_loader_itr : module_loader_map_) {
    const auto& pkg_path = module_loader_itr.first;
    ModuleLoader& module_loader = *(module_loader_itr.second);

    const auto& module_name_list = module_loader.GetLoadedModuleNameList();
    for (const auto& module_name : module_name_list) {
      // 检查重复模块
      AIMRT_CHECK_ERROR_THROW(
          module_wrapper_map_.find(module_name) == module_wrapper_map_.end(),
          "Duplicate module '{}' in lib {} and {}.", module_name,
          module_wrapper_map_.find(module_name)->second->info.pkg_path,
          pkg_path);

      // 初始化module wrapper
      auto module_ptr = module_loader.GetModule(module_name);
      assert(module_ptr != nullptr);
      auto info = module_ptr->info(module_ptr->impl);

      auto module_wrapper_ptr = std::make_unique<ModuleWrapper>(
          ModuleWrapper{
              .info = util::ModuleDetailInfo{
                  .name = module_name,
                  .pkg_path = pkg_path,
                  .major_version = info.major_version,
                  .minor_version = info.minor_version,
                  .patch_version = info.patch_version,
                  .build_version = info.build_version,
                  .author = aimrt::util::ToStdString(info.author),
                  .description = aimrt::util::ToStdString(info.description)},
              .loader_ptr = module_loader_itr.second.get(),
              .module_ptr = module_ptr,
              .core_proxy_ptr = std::make_unique<CoreProxy>()});

      InitModule(module_wrapper_ptr.get());

      module_init_order_.emplace_back(module_name);
      module_detail_info_vec_.emplace_back(&(module_wrapper_ptr->info));
      module_wrapper_map_.emplace(module_name, std::move(module_wrapper_ptr));
    }
  }

  options_node = options_;
}

void ModuleManager::Start() {
  AIMRT_CHECK_ERROR_THROW(
      std::atomic_exchange(&state_, State::Start) == State::Init,
      "Function can only be called when state is 'Init'.");

  for (const auto& module_name : module_init_order_) {
    auto module_wrapper_map_itr = module_wrapper_map_.find(module_name);
    assert(module_wrapper_map_itr != module_wrapper_map_.end());

    auto module_ptr = module_wrapper_map_itr->second->module_ptr;

    bool start_ret = module_ptr->start(module_ptr->impl);
    AIMRT_CHECK_ERROR_THROW(start_ret, "Start module '{}' failed.", module_name);

    AIMRT_INFO("Start module '{}' succeeded.", module_name);
  }
}

void ModuleManager::Shutdown() {
  if (std::atomic_exchange(&state_, State::Shutdown) == State::Shutdown)
    return;

  // 按照反顺序执行Shutdown
  for (auto itr = module_init_order_.rbegin(); itr != module_init_order_.rend(); ++itr) {
    const auto& module_name = *itr;
    auto find_itr = module_wrapper_map_.find(module_name);
    assert(find_itr != module_wrapper_map_.end());

    AIMRT_INFO("Start shutdown module '{}'.", module_name);
    find_itr->second->module_ptr->shutdown(find_itr->second->module_ptr->impl);
    AIMRT_INFO("Shutdown module '{}'.", module_name);
  }

  module_wrapper_map_.clear();

  // module_loader_map_ 不能清理掉，有很多回调是从其他dll中注册过来的

  module_init_order_.clear();

  registered_module_vec_.clear();

  module_proxy_configurator_ = CoreProxyConfigurator();
}

void ModuleManager::RegisterModule(const aimrt_module_base_t* module) {
  AIMRT_CHECK_ERROR_THROW(
      state_.load() == State::PreInit,
      "Function can only be called when state is 'PreInit'.");

  AIMRT_CHECK_ERROR_THROW(module != nullptr, "Register invalid module");

  registered_module_vec_.emplace_back(module);
}

void ModuleManager::RegisterCoreProxyConfigurator(
    CoreProxyConfigurator&& module_proxy_configurator) {
  AIMRT_CHECK_ERROR_THROW(
      state_.load() == State::PreInit,
      "Function can only be called when state is 'PreInit'.");

  module_proxy_configurator_ = std::move(module_proxy_configurator);
}

const std::vector<std::string>& ModuleManager::GetModuleNameList() const {
  AIMRT_CHECK_ERROR_THROW(
      state_.load() == State::Init,
      "Function can only be called when state is 'Init'.");

  return module_init_order_;
}

const std::vector<const util::ModuleDetailInfo*>& ModuleManager::GetModuleDetailInfoList() const {
  AIMRT_CHECK_ERROR_THROW(
      state_.load() == State::Init,
      "Function can only be called when state is 'Init'.");

  return module_detail_info_vec_;
}

void ModuleManager::InitModule(ModuleWrapper* module_wrapper_ptr) {
  const auto& module_name = module_wrapper_ptr->info.name;
  const auto* module_ptr = module_wrapper_ptr->module_ptr;

  auto find_module_options_itr = std::find_if(
      options_.modules_options.begin(), options_.modules_options.end(),
      [&module_name](const auto& module_options) {
        return module_options.name == module_name;
      });

  auto& detail_info = module_wrapper_ptr->info;
  if (find_module_options_itr != options_.modules_options.end()) {
    detail_info.log_lvl = find_module_options_itr->log_lvl;
    detail_info.use_default_log_lvl = find_module_options_itr->use_default_log_lvl;
    detail_info.cfg_file_path = find_module_options_itr->cfg_file_path;
  } else {
    options_.modules_options.emplace_back(Options::ModuleOptions{
        .name = module_name,
        .log_lvl = detail_info.log_lvl,
        .use_default_log_lvl = detail_info.use_default_log_lvl,
        .cfg_file_path = detail_info.cfg_file_path});
  }

  module_proxy_configurator_(detail_info,
                             *(module_wrapper_ptr->core_proxy_ptr));

  // 初始化模块
  bool ret = module_ptr->initialize(
      module_ptr->impl, module_wrapper_ptr->core_proxy_ptr->NativeHandle());

  AIMRT_CHECK_ERROR_THROW(ret, "Init module '{}' failed.", module_name);

  AIMRT_INFO("Init module '{}' succeeded.", module_name);
}

}  // namespace aimrt::runtime::core::module
