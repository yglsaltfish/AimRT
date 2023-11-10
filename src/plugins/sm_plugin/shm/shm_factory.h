#pragma once

#include "shm_base.h"

namespace aimrt::plugins::sm_plugin {

class SharedMemoryFactory {
 public:
  enum class SharedMemoryType {
    XSI,     // XSI (System V)
    POSIX,   // Linux/Unix (POSIX)
    WINDOWS  // Windows
  };

  static SharedMemoryBasePtr Create(SharedMemoryType type = SharedMemoryType::POSIX);
};

}  // namespace aimrt::plugins::sm_plugin