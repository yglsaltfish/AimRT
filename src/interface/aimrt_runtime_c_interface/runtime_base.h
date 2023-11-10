#pragma once

#include "aimrt_module_c_interface/module_base.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  bool (*initialize)(void* impl, aimrt_string_view_t cfg_file_path);

  bool (*start)(void* impl);

  void (*shutdown)(void* impl);

  bool (*register_module)(void* impl, const aimrt_module_base_t* module);

  void* impl;
} aimrt_runtime_base_t;

#ifdef __cplusplus
}
#endif