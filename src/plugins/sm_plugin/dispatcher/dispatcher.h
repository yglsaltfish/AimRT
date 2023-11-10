
#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <string>

#include "../notifier/notifier_base.h"
#include "../segment/context.h"

namespace aimrt::plugins::sm_plugin {

struct DisPatcherAttribute {
  uint64_t host_id;
  uint64_t node_id;
  uint64_t channel_id;
  std::string channel_name;
  std::string msg_type;
  std::string pkg_path;
  std::string module_name;
};

using ListenerHandler = std::function<void(const void*, size_t, Context*)>;

class DisPatcherBase {
 public:
  virtual bool AddListener(const DisPatcherAttribute& attribute, const ListenerHandler& handler) = 0;

  virtual void Run(int wait_time_ms = -1) = 0;
  virtual void Shutdown() = 0;
};

using DisPatcherBasePtr = std::shared_ptr<DisPatcherBase>;

}  // namespace aimrt::plugins::sm_plugin