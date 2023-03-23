// -*- C++ -*-
//
#ifndef WRITER_H
#define WRITER_H

#include <dds/DdsDcpsPublicationC.h>
#include <dds/DCPS/Atomic.h>

#include <ace/Task.h>

class Writer : public ACE_Task_Base
{
public:

  Writer (::DDS::DataWriter_ptr writer,
          int writer_id);

  void start ();

  void end ();

  /** Lanch a thread to write. **/
  virtual int svc ();

  long writer_id () const;

  bool is_finished () const;

  int get_timeout_writes () const;


private:

  ::DDS::DataWriter_var writer_;
  long writer_id_;
  OpenDDS::DCPS::Atomic<int> finished_instances_;
  OpenDDS::DCPS::Atomic<int> timeout_writes_;
};

#endif /* WRITER_H */
