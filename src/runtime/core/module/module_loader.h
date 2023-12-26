#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "aimrt_module_c_interface/module_base.h"
#include "util/dynamic_lib.h"
#include "util/log_util.h"
#include "util/string_util.h"

namespace aimrt::runtime::core::module {

class ModuleLoader {
 public:
  ModuleLoader()
      : logger_ptr_(std::make_shared<common::util::LoggerWrapper>()) {}
  ~ModuleLoader() { UnLoadPkg(); }

  ModuleLoader(const ModuleLoader&) = delete;
  ModuleLoader& operator=(const ModuleLoader&) = delete;

  void SetLogger(const std::shared_ptr<common::util::LoggerWrapper>& logger_ptr) { logger_ptr_ = logger_ptr; }
  const common::util::LoggerWrapper& GetLogger() const { return *logger_ptr_; }

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
  std::shared_ptr<common::util::LoggerWrapper> logger_ptr_;

  std::string pkg_path_;
  common::util::DynamicLib dynamic_lib_;

  common::util::DynamicLib::SymbolType destroy_func_ = nullptr;

  std::vector<std::string> module_name_vec_;
  std::vector<std::string> loaded_module_name_vec_;
  std::unordered_map<
      std::string,
      const aimrt_module_base_t*,
      aimrt::common::util::StringHash,
      std::equal_to<>>
      module_ptr_map_;
};

}  // namespace aimrt::runtime::core::module
