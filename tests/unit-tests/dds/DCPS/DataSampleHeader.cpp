#include <dds/DCPS/DataSampleHeader.h>

#include <gtest/gtest.h>

using namespace OpenDDS::DCPS;

// Test DataSampleHeader::valid_data()
TEST(dds_DCPS_DataSampleHeader, valid_data)
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

TEST(dds_DCPS_DataSampleHeader, serialize_flags)
{
  DataSampleHeader header;
  header.message_id_ = static_cast<char>(SAMPLE_DATA);
  header.submessage_id_ = static_cast<char>(SUBMESSAGE_NONE);
  header.coherent_change_ = true;
  header.historic_sample_ = true;
  header.lifespan_duration_ = true;
  header.group_coherent_ = true;
  header.sequence_repair_ = true;
  header.more_fragments_ = true;
  header.cdr_encapsulation_ = true;
  header.key_fields_only_ = true;

  ACE_Message_Block mb(DataSampleHeader::get_max_serialized_size());
  ASSERT_TRUE(mb << header);

  DataSampleHeader parsed(mb);
  EXPECT_EQ(ACE_CDR_BYTE_ORDER != 0, parsed.byte_order_);
  EXPECT_TRUE(parsed.coherent_change_);
  EXPECT_TRUE(parsed.historic_sample_);
  EXPECT_TRUE(parsed.lifespan_duration_);
  EXPECT_TRUE(parsed.group_coherent_);
  EXPECT_TRUE(parsed.sequence_repair_);
  EXPECT_TRUE(parsed.more_fragments_);
  EXPECT_TRUE(parsed.cdr_encapsulation_);
  EXPECT_TRUE(parsed.key_fields_only_);

  DataSampleHeader content_filter_header;
  content_filter_header.message_id_ = static_cast<char>(SAMPLE_DATA);
  content_filter_header.submessage_id_ = static_cast<char>(SUBMESSAGE_NONE);
  content_filter_header.content_filter_ = true;
  ACE_Message_Block content_filter_mb(DataSampleHeader::get_max_serialized_size());
  ASSERT_TRUE(content_filter_mb << content_filter_header);
  EXPECT_TRUE(DataSampleHeader::test_flag(CONTENT_FILTER_FLAG, &content_filter_mb));
}
