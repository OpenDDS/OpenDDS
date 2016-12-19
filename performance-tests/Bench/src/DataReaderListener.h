// -*- C++ -*-
//

#ifndef DATA_READER_LISTENER_IMPL
#define DATA_READER_LISTENER_IMPL

#include <dds/DdsDcpsSubscriptionExtC.h>
#include <dds/DCPS/LocalObject.h>

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "dds/DCPS/Stats_T.h"

#include <map>

namespace Test {

  class Publication;

  class DataReaderListener
    : public virtual OpenDDS::DCPS::LocalObject<OpenDDS::DCPS::DataReaderListener>
  {
    public:
      DataReaderListener(
        bool collectData = false,
        int rawDataBound = 0,
        OpenDDS::DCPS::DataCollector< double>::OnFull rawDataType
          = OpenDDS::DCPS::DataCollector< double>::KeepOldest,
        bool verbose = false);

      virtual ~DataReaderListener (void);

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

      /// Establish a forwarding destination for received samples.
      void set_destination( Publication* publication);

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

      /// Dump any raw data.
      std::ostream& summaryData( std::ostream& str) const;

      /// Dump any raw data.
      std::ostream& rawData( std::ostream& str) const;

    private:
      /// Abstraction to collect full path statistical data.
      class WriterStats {
        public:
          WriterStats(
            int bound = 0,
            OpenDDS::DCPS::DataCollector< double>::OnFull type
              = OpenDDS::DCPS::DataCollector< double>::KeepOldest
          );

          /// Accumulate another datum.
          void add_stat( DDS::Duration_t delay);

          /// Dump summary data.
          std::ostream& summaryData( std::ostream& str) const;

          /// Dump raw data buffer.
          std::ostream& rawData( std::ostream& str) const;

        private:
          OpenDDS::DCPS::Stats< double> stats_;
      };

      /// Flag to control data collection.
      bool collectData_;

      /// Verbosity flag.
      bool verbose_;

      /// Total number of messages received by this listener.
      int totalMessages_;

      /// Number of valid messages received by this listener.
      int validMessages_;

      /// Size bound for raw statistical data collection buffer.
      int rawDataBound_;

      /// Type of raw statistical data collection buffer behavior on
      /// overflow.
      OpenDDS::DCPS::DataCollector< double>::OnFull rawDataType_;

      /// Forwarding destination for received samples.
      Publication* destination_;

      /// Sample counts.
      std::map< long, long> counts_;

      /// Byte counts.
      std::map< long, long> bytes_;

      /// Writer priorities.
      std::map< long, long> priorities_;

      /// Full path statistics gathering and reporting.
      typedef std::map< long, WriterStats> StatsMap;
      StatsMap stats_;
  };

} // End of namespace Test

#endif /* DATA_READER_LISTENER_IMPL  */
