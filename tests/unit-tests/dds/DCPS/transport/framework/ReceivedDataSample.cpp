/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <gtest/gtest.h>

#include "dds/DCPS/transport/framework/ReceivedDataSample.h"

#include <ace/Malloc_Allocator.h>

#include <cstring>

using namespace OpenDDS::DCPS;

TEST(dds_DCPS_transport_framework_ReceivedDataSample, defctor)
{
  ReceivedDataSample rds;
  EXPECT_FALSE(rds.has_data());
  EXPECT_EQ(0u, rds.data_length());
}

namespace {
  template<size_t N>
  void fill(char (&buffer)[N], char start = 0)
  {
    for (size_t i = 0; i < N; ++i) {
      buffer[i] = static_cast<char>(start + i);
    }
  }

  bool check(const char* data, size_t length, size_t expected)
  {
    if (length != expected) {
      return false;
    }
    for (size_t i = 0; i < length; ++i) {
      if (data[i] != static_cast<char>(i)) {
        return false;
      }
    }
    return true;
  }
}

TEST(dds_DCPS_transport_framework_ReceivedDataSample, msgblock)
{
  char buffer[32];
  fill(buffer);
  ACE_Message_Block mb(buffer, sizeof buffer);
  mb.wr_ptr(sizeof buffer);
  ReceivedDataSample rds(mb);
  EXPECT_TRUE(rds.has_data());
  EXPECT_EQ(sizeof buffer, rds.data_length());

  Message_Block_Ptr data(rds.data());
  EXPECT_TRUE(check(data->rd_ptr(), data->length(), sizeof buffer));

  ReceivedDataSample rds2(rds);
  rds2.clear();
  EXPECT_FALSE(rds2.has_data());
  EXPECT_EQ(0u, rds2.data_length());

  rds = rds2;
  EXPECT_FALSE(rds.has_data());
  EXPECT_EQ(0u, rds.data_length());
}

TEST(dds_DCPS_transport_framework_ReceivedDataSample, alloc)
{
  char buffer[32];
  fill(buffer);
  ACE_Message_Block mb(buffer, sizeof buffer);
  mb.wr_ptr(sizeof buffer);

  ReceivedDataSample rds(mb);
  EXPECT_TRUE(rds.has_data());
  EXPECT_EQ(sizeof buffer, rds.data_length());

  ACE_New_Allocator alloc;
  Message_Block_Ptr data(rds.data(&alloc));
  EXPECT_TRUE(check(data->rd_ptr(), data->length(), sizeof buffer));
  ACE_Allocator* allocators[3];
  data->access_allocators(allocators[0], allocators[1], allocators[2]);
  EXPECT_EQ(static_cast<ACE_Allocator*>(&alloc), allocators[2]);
}

TEST(dds_DCPS_transport_framework_ReceivedDataSample, cont)
{
  char buffer1[32], buffer2[64];
  fill(buffer1);
  fill(buffer2);
  ACE_Message_Block mb1(buffer1, sizeof buffer1);
  mb1.wr_ptr(sizeof buffer1);
  ACE_Message_Block mb2(buffer2, sizeof buffer2);
  mb2.wr_ptr(sizeof buffer2);
  mb1.cont(&mb2);

  ReceivedDataSample rds(mb1);
  EXPECT_TRUE(rds.has_data());
  EXPECT_EQ(sizeof buffer1 + sizeof buffer2, rds.data_length());

  Message_Block_Ptr data(rds.data());
  EXPECT_TRUE(check(data->rd_ptr(), data->length(), sizeof buffer1));
  const ACE_Message_Block* const c = data->cont();
  EXPECT_TRUE(c);
  EXPECT_TRUE(check(c->rd_ptr(), c->length(), sizeof buffer2));
}

TEST(dds_DCPS_transport_framework_ReceivedDataSample, ser_copy_peek)
{
  char buffer1[32], buffer2[64];
  fill(buffer1);
  fill(buffer2, sizeof buffer1);
  ACE_Message_Block mb1(buffer1, sizeof buffer1);
  mb1.wr_ptr(sizeof buffer1);
  ACE_Message_Block mb2(buffer2, sizeof buffer2);
  mb2.wr_ptr(sizeof buffer2);
  mb1.cont(&mb2);

  ReceivedDataSample rds(mb1);
  EXPECT_TRUE(rds.has_data());
  EXPECT_EQ(sizeof buffer1 + sizeof buffer2, rds.data_length());

  ACE_Message_Block mb_out(rds.data_length());
  Serializer ser(&mb_out, Encoding::KIND_XCDR2);
  EXPECT_TRUE(rds.write_data(ser));
  EXPECT_TRUE(check(mb_out.rd_ptr(), mb_out.length(), rds.data_length()));

  const DDS::OctetSeq seq(rds.copy_data());
  EXPECT_EQ(rds.data_length(), seq.length());
  EXPECT_EQ(0, std::memcmp(mb_out.rd_ptr(), seq.get_buffer(), seq.length()));

  EXPECT_EQ(1u, rds.peek(1));
  EXPECT_EQ(32u, rds.peek(32));
}

TEST(dds_DCPS_transport_framework_ReceivedDataSample, modifiers)
{
  char buffer1[32];
  fill(buffer1);
  ACE_Message_Block mb1(buffer1, sizeof buffer1);
  mb1.wr_ptr(sizeof buffer1);
  ReceivedDataSample rds1(mb1);

  char buffer2[32];
  fill(buffer2);
  ACE_Message_Block mb2(buffer2, sizeof buffer2);
  mb2.wr_ptr(sizeof buffer2);
  ReceivedDataSample rds2(mb2);

  ReceivedDataSample rds1a(rds1);
  ReceivedDataSample rds2a(rds2);
  rds2a.prepend(rds1a);
  EXPECT_TRUE(rds2a.has_data());
  EXPECT_EQ(sizeof buffer1 + sizeof buffer2, rds2a.data_length());
  EXPECT_FALSE(rds1a.has_data());
  {
    Message_Block_Ptr data(rds2a.data());
    EXPECT_TRUE(check(data->rd_ptr(), data->length(), sizeof buffer1));
    const ACE_Message_Block* const c = data->cont();
    EXPECT_TRUE(c);
    EXPECT_TRUE(check(c->rd_ptr(), c->length(), sizeof buffer2));
  }

  ReceivedDataSample rds1b(rds1);
  ReceivedDataSample rds2b(rds2);
  rds1b.append(rds2b);
  EXPECT_TRUE(rds1b.has_data());
  EXPECT_EQ(sizeof buffer1 + sizeof buffer2, rds1b.data_length());
  EXPECT_FALSE(rds2b.has_data());
  {
    Message_Block_Ptr data(rds1b.data());
    EXPECT_TRUE(check(data->rd_ptr(), data->length(), sizeof buffer1));
    const ACE_Message_Block* const c = data->cont();
    EXPECT_TRUE(c);
    EXPECT_TRUE(check(c->rd_ptr(), c->length(), sizeof buffer2));
  }

  rds1.append(buffer2, sizeof buffer2);
  {
    Message_Block_Ptr data(rds1.data());
    EXPECT_TRUE(check(data->rd_ptr(), data->length(), sizeof buffer1));
    const ACE_Message_Block* const c = data->cont();
    EXPECT_TRUE(c);
    EXPECT_TRUE(check(c->rd_ptr(), c->length(), sizeof buffer2));
  }

  rds2.replace(buffer1, sizeof buffer1);
  Message_Block_Ptr data(rds2.data());
  EXPECT_TRUE(check(data->rd_ptr(), data->length(), sizeof buffer1));
}
