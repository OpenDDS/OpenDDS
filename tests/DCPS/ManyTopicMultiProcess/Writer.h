// -*- C++ -*-
//
#ifndef WRITER_H
#define WRITER_H

#include "dds/DdsDcpsPublicationC.h"
#include "ace/Task.h"


class Writer : public ACE_Task_Base
{
public:

  explicit Writer(::DDS::DataWriter_ptr writer,
                  int num_thread_to_write = 1,
                  int num_writes_per_thread = 100);

  void start();

  /** Lanch a thread to write. **/
  virtual int svc();

  bool is_finished() const;

private:

  void rsleep(const int wait = 100000); // microseconds

  ::DDS::DataWriter_var writer_;
  int num_thread_to_write_;
  int num_writes_per_thread_;
  bool finished_sending_;
  int max_wait_;
};

#endif /* WRITER_H */
