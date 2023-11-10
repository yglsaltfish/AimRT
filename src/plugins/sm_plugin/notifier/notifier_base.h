
#pragma once

#include <atomic>
#include <memory>
#include <string_view>

#include "notifications.h"

namespace aimrt::plugins::sm_plugin {

class NotifierBase {
 public:
  virtual ~NotifierBase() = default;

  virtual bool Init(const std::string& name) = 0;
  virtual bool Shutdown() = 0;
  virtual bool Notify(const Notification& info) = 0;

  // wait_time_ms < 0: block until notified
  virtual bool Listen(Notification* info, int wait_time_ms = -1) = 0;
};

using NotifierBasePtr = std::shared_ptr<NotifierBase>;

}  // namespace aimrt::plugins::sm_plugin