// -*- C++ -*-
//
#ifndef READER_H
#define READER_H

#include "dds/DdsDcpsSubscriptionC.h"


class Reader
{
public:

  Reader (::DDS::DataReader_ptr reader,
          int use_take = 0,
          int num_reads_per_thread = 1,
          int multiple_instances = 0,
          int reader_id = -1);

  void start ();    // read/take_next_sample
  void start1 ();   // read/take_instance
  void start2 ();   // loan

  long reader_id () const;

private:

  ::DDS::DataReader_var reader_;
  int use_take_;
  int num_reads_per_thread_;
  int multiple_instances_;
  long reader_id_;
};

#endif /* READER_H */
