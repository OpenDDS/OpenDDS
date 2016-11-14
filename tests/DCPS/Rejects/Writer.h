// -*- C++ -*-
//
#ifndef WRITER_H
#define WRITER_H

#include <dds/DdsDcpsPublicationC.h>
#include "MessengerTypeSupportC.h"
#include <ace/Task.h>
#include <ace/Synch_Traits.h>
#include <ace/Condition_T.h>

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

  bool wait_for_registered ();

  void start_sending ();

  bool failed_registration() const;
private:

  ::DDS::DataWriter_var writer_;
  typedef ACE_SYNCH_MUTEX     LockType;
  typedef ACE_Guard<LockType> GuardType;

  LockType register_lock_;
  ACE_Condition<ACE_SYNCH_MUTEX> register_condition_;

  LockType sending_lock_;
  ACE_Condition<ACE_SYNCH_MUTEX> sending_condition_;

  bool registered_;
  ::DDS::InstanceHandle_t instance_handle_;
  CORBA::Long key_;
  ACE_Time_Value sleep_duration_;
  bool failed_registration_;
  bool start_sending_;
  bool ready_to_send_;
};

#endif /* WRITER_H */
