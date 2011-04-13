/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "ace/OS_main.h"

#include "dds/DCPS/DataSampleHeader.h"

#include "../common/TestSupport.h"

using namespace OpenDDS::DCPS;

struct Fragments {
  ACE_Message_Block* head_;
  ACE_Message_Block* tail_;

  ~Fragments()
  {
    ACE_Message_Block::release(head_);
    ACE_Message_Block::release(tail_);
  }
};

int ACE_TMAIN(int, ACE_TCHAR*[])
{
  DataSampleHeader dsh;
  const size_t N = 300;
  dsh.message_length_ = N;
  ACE_Message_Block header_mb(DataSampleHeader::max_marshaled_size());
  header_mb << dsh;

  { // simple case: no content-filter, data in a single messageblock
    ACE_Message_Block data(N);
    data.wr_ptr(N);
    header_mb.cont(&data);
    Fragments f;
    DataSampleHeader::split(header_mb, N / 2 + header_mb.length(),
      f.head_, f.tail_);

    DataSampleHeader header(*f.head_);
    TEST_CHECK(header.more_fragments_);
    TEST_CHECK(header.message_length_ == N / 2);
    TEST_CHECK(f.head_->cont());
    TEST_CHECK(f.head_->cont()->length() == N / 2);
    TEST_CHECK(!f.head_->cont()->cont());

    header = *f.tail_;
    TEST_CHECK(!header.more_fragments_);
    TEST_CHECK(header.message_length_ == N / 2);
    TEST_CHECK(f.tail_->cont());
    TEST_CHECK(f.tail_->cont()->length() == N / 2);
    TEST_CHECK(!f.tail_->cont()->cont());
  }
  { // data split into 3 messageblocks, fragment in the middle of block #2
    ACE_Message_Block data1(N / 3), data2(N / 3), data3(N / 3);
    data1.wr_ptr(N / 3);
    data2.wr_ptr(N / 3);
    data3.wr_ptr(N / 3);
    header_mb.cont(&data1);
    data1.cont(&data2);
    data2.cont(&data3);
    Fragments f;
    DataSampleHeader::split(header_mb, N / 2 + header_mb.length(),
      f.head_, f.tail_);

    DataSampleHeader header(*f.head_);
    TEST_CHECK(header.more_fragments_);
    TEST_CHECK(header.message_length_ == N / 2);
    TEST_CHECK(f.head_->cont());
    TEST_CHECK(f.head_->cont()->length() == N / 3);
    TEST_CHECK(f.head_->cont()->cont()->length() == N / 6);
    TEST_CHECK(!f.head_->cont()->cont()->cont());

    header = *f.tail_;
    TEST_CHECK(!header.more_fragments_);
    TEST_CHECK(header.message_length_ == N / 2);
    TEST_CHECK(f.tail_->cont());
    TEST_CHECK(f.tail_->cont()->length() == N / 6);
    TEST_CHECK(f.tail_->cont()->cont()->length() == N / 3);
    TEST_CHECK(!f.tail_->cont()->cont()->cont());
  }
  { // data split into 4 messageblocks, fragment at the block division
    ACE_Message_Block data1(N / 4), data2(N / 4), data3(N / 4), data4(N / 4);
    data1.wr_ptr(N / 4);
    data2.wr_ptr(N / 4);
    data3.wr_ptr(N / 4);
    data4.wr_ptr(N / 4);
    header_mb.cont(&data1);
    data1.cont(&data2);
    data2.cont(&data3);
    data3.cont(&data4);
    Fragments f;
    DataSampleHeader::split(header_mb, N / 2 + header_mb.length(),
      f.head_, f.tail_);

    DataSampleHeader header(*f.head_);
    TEST_CHECK(header.more_fragments_);
    TEST_CHECK(header.message_length_ == N / 2);
    TEST_CHECK(f.head_->cont());
    TEST_CHECK(f.head_->cont()->length() == N / 4);
    TEST_CHECK(f.head_->cont()->cont()->length() == N / 4);
    TEST_CHECK(!f.head_->cont()->cont()->cont());

    header = *f.tail_;
    TEST_CHECK(!header.more_fragments_);
    TEST_CHECK(header.message_length_ == N / 2);
    TEST_CHECK(f.tail_->cont());
    TEST_CHECK(f.tail_->cont()->length() == N / 4);
    TEST_CHECK(f.tail_->cont()->cont()->length() == N / 4);
    TEST_CHECK(!f.tail_->cont()->cont()->cont());
  }
  { // content filtering flag with no "entries" (adds another MB to the chain)
    ACE_Message_Block data(N);
    data.wr_ptr(N);
    header_mb.cont(&data);
    DataSampleHeader::set_flag(CONTENT_FILTER_FLAG, &header_mb);
    DataSampleHeader::add_cfentries(0, &header_mb);
    Fragments f;
    const size_t FRAG = 100;
    DataSampleHeader::split(header_mb, FRAG, // arbitrary split at 100 bytes
      f.head_, f.tail_);

    DataSampleHeader header(*f.head_);
    TEST_CHECK(header.more_fragments_);
    TEST_CHECK(header.content_filter_);
    TEST_CHECK(header.content_filter_entries_.length() == 0);
    const size_t hdr_len = header_mb.length() + header_mb.cont()->length();
    TEST_CHECK(header.message_length_ == FRAG - hdr_len);
    TEST_CHECK(f.head_->length() == 0); // consumed by DataSampleHeader
    TEST_CHECK(f.head_->cont());
    TEST_CHECK(f.head_->cont()->length() == 0); // consumed by DataSampleHeader
    TEST_CHECK(f.head_->cont()->cont());
    TEST_CHECK(f.head_->cont()->cont()->length() == header.message_length_);
    TEST_CHECK(!f.head_->cont()->cont()->cont());

    header = *f.tail_;
    TEST_CHECK(!header.more_fragments_);
    TEST_CHECK(!header.content_filter_);
    TEST_CHECK(header.message_length_ == N - (FRAG - hdr_len));
    TEST_CHECK(f.tail_->length() == 0); // consumed by DataSampleHeader
    TEST_CHECK(f.tail_->cont());
    TEST_CHECK(f.tail_->cont()->length() == header.message_length_);
    TEST_CHECK(!f.tail_->cont()->cont());
  }
  { // content filtering with some "entries", split inside the entires
    ACE_Message_Block data(N);
    data.wr_ptr(N);
    header_mb.cont(&data);
    DataSampleHeader::set_flag(CONTENT_FILTER_FLAG, &header_mb);
    const size_t CF_ENTRIES = 6; // serializes to 100 bytes
    GUIDSeq entries(CF_ENTRIES);
    entries.length(CF_ENTRIES);
    DataSampleHeader::add_cfentries(&entries, &header_mb);
    Fragments f;
    const size_t FRAG = 68; // MIN_FRAG in TransportSendStrategy.cpp
    DataSampleHeader::split(header_mb, FRAG, f.head_, f.tail_);

    DataSampleHeader header(*f.head_);
    TEST_CHECK(header.more_fragments_);
    TEST_CHECK(header.content_filter_);
    const size_t entries_in_header = header.content_filter_entries_.length();
    TEST_CHECK(entries_in_header > 0);
    TEST_CHECK(header.message_length_ == 0);
    TEST_CHECK(f.head_->length() == 0); // consumed by DataSampleHeader
    TEST_CHECK(f.head_->cont());
    TEST_CHECK(f.head_->cont()->length() == 0); // consumed by DataSampleHeader
    TEST_CHECK(!f.head_->cont()->cont());

    header = *f.tail_;
    TEST_CHECK(!header.more_fragments_);
    TEST_CHECK(header.content_filter_);
    TEST_CHECK(header.content_filter_entries_.length() + entries_in_header == CF_ENTRIES);
    TEST_CHECK(header.message_length_ == N);
    TEST_CHECK(f.tail_->length() == 0); // consumed by DataSampleHeader
    TEST_CHECK(f.tail_->cont());
    TEST_CHECK(f.tail_->cont()->length() == 0); // consumed by DataSampleHeader
    TEST_CHECK(f.tail_->cont()->cont());
    TEST_CHECK(f.tail_->cont()->cont()->length() == N);
    TEST_CHECK(!f.tail_->cont()->cont()->cont());
  }
  return 0;
}
