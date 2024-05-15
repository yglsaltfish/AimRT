#pragma once

#include <string>
#include <string_view>

#include "util/url_encode.h"

namespace aimrt::plugins::ros2_plugin {

/**
 * @brief Ros2NameEncode
 *
 * @param[in] str 待编码字符串
 * @param[in] up 是否转码为大写字符
 * @return std::string 转码后的结果字符串
 */
inline std::string Ros2NameEncode(std::string_view str, bool up = true) {
  std::string ret_str;
  size_t len = str.length();
  ret_str.reserve(len << 1);
  for (size_t i = 0; i < len; ++i) {
    if (isalnum((unsigned char)str[i]) || (str[i] == '/') || (str[i] == '{') || (str[i] == '}')) {
      ret_str += str[i];
    } else {
      ret_str += '_';
      ret_str += aimrt::common::util::ToHex((unsigned char)str[i] >> 4, up);
      ret_str += aimrt::common::util::ToHex((unsigned char)str[i] & 15, up);
    }
  }
  return ret_str;
}

/**
 * @brief Ros2NameDecode
 *
 * @param[in] str 待解码字符串
 * @return std::string 解码后的结果字符串
 */
inline std::string Ros2NameDecode(std::string_view str) {
  std::string ret_str;
  size_t len = str.length();
  ret_str.reserve(len);
  for (size_t i = 0; i < len; ++i) {
    if (str[i] == '_') {
      if (i + 2 < len) {
        unsigned char c = (aimrt::common::util::FromHex((unsigned char)str[++i])) << 4;
        ret_str += (c | aimrt::common::util::FromHex((unsigned char)str[++i]));
      } else {
        break;
      }
    } else {
      ret_str += str[i];
    }
  }
  return ret_str;
}

}  // namespace aimrt::plugins::ros2_plugin