#include "aimrt_pkg_c_interface/pkg_macro.h"
#include "helloworld_module/helloworld_module.h"

static constexpr std::tuple<std::string_view, aimrt::ModuleBase* (*)()>
    aimrt_module_register_array[]{
        {"HelloWorldModule", []() -> aimrt::ModuleBase* {
           return new aimrt::examples::example_helloworld::helloworld_module::
               HelloWorldModule();
         }}};

AIMRT_PKG_MAIN(aimrt_module_register_array)
