#pragma once

#include <map>
#include <string>
#include <vector>

#include "aimrt_module_c_interface/module_base.h"
#include "util/dynamic_lib.h"

namespace aimrt::runtime::core::module {

class ModuleLoader {
 public:
  ModuleLoader() = default;
  ~ModuleLoader() { UnLoadPkg(); }

  ModuleLoader(const ModuleLoader&) = delete;
  ModuleLoader& operator=(const ModuleLoader&) = delete;

  void LoadPkg(std::string_view pkg_path,
               const std::vector<std::string>& disable_modules);

  void UnLoadPkg();

  const std::vector<std::string>& GetModuleNameList() const {
    return module_name_vec_;
  }

  const std::vector<std::string>& GetLoadedModuleNameList() const {
    return loaded_module_name_vec_;
  }

  const aimrt_module_base_t* GetModule(std::string_view module_name);
  void DestroyModule(const aimrt_module_base_t* module_ptr);

 private:
  std::string pkg_path_;
  common::util::DynamicLib dynamic_lib_;

  common::util::DynamicLib::SymbolType destroy_func_ = nullptr;

  std::vector<std::string> module_name_vec_;
  std::vector<std::string> loaded_module_name_vec_;
  std::map<std::string, const aimrt_module_base_t*, std::less<>> module_ptr_map_;
};

}  // namespace aimrt::runtime::core::module
