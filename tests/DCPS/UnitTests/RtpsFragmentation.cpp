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
#include "dds/DCPS/RepoIdConverter.h"
#include "dds/DCPS/DisjointSequence.h"
#include "dds/DCPS/Service_Participant.h"

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
    qos_data.topic_name = "test_topic";
    qos_data.pub_qos = TheServiceParticipant->initial_PublisherQos();
    qos_data.dw_qos = TheServiceParticipant->initial_DataWriterQos();
  }
  const ACE_CDR::UShort plist_len_;
  ParameterList plist_;
  MockTransportSendListener() : plist_len_(24), plist_(1)
  {
    plist_.length(1);
    plist_[0].string_data("test_topic");
    plist_[0]._d(PID_TOPIC_NAME);
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
      ParameterList plist(2);
      plist.length(2);
      plist[0].string_data("my_topic_name");
      plist[0]._d(PID_TOPIC_NAME);
      plist[1].string_data("my_type_name");
      plist[1]._d(PID_TYPE_NAME);
      const DataFragSubmessage expected = {
        {DATA_FRAG, 3, 0}, 0, DATA_FRAG_OCTETS_TO_IQOS, {{1, 2, 3}, 4},
        {{5, 6, 7}, 8}, {0, 9}, {1}, 1, 1024, N, plist};
      matches(header1.submessage_.data_frag_sm(), expected);
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
      const DataFragSubmessage expected = {
        {DATA_FRAG, 1, 0}, 0, DATA_FRAG_OCTETS_TO_IQOS, {{1, 2, 3}, 4},
        {{5, 6, 7}, 8}, {0, 9}, {2}, 2, 1024, N, ParameterList()};
      matches(header2.submessage_.data_frag_sm(), expected);
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
      const DataFragSubmessage expected = {
        {DATA_FRAG, 1, 0}, 0, DATA_FRAG_OCTETS_TO_IQOS, {{1, 2, 3}, 4},
        {{5, 6, 7}, 8}, {0, 9}, {2}, 1, 1024, N, ParameterList()};
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
        {{5, 6, 7}, 8}, {0, 9}, {3}, 1, 1024, N, ParameterList()};
      matches(header4.submessage_.data_frag_sm(), expected);
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
  const RepoId writerGuid(bld);
  const OpenDDS::RTPS::EntityId_t& writerId = writerGuid.entityId;
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
    SubmessageSeq subm, subm_it;
    size_t length = 0, data_len;
    SequenceNumber current_frag = requested_fragments.low();
    for (int i = 0;
         RtpsSampleHeader::populate_data_frag_submessages(subm_it, current_frag,
           dsle, false, requested_fragments, GUID_UNKNOWN, length, data_len);
         ++i) {
      length += header_len(subm_it) + data_len;
      for (CORBA::ULong j = 0; j < subm_it.length(); ++j) {
        const CORBA::ULong seq_len = subm.length();
        subm.length(seq_len + 1);
        subm[seq_len] = subm_it[j];
      }
      subm_it.length(0);
      switch (i) {
      case 0: TEST_ASSERT(current_frag == 5 && data_len == 3*1024); break;
      case 1: TEST_ASSERT(current_frag == 7 && data_len == 1024); break;
      case 2: TEST_ASSERT(current_frag == 18 && data_len == 4*1024); break;
      case 3: TEST_ASSERT(current_frag == 73 && data_len == 55*1024); break;
      default: TEST_ASSERT(false);
      }
      if (current_frag > requested_fragments.high()) {
        break;
      }
    }
    TEST_ASSERT(subm.length() == 5);
    TEST_ASSERT(subm[0]._d() == INFO_TS);
    TEST_ASSERT(subm[1]._d() == DATA_FRAG);
    {
      const DataFragSubmessage expected = {
        {DATA_FRAG, 1, 32 + 3*1024}, 0, DATA_FRAG_OCTETS_TO_IQOS,
        ENTITYID_UNKNOWN, writerId, {0, 23}, {1}, 3, 1024, 75000,
        ParameterList()};
      matches(subm[1].data_frag_sm(), expected);
    }
    TEST_ASSERT(subm[2]._d() == DATA_FRAG);
    {
      const DataFragSubmessage expected = {
        {DATA_FRAG, 1, 32 + 1024}, 0, DATA_FRAG_OCTETS_TO_IQOS,
        ENTITYID_UNKNOWN, writerId, {0, 23}, {5}, 1, 1024, 75000,
        ParameterList()};
      matches(subm[2].data_frag_sm(), expected);
    }
    TEST_ASSERT(subm[3]._d() == DATA_FRAG);
    {
      const DataFragSubmessage expected = {
        {DATA_FRAG, 1, 32 + 4*1024}, 0, DATA_FRAG_OCTETS_TO_IQOS,
        ENTITYID_UNKNOWN, writerId, {0, 23}, {7}, 4, 1024, 75000,
        ParameterList()};
      matches(subm[3].data_frag_sm(), expected);
    }
    TEST_ASSERT(subm[4]._d() == DATA_FRAG);
    {
      const DataFragSubmessage expected = {
        {DATA_FRAG, 1, 32 + 55*1024}, 0, DATA_FRAG_OCTETS_TO_IQOS,
        ENTITYID_UNKNOWN, writerId, {0, 23}, {18}, 55, 1024, 75000,
        ParameterList()};
      matches(subm[4].data_frag_sm(), expected);
    }
    subm.length(0);
    GuidBuilder readerId;
    readerId.guidPrefix0(0x01030507);
    readerId.guidPrefix1(0xFEDCBA98);
    readerId.guidPrefix2(0x76543210);
    readerId.entityKey(123);
    readerId.entityKind(KIND_READER);
    const OpenDDS::RTPS::GUID_t readerGuid = readerId;
    const bool ret = RtpsSampleHeader::populate_data_frag_submessages(subm,
      current_frag, dsle, false, requested_fragments, readerId, 0, data_len);
    TEST_ASSERT(ret);
    TEST_ASSERT(current_frag >= 75);
    TEST_ASSERT(data_len == 1024 + 248);
    TEST_ASSERT(subm.length() == 2);
    TEST_ASSERT(subm[0]._d() == INFO_DST);
    TEST_ASSERT(!std::memcmp(subm[0].info_dst_sm().guidPrefix,
                             readerGuid.guidPrefix,
                             sizeof(OpenDDS::RTPS::GuidPrefix_t)));
    TEST_ASSERT(subm[1]._d() == DATA_FRAG);
    {
      const DataFragSubmessage expected = {
        {DATA_FRAG, 1, static_cast<ACE_CDR::UShort>(32 + data_len)}, 0,
        DATA_FRAG_OCTETS_TO_IQOS, readerGuid.entityId, writerId, {0, 23},
        {73}, 2, 1024, 75000, ParameterList()};
      matches(subm[1].data_frag_sm(), expected);
    }
    // Test w/ inlineQoS
    current_frag = requested_fragments.low();
    subm.length(0);
    TEST_ASSERT(RtpsSampleHeader::populate_data_frag_submessages(subm,
      current_frag, dsle, true, requested_fragments, GUID_UNKNOWN, 0, data_len));
    TEST_ASSERT(subm.length() == 2);
    TEST_ASSERT(subm[0]._d() == INFO_TS);
    TEST_ASSERT(subm[1]._d() == DATA_FRAG);
    {
      const DataFragSubmessage expected = {
        {DATA_FRAG, 3, 32 + 3*1024 + mock_tsl.plist_len_}, 0,
        DATA_FRAG_OCTETS_TO_IQOS, ENTITYID_UNKNOWN, writerId, {0, 23},
        {1}, 3, 1024, 75000, mock_tsl.plist_};
      matches(subm[1].data_frag_sm(), expected);
    }
    length = header_len(subm);
    TEST_ASSERT(RtpsSampleHeader::populate_data_frag_submessages(subm,
      current_frag, dsle, true, requested_fragments, GUID_UNKNOWN,
      length, data_len));
    TEST_ASSERT(subm.length() == 3);
    TEST_ASSERT(subm[2]._d() == DATA_FRAG);
    {
      const DataFragSubmessage expected = {
        {DATA_FRAG, 1, 32 + 1024}, 0,
        DATA_FRAG_OCTETS_TO_IQOS, ENTITYID_UNKNOWN, writerId, {0, 23},
        {5}, 1, 1024, 75000, ParameterList()};
      matches(subm[2].data_frag_sm(), expected);
    }
  }
  return 0;
}
