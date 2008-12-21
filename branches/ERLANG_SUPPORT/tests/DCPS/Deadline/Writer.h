// -*- C++ -*-
//
// $Id$
#ifndef WRITER_H
#define WRITER_H

#include <dds/DdsDcpsPublicationC.h>
#include "MessengerTypeSupportC.h"
#include "DataWriterListenerImpl.h"
#include <ace/Task.h>


class Writer : public ACE_Task_Base
{
public:

  Writer (::DDS::DataWriter_ptr writer);

  void start ();

  void end ();

  /** Lanch a thread to write. **/
  virtual int svc ();

private:

  ::DDS::DataWriter_var writer_;

  // The lock used to synchronize the two write threads.
  ACE_Thread_Mutex lock_;
  // The flag used to synchronize the two write threads.
  bool start_;
  DataWriterListenerImpl* dwl_servant_;
};

#endif /* WRITER_H */
