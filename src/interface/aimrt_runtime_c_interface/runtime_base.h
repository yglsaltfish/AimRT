#pragma once

#include "aimrt_module_c_interface/module_base.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  aimrt_string_view_t cfg_file_path;

  bool dump_cfg_file;
  aimrt_string_view_t dump_cfg_file_path;

} aimrt_runtime_options_t;

typedef struct {
  bool (*initialize)(void* impl, aimrt_runtime_options_t options);

  bool (*start)(void* impl);

  void (*shutdown)(void* impl);

  bool (*register_module)(void* impl, aimrt_string_view_t pkg, const aimrt_module_base_t* module);

  void* impl;
} aimrt_runtime_base_t;

#ifdef __cplusplus
}
#endif