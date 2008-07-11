// -*- C++ -*-
//
// $Id$
#ifndef DATAREADER_LISTENER_IMPL
#define DATAREADER_LISTENER_IMPL

#include "dds/DdsDcpsSubscriptionS.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */


//Class DataReaderListenerImpl
class DataReaderListenerImpl
  : public virtual OpenDDS::DCPS::LocalObject<OpenDDS::DCPS::DataReaderListener>
{
public:
  //Constructor
  DataReaderListenerImpl (void);

  //Destructor
  virtual ~DataReaderListenerImpl (void);

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

  virtual void on_subscription_disconnected (
      ::DDS::DataReader_ptr reader,
      const ::OpenDDS::DCPS::SubscriptionDisconnectedStatus & status
    )
    ACE_THROW_SPEC ((
      CORBA::SystemException
    ));

  virtual void on_subscription_reconnected (
      ::DDS::DataReader_ptr reader,
      const ::OpenDDS::DCPS::SubscriptionReconnectedStatus & status
    )
    ACE_THROW_SPEC ((
      CORBA::SystemException
    ));

  virtual void on_subscription_lost (
      ::DDS::DataReader_ptr reader,
      const ::OpenDDS::DCPS::SubscriptionLostStatus & status
    )
    ACE_THROW_SPEC ((
      CORBA::SystemException
    ));

  virtual void on_connection_deleted (
    ::DDS::DataReader_ptr
    )
    ACE_THROW_SPEC ((
    ::CORBA::SystemException
    ));

  unsigned int deadline_missed() const
  {
    return deadline_missed_;
  }

private:
  unsigned int deadline_missed_;
  ::DDS::SampleInfo last_si_ ;
};

#endif /* DATAREADER_LISTENER_IMPL  */
