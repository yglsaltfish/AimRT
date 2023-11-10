#pragma once

#include "aimrt_runtime_c_interface/runtime_base.h"
#include "aimrt_runtime_c_interface/runtime_export.h"

#ifdef __cplusplus
extern "C" {
#endif

AIMRT_RUNTIME_EXPORT const aimrt_runtime_base_t* AimRTDynlibCreateRuntimeHandle();

AIMRT_RUNTIME_EXPORT void AimRTDynlibDestroyRuntimeHandle(const aimrt_runtime_base_t* runtime_ptr);

#ifdef __cplusplus
}
#endif