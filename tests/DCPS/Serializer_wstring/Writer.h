// -*- C++ -*-
//
#ifndef WRITER_H
#define WRITER_H

#include "MessengerC.h"

#include <dds/DdsDcpsPublicationC.h>
#include <dds/DCPS/Atomic.h>

#include <ace/Task.h>

class Writer : public ACE_Task_Base
{
public:

  Writer (::DDS::DataWriter_ptr writer);

  void start ();

  void end ();

  /** Lanch a thread to write. **/
  virtual int svc ();

  void set_message (Messenger::Message& message, int count);

private:
  ::DDS::DataWriter_var writer_;
};

#endif /* WRITER_H */
