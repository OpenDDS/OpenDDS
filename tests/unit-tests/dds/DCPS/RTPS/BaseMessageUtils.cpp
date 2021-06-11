/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <gtest/gtest.h>

#include "dds/DCPS/Service_Participant.h"

#include <dds/DCPS/DataSampleHeader.h>
#include <dds/DCPS/RepoIdBuilder.h>
#include <dds/DCPS/RepoIdConverter.h>
#include <dds/DCPS/DisjointSequence.h>
#include <dds/DCPS/Serializer.h>
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/RTPS/MessageTypes.h>
#include <dds/DCPS/RTPS/RtpsCoreTypeSupportImpl.h>
#include <dds/DCPS/transport/rtps_udp/RtpsSampleHeader.h>
#include <dds/DCPS/RTPS/BaseMessageUtils.h>

#include <iostream>
#include <cstring>

using namespace OpenDDS::DCPS;
using namespace OpenDDS::RTPS;

struct Fragments {
  Message_Block_Ptr head_;
  Message_Block_Ptr tail_;
};

void matches(const DataFragSubmessage& df, const DataFragSubmessage& expected)
{
  EXPECT_TRUE(df.smHeader.submessageId == expected.smHeader.submessageId);
  EXPECT_TRUE(df.smHeader.flags == expected.smHeader.flags);
  EXPECT_TRUE(df.smHeader.submessageLength == expected.smHeader.submessageLength);
  EXPECT_TRUE(df.extraFlags == expected.extraFlags);
  EXPECT_TRUE(df.octetsToInlineQos == expected.octetsToInlineQos);
  EXPECT_TRUE(df.readerId.entityKey[0] == expected.readerId.entityKey[0]);
  EXPECT_TRUE(df.readerId.entityKey[1] == expected.readerId.entityKey[1]);
  EXPECT_TRUE(df.readerId.entityKey[2] == expected.readerId.entityKey[2]);
  EXPECT_TRUE(df.readerId.entityKind == expected.readerId.entityKind);
  EXPECT_TRUE(df.writerId.entityKey[0] == expected.writerId.entityKey[0]);
  EXPECT_TRUE(df.writerId.entityKey[1] == expected.writerId.entityKey[1]);
  EXPECT_TRUE(df.writerId.entityKey[2] == expected.writerId.entityKey[2]);
  EXPECT_TRUE(df.writerId.entityKind == expected.writerId.entityKind);
  EXPECT_TRUE(df.writerSN.high == expected.writerSN.high);
  EXPECT_TRUE(df.writerSN.low == expected.writerSN.low);
  EXPECT_TRUE(df.fragmentStartingNum.value == expected.fragmentStartingNum.value);
  EXPECT_TRUE(df.fragmentsInSubmessage == expected.fragmentsInSubmessage);
  EXPECT_TRUE(df.fragmentSize == expected.fragmentSize);
  EXPECT_TRUE(df.sampleSize == expected.sampleSize);
  EXPECT_TRUE(df.inlineQos.length() == expected.inlineQos.length());
  for (CORBA::ULong i = 0; i < df.inlineQos.length(); ++i) {
    EXPECT_TRUE(df.inlineQos[i]._d() == expected.inlineQos[i]._d());
    switch (df.inlineQos[i]._d()) {
    case PID_TOPIC_NAME:
    case PID_TYPE_NAME:
      EXPECT_TRUE(!std::strcmp(df.inlineQos[i].string_data(),
                              expected.inlineQos[i].string_data()));
      break;
    default:
      break;
    }
  }
}

TEST(BaseMessageUtils, maintest)
{
  const VendorId_t SomeoneElse = { { '\x04', '\x02' } };

  {
    Duration_t duration = { 7, 0x80000000 };
    TimeDuration result = rtps_duration_to_time_duration(duration, PROTOCOLVERSION_2_4, VENDORID_OPENDDS);
    EXPECT_TRUE(result.value().sec() == 7);
    EXPECT_TRUE(result.value().usec() == 500000);
  }
  {
    Duration_t duration = { 0, 0x10000000 };
    TimeDuration result = rtps_duration_to_time_duration(duration, PROTOCOLVERSION_2_4, VENDORID_OPENDDS);
    EXPECT_TRUE(result.value().sec() == 0);
    EXPECT_TRUE(result.value().usec() == 62500);
  }
  {
    Duration_t duration = { 0, 0x08000000 };
    TimeDuration result = rtps_duration_to_time_duration(duration, PROTOCOLVERSION_2_3, SomeoneElse);
    EXPECT_TRUE(result.value().sec() == 0);
    EXPECT_TRUE(result.value().usec() == 31250);
  }
  {
    Duration_t duration = { 0, 967000 };
    TimeDuration result = rtps_duration_to_time_duration(duration, PROTOCOLVERSION_2_3, VENDORID_OPENDDS);
    EXPECT_TRUE(result.value().sec() == 0);
    EXPECT_TRUE(result.value().usec() == 967);
  }

  Fragments before_fragmentation;
  const size_t N = 3000;

  // Create pre-fragmented headers
  {
    const InfoTimestampSubmessage ts = {
      {INFO_TS, 1, 8}, {1315413839, 822079774}
    };
    DataSubmessage ds = {
      {DATA, 7, 0}, 0, DATA_OCTETS_TO_IQOS, {{1, 2, 3}, 4},
      {{5, 6, 7}, 8}, {0, 9}, ParameterList()
    };
    ds.inlineQos.length(2);
    ds.inlineQos[0].string_data("my_topic_name");
    ds.inlineQos[0]._d(PID_TOPIC_NAME);
    ds.inlineQos[1].string_data("my_type_name");
    ds.inlineQos[1]._d(PID_TYPE_NAME);

    const Encoding encoding(Encoding::KIND_XCDR1, ENDIAN_LITTLE);
    size_t size = 0;
    serialized_size(encoding, size, ts);
    serialized_size(encoding, size, ds);
    before_fragmentation.head_ .reset(new ACE_Message_Block(size));
    Serializer ser(before_fragmentation.head_.get(), encoding);
    EXPECT_TRUE((ser << ts) && (ser << ds));
  }
  ACE_Message_Block& header_mb = *before_fragmentation.head_;
  {
    // Fragment
    ACE_Message_Block data(N);
    data.wr_ptr(N);
    header_mb.cont(&data);
    Fragments f;
    const SequenceRange sr =
      RtpsSampleHeader::split(header_mb, N / 2, f.head_, f.tail_);
    header_mb.cont(0);

    // Check results
    EXPECT_TRUE(sr.first == 1 && sr.second == 3);
    RtpsSampleHeader header1(*f.head_);
    EXPECT_TRUE(header1.valid());
    EXPECT_TRUE(header1.submessage_._d() == INFO_TS);
    header1 = *f.head_;
    EXPECT_TRUE(header1.valid());
    EXPECT_TRUE(header1.submessage_._d() == DATA_FRAG);
    {
      ParameterList plist(2);
      plist.length(2);
      plist[0].string_data("my_topic_name");
      plist[0]._d(PID_TOPIC_NAME);
      plist[1].string_data("my_type_name");
      plist[1]._d(PID_TYPE_NAME);
      const DataFragSubmessage expected = {
        {DATA_FRAG, 3, 0}, 0, DATA_FRAG_OCTETS_TO_IQOS, {{1, 2, 3}, 4},
        {{5, 6, 7}, 8}, {0, 9}, {1}, 1, 1024, N, plist };
      matches(header1.submessage_.data_frag_sm(), expected);
      EXPECT_TRUE(f.head_->cont() && f.head_->cont()->length() == 1024);
    }
    before_fragmentation.tail_.reset(f.tail_->duplicate());
    RtpsSampleHeader header2(*f.tail_);
    EXPECT_TRUE(header2.valid());
    EXPECT_TRUE(header2.submessage_._d() == INFO_TS);
    header2 = *f.tail_;
    EXPECT_TRUE(header2.valid());
    EXPECT_TRUE(header2.submessage_._d() == DATA_FRAG);
    {
      const DataFragSubmessage expected = {
        {DATA_FRAG, 1, 0}, 0, DATA_FRAG_OCTETS_TO_IQOS, {{1, 2, 3}, 4},
        {{5, 6, 7}, 8}, {0, 9}, {2}, 2, 1024, N, ParameterList() };
      matches(header2.submessage_.data_frag_sm(), expected);
      EXPECT_TRUE(f.tail_->cont() && f.tail_->cont()->length() == N - 1024);
    }

    // Fragment the resulting "tail"
    Fragments f2;
    const SequenceRange sr2 =
      RtpsSampleHeader::split(*before_fragmentation.tail_,
                              N / 2, f2.head_, f2.tail_);

    // Check results
    EXPECT_TRUE(sr2.first == 2 && sr2.second == 3);
    RtpsSampleHeader header3(*f2.head_);
    EXPECT_TRUE(header3.valid());
    EXPECT_TRUE(header3.submessage_._d() == INFO_TS);
    header3 = *f2.head_;
    EXPECT_TRUE(header3.valid());
    EXPECT_TRUE(header3.submessage_._d() == DATA_FRAG);
    {
      const DataFragSubmessage expected = {
        {DATA_FRAG, 1, 0}, 0, DATA_FRAG_OCTETS_TO_IQOS, {{1, 2, 3}, 4},
        {{5, 6, 7}, 8}, {0, 9}, {2}, 1, 1024, N, ParameterList() };
      matches(header3.submessage_.data_frag_sm(), expected);
      EXPECT_TRUE(f2.head_->cont() && f2.head_->cont()->length() == 1024);
    }
    RtpsSampleHeader header4(*f2.tail_);
    EXPECT_TRUE(header4.valid());
    EXPECT_TRUE(header4.submessage_._d() == INFO_TS);
    header4 = *f2.tail_;
    EXPECT_TRUE(header4.valid());
    EXPECT_TRUE(header4.submessage_._d() == DATA_FRAG);
    {
      const DataFragSubmessage expected = {
        {DATA_FRAG, 1, 0}, 0, DATA_FRAG_OCTETS_TO_IQOS, {{1, 2, 3}, 4},
        {{5, 6, 7}, 8}, {0, 9}, {3}, 1, 1024, N, ParameterList() };
      matches(header4.submessage_.data_frag_sm(), expected);
      EXPECT_TRUE(f2.tail_->cont() && f2.tail_->cont()->length() == N - 2 * 1024);
    }
  }


  // Test an empty bitmap sequence / zero numbits

  {
    SequenceNumberSet sns;
    sns.numBits = 0;
    sns.bitmap.length(0);
    EXPECT_TRUE(bitmapNonEmpty(sns) == false);
  }

  // Test a bitmap of one unsigned long

  // Test empty maps

  {
    SequenceNumberSet sns;
    sns.numBits = 0;
    sns.bitmap.length(1); // this is not necessarily invalid, could be left over from reducing numBits
    sns.bitmap[0] = 0xFFFFFFFF;
    EXPECT_TRUE(bitmapNonEmpty(sns) == false);
  }

  {
    SequenceNumberSet sns;
    sns.numBits = 1;
    sns.bitmap.length(1);
    sns.bitmap[0] = 0x7FFFFFFF;
    EXPECT_TRUE(bitmapNonEmpty(sns) == false);
  }

  {
    SequenceNumberSet sns;
    sns.numBits = 16;
    sns.bitmap.length(1);
    sns.bitmap[0] = 0x0000FFFF;
    EXPECT_TRUE(bitmapNonEmpty(sns) == false);
  }

  {
    SequenceNumberSet sns;
    sns.numBits = 31;
    sns.bitmap.length(1);
    sns.bitmap[0] = 0x00000001;
    EXPECT_TRUE(bitmapNonEmpty(sns) == false);
  }

  // Test non-empty maps

  {
    SequenceNumberSet sns;
    sns.numBits = 1;
    sns.bitmap.length(1);
    sns.bitmap[0] = 0x80000000;
    EXPECT_TRUE(bitmapNonEmpty(sns) == true);
  }

  {
    SequenceNumberSet sns;
    sns.numBits = 16;
    sns.bitmap.length(1);
    sns.bitmap[0] = 0x80000000;
    EXPECT_TRUE(bitmapNonEmpty(sns) == true);
  }

  {
    SequenceNumberSet sns;
    sns.numBits = 16;
    sns.bitmap.length(1);
    sns.bitmap[0] = 0x00010000;
    EXPECT_TRUE(bitmapNonEmpty(sns) == true);
  }

  {
    SequenceNumberSet sns;
    sns.numBits = 32;
    sns.bitmap.length(1);
    sns.bitmap[0] = 0x80000000;
    EXPECT_TRUE(bitmapNonEmpty(sns) == true);
  }

  {
    SequenceNumberSet sns;
    sns.numBits = 32;
    sns.bitmap.length(1);
    sns.bitmap[0] = 0x00010000;
    EXPECT_TRUE(bitmapNonEmpty(sns) == true);
  }

  {
    SequenceNumberSet sns;
    sns.numBits = 32;
    sns.bitmap.length(1);
    sns.bitmap[0] = 0x00000001;
    EXPECT_TRUE(bitmapNonEmpty(sns) == true);
  }

  // Test a bitmap of more than one unsigned long

  // Test empty maps

  {
    SequenceNumberSet sns;
    sns.numBits = 32;
    sns.bitmap.length(2); // this is not necessarily invalid, could be left over from reducing numBits
    sns.bitmap[0] = 0x0;
    sns.bitmap[1] = 0xFFFFFFFF;
    EXPECT_TRUE(bitmapNonEmpty(sns) == false);
  }

  {
    SequenceNumberSet sns;
    sns.numBits = 33;
    sns.bitmap.length(2);
    sns.bitmap[0] = 0x0;
    sns.bitmap[1] = 0x7FFFFFFF;
    EXPECT_TRUE(bitmapNonEmpty(sns) == false);
  }

  {
    SequenceNumberSet sns;
    sns.numBits = 48;
    sns.bitmap.length(2);
    sns.bitmap[0] = 0x0;
    sns.bitmap[1] = 0x0000FFFF;
    EXPECT_TRUE(bitmapNonEmpty(sns) == false);
  }

  {
    SequenceNumberSet sns;
    sns.numBits = 63;
    sns.bitmap.length(2);
    sns.bitmap[0] = 0x0;
    sns.bitmap[1] = 0x00000001;
    EXPECT_TRUE(bitmapNonEmpty(sns) == false);
  }

  // Test non-empty maps

  {
    SequenceNumberSet sns;
    sns.numBits = 32;
    sns.bitmap.length(2);
    sns.bitmap[0] = 0x80000000;
    sns.bitmap[1] = 0x0;
    EXPECT_TRUE(bitmapNonEmpty(sns) == true);
  }

  {
    SequenceNumberSet sns;
    sns.numBits = 33;
    sns.bitmap.length(2);
    sns.bitmap[0] = 0x0;
    sns.bitmap[1] = 0x80000000;
    EXPECT_TRUE(bitmapNonEmpty(sns) == true);
  }

  {
    SequenceNumberSet sns;
    sns.numBits = 48;
    sns.bitmap.length(2);
    sns.bitmap[0] = 0x0;
    sns.bitmap[1] = 0x80000000;
    EXPECT_TRUE(bitmapNonEmpty(sns) == true);
  }

  {
    SequenceNumberSet sns;
    sns.numBits = 48;
    sns.bitmap.length(2);
    sns.bitmap[0] = 0x0;
    sns.bitmap[1] = 0x00010000;
    EXPECT_TRUE(bitmapNonEmpty(sns) == true);
  }

  {
    SequenceNumberSet sns;
    sns.numBits = 64;
    sns.bitmap.length(2);
    sns.bitmap[0] = 0x0;
    sns.bitmap[1] = 0x80000000;
    EXPECT_TRUE(bitmapNonEmpty(sns) == true);
  }

  {
    SequenceNumberSet sns;
    sns.numBits = 64;
    sns.bitmap.length(2);
    sns.bitmap[0] = 0x0;
    sns.bitmap[1] = 0x00010000;
    EXPECT_TRUE(bitmapNonEmpty(sns) == true);
  }

  {
    SequenceNumberSet sns;
    sns.numBits = 64;
    sns.bitmap.length(2);
    sns.bitmap[0] = 0x0;
    sns.bitmap[1] = 0x00000001;
    EXPECT_TRUE(bitmapNonEmpty(sns) == true);
  }
}
