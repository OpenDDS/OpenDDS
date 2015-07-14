// -*- C++ -*-
//
#ifndef READER_H
#define READER_H

#include "dds/DdsDcpsSubscriptionC.h"
#include "ace/Task.h"
#include "TestStats.h"


class Reader : public ACE_Task_Base
{
public:

  Reader (::DDS::Subscriber_ptr subscriber,
          ::DDS::DataReader_ptr reader,
          int num_publishers,
          int num_samples,
          int data_size);

  void start ();

  void end ();

  /** Lanch a thread to write. **/
  virtual int svc ();

  /// wait for data to become available
  static int wait_for_data (::DDS::Subscriber_ptr sub,
                            int timeout_sec);

  bool is_finished () const;

private:

  ::DDS::Subscriber_var subscriber_;
  ::DDS::DataReader_var reader_;
  int num_publishers_;
  int num_samples_;
  int data_size_;
  int num_floats_per_sample_;
  bool finished_sending_;

  TestStats stats_;
};

#endif /* READER_H */
