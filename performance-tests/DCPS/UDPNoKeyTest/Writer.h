// -*- C++ -*-
//
#ifndef WRITER_H
#define WRITER_H

#include "dds/DdsDcpsPublicationC.h"
#include "ace/Task.h"


class Writer : public ACE_Task_Base
{
public:

  Writer (::DDS::DataWriter_ptr writer,
          int num_messages = 1,
          int size_messages = 128,
          int writer_id = -1,
          int num_readers = 1,
          unsigned throttle_factor = 0);

  void start ();

  void end ();

  /** Lanch a thread to write. **/
  virtual int svc ();

  long writer_id () const;

  bool is_finished () const;

private:

  ::DDS::DataWriter_var writer_;
  int num_messages_;
  int data_size_;
  long writer_id_;
  int num_readers_;
  bool finished_sending_;
  unsigned throttle_factor_;
};

#endif /* WRITER_H */
