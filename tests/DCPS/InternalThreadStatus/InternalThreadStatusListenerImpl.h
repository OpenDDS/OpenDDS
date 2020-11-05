/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef INTERNAL_THREAD_STATUS_LISTENER_IMPL
#define INTERNAL_THREAD_STATUS_LISTENER_IMPL

#include <dds/DdsDcpsSubscriptionC.h>
#include <dds/DCPS/LocalObject.h>
#include <dds/DCPS/GuidUtils.h>

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

typedef void (*callback_t)();

class InternalThreadStatusListenerImpl
  : public virtual OpenDDS::DCPS::LocalObject<DDS::DataReaderListener>
{
public:
  //Constructor
  explicit InternalThreadStatusListenerImpl(OPENDDS_STRING id);

  //Destructor
  virtual ~InternalThreadStatusListenerImpl();

  virtual void on_requested_deadline_missed(
                                            DDS::DataReader_ptr reader,
                                            const DDS::RequestedDeadlineMissedStatus & status);

  virtual void on_requested_incompatible_qos(
                                             DDS::DataReader_ptr reader,
                                             const DDS::RequestedIncompatibleQosStatus & status);

  virtual void on_liveliness_changed(
                                     DDS::DataReader_ptr reader,
                                     const DDS::LivelinessChangedStatus & status);

  virtual void on_subscription_matched(
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

  int get_count() const;
private:

  OPENDDS_STRING id_;
  int count_;

};

#endif /* INTERNAL_THREAD_STATUS_LISTENER_IMPL  */
