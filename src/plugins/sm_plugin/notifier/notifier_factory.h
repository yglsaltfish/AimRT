#pragma once

#include <atomic>
#include <string_view>

#include "notifier_base.h"

namespace aimrt::plugins::sm_plugin {

class NotifierFactory {
 public:
  enum class NotifierType {
    SHM,       // shared memory
    UDP,       // udp
    TCP,       // tcp
    MULTICAST  // multicast
  };

  static NotifierBasePtr Create(NotifierType type = NotifierType::SHM);
};

}  // namespace aimrt::plugins::sm_plugin