#include "WorkerParticipantListener.h"

namespace Bench {

// From DDS::DataWriterListener

void WorkerParticipantListener::on_offered_deadline_missed(DDS::DataWriter_ptr /*writer*/, const DDS::OfferedDeadlineMissedStatus& /*status*/) {
}

void WorkerParticipantListener::on_offered_incompatible_qos(DDS::DataWriter_ptr /*writer*/, const DDS::OfferedIncompatibleQosStatus& /*status*/) {
}

void WorkerParticipantListener::on_liveliness_lost(DDS::DataWriter_ptr /*writer*/, const DDS::LivelinessLostStatus& /*status*/) {
}

void WorkerParticipantListener::on_publication_matched(DDS::DataWriter_ptr /*writer*/, const DDS::PublicationMatchedStatus& /*status*/) {
}

void WorkerParticipantListener::on_requested_deadline_missed(DDS::DataReader_ptr /*reader*/, const DDS::RequestedDeadlineMissedStatus& /*status*/) {
}

void WorkerParticipantListener::on_requested_incompatible_qos(DDS::DataReader_ptr /*reader*/, const DDS::RequestedIncompatibleQosStatus& /*status*/) {
}

// From DDS::SubscriberListener

void WorkerParticipantListener::on_data_on_readers(DDS::Subscriber_ptr /*subscriber*/) {
}

// From DDS::DataReaderListener

void WorkerParticipantListener::on_sample_rejected(DDS::DataReader_ptr /*reader*/, const DDS::SampleRejectedStatus& /*status*/) {
}

void WorkerParticipantListener::on_liveliness_changed(DDS::DataReader_ptr /*reader*/, const DDS::LivelinessChangedStatus& /*status*/) {
}

void WorkerParticipantListener::on_data_available(DDS::DataReader_ptr /*reader*/) {
}

void WorkerParticipantListener::on_subscription_matched(DDS::DataReader_ptr /*reader*/, const DDS::SubscriptionMatchedStatus& /*status*/) {
}

void WorkerParticipantListener::on_sample_lost(DDS::DataReader_ptr /*reader*/, const DDS::SampleLostStatus& /*status*/) {
}

void WorkerParticipantListener::on_inconsistent_topic(DDS::Topic_ptr /*the_topic*/, const DDS::InconsistentTopicStatus& /*status*/) {
}

// From Builder::ParticipantListener

void WorkerParticipantListener::set_participant(Builder::Participant& participant) {
  participant_ = &participant;
}

}

