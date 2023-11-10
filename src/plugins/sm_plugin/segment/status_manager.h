
#pragma once

#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>

#include "block.h"

namespace aimrt::plugins::sm_plugin {

class StatusManager {
 public:
  explicit StatusManager(const uint32_t& ceiling_msg_size);
  virtual ~StatusManager();

  void DecreaseReferenceCounts();

  void IncreaseReferenceCounts();

  void SetNeedRemap(bool need);
  uint32_t FetchAddSeq(uint32_t diff);

  bool need_map() const;
  uint32_t seq() const;
  uint32_t reference_count() const;
  uint32_t ceiling_msg_size() const;

 private:
  std::atomic<bool> need_remap_{false};
  std::atomic<uint32_t> seq_{0};
  std::atomic<uint32_t> reference_count_{0};
  std::atomic<uint32_t> ceiling_msg_size_;
};

}  // namespace aimrt::plugins::sm_plugin
