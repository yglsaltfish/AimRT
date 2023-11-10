
#pragma once

#include <functional>
#include <list>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>

#include "../segment/segment.h"
#include "dispatcher.h"

namespace aimrt::plugins::sm_plugin {

class ShmDisPatcher : public DisPatcherBase {
  using SegmentPtr = std::shared_ptr<Segment>;
  using ListenerHandlerPair = std::pair<NotifierBasePtr, ListenerHandler>;

 public:
  ShmDisPatcher();
  virtual ~ShmDisPatcher();
  bool AddListener(const DisPatcherAttribute& attribute, const ListenerHandler& handler) override;

  void Run(int wait_time_ms = -1) override;
  void Shutdown() override;

 protected:
  std::atomic<bool> running_{true};
  std::unordered_map<uint64_t, SegmentPtr> segment_map_;
  std::unordered_map<uint64_t, ListenerHandlerPair> listener_handler_map_;
};

}  // namespace aimrt::plugins::sm_plugin