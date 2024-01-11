#include "aimrt_runtime_c_interface/runtime_main.h"
#include "core/aimrt_core.h"

extern "C" {

const aimrt_runtime_base_t* AimRTDynlibCreateRuntimeHandle() {
  return new aimrt_runtime_base_t{
      .initialize = [](void* impl, aimrt_runtime_options_t options) -> bool {
        try {
          static_cast<aimrt::runtime::core::AimRTCore*>(impl)->Initialize(
              aimrt::runtime::core::AimRTCore::Options{
                  .cfg_file_path = aimrt::util::ToStdString(options.cfg_file_path),
                  .dump_cfg_file = options.dump_cfg_file,
                  .dump_cfg_file_path = aimrt::util::ToStdString(options.dump_cfg_file_path),
                  .register_signal = options.register_signal,
                  .auto_set_to_global = options.auto_set_to_global});
          return true;
        } catch (const std::exception& e) {
          fprintf(stderr, "aimrt core initialize failed, %s\n", e.what());
          return false;
        }
        return false;
      },
      .start = [](void* impl) -> bool {
        try {
          static_cast<aimrt::runtime::core::AimRTCore*>(impl)->Start();
          return true;
        } catch (const std::exception& e) {
          fprintf(stderr, "aimrt core start failed, %s\n", e.what());
          return false;
        }
        return false;
      },
      .shutdown = [](void* impl) {
        try {
          static_cast<aimrt::runtime::core::AimRTCore*>(impl)->Shutdown();
        } catch (const std::exception& e) {
          fprintf(stderr, "aimrt core shutdown failed, %s\n", e.what());
        }  //
      },
      .register_module = [](void* impl, const aimrt_module_base_t* module) -> bool {
        try {
          static_cast<aimrt::runtime::core::AimRTCore*>(impl)
              ->GetModuleManager()
              .RegisterModule(module);
          return true;
        } catch (const std::exception& e) {
          fprintf(stderr, "aimrt core register module failed, %s\n", e.what());
          return false;
        }
        return false;
      },
      .impl = new aimrt::runtime::core::AimRTCore()};
}

void AimRTDynlibDestroyRuntimeHandle(
    const aimrt_runtime_base_t* runtime_ptr) {
  delete static_cast<aimrt::runtime::core::AimRTCore*>(runtime_ptr->impl);
  delete runtime_ptr;
}
}
