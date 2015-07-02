// -*- C++ -*-
//
#ifndef WRITER_H
#define WRITER_H

#include <dds/DdsDcpsPublicationC.h>
#include <ace/Task.h>
#include "MessengerC.h"


class Writer : public ACE_Task_Base
{
public:

  Writer (::DDS::DataWriter_ptr writer);

  void start ();

  void end ();

  /** Lanch a thread to write. **/
  virtual int svc ();

  bool is_finished () const;

  int get_timeout_writes () const;

  void set_message (Messenger::Message& message, const int& count);


private:

  ::DDS::DataWriter_var writer_;
  ACE_Atomic_Op<ACE_SYNCH_MUTEX, int> finished_instances_;
  ACE_Atomic_Op<ACE_SYNCH_MUTEX, int> timeout_writes_;
};

#endif /* WRITER_H */
