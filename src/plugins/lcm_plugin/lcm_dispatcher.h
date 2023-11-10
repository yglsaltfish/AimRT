
#pragma once

#include <atomic>
#include <memory>
#include <unordered_map>

#include "lcm_manager.h"

namespace aimrt::plugins::lcm_plugin {

using ListenerHandler = std::function<void(const void *, size_t)>;

struct DisPatcherAttribute {
  std::string url;
  std::string channel_name;  // hash<url::topic_name::msg_type>
};

class LcmDispatcher {
 public:
  explicit LcmDispatcher();
  ~LcmDispatcher();
  bool AddListener(const DisPatcherAttribute &attribute, const ListenerHandler &handler);

  void Run(int wait_time_ms = -1);
  void Shutdown();

 private:
  void HandleMessage(const lcm::ReceiveBuffer *rbuf, const std::string &channel);

 private:
  std::atomic<bool> running_{true};
  std::unordered_map<std::string, LcmPtr> lcm_map_;               // url:lcm
  std::unordered_map<std::string, ListenerHandler> handler_map_;  // hash<url::channel>:handler
};

using LcmDispatcherPtr = std::shared_ptr<LcmDispatcher>;

}  // namespace aimrt::plugins::lcm_plugin
