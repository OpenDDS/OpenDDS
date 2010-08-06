#ifndef TESTSTATS_H
#define TESTSTATS_H

#include <ace/Time_Value.h>
#include <ace/Log_Msg.h>


class TestStats
{
  public:

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
    }

    bool all_packets_received() const
    { return packet_count_ == expected_packets_; }

    void samples_received(unsigned long num_received)
    {
      // The first thing that needs to be done is to make sure that
      // we record the start time the first time this method is invoked
      // (and only the first time it is invoked).
      if (!received_first_packet_)
        {
          start_time_ = ACE_OS::gettimeofday();
          received_first_packet_ = true;
        }

      packet_count_ += num_received;
      if (all_packets_received())
        {
          stop_time_  = ACE_OS::gettimeofday();
        }
    }

    void finished (void)
    {
      // Called to manually inform the stats that the test is done.
      if (ACE_Time_Value::zero == stop_time_)
        {
          stop_time_  = ACE_OS::gettimeofday();
        }
    }


    bool is_num_packets(unsigned count) const
    { return count == num_packets_; }

    void dump() const
    {
      ACE_Time_Value atv = stop_time_ - start_time_;

      double total_time = (atv.sec() * 1000000.0) + atv.usec();
      double packet_time = total_time / expected_packets_;

      ACE_DEBUG((LM_DEBUG,
                 "(%P|%t) Results: [%d:%d:%d:%d:%d:%d:%d:%f:%f]\n",
                 num_publishers_,
                 num_packets_,
                 expected_packets_,
                 packet_count_,
                 data_size_,
                 1 << data_size_,
                 num_bytes_per_packet_,
                 total_time,
                 packet_time));

      double received_packet_time = total_time / packet_count_;
      double percent_dropped = (expected_packets_ - packet_count_) * 100.0L
                               / expected_packets_;
      ACE_DEBUG((LM_DEBUG, "usecs/recvd %f usecs/expected "
                           "%f ratio %f %% dropped %f\n",
                           received_packet_time,
                           packet_time,
                           received_packet_time /packet_time,
                           percent_dropped));

// More verbose version below
#if 0
      ACE_DEBUG((LM_DEBUG,
                 "(%P|%t) Test Results:\n"
                 "[Num Publishers           : %d]\n"
                 "[Num Packets per Publisher: %d]\n"
                 "[Expected Num Packets     : %d]\n"
                 "[Num Packets Received     : %d]\n"
                 "[Data Size                : %d]\n"
                 "[Num Floats in Packet     : %d]\n"
                 "[Num Bytes Per Packet     : %d]\n"
                 "[Total Time (microseconds): %f]\n"
                 "[Per Packet (microseconds): %f]\n",
                 num_publishers_,
                 num_packets_,
                 expected_packets_,
                 packet_count_,
                 data_size_,
                 1 << data_size_,
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

    ACE_Time_Value start_time_;
    ACE_Time_Value stop_time_;
};

#endif
