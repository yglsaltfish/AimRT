#pragma once

#include "stdint.h"

#include "aimrt_module_c_interface/util/string.h"

#ifdef __cplusplus
extern "C" {
#endif

/// Some frame fields. Users should not directly modify these fields

/// eg: backend://uri_defined_by_backend
#define AIMRT_CHANNEL_CONTEXT_KEY_FROM_ADDR "aimrt::from_addr"

/// eg: json/pb
#define AIMRT_CHANNEL_CONTEXT_KEY_SERIALIZATION_TYPE "aimrt::serialization_type"

/**
 * @brief Channel context operate interface
 *
 */
typedef struct {
  /**
   * @brief Function to get the msg timestamp(ns)
   * @note Nanosecond since 1970-01-01 00:00:00 UTC
   *
   */
  uint64_t (*get_msg_timestamp_ns)(void* impl);

  /**
   * @brief Function to set the msg timestamp(ns)
   * @note Nanosecond since 1970-01-01 00:00:00 UTC
   *
   */
  void (*set_msg_timestamp_ns)(void* impl, uint64_t t);

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
} aimrt_channel_context_base_ops_t;

/**
 * @brief Channel context interface
 *
 */
typedef struct {
  /// Const pointor to channel context operate interface
  const aimrt_channel_context_base_ops_t* ops;

  /// Implement pointer
  void* impl;
} aimrt_channel_context_base_t;

#ifdef __cplusplus
}
#endif
