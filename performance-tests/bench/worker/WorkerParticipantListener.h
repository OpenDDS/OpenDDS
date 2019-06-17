#pragma once

#include "ParticipantListener.h"

namespace Bench {

class WorkerParticipantListener : public Builder::ParticipantListener {
public:

  // From DDS::DataWriterListener

  void on_offered_deadline_missed(DDS::DataWriter_ptr writer, const DDS::OfferedDeadlineMissedStatus& status) override;
  void on_offered_incompatible_qos(DDS::DataWriter_ptr writer, const DDS::OfferedIncompatibleQosStatus& status) override;
  void on_liveliness_lost(DDS::DataWriter_ptr writer, const DDS::LivelinessLostStatus& status) override;
  void on_publication_matched(DDS::DataWriter_ptr writer, const DDS::PublicationMatchedStatus& status) override;
  void on_requested_deadline_missed(DDS::DataReader_ptr reader, const DDS::RequestedDeadlineMissedStatus& status) override;
  void on_requested_incompatible_qos(DDS::DataReader_ptr reader, const DDS::RequestedIncompatibleQosStatus& status) override;

  // From DDS::SubscriberListener

  void on_data_on_readers(DDS::Subscriber_ptr subscriber) override;

  // From DDS::DataReaderListener

  void on_sample_rejected(DDS::DataReader_ptr reader, const DDS::SampleRejectedStatus& status) override;
  void on_liveliness_changed(DDS::DataReader_ptr reader, const DDS::LivelinessChangedStatus& status) override;
  void on_data_available(DDS::DataReader_ptr reader) override;
  void on_subscription_matched(DDS::DataReader_ptr reader, const DDS::SubscriptionMatchedStatus& status) override;
  void on_sample_lost(DDS::DataReader_ptr reader, const DDS::SampleLostStatus& status) override;
  void on_inconsistent_topic(DDS::Topic_ptr the_topic, const DDS::InconsistentTopicStatus& status) override;

  // From Builder::ParticipantListener

  void set_participant(Builder::Participant& participant) override;

protected:
  std::mutex mutex_;
  Builder::Participant* participant_{0};
};

}

