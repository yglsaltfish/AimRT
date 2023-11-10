#pragma once

#include <cstdint>
#include <string>

namespace aimrt::plugins::sm_plugin {

class Context {
 public:
  Context();
  Context(const Context& other);
  Context(Context&& other);

  virtual ~Context();

  Context& operator=(const Context& other);
  bool operator==(const Context& other) const;
  bool operator!=(const Context& other) const;

  void SetSeq(uint64_t seq);
  void SetSenderId(uint64_t sender_id);
  void SetChannelId(uint64_t channel_id);
  void SetTimestamp(uint64_t timestamp);

  uint64_t seq() const;
  uint64_t sender_id() const;
  uint64_t channel_id() const;
  uint64_t timestamp() const;

 private:
  uint64_t seq_{0};
  uint64_t sender_id_{0};
  uint64_t channel_id_{0};
  uint64_t timestamp_{0};
};

}  // namespace aimrt::plugins::sm_plugin
