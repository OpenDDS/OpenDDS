// -*- C++ -*-
//
#ifndef DATAREADER_LISTENER_IMPL_H
#define DATAREADER_LISTENER_IMPL_H

#include "dds/DdsDcpsSubscriptionExtC.h"
#include "dds/DCPS/Definitions.h"
#include "dds/DCPS/LocalObject.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

class DataReaderListenerImpl
  : public virtual OpenDDS::DCPS::LocalObject<OpenDDS::DCPS::DataReaderListener>
{
public:
  DataReaderListenerImpl();
  virtual ~DataReaderListenerImpl();

  virtual void on_liveliness_changed(DDS::DataReader_ptr reader, const DDS::LivelinessChangedStatus & status);
  virtual void on_data_available(DDS::DataReader_ptr reader);

  // Debug info only
  virtual void on_requested_deadline_missed(DDS::DataReader_ptr, const DDS::RequestedDeadlineMissedStatus&);
  virtual void on_requested_incompatible_qos(DDS::DataReader_ptr, const DDS::RequestedIncompatibleQosStatus&);
  virtual void on_subscription_matched(DDS::DataReader_ptr, const DDS::SubscriptionMatchedStatus&);
  virtual void on_sample_rejected(DDS::DataReader_ptr, const DDS::SampleRejectedStatus&);

  // No-op implementations
  virtual void on_sample_lost(DDS::DataReader_ptr, const DDS::SampleLostStatus&) {}
  virtual void on_subscription_disconnected(DDS::DataReader_ptr, const OpenDDS::DCPS::SubscriptionDisconnectedStatus&) {}
  virtual void on_subscription_reconnected(DDS::DataReader_ptr, const OpenDDS::DCPS::SubscriptionReconnectedStatus&) {}
  virtual void on_subscription_lost(DDS::DataReader_ptr, const OpenDDS::DCPS::SubscriptionLostStatus&) {}
  virtual void on_budget_exceeded(DDS::DataReader_ptr, const OpenDDS::DCPS::BudgetExceededStatus&) {}

  unsigned int deadline_missed() const { return deadline_missed_; }
private:
  unsigned int deadline_missed_;
};

#endif /* DATAREADER_LISTENER_IMPL_H  */
