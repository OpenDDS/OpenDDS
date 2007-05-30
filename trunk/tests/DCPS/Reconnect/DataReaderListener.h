// -*- C++ -*-
//
// $Id$
#ifndef DATAREADER_LISTENER_IMPL
#define DATAREADER_LISTENER_IMPL

#include <dds/DdsDcpsSubscriptionS.h>

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */


//Class DataReaderListenerImpl
class DataReaderListenerImpl
  // note: TAO specific extensions
  : public virtual TAO::DCPS::LocalObject<TAO::DCPS::DataReaderListener>
{
public:

  //Constructor
  DataReaderListenerImpl ();

  //Destructor
  virtual ~DataReaderListenerImpl (void);

  virtual void on_requested_deadline_missed (
    DDS::DataReader_ptr reader,
    const DDS::RequestedDeadlineMissedStatus & status)
    throw (CORBA::SystemException);

  virtual void on_requested_incompatible_qos (
    DDS::DataReader_ptr reader,
    const DDS::RequestedIncompatibleQosStatus & status)
  throw (CORBA::SystemException);

  virtual void on_liveliness_changed (
    DDS::DataReader_ptr reader,
    const DDS::LivelinessChangedStatus & status)
  throw (CORBA::SystemException);

  virtual void on_subscription_match (
    DDS::DataReader_ptr reader,
    const DDS::SubscriptionMatchStatus & status
  )
  throw (CORBA::SystemException);

  virtual void on_sample_rejected(
    DDS::DataReader_ptr reader,
    const DDS::SampleRejectedStatus& status
  )
  throw (CORBA::SystemException);

  virtual void on_data_available(
    DDS::DataReader_ptr reader
  )
  throw (CORBA::SystemException);

  virtual void on_sample_lost(
    DDS::DataReader_ptr reader,
    const DDS::SampleLostStatus& status
  )
  throw (CORBA::SystemException);

  virtual void on_subscription_disconnected (
    DDS::DataReader_ptr reader,
    const ::TAO::DCPS::SubscriptionDisconnectedStatus & status
  )
  throw (CORBA::SystemException);

  virtual void on_subscription_reconnected (
    DDS::DataReader_ptr reader,
    const ::TAO::DCPS::SubscriptionReconnectedStatus & status
  )
  throw (CORBA::SystemException);

  virtual void on_subscription_lost (
    DDS::DataReader_ptr reader,
    const ::TAO::DCPS::SubscriptionLostStatus & status
  )
  throw (CORBA::SystemException);

  virtual void on_connection_deleted (
    DDS::DataReader_ptr reader
  )
  throw (CORBA::SystemException);

  long num_reads() const {
    return num_reads_;
  }

private:

  DDS::DataReader_var reader_;
  int                 num_reads_;
};

#endif /* DATAREADER_LISTENER_IMPL  */
