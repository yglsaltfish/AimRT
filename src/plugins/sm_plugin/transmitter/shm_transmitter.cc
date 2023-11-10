

#include <string.h>
#include <iostream>

#include "shm_transmitter.h"

#include "../notifier/notifier_factory.h"

namespace aimrt::plugins::sm_plugin {

ShmTransmitter::ShmTransmitter(const TransmitterAttribute& attribute) : TransmitterBase(attribute) {
  std::string key = attribute_.channel_name + ":" + attribute_.msg_type;

  segment_ptr_ = std::make_shared<Segment>(key);

  if (!segment_ptr_) {
    std::cout << "create segment failed" << std::endl;
  }

  notifier_ptr_ = NotifierFactory::Create(NotifierFactory::NotifierType::SHM);
  if (!notifier_ptr_) {
    std::cout << "create notifier failed" << std::endl;
  }

  if (notifier_ptr_->Init(key) == false) {
    std::cout << "init notifier failed" << std::endl;
  }
}

ShmTransmitter::~ShmTransmitter() {
  if (notifier_ptr_) {
    notifier_ptr_->Shutdown();
    notifier_ptr_ = nullptr;
  }
  segment_ptr_ = nullptr;
}

bool ShmTransmitter::Transmit(const void* data, size_t size) {
  if (!notifier_ptr_ || !segment_ptr_) {
    return false;
  }

  if (data == nullptr || size == 0) {
    return false;
  }

  Notification notification;
  WritableBlock writable_block;

  try {
    if (!segment_ptr_->TryAcquireBlockForWrite(size, &writable_block)) {
      return false;
    }

    uint32_t index = writable_block.index;
    // context
    writable_block.context->SetTimestamp(std::chrono::system_clock::now().time_since_epoch().count());
    writable_block.context->SetSeq(NextSeq());
    writable_block.context->SetSenderId(attribute_.node_id);
    writable_block.context->SetChannelId(attribute_.channel_id);

    memcpy(writable_block.buf, data, size);

    writable_block.block->SetMsgSize(size);
    segment_ptr_->ReleaseWriteBlock(writable_block);

    notification.SetBlockIndex(index);
    notification.SetHostId(attribute_.host_id);
    notification.SetChannelId(attribute_.channel_id);

    return notifier_ptr_->Notify(notification);
  } catch (...) {
    segment_ptr_->ReleaseWriteBlock(writable_block);
    return false;
  }
}

}  // namespace aimrt::plugins::sm_plugin