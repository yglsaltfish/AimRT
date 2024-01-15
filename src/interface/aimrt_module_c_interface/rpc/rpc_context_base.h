#pragma once

#include "stdint.h"

#include "aimrt_module_c_interface/util/string.h"

#ifdef __cplusplus
extern "C" {
#endif

/// Some frame fields. Users should not directly modify these fields

/// eg: backend://uri_defined_by_backend
#define AIMRT_RPC_CONTEXT_KEY_FROM_ADDR "aimrt::from_addr"

/// eg: backend://uri_defined_by_backend
#define AIMRT_RPC_CONTEXT_KEY_TO_ADDR "aimrt::to_addr"

/// eg: json/pb
#define AIMRT_RPC_CONTEXT_KEY_SERIALIZATION_TYPE "aimrt::serialization_type"

/**
 * @brief Rpc context operate interface
 *
 */
typedef struct {
  /**
   * @brief Function to get the deadline timestamp(ns)
   * @note Nanosecond since 1970-01-01 00:00:00 UTC
   *
   */
  uint64_t (*get_deadline_ns)(void* impl);

  /**
   * @brief Function to set the deadline timestamp(ns)
   * @note Nanosecond since 1970-01-01 00:00:00 UTC
   *
   */
  void (*set_deadline_ns)(void* impl, uint64_t ddl);

  /**
   * @brief Function to get kv meta data
   *
   */
  aimrt_string_view_t (*get_meta_val)(void* impl, aimrt_string_view_t key);

  /**
   * @brief Function to set kv meta data
   *
   */
  void (*set_meta_val)(
      void* impl, aimrt_string_view_t key, aimrt_string_view_t val);
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
