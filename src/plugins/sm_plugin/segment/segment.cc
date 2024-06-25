
#include <memory.h>
#include <iostream>
#include <sstream>

#include "../shm/shm_factory.h"
#include "segment.h"

namespace aimrt::plugins::sm_plugin {

Segment::Segment(const std::string& name)
    : name_(name + ":segment"),
      shm_impl_(SharedMemoryFactory::Create(SharedMemoryFactory::SharedMemoryType::POSIX)) {
}

Segment::~Segment() { Destroy(); }

bool Segment::TryAcquireBlockForRead(ReadableBlock* readable_block) {
  if ((!readable_block) || ((!is_init_) && (!Init()))) {
    return false;
  }

  uint32_t index = readable_block->index;

  if (index >= resource_manager_.block_count()) {
    std::cout << "invalid index [" << index << "]" << std::endl;
    return false;
  }

  if (status_manager_->need_map()) {
    // std::cout << "remap ..." << std::endl;
    if (!Remap()) {
      std::cout << "remap failed" << std::endl;
      return false;
    }
  }

  if (!blocks_[index].TryLockForRead()) {
    std::cout << "try lock for read failed ..." << std::endl;
    return false;
  }

  readable_block->block = &blocks_[index];
  readable_block->context = context_addrs_[index];
  readable_block->buf = block_buf_addrs_[index];

  if (!LegalityCheck(readable_block->buf) || !LegalityCheck(readable_block->context)) {
    return false;
  }

  return true;
}

bool Segment::TryAcquireBlockForWrite(uint32_t msg_size,
                                      WritableBlock* writable_block) {
  if ((!writable_block) || ((!is_init_) && (!Init(true)))) {
    return false;
  }
  bool ret = true;

  if (status_manager_->need_map()) {
    // std::cout << "remap ..." << std::endl;
    if (!Remap()) {
      std::cout << "remap failed" << std::endl;
      return false;
    }
  }

  if (msg_size > resource_manager_.ceiling_msg_size()) {
    std::cout << "msg size [" << msg_size
              << "] larger than current block ceiling msg size ["
              << resource_manager_.ceiling_msg_size() << "] , need recreate ..."
              << std::endl;
    ret = Recreate(msg_size);
  }

  if (!ret) {
    std::cout << "segment update failed ..." << std::endl;
    return ret;
  }

  uint32_t index = GetNextWritableBlockIndex();

  writable_block->index = index;
  writable_block->block = &blocks_[index];
  writable_block->context = context_addrs_[index];
  writable_block->buf = block_buf_addrs_[index];

  if (!LegalityCheck(writable_block->buf) || !LegalityCheck(writable_block->context)) {
    return false;
  }

  return true;
}

void Segment::ReleaseReadBlock(const ReadableBlock& readable_block) {
  uint32_t index = readable_block.index;
  if (index >= resource_manager_.block_count()) {
    std::cout << "invalid index [" << index << "]" << std::endl;
    return;
  }

  blocks_[index].ReleaseReadLock();
}

void Segment::ReleaseWriteBlock(const WritableBlock& writable_block) {
  uint32_t index = writable_block.index;
  if (index >= resource_manager_.block_count()) {
    std::cout << "invalid index [" << index << "]" << std::endl;
    return;
  }

  blocks_[index].ReleaseWriteLock();
}

bool Segment::Remap() {
  is_init_ = false;
  Reset();
  return Init();
}

bool Segment::Recreate(const uint32_t& msg_size) {
  is_init_ = false;
  status_manager_->SetNeedRemap(true);
  Reset();
  Remove();
  if (resource_manager_.UpdateMsgSize(msg_size) == false) {
    return false;
  }
  return Init(true);
}

bool Segment::LegalityCheck(void* addr) {
  if (!addr) {
    return false;
  }

  // the addr should be between managed_shm_ and managed_shm_ + managed_size_
  if ((addr < managed_shm_) ||
      (addr > static_cast<uint8_t*>(managed_shm_) +
                  resource_manager_.managed_size())) {
    std::cout << "addr [" << addr << "] is not in the range of managed shm ["
              << managed_shm_ << ", "
              << static_cast<uint8_t*>(managed_shm_) +
                     resource_manager_.managed_size()
              << "]" << std::endl;
    return false;
  }

  return true;
}

uint32_t Segment::GetNextWritableBlockIndex() {
  const uint32_t block_count = resource_manager_.block_count();
  while (1) {
    uint32_t index = status_manager_->FetchAddSeq(1) % block_count;
    if (blocks_[index].TryLockForWrite()) {
      return index;
    }
  }
  return 0;
}

bool Segment::Init(bool need_create) {
  if (is_init_) return true;

  bool init_result = false;
  init_result = InitOnly();

  if (need_create && init_result == false) {
    init_result = InitOrCreate();
  }

  init_result &= IntegrityCheck();
  is_init_ = init_result;
  return init_result;
}

bool Segment::InitOrCreate() {
  if (is_init_) return true;

  managed_shm_ = Create(resource_manager_.managed_size());

  if (!managed_shm_) {
    std::cout << "open or create managed shm failed ..." << std::endl;
    return false;
  }

  // init status manager
  status_manager_ = new (managed_shm_) StatusManager(resource_manager_.ceiling_msg_size());

  if (!status_manager_) {
    std::cout << "init status manager failed ..." << std::endl;
    Remove();
    managed_shm_ = nullptr;
    return false;
  }

  if (resource_manager_.UpdateMsgSize(status_manager_->ceiling_msg_size()) == false) {
    return false;
  }

  // init blocks
  blocks_ = new (static_cast<uint8_t*>(managed_shm_) + sizeof(StatusManager))
      Block[resource_manager_.block_count()];
  if (!blocks_) {
    std::cout << "init blocks failed ..." << std::endl;
    status_manager_->~StatusManager();
    status_manager_ = nullptr;
    Remove();
    managed_shm_ = nullptr;
    return false;
  }

  // init context and block buf
  for (uint32_t i = 0; i < resource_manager_.block_count(); ++i) {
    uint8_t* addr = static_cast<uint8_t*>(managed_shm_) +
                    sizeof(StatusManager) +
                    resource_manager_.block_count() * sizeof(Block) +
                    i * (sizeof(Context) + resource_manager_.real_block_size());
    // init context
    {
      std::lock_guard<std::mutex> lock(context_lock_);
      context_addrs_[i] = new (addr) Context();
    }

    // init block buf
    {
      std::lock_guard<std::mutex> lock(block_buf_lock_);
      block_buf_addrs_[i] = new (addr + sizeof(Context))
          uint8_t[resource_manager_.real_block_size()];
    }
  }

  // check init result
  if ((context_addrs_.size() != resource_manager_.block_count()) ||
      (block_buf_addrs_.size() != resource_manager_.block_count())) {
    std::cout << "init context or block buf failed ..." << std::endl;
    status_manager_->~StatusManager();
    status_manager_ = nullptr;
    blocks_ = nullptr;
    Remove();
    managed_shm_ = nullptr;
    return false;
  }

  // init extra memory
  if (resource_manager_.extra_size() > 0) {
    // get used memory size
    uint64_t used_size =
        sizeof(StatusManager) +
        resource_manager_.block_count() * sizeof(Block) +
        resource_manager_.block_count() * sizeof(Context) +
        resource_manager_.block_count() * resource_manager_.real_block_size();

    // get extra memory size
    uint64_t extra_size = resource_manager_.managed_size() - used_size;

    if (extra_size != resource_manager_.extra_size()) {
      std::cout << "extra size not match ..." << std::endl;
      status_manager_->~StatusManager();
      status_manager_ = nullptr;
      blocks_ = nullptr;
      Remove();
      managed_shm_ = nullptr;
      return false;
    }

    // init extra memory
    memset(static_cast<uint8_t*>(managed_shm_) + used_size, 'Z',
           resource_manager_.extra_size());
  }

  status_manager_->IncreaseReferenceCounts();
  is_init_ = true;
  return true;
}

bool Segment::InitOnly() {
  if (is_init_) return true;

  managed_shm_ = Open();

  if (!managed_shm_) {
    // std::cout << "Segment::InitOnly() open managed shm failed ..." << std::endl;
    return false;
  }

  // get status manager
  status_manager_ = reinterpret_cast<StatusManager*>(managed_shm_);
  if (!status_manager_) {
    std::cout << "get status manager failed ..." << std::endl;
    Close();
    managed_shm_ = nullptr;
    return false;
  }

  if (resource_manager_.UpdateMsgSize(status_manager_->ceiling_msg_size()) == false) {
    return false;
  }

  // get blocks
  blocks_ = reinterpret_cast<Block*>(static_cast<uint8_t*>(managed_shm_) +
                                     sizeof(StatusManager));
  if (!blocks_) {
    std::cout << "get blocks failed ..." << std::endl;
    status_manager_ = nullptr;
    Close();
    managed_shm_ = nullptr;
    return false;
  }

  // get context and block buf
  for (uint32_t i = 0; i < resource_manager_.block_count(); ++i) {
    uint8_t* addr = static_cast<uint8_t*>(managed_shm_) +
                    sizeof(StatusManager) +
                    resource_manager_.block_count() * sizeof(Block) +
                    i * (sizeof(Context) + resource_manager_.real_block_size());
    // get context
    {
      std::lock_guard<std::mutex> lock(context_lock_);
      context_addrs_[i] = reinterpret_cast<Context*>(addr);
    }

    // get block buf
    {
      std::lock_guard<std::mutex> lock(block_buf_lock_);
      block_buf_addrs_[i] = addr + sizeof(Context);
    }
  }

  // check context and block buf
  if ((context_addrs_.size() != resource_manager_.block_count()) ||
      (block_buf_addrs_.size() != resource_manager_.block_count())) {
    std::cout << "get context or block buf failed ..." << std::endl;
    status_manager_ = nullptr;
    blocks_ = nullptr;
    Close();
    managed_shm_ = nullptr;
    return false;
  }

  status_manager_->IncreaseReferenceCounts();
  is_init_ = true;
  return true;
}

bool Segment::Destroy() {
  if (!is_init_) {
    return true;
  }
  is_init_ = false;

  try {
    status_manager_->DecreaseReferenceCounts();
    if (status_manager_->reference_count() == 0) {
      return Remove();
    }
  } catch (...) {
    return false;
  }

  return true;
}

void Segment::Reset() {
  status_manager_ = nullptr;
  blocks_ = nullptr;

  {
    std::lock_guard<std::mutex> lock(context_lock_);
    context_addrs_.clear();
  }

  {
    std::lock_guard<std::mutex> lock(block_buf_lock_);
    block_buf_addrs_.clear();
  }

  Close();
}

std::string_view Segment::name() const { return name_; }

void* Segment::Create(uint64_t managed_size) {
  try {
    return shm_impl_->Create(name_, managed_size);
  } catch (...) {
    std::cout << "create managed shm failed ..." << std::endl;
  }
  return nullptr;
}

void* Segment::Open() {
  try {
    return shm_impl_->Open(name_);
  } catch (const std::exception& e) {
    (void)e;
    // std::cout << "Segment::Open() open managed shm failed, error : " << e.what() << std::endl;
  }
  return nullptr;
}

bool Segment::Close() {
  try {
    return shm_impl_->Close();
  } catch (...) {
    std::cout << "close managed shm failed ..." << std::endl;
  }
  return false;
}

bool Segment::Remove() {
  try {
    return shm_impl_->Destroy();
  } catch (...) {
    std::cout << "remove managed shm failed ..." << std::endl;
  }
  return false;
}

// get segment info
std::string Segment::Info() {
  std::stringstream ss;
  ss << "segment info : " << std::endl;
  ss << "  name: " << name_ << std::endl;
  ss << "  address : " << managed_shm_ << std::endl;
  ss << "  managed size : " << resource_manager_.managed_size() << " B, "
     << resource_manager_.managed_size() / 1024.0 << " KB, "
     << resource_manager_.managed_size() / 1024.0 / 1024.0 << " MB"
     << std::endl;
  ss << "  block count : " << resource_manager_.block_count() << std::endl;
  ss << "  ceiling msg size : " << resource_manager_.ceiling_msg_size()
     << " B, " << resource_manager_.ceiling_msg_size() / 1024.0 << " KB, "
     << resource_manager_.ceiling_msg_size() / 1024.0 / 1024.0 << " MB"
     << std::endl;
  ss << "  context size : " << resource_manager_.context_size() << " B"
     << std::endl;
  ss << "  real block size : " << resource_manager_.real_block_size() << " B, "
     << resource_manager_.real_block_size() / 1024.0 << " KB, "
     << resource_manager_.real_block_size() / 1024.0 / 1024.0 << " MB"
     << std::endl;
  ss << "  status manager address : " << status_manager_ << std::endl;
  for (uint32_t i = 0; i < resource_manager_.block_count(); ++i) {
    ss << "  index [" << i << "] :" << std::endl;
    ss << "    block address : " << &blocks_[i] << std::endl;
    ss << "    block msg size : " << blocks_[i].msg_size() << std::endl;
    ss << "    context address : " << context_addrs_[i] << std::endl;
    ss << "    block buf address : " << (void*)block_buf_addrs_[i] << std::endl;
  }
  ss << "  extra size : " << resource_manager_.extra_size() << " B"
     << std::endl;
  ss << "intergrity check : " << (IntegrityCheck() ? "true" : "false")
     << std::endl;

  return ss.str();
}

void Segment::PrintInfo() { std::cout << Info() << std::endl; }

bool Segment::IntegrityCheck() {
  // check managed shm
  if (!managed_shm_) {
    std::cout << "managed shm is nullptr ..." << std::endl;
    return false;
  }

  // check status manager
  if (!status_manager_) {
    std::cout << "status manager is nullptr ..." << std::endl;
    return false;
  }

  // check blocks
  if (!blocks_) {
    std::cout << "blocks is nullptr ..." << std::endl;
    return false;
  }

  // check context and block buf
  if ((context_addrs_.size() != resource_manager_.block_count()) ||
      (block_buf_addrs_.size() != resource_manager_.block_count())) {
    std::cout << "context or block buf is nullptr ..." << std::endl;
    return false;
  }

  // check memory address
  for (uint32_t i = 0; i < resource_manager_.block_count(); ++i) {
    if (context_addrs_[i] !=
        reinterpret_cast<Context*>(
            static_cast<uint8_t*>(managed_shm_) + sizeof(StatusManager) +
            resource_manager_.block_count() * sizeof(Block) +
            i * (sizeof(Context) + resource_manager_.real_block_size()))) {
      std::cout << "context[" << i << "] address not match ..." << std::endl;
      return false;
    }

    if (block_buf_addrs_[i] !=
        (static_cast<uint8_t*>(managed_shm_) + sizeof(StatusManager) +
         resource_manager_.block_count() * sizeof(Block) +
         i * (sizeof(Context) + resource_manager_.real_block_size()) +
         sizeof(Context))) {
      std::cout << "block buf[" << i << "] address not match ..." << std::endl;
      return false;
    }
  }

  // check extra memory
  if (resource_manager_.extra_size() > 0) {
    // get used memory size
    uint64_t used_size =
        sizeof(StatusManager) +
        resource_manager_.block_count() * sizeof(Block) +
        resource_manager_.block_count() * sizeof(Context) +
        resource_manager_.block_count() * resource_manager_.real_block_size();

    // get extra memory size
    uint64_t extra_size = resource_manager_.managed_size() - used_size;

    if (extra_size != resource_manager_.extra_size()) {
      std::cout << "extra size not match ..." << std::endl;
      return false;
    }

    // check extra memory
    for (uint64_t i = 0; i < extra_size; ++i) {
      if (*(static_cast<uint8_t*>(managed_shm_) + used_size + i) != 'Z') {
        std::cout << "extra memory check failed ..." << std::endl;
        return false;
      }
    }
  }

  return true;
}

}  // namespace aimrt::plugins::sm_plugin