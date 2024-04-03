#include "core/module/module_loader.h"
#include "aimrt_module_cpp_interface/util/string.h"

namespace aimrt::runtime::core::module {

using DynlibGetModuleNumFunc = size_t (*)();
using DynlibGetModuleNameListFunc = const aimrt_string_view_t* (*)();
using DynlibCreateModuleFunc = const aimrt_module_base_t* (*)(aimrt_string_view_t);
using DynlibDestroyModuleFunc = void (*)(const aimrt_module_base_t*);

static constexpr const char* kDynlibGetModuleNumFuncName = "AimRTDynlibGetModuleNum";
static constexpr const char* kDynlibGetModuleNameListFuncName = "AimRTDynlibGetModuleNameList";
static constexpr const char* kDynlibCreateModuleFuncName = "AimRTDynlibCreateModule";
static constexpr const char* kDynlibDestroyModuleFuncName = "AimRTDynlibDestroyModule";

void ModuleLoader::LoadPkg(std::string_view pkg_path,
                           const std::vector<std::string>& disable_modules) {
  pkg_path_ = pkg_path;

  AIMRT_CHECK_ERROR_THROW(dynamic_lib_.Load(pkg_path_),
                          "Load dynamic lib failed, lib path {}, error info {}",
                          pkg_path_, common::util::DynamicLib::GetErr());

  auto get_module_num_func = dynamic_lib_.GetSymbol(kDynlibGetModuleNumFuncName);
  AIMRT_CHECK_ERROR_THROW(get_module_num_func != nullptr,
                          "Cannot find symbol '{}' in lib {}.",
                          kDynlibGetModuleNumFuncName, pkg_path_);

  auto get_module_name_list_func = dynamic_lib_.GetSymbol(kDynlibGetModuleNameListFuncName);
  AIMRT_CHECK_ERROR_THROW(get_module_name_list_func != nullptr,
                          "Cannot find symbol '{}' in lib {}.",
                          kDynlibGetModuleNameListFuncName, pkg_path_);

  auto create_func = dynamic_lib_.GetSymbol(kDynlibCreateModuleFuncName);
  AIMRT_CHECK_ERROR_THROW(create_func != nullptr,
                          "Cannot find symbol '{}' in lib {}.",
                          kDynlibCreateModuleFuncName, pkg_path_);

  destroy_func_ = dynamic_lib_.GetSymbol(kDynlibDestroyModuleFuncName);
  AIMRT_CHECK_ERROR_THROW(destroy_func_ != nullptr,
                          "Cannot find symbol '{}' in lib {}.",
                          kDynlibDestroyModuleFuncName, pkg_path_);

  size_t module_num = ((DynlibGetModuleNumFunc)get_module_num_func)();
  AIMRT_CHECK_ERROR_THROW(module_num > 0, "No module in lib {}.", pkg_path_);

  const aimrt_string_view_t* module_name_array =
      ((DynlibGetModuleNameListFunc)get_module_name_list_func)();
  AIMRT_CHECK_ERROR_THROW(module_name_array != nullptr,
                          "Module name list is null in lib {}.", pkg_path_);

  // 检查模块列表
  module_name_vec_.reserve(module_num);
  for (size_t ii = 0; ii < module_num; ++ii) {
    auto module_name = aimrt::util::ToStdStringView(module_name_array[ii]);
    AIMRT_CHECK_ERROR_THROW(!module_name.empty(),
                            "Module name index [{}] is empty in lib {}.",
                            ii, pkg_path_);

    AIMRT_CHECK_ERROR_THROW(
        std::find(module_name_vec_.begin(), module_name_vec_.end(), module_name) == module_name_vec_.end(),
        "Module name '{}' repeated in lib {}.", module_name, pkg_path_);

    module_name_vec_.emplace_back(module_name);
  }

  // 加载模块
  loaded_module_name_vec_.reserve(module_num);
  for (size_t ii = 0; ii < module_num; ++ii) {
    const auto& module_name = module_name_vec_[ii];
    auto finditr = std::find(disable_modules.begin(), disable_modules.end(), module_name);

    if (finditr != disable_modules.end()) continue;

    const aimrt_module_base_t* module_ptr =
        ((DynlibCreateModuleFunc)create_func)(aimrt::util::ToAimRTStringView(module_name));

    AIMRT_CHECK_ERROR_THROW(module_ptr != nullptr,
                            "Cannot create module '{}' in lib {}.",
                            module_name, pkg_path_);

    auto module_info = module_ptr->info(module_ptr->impl);
    auto real_module_name = aimrt::util::ToStdStringView(module_info.name);
    AIMRT_CHECK_ERROR_THROW(
        real_module_name == module_name,
        "Require module name '{}', but get module name '{}' in lib {}.",
        module_name, real_module_name, pkg_path_);

    loaded_module_name_vec_.emplace_back(module_name);
    module_ptr_map_.emplace(module_name, module_ptr);
  }
}

void ModuleLoader::UnLoadPkg() {
  if (!dynamic_lib_.IsLoaded()) return;

  for (auto& module_ptr_itr : module_ptr_map_)
    DestroyModule(module_ptr_itr.second);

  module_ptr_map_.clear();
  loaded_module_name_vec_.clear();
  module_name_vec_.clear();
  destroy_func_ = nullptr;

  dynamic_lib_.Unload();
}

const aimrt_module_base_t* ModuleLoader::GetModule(std::string_view module_name) {
  auto finditr = module_ptr_map_.find(module_name);
  if (finditr == module_ptr_map_.end()) {
    AIMRT_ERROR("No module name '{}' in lib {}", module_name, pkg_path_);
    return nullptr;
  }
  return finditr->second;
}

void ModuleLoader::DestroyModule(const aimrt_module_base_t* module_ptr) {
  if (module_ptr == nullptr) return;

  assert(destroy_func_ != nullptr);

  ((DynlibDestroyModuleFunc)destroy_func_)(module_ptr);
}

}  // namespace aimrt::runtime::core::module
