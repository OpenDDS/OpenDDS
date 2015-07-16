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
  ACE_Atomic_Op<ACE_SYNCH_MUTEX, int> finished_instances_;
  ACE_Atomic_Op<ACE_SYNCH_MUTEX, int> timeout_writes_;
};

#endif /* WRITER_H */
