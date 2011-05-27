// -*- C++ -*-
//
// $Id$
#ifndef DATAREADER_LISTENER_IMPL
#define DATAREADER_LISTENER_IMPL

#include "dds/DdsDcpsSubscriptionExtS.h"
#include "dds/DCPS/Definitions.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */


//Class DataReaderListenerImpl
class DataReaderListenerImpl
  : public virtual OpenDDS::DCPS::LocalObject<DDS::DataReaderListener>
{
public:
  //Constructor
  DataReaderListenerImpl ();

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

  virtual void on_subscription_matched (
    ::DDS::DataReader_ptr reader,
    const ::DDS::SubscriptionMatchedStatus & status
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

  ::DDS::DataReader_var reader_;
};

#endif /* DATAREADER_LISTENER_IMPL  */
