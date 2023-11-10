
#include <gtest/gtest.h>

#include "posix_shm.h"

namespace aimrt::plugins::sm_plugin {

TEST(PosixSharedMemoryTest, CreateAndOpen) {
  PosixSharedMemory shm1;
  PosixSharedMemory shm2;

  // Create a shared memory segment
  void* memory1 = shm1.Create("test_shm", 1024);
  ASSERT_NE(memory1, nullptr);

  // Open the same shared memory segment
  void* memory2 = shm2.Open("test_shm");
  ASSERT_NE(memory2, nullptr);

  ASSERT_TRUE(shm2.Close());

  // Destroy the shared memory segment
  ASSERT_TRUE(shm1.Destroy());
}

TEST(PosixSharedMemoryTest, CreateExisting) {
  PosixSharedMemory shm1;
  PosixSharedMemory shm2;

  // Create a shared memory segment
  void* memory1 = shm1.Create("test_shm", 1024);
  ASSERT_NE(memory1, nullptr);

  // Try to create the same shared memory segment again, exception should be thrown
  EXPECT_THROW(shm2.Create("test_shm", 1024), std::runtime_error);

  // Destroy the shared memory segment
  ASSERT_TRUE(shm1.Destroy());
}

TEST(PosixSharedMemoryTest, CreateIllegal) {
  PosixSharedMemory shm;

  // Create a shared memory segment with illegal name
  EXPECT_THROW(shm.Create("", 1024), std::runtime_error);

  // Create a shared memory segment with illegal size
  EXPECT_THROW(shm.Create("test_shm", 0), std::runtime_error);
}

TEST(PosixSharedMemoryTest, OpenNonExisting) {
  PosixSharedMemory shm;
  // Try to open a non-existing shared memory segment
  EXPECT_THROW(shm.Open("non_existing_shm"), std::runtime_error);
}

TEST(PosixSharedMemoryTest, Close) {
  PosixSharedMemory shm;

  // Create a shared memory segment
  void* memory = shm.Create("test_shm", 1024);
  ASSERT_NE(memory, nullptr);

  // Close the shared memory segment
  ASSERT_TRUE(shm.Close());

  // Destroy the shared memory segment
  ASSERT_TRUE(shm.Destroy());
}

TEST(PosixSharedMemoryTest, Destroy) {
  PosixSharedMemory shm;

  // Create a shared memory segment
  void* memory = shm.Create("test_shm", 1024);
  ASSERT_NE(memory, nullptr);

  // Destroy the shared memory segment
  ASSERT_TRUE(shm.Destroy());

  // Try to open the destroyed shared memory segment
  EXPECT_THROW(shm.Open("test_shm"), std::runtime_error);
}

}  // namespace aimrt::plugins::sm_plugin