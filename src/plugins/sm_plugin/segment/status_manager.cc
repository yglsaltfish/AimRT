

#include "status_manager.h"

namespace aimrt::plugins::sm_plugin {

StatusManager::StatusManager(const uint32_t& ceiling_msg_size)
    : ceiling_msg_size_(ceiling_msg_size) {}

StatusManager::~StatusManager() {}

void StatusManager::DecreaseReferenceCounts() {
  uint32_t current_reference_count = reference_count_.load();
  do {
    if (current_reference_count == 0) {
      return;
    }
  } while (!reference_count_.compare_exchange_weak(
      current_reference_count, current_reference_count - 1));
}

void StatusManager::IncreaseReferenceCounts() { reference_count_.fetch_add(1); }

void StatusManager::SetNeedRemap(bool need) { need_remap_.store(need); }

uint32_t StatusManager::FetchAddSeq(uint32_t diff) {
  return seq_.fetch_add(diff);
}

bool StatusManager::need_map() const { return need_remap_.load(); }

uint32_t StatusManager::seq() const { return seq_.load(); }

uint32_t StatusManager::reference_count() const {
  return reference_count_.load();
}

uint32_t StatusManager::ceiling_msg_size() const {
  return ceiling_msg_size_.load();
}

}  // namespace aimrt::plugins::sm_plugin
