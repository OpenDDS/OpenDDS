// -*- C++ -*-
//

#ifndef DATA_READER_LISTENER_IMPL
#define DATA_READER_LISTENER_IMPL

#include <dds/DdsDcpsSubscriptionExtC.h>
#include <dds/DCPS/LocalObject.h>

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include <set>

namespace Test {

  class DataReaderListener
    : public virtual OpenDDS::DCPS::LocalObject<OpenDDS::DCPS::DataReaderListener>
  {
  public:
    DataReaderListener( const bool verbose = false);

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

    virtual void on_budget_exceeded (
        DDS::DataReader_ptr reader,
        const ::OpenDDS::DCPS::BudgetExceededStatus& status);

    virtual void on_connection_deleted (DDS::DataReader_ptr);

    /// Current number of samples that have been received.
    unsigned int count() const;

    /// The data was received and it was valid.
    bool passed() const;

    virtual ~DataReaderListener (void);

  private:
      /// Verbosity flag.
      bool verbose_;

      /// Sample count.
      unsigned int count_;
      std::set<long> received_samples_;
      bool received_samples_invalid_;
      enum PriorityStatus { NOT_RECEIVED, RECEIVED_VALID, RECEIVED_INVALID };
      PriorityStatus priority_sample_;
  };

} // End of namespace Test

#endif /* DATA_READER_LISTENER_IMPL  */
