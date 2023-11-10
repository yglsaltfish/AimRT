
#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

namespace aimrt::plugins::sm_plugin {

class Notification {
 public:
  Notification();
  Notification(const Notification& other);
  Notification(uint64_t host_id, uint64_t channel_id, uint32_t block_index);
  virtual ~Notification();

  Notification& operator=(const Notification& other);
  bool operator==(const Notification& other) const;
  bool operator!=(const Notification& other) const;

  operator bool() const;

  void SetHostId(uint64_t host_id);
  void SetChannelId(uint64_t channel_id);
  void SetBlockIndex(uint32_t block_index);

  bool Serialize(std::string* output) const;
  bool Deserialize(const std::string& input);
  bool Deserialize(const char* input, size_t size);

  uint64_t host_id() const;
  uint64_t channel_id() const;
  uint32_t block_index() const;

 private:
  uint64_t host_id_;
  uint64_t channel_id_;
  uint32_t block_index_;
};

}  // namespace aimrt::plugins::sm_plugin