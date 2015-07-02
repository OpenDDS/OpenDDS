// -*- C++ -*-
//
#ifndef WRITER_H
#define WRITER_H

#include "dds/DdsDcpsPublicationC.h"
#include "ace/Task.h"


class Writer
{
public:

  Writer (::DDS::DataWriter_ptr writer,
          int num_thread_to_write = 1,
          int num_writes_per_thread = 100);

  void start ();

  void end ();

  int run_test (int pass);

  bool is_finished () const;

private:

  ::DDS::DataWriter_var writer_;
  int num_thread_to_write_;
  int num_writes_per_thread_;
  int multiple_instances_;
  bool finished_sending_;
};

#endif /* WRITER_H */
