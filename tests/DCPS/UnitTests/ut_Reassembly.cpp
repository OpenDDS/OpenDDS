
#include <ace/OS_main.h>
#include <ace/Log_Msg.h>
#include "../common/TestSupport.h"

#include "dds/DCPS/transport/framework/TransportReassembly.h"
#include "dds/DCPS/RepoIdGenerator.h"

#include <string.h>

using namespace OpenDDS::DCPS;

namespace {
  RepoIdGenerator gen(0, 17, OpenDDS::DCPS::KIND_PUBLISHER);

  RepoId create_pub_id() {
    return gen.next();
  }

  // Class handling assembly of samples with a messge block
  class Sample {
  public:
    Sample(const RepoId& pub_id, const SequenceNumber& msg_seq, bool more_fragments = true)
    : mb(new ACE_Message_Block(DataSampleHeader::max_marshaled_size())),
      sample(mb)
    {
      sample.header_.publication_id_ = pub_id;
      sample.header_.sequence_ = msg_seq;
      sample.header_.more_fragments_ = more_fragments;
      sample.sample_ = new ACE_Message_Block(DataSampleHeader::max_marshaled_size());
    }
    ACE_Message_Block* mb;     // Released (deleted) by sample
    ReceivedDataSample sample;
  };

  enum Constants {
    BM_LENGTH = 8
  };

  class Gaps {
  public:
    Gaps()
    : result_bits(0)
    {
      memset(&bitmap, 0, sizeof(bitmap));
    }

    // Get the gaps in a TransportReassembly instance
    CORBA::ULong get(TransportReassembly& tr, const SequenceNumber& frag_seq, const RepoId& pub_id) {
      memset(&bitmap, 0, sizeof(bitmap));
      base = tr.get_gaps(frag_seq, pub_id, bitmap, bm_length, result_bits);
      return base;
    }

    bool check_gap(CORBA::ULong frag_seq) {
      // How many bits from base
      CORBA::ULong bit_offset = frag_seq - base;
      // What array index is that
      int index = bit_offset / 32;
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

void test_empty()
{
  TransportReassembly tr;
  Gaps gaps;
  SequenceNumber seq(1);
  RepoId pub_id = create_pub_id();
  TEST_ASSERT(!tr.has_frags(seq, pub_id));
  TEST_ASSERT(0 == gaps.get(tr, seq, pub_id));
}

void test_insert_has_frag()
{
  TransportReassembly tr;
  Gaps gaps;
  SequenceNumber msg_seq(4);
  SequenceNumber frag_seq(1);
  RepoId pub_id = create_pub_id();
  Sample data(pub_id, msg_seq);
  tr.reassemble(frag_seq, true, data.sample);
  TEST_ASSERT(tr.has_frags(msg_seq, pub_id));
}

void test_first_insert_has_no_gaps()
{
  TransportReassembly tr;
  Gaps gaps;
  SequenceNumber msg_seq(18);
  SequenceNumber frag_seq(1);
  RepoId pub_id = create_pub_id();
  Sample data(pub_id, msg_seq);

  TEST_ASSERT(!tr.reassemble(frag_seq, true, data.sample));
  CORBA::ULong base = gaps.get(tr, msg_seq, pub_id);

  TEST_ASSERT(tr.has_frags(msg_seq, pub_id));
  TEST_ASSERT(2 == base);             // Now expecting 2
  TEST_ASSERT(1 == gaps.result_bits); // Only one bit
  TEST_ASSERT(gaps.check_gap(2));     // Gap
  TEST_ASSERT(!gaps.check_gap(3));    // No gap
}

void test_insert_gaps()
{
  TransportReassembly tr;
  Gaps gaps;
  SequenceNumber msg_seq(9);
  SequenceNumber frag_seq(4);
  RepoId pub_id = create_pub_id();
  Sample data(pub_id, msg_seq);

  tr.reassemble(frag_seq, true, data.sample);
  CORBA::ULong base = gaps.get(tr, msg_seq, pub_id);

  TEST_ASSERT(1 == base);             // Gap from 1-3
  TEST_ASSERT(3 == gaps.result_bits); // Only one bit
  TEST_ASSERT(gaps.check_gap(1));     // Gap
  TEST_ASSERT(gaps.check_gap(2));     // Gap
  TEST_ASSERT(gaps.check_gap(3));     // Gap
  TEST_ASSERT(!gaps.check_gap(4));    // No gap
}

void test_insert_one_then_gap()
{
  TransportReassembly tr;
  Gaps gaps;
  SequenceNumber msg_seq(17);
  SequenceNumber frag_seq1(1);
  SequenceNumber frag_seq2(6);
  RepoId pub_id = create_pub_id();
  Sample data(pub_id, msg_seq);

  tr.reassemble(frag_seq1, true, data.sample);
  tr.reassemble(frag_seq2, true, data.sample);
  CORBA::ULong base = gaps.get(tr, msg_seq, pub_id);

  TEST_ASSERT(2 == base);             // Gap from 2-5
  TEST_ASSERT(4 == gaps.result_bits); // Number of bits
  TEST_ASSERT(gaps.check_gap(2));     // Gap
  TEST_ASSERT(gaps.check_gap(3));     // Gap
  TEST_ASSERT(gaps.check_gap(4));     // Gap
  TEST_ASSERT(gaps.check_gap(5));     // Gap
  TEST_ASSERT(!gaps.check_gap(6));    // No gap
}

void test_insert_one_then_split_gap()
{
  TransportReassembly tr;
  Gaps gaps;
  SequenceNumber msg_seq(17);
  RepoId pub_id = create_pub_id();
  Sample data(pub_id, msg_seq);

  TEST_ASSERT(!tr.reassemble(1, true, data.sample)); // 1
  TEST_ASSERT(!tr.reassemble(6, true, data.sample)); // 1,6
  CORBA::ULong base = gaps.get(tr, msg_seq, pub_id);

  TEST_ASSERT(2 == base);             // Gap from 2-5
  TEST_ASSERT(4 == gaps.result_bits); // Number of bits
  TEST_ASSERT(gaps.check_gap(2));     // Gap
  TEST_ASSERT(gaps.check_gap(3));     // Gap
  TEST_ASSERT(gaps.check_gap(4));     // Gap
  TEST_ASSERT(gaps.check_gap(5));     // Gap
  TEST_ASSERT(!gaps.check_gap(6));    // No gap

  TEST_ASSERT(!tr.reassemble(4, true, data.sample)); // 1,4,6
  TEST_ASSERT(!tr.reassemble(3, true, data.sample)); // 1,3-4,6
  base = gaps.get(tr, msg_seq, pub_id);

  TEST_ASSERT(2 == base);             // Gap at 2 and 5
  TEST_ASSERT(4 == gaps.result_bits); // Number of bits
  TEST_ASSERT(gaps.check_gap(2));     // Gap
  TEST_ASSERT(!gaps.check_gap(3));    // No gap
  TEST_ASSERT(!gaps.check_gap(4));    // No gap
  TEST_ASSERT(gaps.check_gap(5));     // Gap
  TEST_ASSERT(!gaps.check_gap(6));    // No gap
}

void test_fill_rtol()
{
  TransportReassembly tr;
  Gaps gaps;
  SequenceNumber msg_seq(17);
  RepoId pub_id = create_pub_id();
  Sample data(pub_id, msg_seq);

  TEST_ASSERT(!tr.reassemble(1, true, data.sample)); // 1
  TEST_ASSERT(!tr.reassemble(6, true, data.sample)); // 1,6
  CORBA::ULong base = gaps.get(tr, msg_seq, pub_id);

  TEST_ASSERT(2 == base);             // Gap from 2-5
  TEST_ASSERT(4 == gaps.result_bits); // Number of bits
  TEST_ASSERT(gaps.check_gap(2));     // Gap
  TEST_ASSERT(gaps.check_gap(3));     // Gap
  TEST_ASSERT(gaps.check_gap(4));     // Gap
  TEST_ASSERT(gaps.check_gap(5));     // Gap
  TEST_ASSERT(!gaps.check_gap(6));    // No gap

  TEST_ASSERT(!tr.reassemble(5, true, data.sample)); // 1,5-6
  TEST_ASSERT(!tr.reassemble(4, true, data.sample)); // 1,4-6
  base = gaps.get(tr, msg_seq, pub_id);

  TEST_ASSERT(2 == base);             // Gap from 2-3
  TEST_ASSERT(2 == gaps.result_bits); // Number of bits
  TEST_ASSERT(gaps.check_gap(2));     // Gap
  TEST_ASSERT(gaps.check_gap(3));     // Gap
  TEST_ASSERT(!gaps.check_gap(4));    // No gap
  TEST_ASSERT(!gaps.check_gap(5));    // No gap
  TEST_ASSERT(!gaps.check_gap(6));    // No gap

  TEST_ASSERT(!tr.reassemble(3, true, data.sample)); // 1,3-6
  TEST_ASSERT(!tr.reassemble(2, true, data.sample)); // 1-6
  base = gaps.get(tr, msg_seq, pub_id);

  TEST_ASSERT(7 == base);             // Gap from 2-3
  TEST_ASSERT(1 == gaps.result_bits); // Number of bits
  TEST_ASSERT(gaps.check_gap(7));     // Gap
  TEST_ASSERT(!gaps.check_gap(8));    // No gap
}

void test_fill_ltor()
{
  TransportReassembly tr;
  Gaps gaps;
  SequenceNumber msg_seq(27);
  RepoId pub_id = create_pub_id();
  Sample data(pub_id, msg_seq);

  TEST_ASSERT(!tr.reassemble(1, true, data.sample)); // 1
  TEST_ASSERT(!tr.reassemble(6, true, data.sample)); // 1,6
  CORBA::ULong base = gaps.get(tr, msg_seq, pub_id);

  TEST_ASSERT(2 == base);             // Gap from 2-5
  TEST_ASSERT(4 == gaps.result_bits); // Number of bits
  TEST_ASSERT(gaps.check_gap(2));     // Gap
  TEST_ASSERT(gaps.check_gap(3));     // Gap
  TEST_ASSERT(gaps.check_gap(4));     // Gap
  TEST_ASSERT(gaps.check_gap(5));     // Gap
  TEST_ASSERT(!gaps.check_gap(6));    // No gap

  TEST_ASSERT(!tr.reassemble(2, true, data.sample)); // 1-2,6
  TEST_ASSERT(!tr.reassemble(3, true, data.sample)); // 1-3,6
  base = gaps.get(tr, msg_seq, pub_id);

  TEST_ASSERT(4 == base);             // Gap from 4-5
  TEST_ASSERT(2 == gaps.result_bits); // Number of bits
  TEST_ASSERT(gaps.check_gap(4));     // Gap
  TEST_ASSERT(gaps.check_gap(5));     // Gap
  TEST_ASSERT(!gaps.check_gap(6));    // No gap

  TEST_ASSERT(!tr.reassemble(4, true, data.sample)); // 1-4,6
  TEST_ASSERT(!tr.reassemble(5, true, data.sample)); // 1-6
  base = gaps.get(tr, msg_seq, pub_id);

  TEST_ASSERT(7 == base);             // Gap from 2-3
  TEST_ASSERT(1 == gaps.result_bits); // Number of bits
  TEST_ASSERT(gaps.check_gap(7));     // Gap
  TEST_ASSERT(!gaps.check_gap(8));    // No gap
}

int
ACE_TMAIN(int, ACE_TCHAR*[])
{
  test_empty();
/*
  test_insert_has_frag();
  test_first_insert_has_no_gaps();
  test_insert_gaps();
  test_insert_one_then_gap();
  test_insert_one_then_split_gap();
  test_fill_rtol();
  test_fill_ltor();
*/
  return 0;
}
