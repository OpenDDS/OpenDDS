// -*- C++ -*-
//
#ifndef WRITER_H
#define WRITER_H

#include "dds/DdsDcpsSubscriptionC.h"

class Writer
{
public:

  Writer (::DDS::DataReader_ptr reader,
          int num_writes_per_thread = 1,
          int multiple_instances = 0,
          int instance_id = 0);

  void start ();

private:

  int num_writes_per_thread_;
  int multiple_instances_;
  ::DDS::DataReader_ptr reader_ ;
};

#endif /* READER_H */
