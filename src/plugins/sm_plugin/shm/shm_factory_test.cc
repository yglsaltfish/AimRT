

#include "shm_factory.h"
#include "posix_shm.h"
#include "xsi_shm.h"

#include <gtest/gtest.h>

using namespace aimrt::plugins::sm_plugin;

TEST(SharedMemoryFactoryTest, CreateXsiSharedMemory) {
  SharedMemoryBasePtr shm = SharedMemoryFactory::Create(SharedMemoryFactory::SharedMemoryType::XSI);
  ASSERT_NE(shm, nullptr);
  // dynamic_pointer_cast is used to convert a shared_ptr to another shared_ptr of a different type.
  auto ptr1 = std::dynamic_pointer_cast<XsiSharedMemory>(shm);
  ASSERT_NE(ptr1, nullptr);

  auto ptr2 = std::dynamic_pointer_cast<PosixSharedMemory>(shm);
  ASSERT_EQ(ptr2, nullptr);
}

TEST(SharedMemoryFactoryTest, CreatePosixSharedMemory) {
  SharedMemoryBasePtr shm = SharedMemoryFactory::Create(SharedMemoryFactory::SharedMemoryType::POSIX);
  ASSERT_NE(shm, nullptr);
  // dynamic_pointer_cast is used to convert a shared_ptr to another shared_ptr of a different type.
  auto ptr1 = std::dynamic_pointer_cast<PosixSharedMemory>(shm);
  ASSERT_NE(ptr1, nullptr);

  auto ptr2 = std::dynamic_pointer_cast<XsiSharedMemory>(shm);
  ASSERT_EQ(ptr2, nullptr);
}

TEST(SharedMemoryFactoryTest, CreateInvalidSharedMemory) {
  SharedMemoryBasePtr shm = SharedMemoryFactory::Create(static_cast<SharedMemoryFactory::SharedMemoryType>(-1));
  EXPECT_EQ(shm, nullptr);
}
