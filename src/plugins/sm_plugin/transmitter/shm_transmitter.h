
#pragma once

#include "transmitter.h"

#include "../notifier/notifier_base.h"
#include "../segment/segment.h"

namespace aimrt::plugins::sm_plugin {

class ShmTransmitter : public TransmitterBase {
  using SegmentPtr = std::shared_ptr<Segment>;

 public:
  explicit ShmTransmitter(const TransmitterAttribute& attribute);
  ~ShmTransmitter();

  bool Transmit(const void* data, size_t size) override;

 private:
  SegmentPtr segment_ptr_{nullptr};
  NotifierBasePtr notifier_ptr_{nullptr};
};

}  // namespace aimrt::plugins::sm_plugin