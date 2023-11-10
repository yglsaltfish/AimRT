
#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <vector>

#include "shm_transmitter.h"

using namespace aimrt::plugins::sm_plugin;

TEST(ShmTransmitterTest, Transmit) {
  // Create a ShmTransmitter object
  TransmitterAttribute attribute;
  attribute.host_id = 1;
  attribute.channel_id = 1;
  attribute.channel_name = "test_channel";
  attribute.msg_type = "test_type";
  std::unique_ptr<ShmTransmitter> transmitter(new ShmTransmitter(attribute));

  // Define the data to transmit
  std::string data = "Hello, world!";
  size_t size = data.size();

  // Transmit the data
  bool result = transmitter->Transmit(data.c_str(), size);

  // Check that the transmission was successful
  EXPECT_TRUE(result);
}

TEST(ShmTransmitterTest, TransmitEmptyData) {
  // Create a ShmTransmitter object
  TransmitterAttribute attribute;
  attribute.host_id = 1;
  attribute.channel_id = 1;
  attribute.channel_name = "test_channel";
  attribute.msg_type = "test_type";
  std::unique_ptr<ShmTransmitter> transmitter(new ShmTransmitter(attribute));

  // Define the data to transmit
  std::string data = "";
  size_t size = data.size();

  // Transmit the data
  bool result = transmitter->Transmit(data.c_str(), size);

  // Check that the transmission was not successful
  EXPECT_FALSE(result);
}

TEST(ShmTransmitterTest, TransmitLargeData) {
  // Create a ShmTransmitter object
  TransmitterAttribute attribute;
  attribute.host_id = 1;
  attribute.channel_id = 1;
  attribute.channel_name = "test_channel";
  attribute.msg_type = "test_type";
  std::unique_ptr<ShmTransmitter> transmitter(new ShmTransmitter(attribute));

  // Define the data to transmit
  std::vector<char> data(1024 * 1024, 'a');
  size_t size = data.size();

  // Transmit the data
  bool result = transmitter->Transmit(data.data(), size);

  // Check that the transmission was not successful
  EXPECT_TRUE(result);
}

TEST(ShmTransmitterTest, TransmitUnsupportedData) {
  // Create a ShmTransmitter object
  TransmitterAttribute attribute;
  attribute.host_id = 1;
  attribute.channel_id = 1;
  attribute.channel_name = "test_channel";
  attribute.msg_type = "test_type";
  std::unique_ptr<ShmTransmitter> transmitter(new ShmTransmitter(attribute));

  // Define the data to transmit
  std::vector<char> data(1024 * 1024 * 512, 'a');
  size_t size = data.size();

  // Transmit the data
  bool result = transmitter->Transmit(data.data(), size);

  // Check that the transmission was not successful
  EXPECT_FALSE(result);
}
