#pragma once

#include "stdint.h"

#include "aimrt_module_c_interface/util/string.h"

#ifdef __cplusplus
extern "C" {
#endif

/// Some frame fields. Users should not directly modify these fields

/// eg: backend://uri_defined_by_backend
#define AIMRT_RPC_CONTEXT_KEY_TO_ADDR "aimrt::to_addr"

/// eg: json/pb
#define AIMRT_RPC_CONTEXT_KEY_SERIALIZATION_TYPE "aimrt::serialization_type"

/// eg: pb:/example.ExampleService/GetFooData
#define AIMRT_RPC_CONTEXT_KEY_FUNCTION_NAME "aimrt::function_name"

/**
 * @brief Rpc context operate interface
 *
 */
typedef struct {
  /**
   * @brief Function to get the timeout(ns)
   *
   */
  uint64_t (*get_timeout_ns)(void* impl);

  /**
   * @brief Function to set the timeout(ns)
   *
   */
  void (*set_timeout_ns)(void* impl, uint64_t timeout);

  /**
   * @brief Function to get kv meta data
   *
   */
  aimrt_string_view_t (*get_meta_val)(void* impl, aimrt_string_view_t key);

  /**
   * @brief Function to set kv meta data
   *
   */
  void (*set_meta_val)(void* impl, aimrt_string_view_t key, aimrt_string_view_t val);

  /**
   * @brief Function to get all meta keys
   *
   */
  aimrt_string_view_array_t (*get_meta_keys)(void* impl);

} aimrt_rpc_context_base_ops_t;

/**
 * @brief Rpc context interface
 *
 */
typedef struct {
  /// Const pointor to rpc context operate interface
  const aimrt_rpc_context_base_ops_t* ops;

  /// Implement pointer
  void* impl;
} aimrt_rpc_context_base_t;

#ifdef __cplusplus
}
#endif
