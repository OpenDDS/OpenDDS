// -*- C++ -*-
//

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
  virtual ~Writer ();
  virtual int svc ();
  bool start ();
  bool end ();
  int get_timeout_writes () const;

private:

  int write (Messenger::MessageDataWriter_ptr message_dw,
             Messenger::Message& message);

private:

  ::DDS::DataWriter_var writer_;

  OpenDDS::DCPS::Atomic<int> timeout_writes_;

  // The lock used to synchronize the two write threads.
  ACE_Thread_Mutex lock_;

  int count_;

  DataWriterListenerImpl* dwl_servant_;
};

#endif /* WRITER_H */
