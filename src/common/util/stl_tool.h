#pragma once

#include <functional>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace aimrt::common::util {

/**
 * @brief Print std::vector with custom print func
 *
 * @tparam T
 * @param v
 * @param f
 * @return std::string
 */
template <typename T>
std::string Vec2Str(const std::vector<T>& v,
                    const std::function<std::string(const T&)>& f) {
  std::stringstream ss;
  ss << "size = " << v.size() << '\n';
  if (!f) return ss.str();

  constexpr size_t kMaxLineLen = 32;

  size_t ct = 0;
  for (size_t ii = 0; ii < v.size(); ++ii) {
    std::string obj_str = f(v[ii]);
    if (obj_str.empty()) obj_str = "<empty string>";

    ss << "[index=" << ct << "]:";
    if (obj_str.length() > kMaxLineLen || obj_str.find('\n') != std::string::npos) {
      ss << '\n';
    }

    ss << obj_str << '\n';

    ++ct;
  }

  return ss.str();
}

/**
 * @brief Print std::vector
 *
 * @tparam T
 * @param v
 * @return std::string
 */
template <typename T>
std::string Vec2Str(const std::vector<T>& v) {
  std::function<std::string(const T&)> f = [](const T& obj) {
    std::stringstream ss;
    ss << obj;
    return ss.str();
  };
  return Vec2Str(v, f);
}

/**
 * @brief Print std::set with custom print func
 *
 * @tparam T
 * @param s
 * @param f
 * @return std::string
 */
template <typename T>
std::string Set2Str(const std::set<T>& s,
                    const std::function<std::string(const T&)>& f) {
  std::stringstream ss;
  ss << "size = " << s.size() << '\n';
  if (!f) return ss.str();

  constexpr size_t kMaxLineLen = 32;

  size_t ct = 0;
  for (auto& itr : s) {
    std::string obj_str = f(itr);
    if (obj_str.empty()) obj_str = "<empty string>";

    ss << "[index=" << ct << "]:";
    if (obj_str.length() > kMaxLineLen || obj_str.find('\n') != std::string::npos) {
      ss << '\n';
    }

    ss << obj_str << '\n';

    ++ct;
  }
  return ss.str();
}

/**
 * @brief Print std::set
 *
 * @tparam T
 * @param s
 * @return std::string
 */
template <typename T>
std::string Set2Str(const std::set<T>& s) {
  std::function<std::string(const T&)> f = [](const T& obj) {
    std::stringstream ss;
    ss << obj;
    return ss.str();
  };
  return Set2Str(s, f);
}

/**
 * @brief Print std::map with custom print func
 *
 * @tparam KeyType
 * @tparam ValType
 * @param m
 * @param fkey
 * @param fval
 * @return std::string
 */
template <typename KeyType, typename ValType>
std::string Map2Str(const std::map<KeyType, ValType>& m,
                    const std::function<std::string(const KeyType&)>& fkey,
                    const std::function<std::string(const ValType&)>& fval) {
  std::stringstream ss;
  ss << "size = " << m.size() << '\n';
  if (!fkey) return ss.str();

  constexpr size_t kMaxLineLen = 32;

  size_t ct = 0;
  for (auto& itr : m) {
    std::string key_str = fkey(itr.first);
    if (key_str.empty()) key_str = "<empty string>";

    std::string val_str;
    if (fval) {
      val_str = fval(itr.second);
      if (val_str.empty()) val_str = "<empty string>";
    } else {
      val_str = "<unable to print>";
    }

    ss << "[index=" << ct << "]:\n  [key]:";
    if (key_str.length() > kMaxLineLen || key_str.find('\n') != std::string::npos) {
      ss << '\n';
    }

    ss << key_str << "\n  [val]:";

    if (val_str.length() > kMaxLineLen ||
        val_str.find('\n') != std::string::npos) {
      ss << '\n';
    }

    ss << val_str << '\n';

    ++ct;
  }
  return ss.str();
}

/**
 * @brief Print std::map
 *
 * @tparam KeyType
 * @tparam ValType
 * @param m
 * @return std::string
 */
template <typename KeyType, typename ValType>
std::string Map2Str(const std::map<KeyType, ValType>& m) {
  std::function<std::string(const KeyType&)> fkey = [](const KeyType& obj) {
    std::stringstream ss;
    ss << obj;
    return ss.str();
  };
  std::function<std::string(const ValType&)> fval = [](const ValType& obj) {
    std::stringstream ss;
    ss << obj;
    return ss.str();
  };
  return Map2Str(m, fkey, fval);
}

/**
 * @brief 判断两个vector是否相等
 *
 * @tparam T vector模板参数，需支持!=运算
 * @param[in] vec1
 * @param[in] vec2
 * @return true 相等
 * @return false 不相等
 */
template <typename T>
bool CheckVectorEqual(const std::vector<T>& vec1, const std::vector<T>& vec2) {
  if (vec1.size() != vec2.size()) return false;
  for (size_t ii = 0; ii < vec1.size(); ++ii) {
    if (vec1[ii] != vec2[ii]) return false;
  }
  return true;
}

/**
 * @brief 判断两个set是否相等
 *
 * @tparam T set模板参数，需支持!=运算
 * @param[in] set1
 * @param[in] set2
 * @return true 相等
 * @return false 不相等
 */
template <typename T>
bool CheckSetEqual(const std::set<T>& set1, const std::set<T>& set2) {
  if (set1.size() != set2.size()) return false;
  auto itr1 = set1.begin();
  auto itr2 = set2.begin();
  for (size_t ii = 0; ii < set1.size(); ++ii) {
    if (*itr1 != *itr2) return false;
    ++itr1;
    ++itr2;
  }
  return true;
}

/**
 * @brief 判断两个map是否相等
 *
 * @tparam KeyType map模板参数，需支持!=运算
 * @tparam ValType map模板参数，需支持!=运算
 * @param[in] map1
 * @param[in] map2
 * @return true 相等
 * @return false 不相等
 */
template <typename KeyType, typename ValType>
bool CheckMapEqual(const std::map<KeyType, ValType>& map1,
                   const std::map<KeyType, ValType>& map2) {
  if (map1.size() != map2.size()) return false;
  auto itr1 = map1.begin();
  auto itr2 = map2.begin();
  for (size_t ii = 0; ii < map1.size(); ++ii) {
    if ((itr1->first != itr2->first) || (itr1->second != itr2->second))
      return false;
    ++itr1;
    ++itr2;
  }
  return true;
}

}  // namespace aimrt::common::util
