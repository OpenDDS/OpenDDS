// -*- C++ -*-
//

#ifndef DATA_READER_LISTENER_IMPL
#define DATA_READER_LISTENER_IMPL

#include <dds/DdsDcpsSubscriptionExtC.h>
#include <dds/DCPS/LocalObject.h>

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include <map>

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

    /// Number of messages received by this listener over its lifetime.
    int total_messages() const;

    /// Number of valid messages received by this listener over its lifetime.
    int valid_messages() const;

    /// Current number of samples that have been received from a writer.
    const std::map< long, long>& counts() const;

    /// Current number of payload bytes that have been received from a writer.
    const std::map< long, long>& bytes() const;

    /// Priorities for the writers we received samples from.
    const std::map< long, long>& priorities() const;

    virtual ~DataReaderListener (void);

    private:
      /// Verbosity flag.
      bool verbose_;

      /// Total number of messages received by this listener.
      int total_messages_;

      /// Number of valid messages received by this listener.
      int valid_messages_;

      /// Sample counts.
      std::map< long, long> counts_;

      /// Byte counts.
      std::map< long, long> bytes_;

      /// Writer priorities.
      std::map< long, long> priorities_;
  };

} // End of namespace Test

#endif /* DATA_READER_LISTENER_IMPL  */
