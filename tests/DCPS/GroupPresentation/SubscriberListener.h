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

  bool verify_result () const {
    return verify_result_;
  }

private:

  void verify (const Messenger::Message& msg,
               const ::DDS::SampleInfo& si,
               const DDS::SubscriberQos& qos,
               const bool reset_last_timestamp);

  DDS::Subscriber_var subscriber_;
  bool  verify_result_;
};

#endif /* SUBSCRIBER_LISTENER_IMPL  */
