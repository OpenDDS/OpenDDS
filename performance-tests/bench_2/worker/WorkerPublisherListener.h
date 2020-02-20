#pragma once

#include "PublisherListener.h"

namespace Bench {

class WorkerPublisherListener : public Builder::PublisherListener {
public:

  // From DDS::DataWriterListener

  void on_offered_deadline_missed(DDS::DataWriter_ptr writer, const DDS::OfferedDeadlineMissedStatus& status) override;
  void on_offered_incompatible_qos(DDS::DataWriter_ptr writer, const DDS::OfferedIncompatibleQosStatus& status) override;
  void on_liveliness_lost(DDS::DataWriter_ptr writer, const DDS::LivelinessLostStatus& status) override;
  void on_publication_matched(DDS::DataWriter_ptr writer, const DDS::PublicationMatchedStatus& status) override;

  // From DDS::PublisherListener

  // From PublisherListener

  void set_publisher(Builder::Publisher& publisher) override;

protected:
  std::mutex mutex_;
  Builder::Publisher* publisher_{0};
};

}

