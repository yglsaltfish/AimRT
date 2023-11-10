#pragma once

#include <stdexcept>
#include <string>

namespace aimrt::common::util {

class AimRTException : public std::exception {
 public:
  template <typename... Args>
    requires std::constructible_from<std::string, Args...>
  AimRTException(Args... args)
      : err_msg_(std::forward<Args>(args)...) {}

  ~AimRTException() noexcept {}

  const char* what() const noexcept { return err_msg_.c_str(); }

 private:
  std::string err_msg_;
};

}  // namespace aimrt::common::util
