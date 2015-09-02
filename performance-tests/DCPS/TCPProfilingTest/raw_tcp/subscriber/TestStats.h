#ifndef TESTSTATS_H
#define TESTSTATS_H

#include "../common/TestData.h"
#include <ace/Time_Value.h>
#include <ace/Log_Msg.h>
#include <ace/Message_Block.h>
#include <ace/OS_NS_sys_time.h>
#include <list>

#define TESTSTATS_MAX_PUBLISHERS 256


class TestStats
{
  public:

    typedef std::list<ACE_Message_Block*> MsgList;

    TestStats()
      : num_publishers_(0),
        num_packets_(0),
        expected_packets_(0),
        data_size_(0),
        num_bytes_per_packet_(0),
        received_first_packet_(false),
        packet_count_(0)
    {}

    void init(unsigned num_publishers,
              unsigned num_packets,
              unsigned data_size,
              unsigned num_bytes_per_packet)
    {
      num_publishers_        = num_publishers;
      num_packets_           = num_packets;
      expected_packets_      = num_publishers * num_packets;
      data_size_             = data_size;
      num_bytes_per_packet_  = num_bytes_per_packet;
      received_first_packet_ = false;
      packet_count_          = 0;
      ACE_OS::memset(pub_stats_,0,TESTSTATS_MAX_PUBLISHERS * sizeof(unsigned));
    }

    bool all_packets_received() const
    { return packet_count_ == expected_packets_; }

    void messages_received(MsgList& msg_list)
    {
      // The first thing that needs to be done is to make sure that
      // we record the start time the first time this method is invoked
      // (and only the first time it is invoked).
      if (!received_first_packet_) {
        start_time_ = ACE_OS::gettimeofday();
        received_first_packet_ = true;
      }

      // Let's count the number of elements in the msg_list (each element
      //   is a header/payload pair of Message Blocks and represents one
      //   packet), and, if these are the last packets that we expect,
      //   capture the stop time.
      unsigned int num_msgs = static_cast<unsigned int>(msg_list.size());
      packet_count_ += num_msgs;

      if (all_packets_received()) { stop_time_  = ACE_OS::gettimeofday(); }

      // Iterate over the collection of "packets", record some statistics
      // by peeking at various fields within the Header, and release each
      // block.
      TestStats::MsgList::iterator itr;
      TestStats::MsgList::iterator endItr = msg_list.end();

      for (itr = msg_list.begin(); itr != endItr; ++itr) {
        ACE_Message_Block* header_block = *itr;
        TestData::Header* header = (TestData::Header*)(header_block->rd_ptr());

        unsigned index = header->publisher_id_;

        // Increment the appropriate publisher packet count.
        ++pub_stats_[index];

        // Release the "packet"
        (*itr)->release();
      }

      // Since we released each block, we better clear the list so that
      // the caller doesn't blow-up if he tries to fiddle with the list.
      msg_list.clear();
    }

    bool is_num_packets(unsigned count) const
    { return count == num_packets_; }

    void dump() const
    {
      ACE_Time_Value atv = stop_time_ - start_time_;

      double total_time = (atv.sec() * 1000000.0) + atv.usec();
      double packet_time = total_time / expected_packets_;

#if 0
      ACE_DEBUG((LM_DEBUG,
                 "(%P|%t) Results: [%d:%d:%d:%d:%d:%d:%f:%f]\n",
                 num_publishers_,
                 num_packets_,
                 expected_packets_,
                 packet_count_,
                 data_size_,
                 num_bytes_per_packet_,
                 total_time,
                 packet_time));

#else
// More verbose version below
      ACE_DEBUG((LM_DEBUG,
                 "(%P|%t) Test Results:\n"
                 "[Num Publishers           : %d]\n"
                 "[Num Packets per Publisher: %d]\n"
                 "[Expected Num Packets     : %d]\n"
                 "[Num Packets Received     : %d]\n"
                 "[Data Size  (2^x)         : %d]\n"
                 "[Num Bytes Per Packet     : %d]\n"
                 "[Total Time (microseconds): %f]\n"
                 "[Per Packet (microseconds): %f]\n",
                 num_publishers_,
                 num_packets_,
                 expected_packets_,
                 packet_count_,
                 data_size_,
                 num_bytes_per_packet_,
                 total_time,
                 packet_time));
#endif
    }


    unsigned num_publishers_;
    unsigned num_packets_;
    unsigned expected_packets_;
    unsigned data_size_;
    unsigned num_bytes_per_packet_;


  private:

    bool     received_first_packet_;
    unsigned packet_count_;

    unsigned pub_stats_[TESTSTATS_MAX_PUBLISHERS];

    ACE_Time_Value start_time_;
    ACE_Time_Value stop_time_;
};

#endif
