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

  void start ();

  void end ();

  /** Lanch a thread to write. **/
  virtual int svc ();

  bool is_finished () const;

  int get_timeout_writes () const;


private:

  int write(Messenger::MessageDataWriter_ptr message_dw,
            ::DDS::InstanceHandle_t& handle,
            Messenger::Message& message,
            int num_messages);

  ::DDS::DataWriter_var writer_;
  ACE_Atomic_Op<ACE_SYNCH_MUTEX, int> finished_instances_;
  ACE_Atomic_Op<ACE_SYNCH_MUTEX, int> timeout_writes_;

  // The lock used to synchronize the two write threads.
  ACE_Thread_Mutex lock_;
  // The flag used to synchronize the two write threads.
  bool start_;
  int count_;
  DataWriterListenerImpl* dwl_servant_;
};

#endif /* WRITER_H */
