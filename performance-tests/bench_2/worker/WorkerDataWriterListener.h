#pragma once

#include "DataWriterListener.h"

namespace Bench {

class WorkerDataWriterListener : public Builder::DataWriterListener {
public:

  WorkerDataWriterListener();
  WorkerDataWriterListener(size_t expected);

  // From DDS::DataWriterListener

  void on_offered_deadline_missed(DDS::DataWriter_ptr writer, const DDS::OfferedDeadlineMissedStatus& status) override;
  void on_offered_incompatible_qos(DDS::DataWriter_ptr writer, const DDS::OfferedIncompatibleQosStatus& status) override;
  void on_liveliness_lost(DDS::DataWriter_ptr writer, const DDS::LivelinessLostStatus& status) override;
  void on_publication_matched(DDS::DataWriter_ptr writer, const DDS::PublicationMatchedStatus& status) override;

  // From Builder::DataWriterListener

  void set_datawriter(Builder::DataWriter& datawriter) override;
  void unset_datawriter(Builder::DataWriter& datawriter) override;

protected:
  std::mutex mutex_;
  size_t expected_count_{0};
  size_t matched_count_{0};
  Builder::DataWriter* datawriter_{0};
  Builder::PropertyIndex last_discovery_time_;
};

}

