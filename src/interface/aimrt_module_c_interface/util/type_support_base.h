#pragma once

#include "aimrt_module_c_interface/util/buffer_base.h"
#include "aimrt_module_c_interface/util/string.h"

extern "C" {

/**
 * @brief Type support interface
 *
 */
typedef struct {
  /**
   * @brief Name of type
   *
   */
  aimrt_string_view_t type_name;

  /**
   * @brief Function to create msg
   * @note
   * Output: Pointer to msg
   */
  void* (*create)();

  /**
   * @brief Function to destory msg
   * @note
   * Input 1: Pointer to msg
   */
  void (*destory)(void* msg);

  /**
   * @brief Function to copy msg
   * @note
   * Input 1: Pointer to the msg to be copied from
   * Input 2: Pointer to the msg to be copied to
   */
  void (*copy)(const void* from, void* to);

  /**
   * @brief Function to move msg
   * @note
   * Input 1: Pointer to the msg to be moved from
   * Input 2: Pointer to the msg to be moved to
   */
  void (*move)(void* from, void* to);

  /**
   * @brief Function to serialize the msg
   * @note
   * Input 1: Serialization type, eg: pb/json
   * Input 2: Pointer to the msg to be serialized
   * Input 3: Pointer to the buffer array, with allocator
   * Output: Serialization result
   */
  bool (*serialize)(
      aimrt_string_view_t serialization_type,
      const void* msg,
      aimrt_buffer_array_t* buffer_array);

  /**
   * @brief Function to deserialize the msg
   * @note
   * Input 1: Serialization type, eg: pb/json
   * Input 2: Buffer array
   * Input 3: Pointer to the msg to be deserialized
   * Output: Deserialization result
   */
  bool (*deserialize)(
      aimrt_string_view_t serialization_type,
      aimrt_buffer_array_view_t buffer_array_view,
      void* msg);

  /**
   * @brief Number of serialization types supported
   *
   */
  size_t serialization_types_supported_num;

  /**
   * @brief List of serialization types supported
   * @note
   * The length of this array is defined by serialization_types_supported_num
   */
  const aimrt_string_view_t* serialization_types_supported_list;

  /**
   * @brief For custom type support
   *
   */
  const void* custom_type_support_ptr;
} aimrt_type_support_base_t;
}
