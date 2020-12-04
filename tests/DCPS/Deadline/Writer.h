// -*- C++ -*-
//
#ifndef WRITER_H
#define WRITER_H

#include "MessengerTypeSupportC.h"
#include "DataWriterListenerImpl.h"

#include <dds/DdsDcpsPublicationC.h>
#include <dds/DCPS/TimeTypes.h>
#include <dds/DCPS/ConditionVariable.h>

#include <ace/Synch_Traits.h>
#include <ace/Task.h>

class Writer : public ACE_Task_Base
{
public:

  Writer(::DDS::DataWriter_ptr writer,
         CORBA::Long key,
         TimeDuration sleep_duration);

  void start ();

  void end ();

  /** Lanch a thread to write. **/
  virtual int svc ();

  ::DDS::InstanceHandle_t get_instance_handle();

  bool wait_for_start ();

private:

  ::DDS::DataWriter_var writer_;
  typedef ACE_SYNCH_MUTEX     LockType;
  typedef ACE_Guard<LockType> GuardType;

  LockType lock_;
  OpenDDS::DCPS::ConditionVariable<ACE_SYNCH_MUTEX> condition_;

  bool associated_;
  DataWriterListenerImpl* dwl_servant_;
  ::DDS::InstanceHandle_t instance_handle_;
  CORBA::Long key_;
  TimeDuration sleep_duration_;
};

#endif /* WRITER_H */
