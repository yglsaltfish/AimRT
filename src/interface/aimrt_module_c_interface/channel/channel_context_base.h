#pragma once

#include "stdint.h"

#include "aimrt_module_c_interface/util/string.h"

#ifdef __cplusplus
extern "C" {
#endif

/// Some frame fields. Users should not directly modify these fields

#define AIMRT_CHANNEL_CONTEXT_KEY_PREFIX "aimrt-"

/// eg: json/pb
#define AIMRT_CHANNEL_CONTEXT_KEY_SERIALIZATION_TYPE "aimrt-serialization_type"

/// eg: local/mqtt/http
#define AIMRT_CHANNEL_CONTEXT_KEY_BACKEND "aimrt-backend"

typedef enum {
  AIMRT_RPC_PUBLISHER_CONTEXT = 0,
  AIMRT_RPC_SUBSCRIBER_CONTEXT = 1,
} aimrt_channel_context_type_t;

/**
 * @brief Channel context operate interface
 *
 */
typedef struct {
  /**
   * @brief Function to check if the context is used already
   *
   */
  bool (*check_used)(void* impl);

  /**
   * @brief Function to set the context to used status
   *
   */
  void (*set_used)(void* impl);

  /**
   * @brief Function to get context type
   *
   */
  aimrt_channel_context_type_t (*get_type)(void* impl);

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

  /**
   * @brief Function to get all meta keys
   *
   */
  aimrt_string_view_array_t (*get_meta_keys)(void* impl);

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
