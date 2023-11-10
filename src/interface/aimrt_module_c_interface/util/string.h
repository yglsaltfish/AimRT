#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief String
 *
 */
typedef struct {
  /// Char buffer
  char* str;

  /// Length of char buffer
  size_t len;
} aimrt_string_t;

/**
 * @brief String view
 *
 */
typedef struct {
  /// Const char buffer
  const char* str;

  /// Length of char buffer
  size_t len;
} aimrt_string_view_t;

#ifdef __cplusplus
}
#endif
