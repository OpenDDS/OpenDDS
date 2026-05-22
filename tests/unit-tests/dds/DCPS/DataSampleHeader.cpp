#include <dds/DCPS/DataSampleHeader.h>

#include <gtest/gtest.h>

#include <string>

using namespace OpenDDS::DCPS;

namespace {

void serialized_sample(const std::string& payload, size_t& header_len, Message_Block_Ptr& mb)
{
  DataSampleHeader header;
  header.message_id_ = SAMPLE_DATA;
  header.submessage_id_ = SUBMESSAGE_NONE;
  header.message_length_ = static_cast<ACE_UINT32>(payload.size());
  header.sequence_ = SequenceNumber(7);

  mb.reset(new ACE_Message_Block(DataSampleHeader::get_max_serialized_size() + payload.size()));
  EXPECT_TRUE(*mb << header);
  header_len = mb->length();
  EXPECT_EQ(0, mb->copy(payload.data(), payload.size()));
}

std::string message_block_data(const ACE_Message_Block* mb)
{
  std::string data;
  for (const ACE_Message_Block* block = mb; block; block = block->cont()) {
    data.append(block->rd_ptr(), block->length());
  }
  return data;
}

DataSampleHeader read_header(const Message_Block_Ptr& mb)
{
  Message_Block_Ptr dup(mb->duplicate());
  return DataSampleHeader(*dup);
}

std::string payload_after_header(const Message_Block_Ptr& mb)
{
  Message_Block_Ptr dup(mb->duplicate());
  const DataSampleHeader header(*dup);
  ACE_UNUSED_ARG(header);
  return message_block_data(dup.get());
}

}

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

TEST(dds_DCPS_DataSampleHeader, split_header_and_payload_in_same_message_block)
{
  const std::string payload = "abcdefghijklmnopqrstuvwxyz";
  size_t header_len = 0;
  Message_Block_Ptr orig;
  serialized_sample(payload, header_len, orig);

  Message_Block_Ptr head;
  Message_Block_Ptr tail;
  const size_t head_payload_len = 10;
  DataSampleHeader::split(*orig, header_len + head_payload_len, head, tail);

  ASSERT_TRUE(head.get());
  ASSERT_TRUE(tail.get());

  const DataSampleHeader head_header = read_header(head);
  EXPECT_TRUE(head_header.more_fragments_);
  EXPECT_EQ(head_payload_len, head_header.message_length_);
  EXPECT_EQ(payload.substr(0, head_payload_len), payload_after_header(head));

  const DataSampleHeader tail_header = read_header(tail);
  EXPECT_FALSE(tail_header.more_fragments_);
  EXPECT_EQ(payload.size() - head_payload_len, tail_header.message_length_);
  EXPECT_EQ(payload.substr(head_payload_len), payload_after_header(tail));
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
