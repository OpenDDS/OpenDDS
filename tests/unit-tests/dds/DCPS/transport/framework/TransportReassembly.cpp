/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <gtest/gtest.h>

#include <ace/Log_Msg.h>

#include "dds/DdsDcpsDomainC.h"

#include "dds/DCPS/DataSampleHeader.h"
#include "dds/DCPS/RepoIdGenerator.h"

#include "dds/DCPS/transport/framework/ReceivedDataSample.h"
#include "dds/DCPS/transport/framework/TransportReassembly.h"

#include <cstring>

using namespace OpenDDS::DCPS;

namespace {
  RepoIdGenerator gen(0, 17, OpenDDS::DCPS::KIND_PUBLISHER);

  GUID_t create_pub_id() {
    return gen.next();
  }

  // Class handling assembly of samples with a messge block
  class Sample : public virtual RcObject {
  public:
    Sample(const GUID_t& pub_id, const SequenceNumber& msg_seq, bool more_fragments = true, size_t size = 0, unsigned char fill = 0)
      : mb(new ACE_Message_Block(size == 0 ? DataSampleHeader::get_max_serialized_size() : size))
      , sample()
    {
      if (fill != 0) {
        std::memset(mb->wr_ptr(), fill, size == 0 ? DataSampleHeader::get_max_serialized_size() : size);
      }
      mb->wr_ptr(size == 0 ? DataSampleHeader::get_max_serialized_size() : size);
      sample = ReceivedDataSample(*mb);
      sample.header_.publication_id_ = pub_id;
      sample.header_.sequence_ = msg_seq;
      sample.header_.more_fragments_ = more_fragments;
      sample.fragment_size_ = 1024;
    }
    Message_Block_Ptr mb;
    ReceivedDataSample sample;
  };

  enum Constants {
    BM_LENGTH = 8
  };
  typedef RcHandle<Sample> Sample_rch;

  class Gaps {
  public:
    Gaps()
    : result_bits(0)
    , base()
    {
      std::memset(&bitmap, 0, sizeof(bitmap));
    }

    // Get the gaps in a TransportReassembly instance
    CORBA::ULong get(TransportReassembly& tr, const SequenceNumber& frag_seq, const GUID_t& pub_id) {
      std::memset(&bitmap, 0, sizeof(bitmap));
      base = tr.get_gaps(frag_seq, pub_id, bitmap, bm_length, result_bits);
      return base;
    }

    bool check_gap(CORBA::ULong frag_seq) {
      // How many bits from base
      CORBA::ULong bit_offset = frag_seq - base;
      // What array index is that
      CORBA::ULong index = bit_offset / 32;
      // Find that elemnet
      CORBA::Long& entry = bitmap[index];
      // Now build a bitmask, offset 0 is HIGH ORDER bit
      CORBA::Long mask = 1 << (31 - bit_offset);
      bool gap_flag = entry & mask;
      return gap_flag;
    }

    static CORBA::ULong bm_length;
    CORBA::Long bitmap[BM_LENGTH];
    CORBA::ULong result_bits;
    CORBA::ULong base;
  };
  CORBA::ULong Gaps::bm_length(BM_LENGTH);
};

struct Fragments {
  Message_Block_Ptr head_;
  Message_Block_Ptr tail_;
};

void release_cfentries(ACE_Message_Block& mb_hdr)
{
  ACE_Message_Block* cf = mb_hdr.cont();
  mb_hdr.cont(mb_hdr.cont()->cont());
  cf->cont(0);
  cf->release();
}

void fill(ACE_Message_Block& data, size_t n)
{
  for (size_t i = 0; i < n; ++i) {
    data.base()[i] = char(i);
  }
  data.wr_ptr(n);
}

bool check_reassembled(const ReceivedDataSample& rds)
{
  Message_Block_Ptr data(rds.data());
  for (size_t i = 0; i < data->length(); ++i) {
    if (data->base()[i] != char(i)) {
      ACE_ERROR((LM_ERROR, "(%P|%t) index %d expected %d actual %d\n",
                 int(i), char(i), data->base()[i]));
      return false;
    }
  }
  return true;
}

TEST(dds_DCPS_transport_framework_TransportReassembly, Main_Test)
{
  DataSampleHeader dsh;
  const size_t N = 300;
  dsh.message_length_ = N;
  ACE_Message_Block header_mb(DataSampleHeader::get_max_serialized_size());
  header_mb << dsh;

  { // simple case: no content-filter, data in a single messageblock
    ACE_Message_Block data(N);
    fill(data, N);
    header_mb.cont(&data);
    Fragments f;
    DataSampleHeader::split(header_mb, N / 2 + header_mb.length(),
                            f.head_, f.tail_);

    DataSampleHeader header1(*f.head_);
    EXPECT_TRUE(header1.more_fragments_);
    EXPECT_TRUE(header1.message_length_ == N / 2);
    EXPECT_TRUE(f.head_->cont());
    EXPECT_TRUE(f.head_->cont()->length() == N / 2);
    EXPECT_TRUE(!f.head_->cont()->cont());

    DataSampleHeader header2(*f.tail_);
    EXPECT_TRUE(!header2.more_fragments_);
    EXPECT_TRUE(header2.message_length_ == N / 2);
    EXPECT_TRUE(f.tail_->cont());
    EXPECT_TRUE(f.tail_->cont()->length() == N / 2);
    EXPECT_TRUE(!f.tail_->cont()->cont());

    DataSampleHeader reassembled;
    EXPECT_TRUE(DataSampleHeader::join(header1, header2, reassembled));
    EXPECT_TRUE(reassembled.sequence_ == header1.sequence_);
    EXPECT_TRUE(reassembled.sequence_ == header2.sequence_);
    EXPECT_TRUE(!reassembled.more_fragments_);
    EXPECT_TRUE(reassembled.message_length_ == N);

    ReceivedDataSample rds1(*f.head_->cont()),
      rds2(*f.tail_->cont());
    rds1.header_ = header1;
    rds2.header_ = header2;
    TransportReassembly tr;
    EXPECT_TRUE(!tr.reassemble(4, true, rds1));
    EXPECT_TRUE(tr.reassemble(5, false, rds2));
    EXPECT_EQ(rds2.data_length(), N);
    EXPECT_TRUE(check_reassembled(rds2));

    ReceivedDataSample rds1b(*f.head_->cont()),
      rds2b(*f.tail_->cont());
    rds1b.header_ = header1;
    rds2b.header_ = header2;
    TransportReassembly tr2;
    EXPECT_TRUE(!tr2.reassemble(5, false, rds2b));
    EXPECT_TRUE(tr2.reassemble(4, true, rds1b));
    EXPECT_EQ(rds1b.data_length(), N);
    EXPECT_TRUE(check_reassembled(rds1b));
  }
  { // data split into 3 messageblocks, fragment in the middle of block #2
    ACE_Message_Block data1(N / 3), data2(N / 3), data3(N / 3);
    fill(data1, N / 3);
    fill(data2, N / 3);
    fill(data3, N / 3);
    header_mb.cont(&data1);
    data1.cont(&data2);
    data2.cont(&data3);
    Fragments f;
    DataSampleHeader::split(header_mb, N / 2 + header_mb.length(),
                            f.head_, f.tail_);

    DataSampleHeader header(*f.head_);
    EXPECT_TRUE(header.more_fragments_);
    EXPECT_TRUE(header.message_length_ == N / 2);
    EXPECT_TRUE(f.head_->cont());
    EXPECT_TRUE(f.head_->cont()->length() == N / 3);
    EXPECT_TRUE(f.head_->cont()->cont()->length() == N / 6);
    EXPECT_TRUE(!f.head_->cont()->cont()->cont());

    header = *f.tail_;
    EXPECT_TRUE(!header.more_fragments_);
    EXPECT_TRUE(header.message_length_ == N / 2);
    EXPECT_TRUE(f.tail_->cont());
    EXPECT_TRUE(f.tail_->cont()->length() == N / 6);
    EXPECT_TRUE(f.tail_->cont()->cont()->length() == N / 3);
    EXPECT_TRUE(!f.tail_->cont()->cont()->cont());
  }
  { // data split into 4 messageblocks, fragment at the block division
    ACE_Message_Block data1(N / 4), data2(N / 4), data3(N / 4), data4(N / 4);
    fill(data1, N / 4);
    fill(data2, N / 4);
    fill(data3, N / 4);
    fill(data4, N / 4);
    header_mb.cont(&data1);
    data1.cont(&data2);
    data2.cont(&data3);
    data3.cont(&data4);
    Fragments f;
    DataSampleHeader::split(header_mb, N / 2 + header_mb.length(),
                            f.head_, f.tail_);

    DataSampleHeader header(*f.head_);
    EXPECT_TRUE(header.more_fragments_);
    EXPECT_TRUE(header.message_length_ == N / 2);
    EXPECT_TRUE(f.head_->cont());
    EXPECT_TRUE(f.head_->cont()->length() == N / 4);
    EXPECT_TRUE(f.head_->cont()->cont()->length() == N / 4);
    EXPECT_TRUE(!f.head_->cont()->cont()->cont());

    header = *f.tail_;
    EXPECT_TRUE(!header.more_fragments_);
    EXPECT_TRUE(header.message_length_ == N / 2);
    EXPECT_TRUE(f.tail_->cont());
    EXPECT_TRUE(f.tail_->cont()->length() == N / 4);
    EXPECT_TRUE(f.tail_->cont()->cont()->length() == N / 4);
    EXPECT_TRUE(!f.tail_->cont()->cont()->cont());
  }
  { // 3 fragments needed: first split into 2 and then split the 2nd
    ACE_Message_Block data(N);
    fill(data, N);
    header_mb.cont(&data);
    Fragments f;
    DataSampleHeader::split(header_mb, N / 3 + header_mb.length(),
                            f.head_, f.tail_);

    DataSampleHeader header1(*f.head_);
    EXPECT_TRUE(header1.more_fragments_);
    EXPECT_TRUE(header1.message_length_ == N / 3);
    EXPECT_TRUE(f.head_->cont());
    EXPECT_TRUE(f.head_->cont()->length() == N / 3);
    EXPECT_TRUE(!f.head_->cont()->cont());

    DataSampleHeader header2(*f.tail_);
    EXPECT_TRUE(!header2.more_fragments_);
    EXPECT_TRUE(header2.message_length_ == 2 * N / 3);
    EXPECT_TRUE(f.tail_->cont());
    EXPECT_TRUE(f.tail_->cont()->length() == 2 * N / 3);
    EXPECT_TRUE(!f.tail_->cont()->cont());

    Fragments f2;
    f.tail_->rd_ptr(f.tail_->base());
    DataSampleHeader::split(*f.tail_, N / 3 + header_mb.length(),
                            f2.head_, f2.tail_);

    DataSampleHeader header2a(*f2.head_);
    EXPECT_TRUE(header2a.more_fragments_);
    EXPECT_TRUE(header2a.message_length_ == N / 3);
    EXPECT_TRUE(f2.head_->cont());
    EXPECT_TRUE(f2.head_->cont()->length() == N / 3);
    EXPECT_TRUE(!f2.head_->cont()->cont());

    DataSampleHeader header2b(*f2.tail_);
    EXPECT_TRUE(!header2b.more_fragments_);
    EXPECT_TRUE(header2b.message_length_ == N / 3);
    EXPECT_TRUE(f2.tail_->cont());
    EXPECT_TRUE(f2.tail_->cont()->length() == N / 3);
    EXPECT_TRUE(!f2.tail_->cont()->cont());

    // Test all permutations of order of reception of the 3 fragments
    {
      ReceivedDataSample rds1(*f.head_->cont()),
        rds2(*f2.head_->cont()),
        rds3(*f2.tail_->cont());
      rds1.header_ = header1;
      rds2.header_ = header2a;
      rds3.header_ = header2b;
      TransportReassembly tr;
      EXPECT_TRUE(!tr.reassemble(1, true, rds1));
      EXPECT_TRUE(!tr.reassemble(2, false, rds2));
      EXPECT_TRUE(tr.reassemble(3, false, rds3));
      EXPECT_EQ(rds3.data_length(), N);
      EXPECT_TRUE(check_reassembled(rds3));
    }
    {
      ReceivedDataSample rds1(*f.head_->cont()),
        rds2(*f2.head_->cont()),
        rds3(*f2.tail_->cont());
      rds1.header_ = header1;
      rds2.header_ = header2a;
      rds3.header_ = header2b;
      TransportReassembly tr;
      EXPECT_TRUE(!tr.reassemble(1, true, rds1));
      EXPECT_TRUE(!tr.reassemble(3, false, rds3));
      EXPECT_TRUE(tr.reassemble(2, false, rds2));
      EXPECT_EQ(rds2.data_length(), N);
      EXPECT_TRUE(check_reassembled(rds2));
    }
    {
      ReceivedDataSample rds1(*f.head_->cont()),
        rds2(*f2.head_->cont()),
        rds3(*f2.tail_->cont());
      rds1.header_ = header1;
      rds2.header_ = header2a;
      rds3.header_ = header2b;
      TransportReassembly tr;
      EXPECT_TRUE(!tr.reassemble(2, false, rds2));
      EXPECT_TRUE(!tr.reassemble(1, true, rds1));
      EXPECT_TRUE(tr.reassemble(3, false, rds3));
      EXPECT_EQ(rds3.data_length(), N);
      EXPECT_TRUE(check_reassembled(rds3));
    }
    {
      ReceivedDataSample rds1(*f.head_->cont()),
        rds2(*f2.head_->cont()),
        rds3(*f2.tail_->cont());
      rds1.header_ = header1;
      rds2.header_ = header2a;
      rds3.header_ = header2b;
      TransportReassembly tr;
      EXPECT_TRUE(!tr.reassemble(2, false, rds2));
      EXPECT_TRUE(!tr.reassemble(3, false, rds3));
      EXPECT_TRUE(tr.reassemble(1, true, rds1));
      EXPECT_EQ(rds1.data_length(), N);
      EXPECT_TRUE(check_reassembled(rds1));
    }
    {
      ReceivedDataSample rds1(*f.head_->cont()),
        rds2(*f2.head_->cont()),
        rds3(*f2.tail_->cont());
      rds1.header_ = header1;
      rds2.header_ = header2a;
      rds3.header_ = header2b;
      TransportReassembly tr;
      EXPECT_TRUE(!tr.reassemble(3, false, rds3));
      EXPECT_TRUE(!tr.reassemble(1, true, rds1));
      EXPECT_TRUE(tr.reassemble(2, false, rds2));
      EXPECT_EQ(rds2.data_length(), N);
      EXPECT_TRUE(check_reassembled(rds2));
    }
    {
      ReceivedDataSample rds1(*f.head_->cont()),
        rds2(*f2.head_->cont()),
        rds3(*f2.tail_->cont());
      rds1.header_ = header1;
      rds2.header_ = header2a;
      rds3.header_ = header2b;
      TransportReassembly tr;
      EXPECT_TRUE(!tr.reassemble(3, false, rds3));
      EXPECT_TRUE(!tr.reassemble(2, false, rds2));
      EXPECT_TRUE(tr.reassemble(1, true, rds1));
      EXPECT_EQ(rds1.data_length(), N);
      EXPECT_TRUE(check_reassembled(rds1));
    }
    // Retest permutations with some duplicates
    {
      ReceivedDataSample rds1(*f.head_->cont()),
        rds2(*f2.head_->cont()),
        rds3(*f2.tail_->cont());
      rds1.header_ = header1;
      rds2.header_ = header2a;
      rds3.header_ = header2b;
      TransportReassembly tr;
      EXPECT_TRUE(!tr.reassemble(1, true, rds1));
      EXPECT_TRUE(!tr.reassemble(2, false, rds2));
      EXPECT_TRUE(!tr.reassemble(1, true, rds1));
      EXPECT_TRUE(tr.reassemble(3, false, rds3));
      EXPECT_EQ(rds3.data_length(), N);
      EXPECT_TRUE(check_reassembled(rds3));
    }
    {
      ReceivedDataSample rds1(*f.head_->cont()),
        rds2(*f2.head_->cont()),
        rds3(*f2.tail_->cont());
      rds1.header_ = header1;
      rds2.header_ = header2a;
      rds3.header_ = header2b;
      TransportReassembly tr;
      EXPECT_TRUE(!tr.reassemble(1, true, rds1));
      EXPECT_TRUE(!tr.reassemble(3, false, rds3));
      EXPECT_TRUE(!tr.reassemble(1, true, rds1));
      EXPECT_TRUE(!tr.reassemble(3, false, rds3));
      EXPECT_TRUE(tr.reassemble(2, false, rds2));
      EXPECT_EQ(rds2.data_length(), N);
      EXPECT_TRUE(check_reassembled(rds2));
    }
    {
      ReceivedDataSample rds1(*f.head_->cont()),
        rds2(*f2.head_->cont()),
        rds3(*f2.tail_->cont());
      rds1.header_ = header1;
      rds2.header_ = header2a;
      rds3.header_ = header2b;
      TransportReassembly tr;
      EXPECT_TRUE(!tr.reassemble(2, false, rds2));
      EXPECT_TRUE(!tr.reassemble(1, true, rds1));
      EXPECT_TRUE(!tr.reassemble(2, false, rds2));
      EXPECT_TRUE(!tr.reassemble(1, true, rds1));
      EXPECT_TRUE(tr.reassemble(3, false, rds3));
      EXPECT_EQ(rds3.data_length(), N);
      EXPECT_TRUE(check_reassembled(rds3));
    }
    {
      ReceivedDataSample rds1(*f.head_->cont()),
        rds2(*f2.head_->cont()),
        rds3(*f2.tail_->cont());
      rds1.header_ = header1;
      rds2.header_ = header2a;
      rds3.header_ = header2b;
      TransportReassembly tr;
      EXPECT_TRUE(!tr.reassemble(2, false, rds2));
      EXPECT_TRUE(!tr.reassemble(3, false, rds3));
      EXPECT_TRUE(!tr.reassemble(2, false, rds2));
      EXPECT_TRUE(!tr.reassemble(3, false, rds3));
      EXPECT_TRUE(tr.reassemble(1, true, rds1));
      EXPECT_EQ(rds1.data_length(), N);
      EXPECT_TRUE(check_reassembled(rds1));
    }
    {
      ReceivedDataSample rds1(*f.head_->cont()),
        rds2(*f2.head_->cont()),
        rds3(*f2.tail_->cont());
      rds1.header_ = header1;
      rds2.header_ = header2a;
      rds3.header_ = header2b;
      TransportReassembly tr;
      EXPECT_TRUE(!tr.reassemble(3, false, rds3));
      EXPECT_TRUE(!tr.reassemble(1, true, rds1));
      EXPECT_TRUE(!tr.reassemble(3, false, rds3));
      EXPECT_TRUE(!tr.reassemble(1, true, rds1));
      EXPECT_TRUE(tr.reassemble(2, false, rds2));
      EXPECT_EQ(rds2.data_length(), N);
      EXPECT_TRUE(check_reassembled(rds2));
    }
    {
      ReceivedDataSample rds1(*f.head_->cont()),
        rds2(*f2.head_->cont()),
        rds3(*f2.tail_->cont());
      rds1.header_ = header1;
      rds2.header_ = header2a;
      rds3.header_ = header2b;
      TransportReassembly tr;
      EXPECT_TRUE(!tr.reassemble(3, false, rds3));
      EXPECT_TRUE(!tr.reassemble(2, false, rds2));
      EXPECT_TRUE(!tr.reassemble(3, false, rds3));
      EXPECT_TRUE(!tr.reassemble(2, false, rds2));
      EXPECT_TRUE(tr.reassemble(1, true, rds1));
      EXPECT_EQ(rds1.data_length(), N);
      EXPECT_TRUE(check_reassembled(rds1));
    }
    // Test ignoring of frags from previously completed sequence numbers
    {
      ReceivedDataSample rds1(*f.head_->cont()),
        rds2(*f2.head_->cont()),
        rds3(*f2.tail_->cont());
      rds1.header_ = header1;
      rds2.header_ = header2a;
      rds3.header_ = header2b;
      TransportReassembly tr;
      EXPECT_TRUE(!tr.reassemble(1, true, rds1));
      EXPECT_TRUE(!tr.reassemble(2, false, rds2));
      EXPECT_TRUE(tr.reassemble(3, false, rds3));
      EXPECT_EQ(rds3.data_length(), N);
      EXPECT_TRUE(check_reassembled(rds3));
      // Now we're ignoring
      ReceivedDataSample rds4(*f.head_->cont()),
        rds5(*f2.head_->cont()),
        rds6(*f2.tail_->cont());
      rds4.header_ = header1;
      rds5.header_ = header2a;
      rds6.header_ = header2b;
      EXPECT_TRUE(!tr.reassemble(1, true, rds4));
      EXPECT_TRUE(!tr.reassemble(2, false, rds5));
      EXPECT_TRUE(!tr.reassemble(3, false, rds6));
      // Back to normal
      tr.clear_completed(rds1.header_.publication_id_); // clears completed SN
      ReceivedDataSample rds7(*f.head_->cont()),
        rds8(*f2.head_->cont()),
        rds9(*f2.tail_->cont());
      rds7.header_ = header1;
      rds8.header_ = header2a;
      rds9.header_ = header2b;
      EXPECT_TRUE(!tr.reassemble(1, true, rds7));
      EXPECT_TRUE(!tr.reassemble(2, false, rds8));
      EXPECT_TRUE(tr.reassemble(3, false, rds9));
      EXPECT_EQ(rds9.data_length(), N);
      EXPECT_TRUE(check_reassembled(rds9));
    }
    // Test data_unavailable() scenarios
    {
      ReceivedDataSample rds1(*f.head_->cont()),
        rds3(*f2.tail_->cont());
      rds1.header_ = header1;
      rds3.header_ = header2b;
      TransportReassembly tr;
      EXPECT_TRUE(!tr.reassemble(1, true, rds1));
      tr.data_unavailable(FragmentRange(2, 2));
      EXPECT_TRUE(!tr.reassemble(3, false, rds3));
    }
    {
      ReceivedDataSample rds1(*f.head_->cont()),
        rds3(*f2.tail_->cont());
      rds1.header_ = header1;
      rds3.header_ = header2b;
      TransportReassembly tr;
      EXPECT_TRUE(!tr.reassemble(3, false, rds3));
      tr.data_unavailable(FragmentRange(2, 2));
      EXPECT_TRUE(!tr.reassemble(1, true, rds1));
    }
    {
      ReceivedDataSample rds1(*f.head_->cont()),
        rds2(*f2.head_->cont());
      rds1.header_ = header1;
      rds2.header_ = header2a;
      TransportReassembly tr;
      EXPECT_TRUE(!tr.reassemble(2, false, rds2));
      tr.data_unavailable(FragmentRange(3, 3));
      EXPECT_TRUE(!tr.reassemble(1, true, rds1));
    }
  }
  { // 4 fragments needed: first split into 2 and then split each again
    ACE_Message_Block data(N);
    fill(data, N);
    header_mb.cont(&data);
    Fragments f0;
    DataSampleHeader::split(header_mb, N / 2 + header_mb.length(), f0.head_, f0.tail_);

    DataSampleHeader header0a(*f0.head_);
    EXPECT_TRUE(header0a.more_fragments_);
    EXPECT_TRUE(header0a.message_length_ == N / 2);
    EXPECT_TRUE(f0.head_->cont());
    EXPECT_TRUE(f0.head_->cont()->length() == N / 2);
    EXPECT_TRUE(!f0.head_->cont()->cont());

    DataSampleHeader header0b(*f0.tail_);
    EXPECT_TRUE(!header0b.more_fragments_);
    EXPECT_TRUE(header0b.message_length_ == N / 2);
    EXPECT_TRUE(f0.tail_->cont());
    EXPECT_TRUE(f0.tail_->cont()->length() == N / 2);
    EXPECT_TRUE(!f0.tail_->cont()->cont());

    Fragments f1;
    f0.head_->rd_ptr(f0.head_->base());
    DataSampleHeader::split(*f0.head_, N / 4 + header_mb.length(), f1.head_, f1.tail_);

    DataSampleHeader header1a(*f1.head_);
    EXPECT_TRUE(header1a.more_fragments_);
    EXPECT_TRUE(header1a.message_length_ == N / 4);
    EXPECT_TRUE(f1.head_->cont());
    EXPECT_TRUE(f1.head_->cont()->length() == N / 4);
    EXPECT_TRUE(!f1.head_->cont()->cont());

    DataSampleHeader header1b(*f1.tail_);
    header1b.more_fragments_ = true;
    EXPECT_TRUE(header1b.more_fragments_);
    EXPECT_TRUE(header1b.message_length_ == N / 4);
    EXPECT_TRUE(f1.tail_->cont());
    EXPECT_TRUE(f1.tail_->cont()->length() == N / 4);
    EXPECT_TRUE(!f1.tail_->cont()->cont());

    Fragments f2;
    f0.tail_->rd_ptr(f0.tail_->base());
    DataSampleHeader::split(*f0.tail_, N / 4 + header_mb.length(), f2.head_, f2.tail_);

    DataSampleHeader header2a(*f2.head_);
    EXPECT_TRUE(header2a.more_fragments_);
    EXPECT_TRUE(header2a.message_length_ == N / 4);
    EXPECT_TRUE(f2.head_->cont());
    EXPECT_TRUE(f2.head_->cont()->length() == N / 4);
    EXPECT_TRUE(!f2.head_->cont()->cont());

    DataSampleHeader header2b(*f2.tail_);
    EXPECT_TRUE(!header2b.more_fragments_);
    EXPECT_TRUE(header2b.message_length_ == N / 4);
    EXPECT_TRUE(f2.tail_->cont());
    EXPECT_TRUE(f2.tail_->cont()->length() == N / 4);
    EXPECT_TRUE(!f2.tail_->cont()->cont());

    // Test all permutations of order of reception of the 3 fragments
    {
      ReceivedDataSample rds1(*f1.head_->cont()),
        rds2(*f1.tail_->cont()),
        rds3(*f2.head_->cont()),
        rds4(*f2.tail_->cont());
      rds1.header_ = header1a;
      rds2.header_ = header1b;
      rds3.header_ = header2a;
      rds4.header_ = header2b;
      TransportReassembly tr;
      EXPECT_TRUE(!tr.reassemble(FragmentRange(1, 5), rds1));
      EXPECT_TRUE(!tr.reassemble(FragmentRange(6, 10), rds2));
      EXPECT_TRUE(!tr.reassemble(FragmentRange(11, 15), rds3));
      EXPECT_TRUE(tr.reassemble(FragmentRange(16, 20), rds4));
      EXPECT_EQ(rds4.data_length(), N);
      EXPECT_TRUE(check_reassembled(rds4));
    }
    {
      ReceivedDataSample rds1(*f1.head_->cont()),
        rds2(*f1.tail_->cont()),
        rds3(*f2.head_->cont()),
        rds4(*f2.tail_->cont());
      rds1.header_ = header1a;
      rds2.header_ = header1b;
      rds3.header_ = header2a;
      rds4.header_ = header2b;
      TransportReassembly tr;
      EXPECT_TRUE(!tr.reassemble(FragmentRange(1, 5), rds1));
      EXPECT_TRUE(!tr.reassemble(FragmentRange(16, 20), rds4));
      EXPECT_TRUE(!tr.reassemble(FragmentRange(6, 10), rds2));
      EXPECT_TRUE(tr.reassemble(FragmentRange(11, 15), rds3));
      EXPECT_EQ(rds3.data_length(), N);
      EXPECT_TRUE(check_reassembled(rds3));
    }
    {
      ReceivedDataSample rds1(*f1.head_->cont()),
        rds2(*f1.tail_->cont()),
        rds3(*f2.head_->cont()),
        rds4(*f2.tail_->cont());
      rds1.header_ = header1a;
      rds2.header_ = header1b;
      rds3.header_ = header2a;
      rds4.header_ = header2b;
      TransportReassembly tr;
      EXPECT_TRUE(!tr.reassemble(FragmentRange(1, 5), rds1));
      EXPECT_TRUE(!tr.reassemble(FragmentRange(16, 20), rds4));
      EXPECT_TRUE(!tr.reassemble(FragmentRange(11, 15), rds3));
      EXPECT_TRUE(tr.reassemble(FragmentRange(6, 10), rds2));
      EXPECT_EQ(rds2.data_length(), N);
      EXPECT_TRUE(check_reassembled(rds2));
    }
  }
  { // content filtering flag with no "entries" (adds another MB to the chain)
    ACE_Message_Block data(N);
    fill(data, N);
    header_mb.cont(&data);
    DataSampleHeader::set_flag(CONTENT_FILTER_FLAG, &header_mb);
    DataSampleHeader::add_cfentries(0, &header_mb);
    Fragments f;
    const size_t FRAG = 100; // arbitrary split at 100 bytes
    DataSampleHeader::split(header_mb, FRAG, f.head_, f.tail_);

    DataSampleHeader header1(*f.head_);
    EXPECT_TRUE(header1.more_fragments_);
    EXPECT_TRUE(header1.content_filter_);
    EXPECT_TRUE(header1.content_filter_entries_.length() == 0);
    const size_t hdr_len = header_mb.length() + header_mb.cont()->length();
    release_cfentries(header_mb);

    EXPECT_TRUE(header1.message_length_ == FRAG - hdr_len);
    EXPECT_TRUE(f.head_->length() == 0); // consumed by DataSampleHeader
    EXPECT_TRUE(f.head_->cont());
    EXPECT_TRUE(f.head_->cont()->length() == 0); // consumed by DataSampleHeader
    EXPECT_TRUE(f.head_->cont()->cont());
    EXPECT_TRUE(f.head_->cont()->cont()->length() == header1.message_length_);
    EXPECT_TRUE(!f.head_->cont()->cont()->cont());

    DataSampleHeader header2(*f.tail_);
    EXPECT_TRUE(!header2.more_fragments_);
    EXPECT_TRUE(!header2.content_filter_);
    EXPECT_TRUE(header2.message_length_ == N - (FRAG - hdr_len));
    EXPECT_TRUE(f.tail_->length() == 0); // consumed by DataSampleHeader
    EXPECT_TRUE(f.tail_->cont());
    EXPECT_TRUE(f.tail_->cont()->length() == header2.message_length_);
    EXPECT_TRUE(!f.tail_->cont()->cont());

    DataSampleHeader reassembled;
    EXPECT_TRUE(DataSampleHeader::join(header1, header2, reassembled));
    EXPECT_TRUE(reassembled.sequence_ == header1.sequence_);
    EXPECT_TRUE(reassembled.sequence_ == header2.sequence_);
    EXPECT_TRUE(!reassembled.more_fragments_);
    EXPECT_TRUE(reassembled.message_length_ == N);
    EXPECT_TRUE(reassembled.content_filter_);
  }
  { // content filtering with some "entries" that all fit in the fragment
    ACE_Message_Block data(N);
    fill(data, N);
    header_mb.cont(&data);
    DataSampleHeader::set_flag(CONTENT_FILTER_FLAG, &header_mb);
    const size_t CF_ENTRIES = 6; // serializes to 100 bytes
    GUIDSeq entries(CF_ENTRIES);
    entries.length(CF_ENTRIES);
    DataSampleHeader::add_cfentries(&entries, &header_mb);
    Fragments f;
    const size_t FRAG = 200;
    DataSampleHeader::split(header_mb, FRAG, f.head_, f.tail_);
    release_cfentries(header_mb);

    DataSampleHeader header1(*f.head_);
    EXPECT_TRUE(header1.more_fragments_);
    EXPECT_TRUE(header1.content_filter_);
    EXPECT_TRUE(header1.content_filter_entries_.length() == CF_ENTRIES);
    EXPECT_TRUE(header1.message_length_ > 0);
    EXPECT_TRUE(f.head_->length() == 0); // consumed by DataSampleHeader
    EXPECT_TRUE(f.head_->cont());
    EXPECT_TRUE(f.head_->cont()->length() == 0); // consumed by DataSampleHeader
    EXPECT_TRUE(f.head_->cont()->cont());
    const size_t frag_payload = f.head_->cont()->cont()->length();
    EXPECT_TRUE(frag_payload > 0);
    EXPECT_TRUE(!f.head_->cont()->cont()->cont());

    DataSampleHeader header2(*f.tail_);
    EXPECT_TRUE(!header2.more_fragments_);
    EXPECT_TRUE(!header2.content_filter_);
    EXPECT_TRUE(header2.message_length_ + frag_payload == N);
    EXPECT_TRUE(f.tail_->length() == 0); // consumed by DataSampleHeader
    EXPECT_TRUE(f.tail_->cont());
    EXPECT_TRUE(f.tail_->cont()->length() == header2.message_length_);
    EXPECT_TRUE(!f.tail_->cont()->cont());

    DataSampleHeader reassembled;
    EXPECT_TRUE(DataSampleHeader::join(header1, header2, reassembled));
    EXPECT_TRUE(reassembled.sequence_ == header1.sequence_);
    EXPECT_TRUE(reassembled.sequence_ == header2.sequence_);
    EXPECT_TRUE(!reassembled.more_fragments_);
    EXPECT_TRUE(reassembled.message_length_ == N);
    EXPECT_TRUE(reassembled.content_filter_);
    EXPECT_TRUE(reassembled.content_filter_entries_.length() == CF_ENTRIES);
  }
  { // content filtering with some "entries", split inside the entires
    ACE_Message_Block data(N);
    fill(data, N);
    header_mb.cont(&data);
    DataSampleHeader::set_flag(CONTENT_FILTER_FLAG, &header_mb);
    const size_t CF_ENTRIES = 6; // serializes to 100 bytes
    GUIDSeq entries(CF_ENTRIES);
    entries.length(CF_ENTRIES);
    DataSampleHeader::add_cfentries(&entries, &header_mb);
    Fragments f;
    const size_t FRAG = 68;
    DataSampleHeader::split(header_mb, FRAG, f.head_, f.tail_);
    release_cfentries(header_mb);

    DataSampleHeader header1(*f.head_);
    EXPECT_TRUE(header1.more_fragments_);
    EXPECT_TRUE(header1.content_filter_);
    const size_t entries_in_header = header1.content_filter_entries_.length();
    EXPECT_TRUE(entries_in_header > 0);
    EXPECT_TRUE(header1.message_length_ == 0);
    EXPECT_TRUE(f.head_->length() == 0); // consumed by DataSampleHeader
    EXPECT_TRUE(f.head_->cont());
    EXPECT_TRUE(f.head_->cont()->length() == 0); // consumed by DataSampleHeader
    EXPECT_TRUE(!f.head_->cont()->cont());

    DataSampleHeader header2(*f.tail_);
    EXPECT_TRUE(!header2.more_fragments_);
    EXPECT_TRUE(header2.content_filter_);
    EXPECT_TRUE(header2.content_filter_entries_.length()
                + entries_in_header == CF_ENTRIES);
    EXPECT_TRUE(header2.message_length_ == N);
    EXPECT_TRUE(f.tail_->length() == 0); // consumed by DataSampleHeader
    EXPECT_TRUE(f.tail_->cont());
    EXPECT_TRUE(f.tail_->cont()->length() == 0); // consumed by DataSampleHeader
    EXPECT_TRUE(f.tail_->cont()->cont());
    EXPECT_TRUE(f.tail_->cont()->cont()->length() == N);
    EXPECT_TRUE(!f.tail_->cont()->cont()->cont());

    DataSampleHeader reassembled;
    EXPECT_TRUE(DataSampleHeader::join(header1, header2, reassembled));
    EXPECT_TRUE(reassembled.sequence_ == header1.sequence_);
    EXPECT_TRUE(reassembled.sequence_ == header2.sequence_);
    EXPECT_TRUE(!reassembled.more_fragments_);
    EXPECT_TRUE(reassembled.message_length_ == N);
    EXPECT_TRUE(reassembled.content_filter_);
    EXPECT_TRUE(reassembled.content_filter_entries_.length() == CF_ENTRIES);
  }
}

TEST(dds_DCPS_transport_framework_TransportReassembly, Test_Empty)
{
  TransportReassembly tr;
  Gaps gaps;
  SequenceNumber seq(1);
  GUID_t pub_id = create_pub_id();
  EXPECT_TRUE(!tr.has_frags(seq, pub_id));
  EXPECT_TRUE(0 == gaps.get(tr, seq, pub_id));
}

TEST(dds_DCPS_transport_framework_TransportReassembly, Test_Insert_Has_Frag)
{
  TransportReassembly tr;
  Gaps gaps;
  SequenceNumber msg_seq(4);
  SequenceNumber frag_seq(1);
  GUID_t pub_id = create_pub_id();
  Sample data(pub_id, msg_seq);
  bool reassembled = tr.reassemble(frag_seq, true, data.sample);
  EXPECT_TRUE(false == reassembled);
  EXPECT_TRUE(tr.has_frags(msg_seq, pub_id));
}

TEST(dds_DCPS_transport_framework_TransportReassembly, Test_First_Insert_Has_No_Gaps)
{
  TransportReassembly tr;
  Gaps gaps;
  SequenceNumber msg_seq(18);
  SequenceNumber frag_seq(1);
  GUID_t pub_id = create_pub_id();
  Sample data(pub_id, msg_seq);

  EXPECT_TRUE(!tr.reassemble(frag_seq, true, data.sample));
  CORBA::ULong base = gaps.get(tr, msg_seq, pub_id);

  EXPECT_TRUE(tr.has_frags(msg_seq, pub_id));
  EXPECT_TRUE(2 == base);             // Now expecting 2
  EXPECT_TRUE(1 == gaps.result_bits); // Only one bit
  EXPECT_TRUE(gaps.check_gap(2));     // Gap
  EXPECT_TRUE(!gaps.check_gap(3));    // No gap
}

TEST(dds_DCPS_transport_framework_TransportReassembly, Test_Insert_Gaps)
{
  TransportReassembly tr;
  Gaps gaps;
  SequenceNumber msg_seq(9);
  SequenceNumber frag_seq(4);
  GUID_t pub_id = create_pub_id();
  Sample data(pub_id, msg_seq);

  bool reassembled = tr.reassemble(frag_seq, true, data.sample);
  EXPECT_TRUE(false == reassembled);

  CORBA::ULong base = gaps.get(tr, msg_seq, pub_id);

  EXPECT_TRUE(1 == base);             // Gap from 1-3
  EXPECT_TRUE(3 == gaps.result_bits); // Only one bit
  EXPECT_TRUE(gaps.check_gap(1));     // Gap
  EXPECT_TRUE(gaps.check_gap(2));     // Gap
  EXPECT_TRUE(gaps.check_gap(3));     // Gap
  EXPECT_TRUE(!gaps.check_gap(4));    // No gap
}

TEST(dds_DCPS_transport_framework_TransportReassembly, Test_Insert_One_Then_Gap)
{
  TransportReassembly tr;
  Gaps gaps;
  SequenceNumber msg_seq(17);
  SequenceNumber frag_seq1(1);
  SequenceNumber frag_seq2(6);
  GUID_t pub_id = create_pub_id();
  Sample data(pub_id, msg_seq);

  bool reassembled = tr.reassemble(frag_seq1, true, data.sample);
  EXPECT_TRUE(false == reassembled);

  reassembled = tr.reassemble(frag_seq2, true, data.sample);
  EXPECT_TRUE(false == reassembled);

  CORBA::ULong base = gaps.get(tr, msg_seq, pub_id);

  EXPECT_TRUE(2 == base);             // Gap from 2-5
  EXPECT_TRUE(4 == gaps.result_bits); // Number of bits
  EXPECT_TRUE(gaps.check_gap(2));     // Gap
  EXPECT_TRUE(gaps.check_gap(3));     // Gap
  EXPECT_TRUE(gaps.check_gap(4));     // Gap
  EXPECT_TRUE(gaps.check_gap(5));     // Gap
  EXPECT_TRUE(!gaps.check_gap(6));    // No gap
}

TEST(dds_DCPS_transport_framework_TransportReassembly, Test_Insert_One_Then_Split_Gap)
{
  TransportReassembly tr;
  Gaps gaps;
  SequenceNumber msg_seq(17);
  GUID_t pub_id = create_pub_id();
  Sample data(pub_id, msg_seq);

  EXPECT_TRUE(!tr.reassemble(1, true, data.sample)); // 1
  EXPECT_TRUE(!tr.reassemble(6, true, data.sample)); // 1,6
  CORBA::ULong base = gaps.get(tr, msg_seq, pub_id);

  EXPECT_TRUE(2 == base);             // Gap from 2-5
  EXPECT_TRUE(4 == gaps.result_bits); // Number of bits
  EXPECT_TRUE(gaps.check_gap(2));     // Gap
  EXPECT_TRUE(gaps.check_gap(3));     // Gap
  EXPECT_TRUE(gaps.check_gap(4));     // Gap
  EXPECT_TRUE(gaps.check_gap(5));     // Gap
  EXPECT_TRUE(!gaps.check_gap(6));    // No gap

  EXPECT_TRUE(!tr.reassemble(4, true, data.sample)); // 1,4,6
  EXPECT_TRUE(!tr.reassemble(3, true, data.sample)); // 1,3-4,6
  base = gaps.get(tr, msg_seq, pub_id);

  EXPECT_TRUE(2 == base);             // Gap at 2 and 5
  EXPECT_TRUE(4 == gaps.result_bits); // Number of bits
  EXPECT_TRUE(gaps.check_gap(2));     // Gap
  EXPECT_TRUE(!gaps.check_gap(3));    // No gap
  EXPECT_TRUE(!gaps.check_gap(4));    // No gap
  EXPECT_TRUE(gaps.check_gap(5));     // Gap
  EXPECT_TRUE(!gaps.check_gap(6));    // No gap
}

TEST(dds_DCPS_transport_framework_TransportReassembly, Test_Fill_Right_To_Left)
{
  TransportReassembly tr;
  Gaps gaps;
  SequenceNumber msg_seq(17);
  GUID_t pub_id = create_pub_id();
  Sample data(pub_id, msg_seq);

  EXPECT_TRUE(!tr.reassemble(1, true, data.sample)); // 1
  EXPECT_TRUE(!tr.reassemble(6, true, data.sample)); // 1,6
  CORBA::ULong base = gaps.get(tr, msg_seq, pub_id);

  EXPECT_TRUE(2 == base);             // Gap from 2-5
  EXPECT_TRUE(4 == gaps.result_bits); // Number of bits
  EXPECT_TRUE(gaps.check_gap(2));     // Gap
  EXPECT_TRUE(gaps.check_gap(3));     // Gap
  EXPECT_TRUE(gaps.check_gap(4));     // Gap
  EXPECT_TRUE(gaps.check_gap(5));     // Gap
  EXPECT_TRUE(!gaps.check_gap(6));    // No gap

  EXPECT_TRUE(!tr.reassemble(5, true, data.sample)); // 1,5-6
  EXPECT_TRUE(!tr.reassemble(4, true, data.sample)); // 1,4-6
  base = gaps.get(tr, msg_seq, pub_id);

  EXPECT_TRUE(2 == base);             // Gap from 2-3
  EXPECT_TRUE(2 == gaps.result_bits); // Number of bits
  EXPECT_TRUE(gaps.check_gap(2));     // Gap
  EXPECT_TRUE(gaps.check_gap(3));     // Gap
  EXPECT_TRUE(!gaps.check_gap(4));    // No gap
  EXPECT_TRUE(!gaps.check_gap(5));    // No gap
  EXPECT_TRUE(!gaps.check_gap(6));    // No gap

  EXPECT_TRUE(!tr.reassemble(3, true, data.sample)); // 1,3-6
  EXPECT_TRUE(!tr.reassemble(2, true, data.sample)); // 1-6
  base = gaps.get(tr, msg_seq, pub_id);

  EXPECT_TRUE(7 == base);             // Gap from 2-3
  EXPECT_TRUE(1 == gaps.result_bits); // Number of bits
  EXPECT_TRUE(gaps.check_gap(7));     // Gap
  EXPECT_TRUE(!gaps.check_gap(8));    // No gap
}

TEST(dds_DCPS_transport_framework_TransportReassembly, Test_Fill_Left_To_Right)
{
  TransportReassembly tr;
  Gaps gaps;
  SequenceNumber msg_seq(27);
  GUID_t pub_id = create_pub_id();
  Sample data(pub_id, msg_seq);

  EXPECT_TRUE(!tr.reassemble(1, true, data.sample)); // 1
  EXPECT_TRUE(!tr.reassemble(6, true, data.sample)); // 1,6
  CORBA::ULong base = gaps.get(tr, msg_seq, pub_id);

  EXPECT_TRUE(2 == base);             // Gap from 2-5
  EXPECT_TRUE(4 == gaps.result_bits); // Number of bits
  EXPECT_TRUE(gaps.check_gap(2));     // Gap
  EXPECT_TRUE(gaps.check_gap(3));     // Gap
  EXPECT_TRUE(gaps.check_gap(4));     // Gap
  EXPECT_TRUE(gaps.check_gap(5));     // Gap
  EXPECT_TRUE(!gaps.check_gap(6));    // No gap

  EXPECT_TRUE(!tr.reassemble(2, true, data.sample)); // 1-2,6
  EXPECT_TRUE(!tr.reassemble(3, true, data.sample)); // 1-3,6
  base = gaps.get(tr, msg_seq, pub_id);

  EXPECT_TRUE(4 == base);             // Gap from 4-5
  EXPECT_TRUE(2 == gaps.result_bits); // Number of bits
  EXPECT_TRUE(gaps.check_gap(4));     // Gap
  EXPECT_TRUE(gaps.check_gap(5));     // Gap
  EXPECT_TRUE(!gaps.check_gap(6));    // No gap

  EXPECT_TRUE(!tr.reassemble(4, true, data.sample)); // 1-4,6
  EXPECT_TRUE(!tr.reassemble(5, true, data.sample)); // 1-6
  base = gaps.get(tr, msg_seq, pub_id);

  EXPECT_TRUE(7 == base);             // Gap from 2-3
  EXPECT_TRUE(1 == gaps.result_bits); // Number of bits
  EXPECT_TRUE(gaps.check_gap(7));     // Gap
  EXPECT_TRUE(!gaps.check_gap(8));    // No gap
}

TEST(dds_DCPS_transport_framework_TransportReassembly, Test_Fill_Out_Of_Order)
{
  TransportReassembly tr;
  Gaps gaps;
  SequenceNumber msg_seq(3);
  GUID_t pub_id = create_pub_id();
  Sample data1(pub_id, msg_seq);
  Sample data2(pub_id, msg_seq);
  Sample data3(pub_id, msg_seq);
  Sample data4(pub_id, msg_seq);
  Sample data5(pub_id, msg_seq, false);
  Sample data6(pub_id, msg_seq, false);
  Sample data7(pub_id, msg_seq);
  Sample data8(pub_id, msg_seq);
  Sample data9(pub_id, msg_seq);
  Sample data10(pub_id, msg_seq);

  EXPECT_TRUE(!tr.reassemble(FragmentRange(1, 63), data1.sample, 251)); // 1-63
  EXPECT_TRUE(!tr.reassemble(FragmentRange(1, 63), data2.sample, 251)); // 1-63
  EXPECT_TRUE(!tr.reassemble(FragmentRange(127, 189), data3.sample, 251)); // 1-63, 127-189
  EXPECT_TRUE(!tr.reassemble(FragmentRange(127, 189), data4.sample, 251)); // 1-63, 127-189
  EXPECT_TRUE(!tr.reassemble(FragmentRange(190, 251), data5.sample, 251)); // 1-63, 127-251
  EXPECT_TRUE(!tr.reassemble(FragmentRange(190, 251), data6.sample, 251)); // 1-63, 127-251
  EXPECT_TRUE(tr.reassemble(FragmentRange(64, 126), data7.sample, 251)); // 1-251
  EXPECT_TRUE(!tr.reassemble(FragmentRange(1, 63), data8.sample, 251)); // 1-251
  EXPECT_TRUE(!tr.reassemble(FragmentRange(64, 126), data9.sample, 251)); // 1-251
}

namespace {

size_t fact(size_t val)
{
  return val < 2 ? 1 : val * fact(val - 1);
}

template <typename T>
void swap(T* a, T* b)
{
  T temp = *a;
  *a = *b;
  *b = temp;
}

template <typename T>
bool permute(T* list, size_t size, size_t state)
{
  if (size == 1) return true;
  const size_t sub_size = fact(size - 1);
  const size_t swap_index = state / sub_size;
  const size_t mod = state % sub_size;
  if (swap_index) {
    swap(list, list + swap_index);
  }
  permute(list + 1, size - 1, mod);
  return state + 1 == size * sub_size;
}

template <typename T>
bool unpermute(T* list, size_t size, size_t state)
{
  if (size == 1) return true;
  const size_t sub_size = fact(size - 1);
  const size_t swap_index = state / sub_size;
  const size_t mod = state % sub_size;
  unpermute(list + 1, size - 1, mod);
  if (swap_index) {
    swap(list, list + swap_index);
  }
  return state + 1 == size * sub_size;
}

} // namespace

TEST(dds_DCPS_transport_framework_TransportReassembly, Test_Permutations)
{
  const SequenceNumber msg_seq(2);
  const GUID_t pub_id = create_pub_id();
  const bool debug_logging = false;
  const size_t sample_count = 5;
  const size_t fragments_per_sample = 3;
  const size_t fragment_count = sample_count * fragments_per_sample;

  std::vector<size_t> order_list(sample_count, 0);
  for (size_t i = 0; i < sample_count; ++i) {
    order_list[i] = sample_count - i - 1;
  }

  size_t state = 0;
  bool stop = false;

  while (!stop) {
    permute(&order_list[0], sample_count, state);

    if (debug_logging) {
      std::cout << "permuted order_list[] = { ";
      for (size_t i = 0; i < sample_count; ++i) {
        if (i == 0) {
          std::cout << order_list[i];
        } else {
          std::cout << ", " << order_list[i];
        }
      }
      std::cout << " }" << std::endl;
    }

#ifdef ACE_HAS_CPP11
    uint32_t start_hash = 0;
#endif
    std::vector<Sample_rch> samples;
    for (size_t i = 0; i < sample_count; ++i) {
      if (debug_logging) {
        const size_t sn = i;
        const FragmentNumber fn1 = static_cast<FragmentNumber>(sn * fragments_per_sample + 1);
        const FragmentNumber fn2 = static_cast<FragmentNumber>((sn + 1) * fragments_per_sample);
        std::cout << " - Creating sample " << sn << " with fragments (" << fn1 << "-" << fn2 << ") and 'more_fragments' set to " << ((i + 1) != sample_count ? "true" : "false") << std::endl;
      }
      samples.push_back(make_rch<Sample>(pub_id, msg_seq, (i + 1) != sample_count, 1024 * fragments_per_sample, static_cast<unsigned char>('a' + i)));
      DDS::OctetSeq data = samples.back()->sample.copy_data();
      ASSERT_EQ(data.length(), 1024 * fragments_per_sample);
#ifdef ACE_HAS_CPP11
      start_hash = one_at_a_time_hash(&data[0], 1024 * fragments_per_sample, start_hash);
#endif
      ASSERT_EQ(data[0], 'a' + i);
      ASSERT_EQ(data[1023], 'a' + i);
    }

    TransportReassembly tr;

    for (size_t i = 0; i < sample_count; ++i) {
      const size_t sn = order_list[i];
      const FragmentNumber fn1 = static_cast<FragmentNumber>(sn * fragments_per_sample + 1);
      const FragmentNumber fn2 = static_cast<FragmentNumber>((sn + 1) * fragments_per_sample);
      if (debug_logging) {
        std::cout << " - Inserting fragments (" << fn1 << "-" << fn2 << ")" << std::endl;
      }
      if ((i + 1) != sample_count) {
        EXPECT_FALSE(tr.reassemble(FragmentRange(fn1, fn2), samples[sn]->sample, fragment_count));
      } else {
        EXPECT_TRUE(tr.reassemble(FragmentRange(fn1, fn2), samples[sn]->sample, fragment_count));
        DDS::OctetSeq data = samples[sn]->sample.copy_data();
        ASSERT_EQ(data.length(), 1024 * fragment_count);
#ifdef ACE_HAS_CPP11
        uint32_t end_hash = 0;
#endif
        for (size_t j = 0; j < sample_count; ++j) {
          ASSERT_EQ(data[static_cast<CORBA::ULong>(j * 1024 * fragments_per_sample)], 'a' + j);
          ASSERT_EQ(data[static_cast<CORBA::ULong>((j + 1) * 1024 * fragments_per_sample - 1)], 'a' + j);
#ifdef ACE_HAS_CPP11
          end_hash = one_at_a_time_hash(&data[static_cast<CORBA::ULong>(j * 1024 * fragments_per_sample)], 1024 * fragments_per_sample, end_hash);
#endif
        }
#ifdef ACE_HAS_CPP11
        ASSERT_EQ(end_hash, start_hash);
#endif
      }
    }

    stop = unpermute(&order_list[0], sample_count, state);
    ++state;
  }
}

TEST(dds_DCPS_transport_framework_TransportReassembly, Test_Fill_Overlapping_Inputs)
{
  TransportReassembly tr;
  SequenceNumber msg_seq(2);
  GUID_t pub_id = create_pub_id();
  Sample data1(pub_id, msg_seq, true, 1024 * 3);
  Sample data2(pub_id, msg_seq, false, 1024 * 3);
  Sample data3(pub_id, msg_seq, true, 1024 * 6);

  EXPECT_TRUE(!tr.reassemble(FragmentRange(1, 3), data1.sample, 8)); // 1-3
  EXPECT_TRUE(!tr.reassemble(FragmentRange(6, 8), data2.sample, 8)); // 1-3, 6-8
  EXPECT_TRUE(tr.reassemble(FragmentRange(2, 7), data3.sample, 8)); // 1-8

  Gaps gaps;
  CORBA::ULong base = gaps.get(tr, msg_seq, pub_id);
  EXPECT_EQ(0u, base);
  EXPECT_EQ(0u, gaps.result_bits);
}

TEST(dds_DCPS_transport_framework_TransportReassembly, Test_Fill_Overlapping_Inputs_2)
{
  TransportReassembly tr;
  SequenceNumber msg_seq(2);
  GUID_t pub_id = create_pub_id();
  Sample data1(pub_id, msg_seq, true, 1024 * 1);
  Sample data2(pub_id, msg_seq, true, 1024 * 1);
  Sample data3(pub_id, msg_seq, true, 1024 * 6);
  Sample data4(pub_id, msg_seq, false, 1024 * 1);
  Sample data5(pub_id, msg_seq, true, 1024 * 1);

  EXPECT_TRUE(!tr.reassemble(FragmentRange(3, 3), data1.sample, 8)); // 3-3
  EXPECT_TRUE(!tr.reassemble(FragmentRange(6, 6), data2.sample, 8)); // 3-3, 6-6
  EXPECT_TRUE(!tr.reassemble(FragmentRange(2, 7), data3.sample, 8)); // 2-7
  EXPECT_TRUE(!tr.reassemble(FragmentRange(8, 8), data4.sample, 8)); // 2-8
  EXPECT_TRUE(tr.reassemble(FragmentRange(1, 1), data5.sample, 8)); // 1-8

  Gaps gaps;
  CORBA::ULong base = gaps.get(tr, msg_seq, pub_id);
  EXPECT_EQ(0u, base);
  EXPECT_EQ(0u, gaps.result_bits);
}
