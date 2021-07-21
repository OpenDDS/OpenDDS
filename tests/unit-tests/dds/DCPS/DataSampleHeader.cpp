#include <dds/DCPS/DataSampleHeader.h>

#include <gtest/gtest.h>

using namespace OpenDDS::DCPS;

// Test DataSampleHeader::valid_data()
TEST(data_sample_header_test, valid_data)
{
  const size_t message_id_count = MESSAGE_ID_MAX;
  for (size_t i = 0; i < message_id_count; ++i) {
    MessageId msg_id = static_cast<MessageId>(i);
    DataSampleHeader header;
    header.message_id_ = static_cast<char>(msg_id);
    EXPECT_EQ(header.valid_data(), msg_id == SAMPLE_DATA)
      << "msg_id is " << to_string(msg_id);
  }
}
