// -*- C++ -*-
//

#ifndef DATA_READER_LISTENER_IMPL
#define DATA_READER_LISTENER_IMPL

#include "Partition_Table.h"

#include <tests/Utils/DistributedConditionSet.h>

#include <dds/DdsDcpsSubscriptionExtC.h>
#include <dds/DCPS/LocalObject.h>

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */


namespace Test
{
  class DataReaderListener
    : public virtual OpenDDS::DCPS::LocalObject<OpenDDS::DCPS::DataReaderListener>
  {
  public:
    DataReaderListener (DistributedConditionSet_rch dcs,
                        const Test::PartitionConfig& config);

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

    virtual void on_data_available(DDS::DataReader_ptr reader);

    virtual void on_sample_lost(DDS::DataReader_ptr reader,
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

  protected:

    virtual ~DataReaderListener (void);

  private:

    void display_partitions (DDS::DataReader_ptr reader) const;

  private:
    DistributedConditionSet_rch dcs_;
    const Test::PartitionConfig& config_;

    /// The actual number of subscription matches.
    OpenDDS::DCPS::Atomic<long> subscription_matches_;

  };
}

#endif /* DATA_READER_LISTENER_IMPL  */
