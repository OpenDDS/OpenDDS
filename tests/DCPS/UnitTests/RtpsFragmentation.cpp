/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "ace/OS_main.h"

#include "dds/DCPS/DataSampleList.h"
#include "dds/DCPS/RepoIdBuilder.h"
#include "dds/DCPS/DisjointSequence.h"

#include "dds/DCPS/RTPS/MessageTypes.h"
#include "dds/DCPS/RTPS/RtpsMessageTypesTypeSupportImpl.h"

#include "dds/DCPS/transport/rtps_udp/RtpsSampleHeader.h"

#include "../common/TestSupport.h"

using namespace OpenDDS::DCPS;
using namespace OpenDDS::RTPS;

struct Fragments {
  ACE_Message_Block* head_;
  ACE_Message_Block* tail_;

  Fragments() : head_(0), tail_(0) {}

  ~Fragments()
  {
    ACE_Message_Block::release(head_);
    ACE_Message_Block::release(tail_);
  }
};

const bool SWAP =
#ifdef ACE_LITTLE_ENDIAN
  false;
#else
  true;
#endif

struct MockTransportSendListener : TransportSendListener {
  void notify_publication_disconnected(const ReaderIdSeq&) {}
  void notify_publication_reconnected(const ReaderIdSeq&) {}
  void notify_publication_lost(const ReaderIdSeq&) {}
  void notify_connection_deleted() {}
  void remove_associations(const ReaderIdSeq&, bool) {}
  void retrieve_inline_qos_data(InlineQosData& qos_data) const
  {
    //TODO
  }
} mock_tsl;

size_t header_len(const SubmessageSeq& subm)
{
  size_t size = 0, padding = 0;
  for (CORBA::ULong i = 0; i < subm.length(); ++i) {
    if ((size + padding) % 4) {
      padding += 4 - ((size + padding) % 4);
    }
    gen_find_size(subm[i], size, padding);
  }
  return size + padding;
}

int ACE_TMAIN(int, ACE_TCHAR*[])
{
  Fragments before_fragmentation;
  const size_t N = 3000;
  // Create pre-fragmented headers
  {
    const InfoTimestampSubmessage ts = {
      {INFO_TS, 1, 8}, {1315413839, 822079774}
    };
    DataSubmessage ds = {
      {DATA, 7, 0}, 0, DATA_OCTETS_TO_IQOS, { {1, 2, 3}, 4},
      { {5, 6, 7}, 8}, {0, 9}, ParameterList()
    };
    ds.inlineQos.length(2);
    ds.inlineQos[0].string_data("my_topic_name");
    ds.inlineQos[0]._d(PID_TOPIC_NAME);
    ds.inlineQos[1].string_data("my_type_name");
    ds.inlineQos[1]._d(PID_TYPE_NAME);
    size_t size = 0, padding = 0;
    gen_find_size(ts, size, padding);
    gen_find_size(ds, size, padding);
    before_fragmentation.head_ = new ACE_Message_Block(size + padding);
    Serializer ser(before_fragmentation.head_, SWAP, Serializer::ALIGN_CDR);
    TEST_CHECK((ser << ts) && (ser << ds));
  }
  ACE_Message_Block& header_mb = *before_fragmentation.head_;
  {
    // Fragment
    ACE_Message_Block data(N);
    data.wr_ptr(N);
    header_mb.cont(&data);
    Fragments f;
    RtpsSampleHeader::split(header_mb, N / 2, f.head_, f.tail_);
    header_mb.cont(0);

    // Check results
    RtpsSampleHeader header1(*f.head_);
    TEST_CHECK(header1.valid());
    TEST_CHECK(header1.submessage_._d() == INFO_TS);
    header1 = *f.head_;
    TEST_CHECK(header1.valid());
    TEST_CHECK(header1.submessage_._d() == DATA_FRAG);
    {
      const DataFragSubmessage& df = header1.submessage_.data_frag_sm();
      TEST_CHECK(df.smHeader.flags == 3);
      TEST_CHECK(df.extraFlags == 0);
      TEST_CHECK(df.octetsToInlineQos == DATA_FRAG_OCTETS_TO_IQOS);
      TEST_CHECK(df.readerId.entityKey[0] == 1);
      TEST_CHECK(df.readerId.entityKey[1] == 2);
      TEST_CHECK(df.readerId.entityKey[2] == 3);
      TEST_CHECK(df.readerId.entityKind == 4);
      TEST_CHECK(df.writerId.entityKey[0] == 5);
      TEST_CHECK(df.writerId.entityKey[1] == 6);
      TEST_CHECK(df.writerId.entityKey[2] == 7);
      TEST_CHECK(df.writerId.entityKind == 8);
      TEST_CHECK(df.writerSN.high == 0);
      TEST_CHECK(df.writerSN.low == 9);
      TEST_CHECK(df.fragmentStartingNum.value == 1);
      TEST_CHECK(df.fragmentsInSubmessage == 1);
      TEST_CHECK(df.fragmentSize == 1024);
      TEST_CHECK(df.inlineQos.length() == 2);
      TEST_CHECK(df.inlineQos[0]._d() == PID_TOPIC_NAME);
      TEST_CHECK(!std::strcmp(df.inlineQos[0].string_data(), "my_topic_name"));
      TEST_CHECK(df.inlineQos[1]._d() == PID_TYPE_NAME);
      TEST_CHECK(!std::strcmp(df.inlineQos[1].string_data(), "my_type_name"));
      TEST_CHECK(f.head_->cont() && f.head_->cont()->length() == 1024);
    }
    before_fragmentation.tail_ = f.tail_->duplicate();
    RtpsSampleHeader header2(*f.tail_);
    TEST_CHECK(header2.valid());
    TEST_CHECK(header2.submessage_._d() == INFO_TS);
    header2 = *f.tail_;
    TEST_CHECK(header2.valid());
    TEST_CHECK(header2.submessage_._d() == DATA_FRAG);
    {
      const DataFragSubmessage& df = header2.submessage_.data_frag_sm();
      TEST_CHECK(df.smHeader.flags == 1);
      TEST_CHECK(df.extraFlags == 0);
      TEST_CHECK(df.octetsToInlineQos == DATA_FRAG_OCTETS_TO_IQOS);
      TEST_CHECK(df.readerId.entityKey[0] == 1);
      TEST_CHECK(df.readerId.entityKey[1] == 2);
      TEST_CHECK(df.readerId.entityKey[2] == 3);
      TEST_CHECK(df.readerId.entityKind == 4);
      TEST_CHECK(df.writerId.entityKey[0] == 5);
      TEST_CHECK(df.writerId.entityKey[1] == 6);
      TEST_CHECK(df.writerId.entityKey[2] == 7);
      TEST_CHECK(df.writerId.entityKind == 8);
      TEST_CHECK(df.writerSN.high == 0);
      TEST_CHECK(df.writerSN.low == 9);
      TEST_CHECK(df.fragmentStartingNum.value == 2);
      TEST_CHECK(df.fragmentsInSubmessage == 2);
      TEST_CHECK(df.fragmentSize == 1024);
      TEST_CHECK(df.inlineQos.length() == 0);
      TEST_CHECK(f.tail_->cont() && f.tail_->cont()->length() == N - 1024);
    }

    // Fragment the resulting "tail"
    Fragments f2;
    RtpsSampleHeader::split(*before_fragmentation.tail_,
                            N / 2, f2.head_, f2.tail_);

    // Check results
    RtpsSampleHeader header3(*f2.head_);
    TEST_CHECK(header3.valid());
    TEST_CHECK(header3.submessage_._d() == INFO_TS);
    header3 = *f2.head_;
    TEST_CHECK(header3.valid());
    TEST_CHECK(header3.submessage_._d() == DATA_FRAG);
    {
      const DataFragSubmessage& df = header3.submessage_.data_frag_sm();
      TEST_CHECK(df.smHeader.flags == 1);
      TEST_CHECK(df.extraFlags == 0);
      TEST_CHECK(df.octetsToInlineQos == DATA_FRAG_OCTETS_TO_IQOS);
      TEST_CHECK(df.readerId.entityKey[0] == 1);
      TEST_CHECK(df.readerId.entityKey[1] == 2);
      TEST_CHECK(df.readerId.entityKey[2] == 3);
      TEST_CHECK(df.readerId.entityKind == 4);
      TEST_CHECK(df.writerId.entityKey[0] == 5);
      TEST_CHECK(df.writerId.entityKey[1] == 6);
      TEST_CHECK(df.writerId.entityKey[2] == 7);
      TEST_CHECK(df.writerId.entityKind == 8);
      TEST_CHECK(df.writerSN.high == 0);
      TEST_CHECK(df.writerSN.low == 9);
      TEST_CHECK(df.fragmentStartingNum.value == 2);
      TEST_CHECK(df.fragmentsInSubmessage == 1);
      TEST_CHECK(df.fragmentSize == 1024);
      TEST_CHECK(df.inlineQos.length() == 0);
      TEST_CHECK(f2.head_->cont() && f2.head_->cont()->length() == 1024);
    }
    RtpsSampleHeader header4(*f2.tail_);
    TEST_CHECK(header4.valid());
    TEST_CHECK(header4.submessage_._d() == INFO_TS);
    header4 = *f2.tail_;
    TEST_CHECK(header4.valid());
    TEST_CHECK(header4.submessage_._d() == DATA_FRAG);
    {
      const DataFragSubmessage& df = header4.submessage_.data_frag_sm();
      TEST_CHECK(df.smHeader.flags == 1);
      TEST_CHECK(df.extraFlags == 0);
      TEST_CHECK(df.octetsToInlineQos == DATA_FRAG_OCTETS_TO_IQOS);
      TEST_CHECK(df.readerId.entityKey[0] == 1);
      TEST_CHECK(df.readerId.entityKey[1] == 2);
      TEST_CHECK(df.readerId.entityKey[2] == 3);
      TEST_CHECK(df.readerId.entityKind == 4);
      TEST_CHECK(df.writerId.entityKey[0] == 5);
      TEST_CHECK(df.writerId.entityKey[1] == 6);
      TEST_CHECK(df.writerId.entityKey[2] == 7);
      TEST_CHECK(df.writerId.entityKind == 8);
      TEST_CHECK(df.writerSN.high == 0);
      TEST_CHECK(df.writerSN.low == 9);
      TEST_CHECK(df.fragmentStartingNum.value == 3);
      TEST_CHECK(df.fragmentsInSubmessage == 1);
      TEST_CHECK(df.fragmentSize == 1024);
      TEST_CHECK(df.inlineQos.length() == 0);
      TEST_CHECK(f2.tail_->cont() && f2.tail_->cont()->length() == N - 2 * 1024);
    }
  }
  // Tests for RtpsSampleHeader::populate_data_frag_submessages(), which is used
  // to send the DATA_FRAG messages which are replies to NACK_FRAGs.
  RepoIdBuilder bld;
  bld.federationId(1);
  bld.participantId(2);
  bld.entityKey(3);
  bld.entityKind(KIND_WRITER);
  DataSampleListElement dsle(bld, &mock_tsl, 0, 0, 0);
  dsle.header_.byte_order_ = true;
  dsle.header_.source_timestamp_sec_ = 1349190278;
  dsle.header_.source_timestamp_nanosec_ = 387505069;
  dsle.header_.sequence_ = 23;
  dsle.header_.message_length_ = 75000; // 73 full frags, 1 frag of 248 bytes
  {
    DisjointSequence requested_fragments;
    requested_fragments.insert(SequenceRange(1, 3));
    requested_fragments.insert(5);
    requested_fragments.insert(SequenceRange(7, 10));
    requested_fragments.insert(SequenceRange(18, 74)); // won't fit in 1 msg
    SubmessageSeq subm;
    size_t length = 0, data_len;
    SequenceNumber current_frag = requested_fragments.low();
    for (int i = 0;
         RtpsSampleHeader::populate_data_frag_submessages(subm, current_frag,
           dsle, false, requested_fragments, GUID_UNKNOWN, length, data_len);
         ++i) {
      length += header_len(subm) + data_len;
      if (current_frag > requested_fragments.high()) {
        break;
      }
    }
    TEST_ASSERT(subm.length() == 5);
  }
  return 0;
}
