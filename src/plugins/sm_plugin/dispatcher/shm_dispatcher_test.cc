
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "../transmitter/shm_transmitter.h"
#include "shm_dispatcher.h"

using namespace aimrt::plugins::sm_plugin;

TEST(ShmDisPatcherTest, AddListenerTest) {
  ShmDisPatcher dispatcher;

  TransmitterAttribute transmitter_attribute;
  transmitter_attribute.host_id = 1;
  transmitter_attribute.channel_id = 1;
  transmitter_attribute.channel_name = "test_channel";
  transmitter_attribute.msg_type = "test_type";
  std::unique_ptr<ShmTransmitter> transmitter(new ShmTransmitter(transmitter_attribute));

  DisPatcherAttribute dispatcher_attribute;
  dispatcher_attribute.host_id = 1;
  dispatcher_attribute.channel_id = 1;
  dispatcher_attribute.channel_name = "test_channel";
  dispatcher_attribute.msg_type = "test_type";

  ListenerHandler handler = [](const void* data, size_t size, Context* context) {
    EXPECT_TRUE((memcmp(data, "Hello, world!", size) == 0));
    EXPECT_EQ(size, 13);
  };

  EXPECT_TRUE(dispatcher.AddListener(dispatcher_attribute, handler));

  // mock Notifier
  std::string data = "Hello, world!";
  size_t size = data.size();

  // Transmit the data
  bool result = transmitter->Transmit(data.c_str(), size);

  // Check that the transmission was successful
  EXPECT_TRUE(result);

  EXPECT_NO_THROW(dispatcher.Run(0));
}

TEST(ShmDisPatcherTest, RunTimeout) {
  ShmDisPatcher dispatcher;

  DisPatcherAttribute dispatcher_attribute;
  dispatcher_attribute.host_id = 1;
  dispatcher_attribute.channel_id = 1;
  dispatcher_attribute.channel_name = "test_channel";
  dispatcher_attribute.msg_type = "test_type";

  ListenerHandler handler = [](const void*, size_t, Context*) {};

  EXPECT_TRUE(dispatcher.AddListener(dispatcher_attribute, handler));

  EXPECT_NO_THROW(dispatcher.Run(100));
}

TEST(ShmDisPatcherTest, Shutdown) {
  ShmDisPatcher dispatcher;

  std::thread thread([&]() {
    EXPECT_NO_THROW(dispatcher.Run());
  });

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  EXPECT_NO_THROW(dispatcher.Shutdown());

  if (thread.joinable()) {
    thread.join();
  }
}
