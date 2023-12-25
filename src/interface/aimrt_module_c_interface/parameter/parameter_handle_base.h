#pragma once

#include "inttypes.h"

#include "aimrt_module_c_interface/util/function_base.h"
#include "aimrt_module_c_interface/util/string.h"

#ifdef __cplusplus
extern "C" {
#endif

/// Parameter type
typedef enum {
  AIMRT_PARAMETER_TYPE_NULL = 0,
  AIMRT_PARAMETER_TYPE_BOOL = 1,                    // use b
  AIMRT_PARAMETER_TYPE_INTEGER = 2,                 // use i
  AIMRT_PARAMETER_TYPE_UNSIGNED_INTEGER = 3,        // use u
  AIMRT_PARAMETER_TYPE_DOUBLE = 4,                  // use f
  AIMRT_PARAMETER_TYPE_STRING = 5,                  // use array, char array
  AIMRT_PARAMETER_TYPE_BYTE_ARRAY = 6,              // use array, char array
  AIMRT_PARAMETER_TYPE_BOOL_ARRAY = 7,              // use array, char array
  AIMRT_PARAMETER_TYPE_INTEGER_ARRAY = 8,           // use array, int64 array
  AIMRT_PARAMETER_TYPE_UNSIGNED_INTEGER_ARRAY = 9,  // use array, uint64 array
  AIMRT_PARAMETER_TYPE_DOUBLE_ARRAY = 10,           // use array, double array
  AIMRT_PARAMETER_TYPE_STRING_ARRAY = 11,           // use array, aimrt_string_view_t array
} aimrt_parameter_type_t;

/// Parameter variant
typedef struct {
  aimrt_parameter_type_t type;
  union {
    bool b;

    int64_t i;

    uint64_t u;

    double f;

    struct {
      const void* data;
      size_t len;
    } array;

  } data;
} aimrt_parameter_view_t;

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
  aimrt_parameter_view_t parameter_view;

  /**
   * @brief release callback
   * @note ops type is aimrt_function_parameter_ref_release_callback_ops_t
   * When parameter type is simple (not array), callback will be null
   */
  aimrt_function_base_t* release_callback;
} aimrt_parameter_view_holder_t;

/// Parameter handle
typedef struct {
  /**
   * @brief Get parameter reference
   * @note
   * Input 1: Implement pointer to parameter handle
   * Input 2: Parameter name
   * Output: Parameter reference with release callback
   */
  aimrt_parameter_view_holder_t (*get_parameter)(void* impl, aimrt_string_view_t name);

  /**
   * @brief Set parameter
   * @note
   * Input 1: Implement pointer to parameter handle
   * Input 2: Parameter name
   * Input 3: Parameter value
   * Output: Set result
   */
  bool (*set_parameter)(void* impl, aimrt_string_view_t name, aimrt_parameter_view_t parameter);

  /// Implement pointer
  void* impl;
} aimrt_parameter_handle_base_t;

#ifdef __cplusplus
}
#endif
