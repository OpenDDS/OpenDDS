/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <ace/OS_main.h>

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

#include "../../../../DCPS/common/TestSupport.h"

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
  TEST_CHECK(df.smHeader.submessageId == expected.smHeader.submessageId);
  TEST_CHECK(df.smHeader.flags == expected.smHeader.flags);
  TEST_CHECK(df.smHeader.submessageLength == expected.smHeader.submessageLength);
  TEST_CHECK(df.extraFlags == expected.extraFlags);
  TEST_CHECK(df.octetsToInlineQos == expected.octetsToInlineQos);
  TEST_CHECK(df.readerId.entityKey[0] == expected.readerId.entityKey[0]);
  TEST_CHECK(df.readerId.entityKey[1] == expected.readerId.entityKey[1]);
  TEST_CHECK(df.readerId.entityKey[2] == expected.readerId.entityKey[2]);
  TEST_CHECK(df.readerId.entityKind == expected.readerId.entityKind);
  TEST_CHECK(df.writerId.entityKey[0] == expected.writerId.entityKey[0]);
  TEST_CHECK(df.writerId.entityKey[1] == expected.writerId.entityKey[1]);
  TEST_CHECK(df.writerId.entityKey[2] == expected.writerId.entityKey[2]);
  TEST_CHECK(df.writerId.entityKind == expected.writerId.entityKind);
  TEST_CHECK(df.writerSN.high == expected.writerSN.high);
  TEST_CHECK(df.writerSN.low == expected.writerSN.low);
  TEST_CHECK(df.fragmentStartingNum.value == expected.fragmentStartingNum.value);
  TEST_CHECK(df.fragmentsInSubmessage == expected.fragmentsInSubmessage);
  TEST_CHECK(df.fragmentSize == expected.fragmentSize);
  TEST_CHECK(df.sampleSize == expected.sampleSize);
  TEST_CHECK(df.inlineQos.length() == expected.inlineQos.length());
  for (CORBA::ULong i = 0; i < df.inlineQos.length(); ++i) {
    TEST_CHECK(df.inlineQos[i]._d() == expected.inlineQos[i]._d());
    switch (df.inlineQos[i]._d()) {
    case PID_TOPIC_NAME:
    case PID_TYPE_NAME:
      TEST_CHECK(!std::strcmp(df.inlineQos[i].string_data(),
                              expected.inlineQos[i].string_data()));
      break;
    default:
      break;
    }
  }
}

int ACE_TMAIN(int, ACE_TCHAR*[])
{
  const VendorId_t SomeoneElse = { { '\x04', '\x02' } };

  try
  {
    {
      Duration_t duration = { 7, 0x80000000 };
      TimeDuration result = rtps_duration_to_time_duration(duration, PROTOCOLVERSION_2_4, VENDORID_OPENDDS);
      TEST_CHECK(result.value().sec() == 7);
      TEST_CHECK(result.value().usec() == 500000);
    }
    {
      Duration_t duration = { 0, 0x10000000 };
      TimeDuration result = rtps_duration_to_time_duration(duration, PROTOCOLVERSION_2_4, VENDORID_OPENDDS);
      TEST_CHECK(result.value().sec() == 0);
      TEST_CHECK(result.value().usec() == 62500);
    }
    {
      Duration_t duration = { 0, 0x08000000 };
      TimeDuration result = rtps_duration_to_time_duration(duration, PROTOCOLVERSION_2_3, SomeoneElse);
      TEST_CHECK(result.value().sec() == 0);
      TEST_CHECK(result.value().usec() == 31250);
    }
    {
      Duration_t duration = { 0, 967000 };
      TimeDuration result = rtps_duration_to_time_duration(duration, PROTOCOLVERSION_2_3, VENDORID_OPENDDS);
      TEST_CHECK(result.value().sec() == 0);
      TEST_CHECK(result.value().usec() == 967);
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
      TEST_CHECK((ser << ts) && (ser << ds));
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
      TEST_CHECK(sr.first == 1 && sr.second == 3);
      RtpsSampleHeader header1(*f.head_);
      TEST_CHECK(header1.valid());
      TEST_CHECK(header1.submessage_._d() == INFO_TS);
      header1 = *f.head_;
      TEST_CHECK(header1.valid());
      TEST_CHECK(header1.submessage_._d() == DATA_FRAG);
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
        TEST_CHECK(f.head_->cont() && f.head_->cont()->length() == 1024);
      }
      before_fragmentation.tail_.reset(f.tail_->duplicate());
      RtpsSampleHeader header2(*f.tail_);
      TEST_CHECK(header2.valid());
      TEST_CHECK(header2.submessage_._d() == INFO_TS);
      header2 = *f.tail_;
      TEST_CHECK(header2.valid());
      TEST_CHECK(header2.submessage_._d() == DATA_FRAG);
      {
        const DataFragSubmessage expected = {
          {DATA_FRAG, 1, 0}, 0, DATA_FRAG_OCTETS_TO_IQOS, {{1, 2, 3}, 4},
          {{5, 6, 7}, 8}, {0, 9}, {2}, 2, 1024, N, ParameterList() };
        matches(header2.submessage_.data_frag_sm(), expected);
        TEST_CHECK(f.tail_->cont() && f.tail_->cont()->length() == N - 1024);
      }

      // Fragment the resulting "tail"
      Fragments f2;
      const SequenceRange sr2 =
        RtpsSampleHeader::split(*before_fragmentation.tail_,
                                N / 2, f2.head_, f2.tail_);

      // Check results
      TEST_CHECK(sr2.first == 2 && sr2.second == 3);
      RtpsSampleHeader header3(*f2.head_);
      TEST_CHECK(header3.valid());
      TEST_CHECK(header3.submessage_._d() == INFO_TS);
      header3 = *f2.head_;
      TEST_CHECK(header3.valid());
      TEST_CHECK(header3.submessage_._d() == DATA_FRAG);
      {
        const DataFragSubmessage expected = {
          {DATA_FRAG, 1, 0}, 0, DATA_FRAG_OCTETS_TO_IQOS, {{1, 2, 3}, 4},
          {{5, 6, 7}, 8}, {0, 9}, {2}, 1, 1024, N, ParameterList() };
        matches(header3.submessage_.data_frag_sm(), expected);
        TEST_CHECK(f2.head_->cont() && f2.head_->cont()->length() == 1024);
      }
      RtpsSampleHeader header4(*f2.tail_);
      TEST_CHECK(header4.valid());
      TEST_CHECK(header4.submessage_._d() == INFO_TS);
      header4 = *f2.tail_;
      TEST_CHECK(header4.valid());
      TEST_CHECK(header4.submessage_._d() == DATA_FRAG);
      {
        const DataFragSubmessage expected = {
          {DATA_FRAG, 1, 0}, 0, DATA_FRAG_OCTETS_TO_IQOS, {{1, 2, 3}, 4},
          {{5, 6, 7}, 8}, {0, 9}, {3}, 1, 1024, N, ParameterList() };
        matches(header4.submessage_.data_frag_sm(), expected);
        TEST_CHECK(f2.tail_->cont() && f2.tail_->cont()->length() == N - 2 * 1024);
      }
    }


    // Test an empty bitmap sequence / zero numbits

    {
      SequenceNumberSet sns;
      sns.numBits = 0;
      sns.bitmap.length(0);
      TEST_CHECK(bitmapNonEmpty(sns) == false);
    }

    // Test a bitmap of one unsigned long

    // Test empty maps

    {
      SequenceNumberSet sns;
      sns.numBits = 0;
      sns.bitmap.length(1); // this is not necessarily invalid, could be left over from reducing numBits
      sns.bitmap[0] = 0xFFFFFFFF;
      TEST_CHECK(bitmapNonEmpty(sns) == false);
    }

    {
      SequenceNumberSet sns;
      sns.numBits = 1;
      sns.bitmap.length(1);
      sns.bitmap[0] = 0x7FFFFFFF;
      TEST_CHECK(bitmapNonEmpty(sns) == false);
    }

    {
      SequenceNumberSet sns;
      sns.numBits = 16;
      sns.bitmap.length(1);
      sns.bitmap[0] = 0x0000FFFF;
      TEST_CHECK(bitmapNonEmpty(sns) == false);
    }

    {
      SequenceNumberSet sns;
      sns.numBits = 31;
      sns.bitmap.length(1);
      sns.bitmap[0] = 0x00000001;
      TEST_CHECK(bitmapNonEmpty(sns) == false);
    }

    // Test non-empty maps

    {
      SequenceNumberSet sns;
      sns.numBits = 1;
      sns.bitmap.length(1);
      sns.bitmap[0] = 0x80000000;
      TEST_CHECK(bitmapNonEmpty(sns) == true);
    }

    {
      SequenceNumberSet sns;
      sns.numBits = 16;
      sns.bitmap.length(1);
      sns.bitmap[0] = 0x80000000;
      TEST_CHECK(bitmapNonEmpty(sns) == true);
    }

    {
      SequenceNumberSet sns;
      sns.numBits = 16;
      sns.bitmap.length(1);
      sns.bitmap[0] = 0x00010000;
      TEST_CHECK(bitmapNonEmpty(sns) == true);
    }

    {
      SequenceNumberSet sns;
      sns.numBits = 32;
      sns.bitmap.length(1);
      sns.bitmap[0] = 0x80000000;
      TEST_CHECK(bitmapNonEmpty(sns) == true);
    }

    {
      SequenceNumberSet sns;
      sns.numBits = 32;
      sns.bitmap.length(1);
      sns.bitmap[0] = 0x00010000;
      TEST_CHECK(bitmapNonEmpty(sns) == true);
    }

    {
      SequenceNumberSet sns;
      sns.numBits = 32;
      sns.bitmap.length(1);
      sns.bitmap[0] = 0x00000001;
      TEST_CHECK(bitmapNonEmpty(sns) == true);
    }

    // Test a bitmap of more than one unsigned long

    // Test empty maps

    {
      SequenceNumberSet sns;
      sns.numBits = 32;
      sns.bitmap.length(2); // this is not necessarily invalid, could be left over from reducing numBits
      sns.bitmap[0] = 0x0;
      sns.bitmap[1] = 0xFFFFFFFF;
      TEST_CHECK(bitmapNonEmpty(sns) == false);
    }

    {
      SequenceNumberSet sns;
      sns.numBits = 33;
      sns.bitmap.length(2);
      sns.bitmap[0] = 0x0;
      sns.bitmap[1] = 0x7FFFFFFF;
      TEST_CHECK(bitmapNonEmpty(sns) == false);
    }

    {
      SequenceNumberSet sns;
      sns.numBits = 48;
      sns.bitmap.length(2);
      sns.bitmap[0] = 0x0;
      sns.bitmap[1] = 0x0000FFFF;
      TEST_CHECK(bitmapNonEmpty(sns) == false);
    }

    {
      SequenceNumberSet sns;
      sns.numBits = 63;
      sns.bitmap.length(2);
      sns.bitmap[0] = 0x0;
      sns.bitmap[1] = 0x00000001;
      TEST_CHECK(bitmapNonEmpty(sns) == false);
    }

    // Test non-empty maps

    {
      SequenceNumberSet sns;
      sns.numBits = 32;
      sns.bitmap.length(2);
      sns.bitmap[0] = 0x80000000;
      sns.bitmap[1] = 0x0;
      TEST_CHECK(bitmapNonEmpty(sns) == true);
    }

    {
      SequenceNumberSet sns;
      sns.numBits = 33;
      sns.bitmap.length(2);
      sns.bitmap[0] = 0x0;
      sns.bitmap[1] = 0x80000000;
      TEST_CHECK(bitmapNonEmpty(sns) == true);
    }

    {
      SequenceNumberSet sns;
      sns.numBits = 48;
      sns.bitmap.length(2);
      sns.bitmap[0] = 0x0;
      sns.bitmap[1] = 0x80000000;
      TEST_CHECK(bitmapNonEmpty(sns) == true);
    }

    {
      SequenceNumberSet sns;
      sns.numBits = 48;
      sns.bitmap.length(2);
      sns.bitmap[0] = 0x0;
      sns.bitmap[1] = 0x00010000;
      TEST_CHECK(bitmapNonEmpty(sns) == true);
    }

    {
      SequenceNumberSet sns;
      sns.numBits = 64;
      sns.bitmap.length(2);
      sns.bitmap[0] = 0x0;
      sns.bitmap[1] = 0x80000000;
      TEST_CHECK(bitmapNonEmpty(sns) == true);
    }

    {
      SequenceNumberSet sns;
      sns.numBits = 64;
      sns.bitmap.length(2);
      sns.bitmap[0] = 0x0;
      sns.bitmap[1] = 0x00010000;
      TEST_CHECK(bitmapNonEmpty(sns) == true);
    }

    {
      SequenceNumberSet sns;
      sns.numBits = 64;
      sns.bitmap.length(2);
      sns.bitmap[0] = 0x0;
      sns.bitmap[1] = 0x00000001;
      TEST_CHECK(bitmapNonEmpty(sns) == true);
    }

  }
  catch (...) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("ERROR: Exception Caught")));
    return 1;
  }

  return 0;
}
