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
  bool start (int i_threads, int i_msg_cnt, int i_subject_1st);
  bool end ();
  int get_timeout_writes () const;

  int write_loop(int i_subject, char const *pc_from, char const *pc_subj, char const *pc_text, int i_msgs);

  int set_count(int i_count);

private:

  ::DDS::ReturnCode_t write_one(Messenger::MessageDataWriter_ptr message_dw, ::DDS::InstanceHandle_t& handle, Messenger::Message& message, int i_count);

private:

  ::DDS::DataWriter_var writer_;

  ACE_Atomic_Op<ACE_SYNCH_MUTEX, int> timeout_writes_;

  // The lock used to synchronize the two write threads.
  ACE_Thread_Mutex lock_;

  int count_;

  DataWriterListenerImpl* dwl_servant_;
};

#endif /* WRITER_H */
