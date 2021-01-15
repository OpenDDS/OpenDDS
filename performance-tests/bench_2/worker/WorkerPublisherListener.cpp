#include "WorkerPublisherListener.h"

namespace Bench {

WorkerPublisherListener::WorkerPublisherListener()
{
}

WorkerPublisherListener::WorkerPublisherListener(const Builder::PropertySeq&)
{
}

WorkerPublisherListener::~WorkerPublisherListener()
{
}

// From DDS::DataWriterListener

void WorkerPublisherListener::on_offered_deadline_missed(DDS::DataWriter_ptr /*writer*/, const DDS::OfferedDeadlineMissedStatus& /*status*/)
{
}

void WorkerPublisherListener::on_offered_incompatible_qos(DDS::DataWriter_ptr /*writer*/, const DDS::OfferedIncompatibleQosStatus& /*status*/)
{
}

void WorkerPublisherListener::on_liveliness_lost(DDS::DataWriter_ptr /*writer*/, const DDS::LivelinessLostStatus& /*status*/)
{
}

void WorkerPublisherListener::on_publication_matched(DDS::DataWriter_ptr /*writer*/, const DDS::PublicationMatchedStatus& /*status*/)
{
}

// From DDS::PublisherListener

// From PublisherListener

void WorkerPublisherListener::set_publisher(Builder::Publisher& publisher)
{
  publisher_ = &publisher;
}

void WorkerPublisherListener::unset_publisher(Builder::Publisher& publisher)
{
  if (publisher_ == &publisher) {
    publisher_ = nullptr;
  }
}

}
