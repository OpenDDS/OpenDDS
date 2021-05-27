// -*- C++ -*-
//

#ifndef DATAREADER_LISTENER_IMPL
#define DATAREADER_LISTENER_IMPL

#include "tests/Utils/DistributedConditionSet.h"

#include <dds/DCPS/LocalObject.h>
#include <dds/DCPS/PoolAllocator.h>
#include <dds/DdsDcpsSubscriptionExtC.h>

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

const char SUBSCRIBER_ACTOR_1[] = "Subscriber_1";
const char SUBSCRIBER_ACTOR_2[] = "Subscriber_2";
const char EXPECTED_READS_1[] = "expected_reads_1";
const char EXPECTED_READS_2[] = "expected_reads_2";

class DataReaderListenerImpl
  : public virtual OpenDDS::DCPS::LocalObject<OpenDDS::DCPS::DataReaderListener>
{
public:
  DataReaderListenerImpl(DistributedConditionSet_rch dcs,
                         const OPENDDS_STRING& actor);

  void reset(const OPENDDS_STRING& condition,
             int expected_read_count)
  {
    condition_ = condition;
    expected_read_count_ = expected_read_count;
  }

  virtual void on_requested_deadline_missed(DDS::DataReader_ptr,
                                            const DDS::RequestedDeadlineMissedStatus&) {}

  virtual void on_requested_incompatible_qos(DDS::DataReader_ptr,
                                             const DDS::RequestedIncompatibleQosStatus&) {}

  virtual void on_liveliness_changed(DDS::DataReader_ptr,
                                     const DDS::LivelinessChangedStatus&) {}

  virtual void on_subscription_matched(DDS::DataReader_ptr,
                                       const DDS::SubscriptionMatchedStatus&) {}

  virtual void on_sample_rejected(DDS::DataReader_ptr,
                                  const DDS::SampleRejectedStatus&) {}

  virtual void on_data_available(DDS::DataReader_ptr reader);

  virtual void on_sample_lost(DDS::DataReader_ptr,
                              const DDS::SampleLostStatus&) {}

  virtual void on_subscription_disconnected(DDS::DataReader_ptr,
                                            const ::OpenDDS::DCPS::SubscriptionDisconnectedStatus&) {}

  virtual void on_subscription_reconnected(DDS::DataReader_ptr,
                                           const ::OpenDDS::DCPS::SubscriptionReconnectedStatus&) {}

  virtual void on_subscription_lost(DDS::DataReader_ptr,
                                    const ::OpenDDS::DCPS::SubscriptionLostStatus&) {}

  virtual void on_budget_exceeded(DDS::DataReader_ptr,
                                  const ::OpenDDS::DCPS::BudgetExceededStatus&) {}

  int read_count() const {
    return read_count_;
  }

private:
  DistributedConditionSet_rch dcs_;
  const OPENDDS_STRING actor_;
  OPENDDS_STRING condition_;
  int expected_read_count_;
  int read_count_;
};

#endif /* DATAREADER_LISTENER_IMPL  */
