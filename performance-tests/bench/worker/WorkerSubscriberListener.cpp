#include "WorkerSubscriberListener.h"

namespace Bench {

// From DDS::DataReaderListener

void WorkerSubscriberListener::on_requested_deadline_missed(DDS::DataReader_ptr /*reader*/, const DDS::RequestedDeadlineMissedStatus& /*status*/) {
}

void WorkerSubscriberListener::on_requested_incompatible_qos(DDS::DataReader_ptr /*reader*/, const DDS::RequestedIncompatibleQosStatus& /*status*/) {
}

void WorkerSubscriberListener::on_sample_rejected(DDS::DataReader_ptr /*reader*/, const DDS::SampleRejectedStatus& /*status*/) {
}

void WorkerSubscriberListener::on_liveliness_changed(DDS::DataReader_ptr /*reader*/, const DDS::LivelinessChangedStatus& /*status*/) {
}

void WorkerSubscriberListener::on_data_available(DDS::DataReader_ptr /*reader*/) {
}

void WorkerSubscriberListener::on_subscription_matched(DDS::DataReader_ptr /*reader*/, const DDS::SubscriptionMatchedStatus& /*status*/) {
}

void WorkerSubscriberListener::on_sample_lost(DDS::DataReader_ptr /*reader*/, const DDS::SampleLostStatus& /*status*/) {
}

// From DDS::SubscriberListener

void WorkerSubscriberListener::on_data_on_readers(DDS::Subscriber_ptr /*subs*/) {
}

// From Builder::SubscriberListener

void WorkerSubscriberListener::set_subscriber(Builder::Subscriber& subscriber) {
  subscriber_ = &subscriber;
}

}

