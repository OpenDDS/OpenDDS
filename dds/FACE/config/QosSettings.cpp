#include "QosSettings.h"

namespace OpenDDS { namespace FACE { namespace config {

QosSettings::QosSettings() : 
  publisher_qos_()
, writer_qos_()
, subscriber_qos_()
, reader_qos_()
{

}

void
QosSettings::apply_to(DDS::PublisherQos&  target) const
{
  target = publisher_qos_;
}

void
QosSettings::apply_to(DDS::SubscriberQos&  target) const
{
  target = subscriber_qos_;
}

void
QosSettings::apply_to(DDS::DataWriterQos&  target) const
{
  target = writer_qos_;
}

void
QosSettings::apply_to(DDS::DataReaderQos&  target) const
{
  target = reader_qos_;
}



} } }

