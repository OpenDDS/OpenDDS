/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef INTERNAL_THREAD_STATUS_LISTENER_IMPL
#define INTERNAL_THREAD_STATUS_LISTENER_IMPL

#include <dds/DCPS/LocalObject.h>
#include <dds/DCPS/GuidUtils.h>

#include <dds/DdsDcpsSubscriptionC.h>

#include <tests/Utils/DistributedConditionSet.h>

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

const char DISPOSE_RECEIVED[] = "DISPOSED_RECEIVED";

class InternalThreadStatusListenerImpl
  : public virtual OpenDDS::DCPS::LocalObject<DDS::DataReaderListener>
{
public:
  //Constructor
  InternalThreadStatusListenerImpl(const OpenDDS::DCPS::String& id,
                                   DistributedConditionSet_rch dcs);

  //Destructor
  virtual ~InternalThreadStatusListenerImpl();

  virtual void on_requested_deadline_missed(DDS::DataReader_ptr reader,
                                            const DDS::RequestedDeadlineMissedStatus & status);

  virtual void on_requested_incompatible_qos(DDS::DataReader_ptr reader,
                                             const DDS::RequestedIncompatibleQosStatus & status);

  virtual void on_liveliness_changed(DDS::DataReader_ptr reader,
                                     const DDS::LivelinessChangedStatus & status);

  virtual void on_subscription_matched(DDS::DataReader_ptr reader,
                                       const DDS::SubscriptionMatchedStatus & status);

  virtual void on_sample_rejected(DDS::DataReader_ptr reader,
                                  const DDS::SampleRejectedStatus& status);

  virtual void on_data_available(DDS::DataReader_ptr reader);

  virtual void on_sample_lost(DDS::DataReader_ptr reader,
                              const DDS::SampleLostStatus& status);

  const OpenDDS::DCPS::String& id() const { return id_; }
  int get_count() const;
  size_t disposes() const;

private:
  const OpenDDS::DCPS::String id_;
  DistributedConditionSet_rch dcs_;
  int count_;
  size_t disposes_;
};

#endif /* INTERNAL_THREAD_STATUS_LISTENER_IMPL  */
