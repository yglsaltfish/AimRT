#pragma once

#include "inttypes.h"

#include "aimrt_module_c_interface/util/buffer_base.h"
#include "aimrt_module_c_interface/util/function_base.h"
#include "aimrt_module_c_interface/util/string.h"

#ifdef __cplusplus
extern "C" {
#endif

/// Parameter type
typedef enum {
  AIMRT_PARAMETER_TYPE_BOOL = 0,
  AIMRT_PARAMETER_TYPE_INTEGER = 1,
  AIMRT_PARAMETER_TYPE_DOUBLE = 2,
  AIMRT_PARAMETER_TYPE_STRING = 3,
  AIMRT_PARAMETER_TYPE_BYTE_ARRAY = 4,
  AIMRT_PARAMETER_TYPE_BOOL_ARRAY = 5,
  AIMRT_PARAMETER_TYPE_INTEGER_ARRAY = 6,
  AIMRT_PARAMETER_TYPE_DOUBLE_ARRAY = 7,
  AIMRT_PARAMETER_TYPE_STRING_ARRAY = 8,
} aimrt_parameter_type_t;

/// Parameter variant
typedef struct {
  aimrt_parameter_type_t type;
  union {
    /// BOOL
    bool b;

    /// INTEGER
    int64_t i;

    /// DOUBLE
    double d;

    /// STRING & ARRAY
    aimrt_buffer_view_t buf;

  } data;
} aimrt_parameter_t;

/**
 * @brief Operate struct for parameter ref release callback
 * @note Signature form: void(*)()
 *
 */
typedef struct {
  void (*invoker)(void* object);
  void (*relocator)(void* from, void* to);
  void (*destroyer)(void* object);
} aimrt_function_parameter_ref_release_callback_ops_t;

/// Parameter reference with release callback
typedef struct {
  /// parameter
  aimrt_parameter_t parameter;

  /**
   * @brief release callback
   * @note ops type is aimrt_function_parameter_ref_release_callback_ops_t
   *
   */
  aimrt_function_base_t* callback;
} aimrt_parameter_ref_t;

/// Parameter handle
typedef struct {
  /**
   * @brief Get parameter reference
   * @note
   * Input 1: Implement pointer to parameter handle
   * Input 2: Parameter name
   * Output: Parameter reference with release callback
   */
  aimrt_parameter_ref_t (*get_parameter)(void* impl, aimrt_string_view_t name);

  /**
   * @brief Set parameter
   * @note
   * Input 1: Implement pointer to parameter handle
   * Input 2: Parameter name
   * Input 3: Parameter value
   */
  void (*set_parameter)(void* impl, aimrt_string_view_t name, aimrt_parameter_t parameter);

  /// Implement pointer
  void* impl;
} aimrt_parameter_handle_base_t;

#ifdef __cplusplus
}
#endif
