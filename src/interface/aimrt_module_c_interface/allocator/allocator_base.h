#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  /**
   * @brief Allocate thread local buf from core
   *
   */
  void* (*allocate_thread_local_buf)(void* impl, size_t buf_size);

  /**
   * @brief Release thread local buf
   *
   */
  void (*release_thread_local_buf)(void* impl);

  /// Implement pointer
  void* impl;
} aimrt_allocator_base_t;

#ifdef __cplusplus
}
#endif
