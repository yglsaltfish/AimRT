#include "aimrt_pkg_c_interface/pkg_macro.h"
#include "executor_co_module/executor_co_module.h"
#include "executor_module/executor_module.h"

static constexpr std::tuple<std::string_view, aimrt::ModuleBase* (*)()>
    aimrt_module_register_array[]{
        {"ExecutorModule", []() -> aimrt::ModuleBase* {
           return new aimrt::examples::cpp::executor::executor_module::ExecutorModule();
         }},
        {"ExecutorCoModule", []() -> aimrt::ModuleBase* {
           return new aimrt::examples::cpp::executor::executor_co_module::ExecutorCoModule();
         }}};

AIMRT_PKG_MAIN(aimrt_module_register_array)
