/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef SUBSCRIBER_LISTENER_IMPL
#define SUBSCRIBER_LISTENER_IMPL

#include <dds/DdsDcpsSubscriptionC.h>
#include "MessengerC.h"
#include <ace/Thread_Mutex.h>

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

class SubscriberListenerImpl
  : public virtual OpenDDS::DCPS::LocalObject<DDS::SubscriberListener> {

public:
  SubscriberListenerImpl();

  virtual ~SubscriberListenerImpl();

  virtual void on_data_on_readers(
    DDS::Subscriber_ptr subs);

  virtual void on_requested_deadline_missed(
    DDS::DataReader_ptr reader,
    const DDS::RequestedDeadlineMissedStatus& status);

  virtual void on_requested_incompatible_qos(
    DDS::DataReader_ptr reader,
    const DDS::RequestedIncompatibleQosStatus& status);

  virtual void on_liveliness_changed(
    DDS::DataReader_ptr reader,
    const DDS::LivelinessChangedStatus& status);

  virtual void on_subscription_matched(
    DDS::DataReader_ptr reader,
    const DDS::SubscriptionMatchedStatus& status);

  virtual void on_sample_rejected(
    DDS::DataReader_ptr reader,
    const DDS::SampleRejectedStatus& status);

  virtual void on_data_available(
    DDS::DataReader_ptr reader);

  virtual void on_sample_lost(
    DDS::DataReader_ptr reader,
    const DDS::SampleLostStatus& status);

  bool verify_result() const {
    ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
    return verify_result_;
  }

  long num_valid_data() const {
    ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
    return num_valid_data_;
  }

  bool wait_valid_data(long expected) const {
    ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
    OpenDDS::DCPS::ThreadStatusManager& thread_status_manager = TheServiceParticipant->get_thread_status_manager();
    while (num_valid_data_ < expected) {
      cond_.wait(thread_status_manager);
    }
    return num_valid_data_ >= expected;
  }

private:

  void verify(const Messenger::Message& msg,
              const ::DDS::SampleInfo& si,
              const DDS::SubscriberQos& qos,
              const bool reset_last_timestamp);

  mutable ACE_Thread_Mutex mutex_;
  mutable OpenDDS::DCPS::ConditionVariable<ACE_Thread_Mutex> cond_;
  DDS::Subscriber_var subscriber_;
  long num_valid_data_;
  bool verify_result_;
};

#endif /* SUBSCRIBER_LISTENER_IMPL  */
