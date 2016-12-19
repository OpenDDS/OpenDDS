// -*- C++ -*-
// *******************************************************************
//
// (c) Copyright 2006, Object Computing, Inc.
// All Rights Reserved.
//
// *******************************************************************

#ifndef EXCHANGE_EVENT_DATAREADER_LISTENER_IMPL
#define EXCHANGE_EVENT_DATAREADER_LISTENER_IMPL

#include <dds/DdsDcpsSubscriptionC.h>
#include <ace/Mutex.h>
#include <dds/DCPS/LocalObject.h>

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */


class ExchangeEventDataReaderListenerImpl
  : public virtual OpenDDS::DCPS::LocalObject<DDS::DataReaderListener>
{
public:
  ExchangeEventDataReaderListenerImpl ();

  virtual ~ExchangeEventDataReaderListenerImpl (void);

  // app-specific
  CORBA::Boolean is_exchange_closed_received();

  virtual void on_requested_deadline_missed (
    DDS::DataReader_ptr reader,
    const DDS::RequestedDeadlineMissedStatus & status);

  virtual void on_requested_incompatible_qos (
    DDS::DataReader_ptr reader,
    const DDS::RequestedIncompatibleQosStatus & status);

  virtual void on_liveliness_changed (
    DDS::DataReader_ptr reader,
    const DDS::LivelinessChangedStatus & status);

  virtual void on_subscription_matched (
    DDS::DataReader_ptr reader,
    const DDS::SubscriptionMatchedStatus & status);

  virtual void on_sample_rejected(
    DDS::DataReader_ptr reader,
    const DDS::SampleRejectedStatus& status);

  virtual void on_data_available(
    DDS::DataReader_ptr reader);

  virtual void on_sample_lost(
    DDS::DataReader_ptr reader,
    const DDS::SampleLostStatus& status);

private:
  CORBA::Boolean is_exchange_closed_received_;
  ACE_Mutex lock_;
};

#endif /* EXCHANGE_EVENT_DATAREADER_LISTENER_IMPL  */
