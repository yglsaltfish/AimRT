
#include "notifications.h"

namespace aimrt::plugins::sm_plugin {

Notification::Notification() : host_id_(0), channel_id_(0), block_index_(0) {}

Notification::Notification(const Notification& other) {
  host_id_ = other.host_id_;
  channel_id_ = other.channel_id_;
  block_index_ = other.block_index_;
}

Notification::Notification(uint64_t host_id, uint64_t channel_id, uint32_t block_index) {
  host_id_ = host_id;
  channel_id_ = channel_id;
  block_index_ = block_index;
}

Notification::~Notification() {}

Notification& Notification::operator=(const Notification& other) {
  host_id_ = other.host_id_;
  channel_id_ = other.channel_id_;
  block_index_ = other.block_index_;
  return *this;
}

bool Notification::operator==(const Notification& other) const {
  return host_id_ == other.host_id_ && channel_id_ == other.channel_id_ && block_index_ == other.block_index_;
}

bool Notification::operator!=(const Notification& other) const { return !(*this == other); }

Notification::operator bool() const { return host_id_ != 0 && channel_id_ != 0; }

void Notification::SetHostId(uint64_t host_id) { host_id_ = host_id; }

void Notification::SetChannelId(uint64_t channel_id) { channel_id_ = channel_id; }

void Notification::SetBlockIndex(uint32_t block_index) { block_index_ = block_index; }

bool Notification::Serialize(std::string* output) const {
  if (output == nullptr) {
    return false;
  }

  output->clear();
  output->append(reinterpret_cast<const char*>(&host_id_), sizeof(host_id_));
  output->append(reinterpret_cast<const char*>(&channel_id_), sizeof(channel_id_));
  output->append(reinterpret_cast<const char*>(&block_index_), sizeof(block_index_));
  return true;
}

bool Notification::Deserialize(const std::string& input) { return Deserialize(input.data(), input.size()); }

bool Notification::Deserialize(const char* input, size_t size) {
  if (input == nullptr || size != sizeof(host_id_) + sizeof(channel_id_) + sizeof(block_index_)) {
    return false;
  }

  host_id_ = *reinterpret_cast<const uint64_t*>(input);
  channel_id_ = *reinterpret_cast<const uint64_t*>(input + sizeof(host_id_));
  block_index_ = *reinterpret_cast<const uint32_t*>(input + sizeof(host_id_) + sizeof(channel_id_));
  return true;
}

uint64_t Notification::host_id() const { return host_id_; }

uint64_t Notification::channel_id() const { return channel_id_; }

uint32_t Notification::block_index() const { return block_index_; }

}  // namespace aimrt::plugins::sm_plugin