// -*- C++ -*-
//
#ifndef WRITER_H
#define WRITER_H

#include <dds/DdsDcpsPublicationC.h>
#include "MessengerTypeSupportC.h"
#include "DataWriterListenerImpl.h"

#include <ace/Condition_T.h>
#include <ace/Synch_Traits.h>
#include <ace/Task.h>

class Writer : public ACE_Task_Base
{
public:

  Writer (::DDS::DataWriter_ptr writer,
          CORBA::Long key,
          ACE_Time_Value sleep_duration);

  void start ();

  void end ();

  /** Lanch a thread to write. **/
  virtual int svc ();

  ::DDS::InstanceHandle_t get_instance_handle();

  ACE_Time_Value get_start_time ();

  bool wait_for_start ();

private:

  ::DDS::DataWriter_var writer_;
  typedef ACE_SYNCH_MUTEX     LockType;
  typedef ACE_Guard<LockType> GuardType;

  LockType lock_;
  ACE_Condition<ACE_SYNCH_MUTEX> condition_;

  bool associated_;
  DataWriterListenerImpl* dwl_servant_;
  ::DDS::InstanceHandle_t instance_handle_;
  CORBA::Long key_;
  ACE_Time_Value sleep_duration_;
};

#endif /* WRITER_H */
