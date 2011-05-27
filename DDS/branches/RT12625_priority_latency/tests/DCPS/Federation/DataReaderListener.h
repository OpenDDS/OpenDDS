// -*- C++ -*-
//
// $Id$
#ifndef DATAREADERLISTENER_H
#define DATAREADERLISTENER_H

#include "dds/DdsDcpsSubscriptionExtS.h"
#include "dds/DCPS/PublisherImpl.h"
#include "dds/DCPS/Service_Participant.h"
#include "ace/Condition_T.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

class DataReaderListenerImpl
  : public virtual OpenDDS::DCPS::LocalObject<DDS::DataReaderListener>
{
public:
  //Constructor
  DataReaderListenerImpl( int expected);

  //Destructor
  virtual ~DataReaderListenerImpl();

  /// Wait for signal from the receiving thread executing on_data_available().
  void waitForCompletion();

  virtual void on_requested_deadline_missed (
    ::DDS::DataReader_ptr reader,
    const ::DDS::RequestedDeadlineMissedStatus & status
  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
  ));

 virtual void on_requested_incompatible_qos (
    ::DDS::DataReader_ptr reader,
    const ::DDS::RequestedIncompatibleQosStatus & status
  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
  ));

  virtual void on_liveliness_changed (
    ::DDS::DataReader_ptr reader,
    const ::DDS::LivelinessChangedStatus & status
  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
  ));

  virtual void on_subscription_match (
    ::DDS::DataReader_ptr reader,
    const ::DDS::SubscriptionMatchStatus & status
  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
  ));

  virtual void on_sample_rejected(
    ::DDS::DataReader_ptr reader,
    const DDS::SampleRejectedStatus& status
  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
  ));

  virtual void on_data_available(
    ::DDS::DataReader_ptr reader
  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
  ));

  virtual void on_sample_lost(
    ::DDS::DataReader_ptr reader,
    const DDS::SampleLostStatus& status
  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
  ));

private:
  /// Number samples received.
  unsigned int samples_;

  /// Number of samples expected.
  unsigned int expected_;

  /// Lock used by the condition below.
  ACE_SYNCH_MUTEX lock_;

  /// Condition used to signal main processing loop when complete.
  ACE_Condition<ACE_SYNCH_MUTEX> condition_;
};

#endif /* DATAREADERLISTENER_H  */

