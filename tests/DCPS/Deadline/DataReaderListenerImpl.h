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
  DataReaderListenerImpl();

  bool wait_matched(int count, const OpenDDS::DCPS::TimeDuration& max_wait) const;
  virtual void on_subscription_matched(DDS::DataReader_ptr reader, const DDS::SubscriptionMatchedStatus& status);

  int wait_all_received() const;
  virtual void on_data_available(DDS::DataReader_ptr reader);

  CORBA::Long requested_deadline_total_count() const;
  virtual void on_requested_deadline_missed(DDS::DataReader_ptr reader, const DDS::RequestedDeadlineMissedStatus& status);

  virtual void on_requested_incompatible_qos(DDS::DataReader_ptr reader, const DDS::RequestedIncompatibleQosStatus& status);
  virtual void on_liveliness_changed(DDS::DataReader_ptr reader, const DDS::LivelinessChangedStatus& status);
  virtual void on_sample_rejected(DDS::DataReader_ptr reader, const DDS::SampleRejectedStatus& status);
  virtual void on_sample_lost(DDS::DataReader_ptr reader, const DDS::SampleLostStatus& status);
  virtual void on_subscription_disconnected(DDS::DataReader_ptr reader, const OpenDDS::DCPS::SubscriptionDisconnectedStatus& status);
  virtual void on_subscription_reconnected(DDS::DataReader_ptr reader, const OpenDDS::DCPS::SubscriptionReconnectedStatus& status);
  virtual void on_subscription_lost(DDS::DataReader_ptr reader, const OpenDDS::DCPS::SubscriptionLostStatus& status);
  virtual void on_budget_exceeded(DDS::DataReader_ptr reader, const OpenDDS::DCPS::BudgetExceededStatus& status);

protected:
  virtual ~DataReaderListenerImpl();

private:
  typedef ACE_Thread_Mutex Mutex;
  typedef ACE_Guard<Mutex> Lock;
  typedef OpenDDS::DCPS::ConditionVariable<Mutex> CV;
  bool all_received();

  mutable Mutex matched_mutex_;
  mutable CV matched_cv_;
  int matched_;
  mutable Mutex received_mutex_;
  mutable CV received_cv_;
  int received_;
  mutable Mutex count_mutex_;
  CORBA::Long requested_deadline_total_count_;
};

#endif /* DATAREADER_LISTENER_IMPL  */
