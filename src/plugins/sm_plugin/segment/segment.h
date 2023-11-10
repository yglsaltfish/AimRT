
#pragma once

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

#include "block.h"
#include "context.h"
#include "resource_manager.h"
#include "status_manager.h"

#include "../shm/shm_base.h"

namespace aimrt::plugins::sm_plugin {

struct SegmentBlock {
  uint32_t index = 0;
  Block* block = nullptr;
  Context* context = nullptr;
  uint8_t* buf = nullptr;
};

using WritableBlock = SegmentBlock;
using ReadableBlock = SegmentBlock;

class Segment {
 public:
  explicit Segment(const std::string& name);
  virtual ~Segment();

  /**
   * @brief try to acquire a block for read
   * @param readable_block
   * @param index: the index of the block to be acquired, input parameter
   * @param block: the block to be acquired, output parameter
   * @param context: the context of the block to be acquired, output parameter
   * @param buf: the buf of the block to be acquired, output parameter
   * @return true if success, otherwise false
   */
  bool TryAcquireBlockForRead(ReadableBlock* readable_block);

  /**
   * @brief try to acquire a block for write
   * @param msg_size: the size of the message to be written
   * @param writable_block
   * @param index: the index of the block to be acquired, output parameter
   * @param block: the block to be acquired, output parameter
   * @param context: the context of the block to be acquired, output parameter
   * @param buf: the buf of the block to be acquired, output parameter
   * @return true if success, otherwise false
   */
  bool TryAcquireBlockForWrite(uint32_t msg_size,
                               WritableBlock* writable_block);

  void ReleaseReadBlock(const ReadableBlock& readable_block);
  void ReleaseWriteBlock(const WritableBlock& writable_block);

  void PrintInfo();
  std::string Info();

  // share memory integrity check
  bool IntegrityCheck();

  std::string_view name() const;

 protected:
  bool Init(bool need_create = false);
  bool InitOrCreate();
  bool InitOnly();
  bool Destroy();
  void Reset();

  // implement
  virtual void* Create(uint64_t managed_size);
  virtual void* Open();
  virtual bool Close();
  virtual bool Remove();

 private:
  bool Remap();
  bool Recreate(const uint32_t& msg_size);
  bool LegalityCheck(void* addr);
  uint32_t GetNextWritableBlockIndex();

 protected:
  bool is_init_{false};
  std::string name_;
  ResourceManager resource_manager_;

  void* managed_shm_{nullptr};
  SharedMemoryBasePtr shm_impl_{nullptr};
  StatusManager* status_manager_{nullptr};
  Block* blocks_{nullptr};
  std::mutex context_lock_;
  std::mutex block_buf_lock_;
  std::unordered_map<uint32_t, Context*> context_addrs_;
  std::unordered_map<uint32_t, uint8_t*> block_buf_addrs_;
};

}  // namespace aimrt::plugins::sm_plugin