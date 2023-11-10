
#pragma once

#include <atomic>
#include <memory>
#include <string>

namespace aimrt::plugins::sm_plugin {

struct TransmitterAttribute {
  uint64_t host_id;
  uint64_t node_id;
  uint64_t channel_id;
  std::string channel_name;
  std::string msg_type;
  std::string pkg_path;
  std::string module_name;
};

class TransmitterBase {
 public:
  TransmitterBase(const TransmitterAttribute& attribute) : attribute_(attribute) {}
  virtual ~TransmitterBase() = default;

  uint64_t NextSeq() { return ++seq_; }
  uint64_t seq() const { return seq_; }

  virtual bool Transmit(const void* data, size_t size) = 0;

 protected:
  std::atomic<uint64_t> seq_{0};
  TransmitterAttribute attribute_;
};

using TransmitterBasePtr = std::shared_ptr<TransmitterBase>;

}  // namespace aimrt::plugins::sm_plugin