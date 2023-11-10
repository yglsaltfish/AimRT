
#include <iostream>

#include "block.h"
#include "context.h"
#include "resource_manager.h"
#include "status_manager.h"

namespace aimrt::plugins::sm_plugin {

static constexpr uint32_t ConstExtraSize = 1024;
static constexpr uint32_t ConstBlockSize = sizeof(Block);
static constexpr uint32_t ConstContextSize = sizeof(Context);
static constexpr uint32_t ConstStatusManagerSize = sizeof(StatusManager);

static constexpr uint32_t ConstMsgSize2K = 1024 * 2;
static constexpr uint32_t ConstMsgSize4K = 1024 * 4;
static constexpr uint32_t ConstMsgSize8K = 1024 * 8;
static constexpr uint32_t ConstMsgSize16K = 1024 * 16;
static constexpr uint32_t ConstMsgSize128K = 1024 * 128;
static constexpr uint32_t ConstMsgSize512K = 1024 * 512;
static constexpr uint32_t ConstMsgSize1M = 1024 * 1024;
static constexpr uint32_t ConstMsgSize2M = 1024 * 1024 * 2;
static constexpr uint32_t ConstMsgSize4M = 1024 * 1024 * 4;
static constexpr uint32_t ConstMsgSize8M = 1024 * 1024 * 8;
static constexpr uint32_t ConstMsgSize16M = 1024 * 1024 * 16;
static constexpr uint32_t ConstMsgSize32M = 1024 * 1024 * 32;
static constexpr uint32_t ConstMsgSize64M = 1024 * 1024 * 64;
static constexpr uint32_t ConstMsgSize128M = 1024 * 1024 * 128;
static constexpr uint32_t ConstMsgSize256M = 1024 * 1024 * 256;

static constexpr uint32_t ConstBlockNum2k = 1024;
static constexpr uint32_t ConstBlockNum4k = 512;
static constexpr uint32_t ConstBlockNum8k = 512;
static constexpr uint32_t ConstBlockNum16k = 256;
static constexpr uint32_t ConstBlockNum128k = 256;
static constexpr uint32_t ConstBlockNum512k = 128;
static constexpr uint32_t ConstBlockNum1M = 64;
static constexpr uint32_t ConstBlockNum2M = 32;
static constexpr uint32_t ConstBlockNum4M = 32;
static constexpr uint32_t ConstBlockNum8M = 16;
static constexpr uint32_t ConstBlockNum16M = 16;
static constexpr uint32_t ConstBlockNum32M = 8;
static constexpr uint32_t ConstBlockNum64M = 8;
static constexpr uint32_t ConstBlockNum128M = 4;
static constexpr uint32_t ConstBlockNum256M = 4;

static inline uint32_t GetCeilingMessageSize(const uint32_t msg_size) {
  uint32_t ceiling_msg_size = 0;
  if (msg_size == 0) {
    ceiling_msg_size = 0;
  } else if (msg_size <= ConstMsgSize2K) {
    ceiling_msg_size = ConstMsgSize2K;
  } else if (msg_size <= ConstMsgSize4K) {
    ceiling_msg_size = ConstMsgSize4K;
  } else if (msg_size <= ConstMsgSize8K) {
    ceiling_msg_size = ConstMsgSize8K;
  } else if (msg_size <= ConstMsgSize16K) {
    ceiling_msg_size = ConstMsgSize16K;
  } else if (msg_size <= ConstMsgSize128K) {
    ceiling_msg_size = ConstMsgSize128K;
  } else if (msg_size <= ConstMsgSize512K) {
    ceiling_msg_size = ConstMsgSize512K;
  } else if (msg_size <= ConstMsgSize1M) {
    ceiling_msg_size = ConstMsgSize1M;
  } else if (msg_size <= ConstMsgSize2M) {
    ceiling_msg_size = ConstMsgSize2M;
  } else if (msg_size <= ConstMsgSize4M) {
    ceiling_msg_size = ConstMsgSize4M;
  } else if (msg_size <= ConstMsgSize8M) {
    ceiling_msg_size = ConstMsgSize8M;
  } else if (msg_size <= ConstMsgSize16M) {
    ceiling_msg_size = ConstMsgSize16M;
  } else if (msg_size <= ConstMsgSize32M) {
    ceiling_msg_size = ConstMsgSize32M;
  } else if (msg_size <= ConstMsgSize64M) {
    ceiling_msg_size = ConstMsgSize64M;
  } else if (msg_size <= ConstMsgSize128M) {
    ceiling_msg_size = ConstMsgSize128M;
  } else if (msg_size <= ConstMsgSize256M) {
    ceiling_msg_size = ConstMsgSize256M;
  } else {
    ceiling_msg_size = 0;
  }
  return ceiling_msg_size;
}

static inline uint32_t GetBlockNum(const uint32_t ceiling_msg_size) {
  uint32_t num = 0;
  switch (ceiling_msg_size) {
    case ConstMsgSize2K:
      num = ConstBlockNum2k;
      break;
    case ConstMsgSize4K:
      num = ConstBlockNum4k;
      break;
    case ConstMsgSize8K:
      num = ConstBlockNum8k;
      break;
    case ConstMsgSize16K:
      num = ConstBlockNum16k;
      break;
    case ConstMsgSize128K:
      num = ConstBlockNum128k;
      break;
    case ConstMsgSize512K:
      num = ConstBlockNum512k;
      break;
    case ConstMsgSize1M:
      num = ConstBlockNum1M;
      break;
    case ConstMsgSize2M:
      num = ConstBlockNum2M;
      break;
    case ConstMsgSize4M:
      num = ConstBlockNum4M;
      break;
    case ConstMsgSize8M:
      num = ConstBlockNum8M;
      break;
    case ConstMsgSize16M:
      num = ConstBlockNum16M;
      break;
    case ConstMsgSize32M:
      num = ConstBlockNum32M;
      break;
    case ConstMsgSize64M:
      num = ConstBlockNum64M;
      break;
    case ConstMsgSize128M:
      num = ConstBlockNum128M;
      break;
    case ConstMsgSize256M:
      num = ConstBlockNum256M;
      break;
    default:
      std::cout << "unknown ceiling message size [" << ceiling_msg_size << "]"
                << std::endl;
  }
  return num;
}

ResourceManager::ResourceManager() { UpdateMsgSize(ConstMsgSize2K); }
ResourceManager::~ResourceManager() {}

bool ResourceManager::UpdateMsgSize(const uint32_t msg_size) {
  ceiling_msg_size_ = GetCeilingMessageSize(msg_size);

  if (ceiling_msg_size_ == 0) {
    std::cout << "update message size [" << msg_size << "] failed ..." << std::endl;
    return false;
  }

  real_block_size_ = ceiling_msg_size_;
  context_size_ = ConstContextSize;
  block_count_ = GetBlockNum(ceiling_msg_size_);
  extra_size_ = ConstExtraSize;
  managed_size_ =
      extra_size_ + ConstStatusManagerSize +
      ((ConstBlockSize + context_size_ + real_block_size_) * block_count_);

  return true;
}

uint32_t ResourceManager::managed_size() const { return managed_size_; }
uint32_t ResourceManager::real_block_size() const { return real_block_size_; }
uint32_t ResourceManager::ceiling_msg_size() const { return ceiling_msg_size_; }
uint32_t ResourceManager::context_size() const { return context_size_; }
uint32_t ResourceManager::block_count() const { return block_count_; }
uint32_t ResourceManager::extra_size() const { return extra_size_; }

}  // namespace aimrt::plugins::sm_plugin