#ifndef DATAREADER_LISTENER_IMPL
#define DATAREADER_LISTENER_IMPL

#include <dds/DCPS/LocalObject.h>
#include <dds/DCPS/ConditionVariable.h>
#include <dds/DCPS/TimeDuration.h>

#include <dds/DdsDcpsSubscriptionExtC.h>

#include <ace/Thread_Mutex.h>

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

class DataReaderListenerImpl
  : public virtual OpenDDS::DCPS::LocalObject<OpenDDS::DCPS::DataReaderListener>
{
public:
  DataReaderListenerImpl ();

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

  virtual void on_subscription_disconnected (
    DDS::DataReader_ptr reader,
    const ::OpenDDS::DCPS::SubscriptionDisconnectedStatus & status);

  virtual void on_subscription_reconnected (
    DDS::DataReader_ptr reader,
    const ::OpenDDS::DCPS::SubscriptionReconnectedStatus & status);

  virtual void on_subscription_lost (
    DDS::DataReader_ptr reader,
    const ::OpenDDS::DCPS::SubscriptionLostStatus & status);

  virtual void on_budget_exceeded(
    DDS::DataReader_ptr reader,
    const ::OpenDDS::DCPS::BudgetExceededStatus& status);

  long num_arrived() const {
    return num_arrived_;
  }

  bool wait_matched(
    long count, const OpenDDS::DCPS::TimeDuration& max_wait) const;

  CORBA::Long requested_deadline_total_count (void) const;

protected:

  virtual ~DataReaderListenerImpl (void);

private:

  mutable ACE_Thread_Mutex mutex_;
  mutable OpenDDS::DCPS::ConditionVariable<ACE_Thread_Mutex> matched_condition_;
  long matched_;
  long num_arrived_;
  CORBA::Long requested_deadline_total_count_;
};

#endif /* DATAREADER_LISTENER_IMPL  */
