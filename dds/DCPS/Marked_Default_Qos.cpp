/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "Marked_Default_Qos.h"
#include "Service_Participant.h"

#include <cstring>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

int const INVALID_ENUM_VALUE = 999;

DDS::DomainParticipantQos
Marked_Default_Qos::marked_default_DomainParticipantQos()
{
  DDS::DomainParticipantQos qos =
    TheServiceParticipant->initial_DomainParticipantQos();
  void* const mem = &qos.entity_factory.autoenable_created_entities;
  *static_cast<char*>(mem) = 3;
  return qos;
}

DDS::TopicQos
Marked_Default_Qos::marked_default_TopicQos()
{
  DDS::TopicQos qos = TheServiceParticipant->initial_TopicQos();
  std::memcpy(&qos.liveliness.kind, &INVALID_ENUM_VALUE,
              sizeof qos.liveliness.kind);
  return qos;
}

DDS::DataWriterQos
Marked_Default_Qos::marked_default_DataWriterQos()
{
  DDS::DataWriterQos qos = TheServiceParticipant->initial_DataWriterQos();
  std::memcpy(&qos.liveliness.kind, &INVALID_ENUM_VALUE,
              sizeof qos.liveliness.kind);
  return qos;
}

DDS::PublisherQos
Marked_Default_Qos::marked_default_PublisherQos()
{
  DDS::PublisherQos qos = TheServiceParticipant->initial_PublisherQos();
  std::memcpy(&qos.presentation.access_scope, &INVALID_ENUM_VALUE,
              sizeof qos.presentation.access_scope);
  return qos;
}

DDS::DataReaderQos
Marked_Default_Qos::marked_default_DataReaderQos()
{
  DDS::DataReaderQos qos = TheServiceParticipant->initial_DataReaderQos();
  std::memcpy(&qos.liveliness.kind, &INVALID_ENUM_VALUE,
              sizeof qos.liveliness.kind);
  return qos;
}

DDS::SubscriberQos
Marked_Default_Qos::marked_default_SubscriberQos()
{
  DDS::SubscriberQos qos = TheServiceParticipant->initial_SubscriberQos();
  std::memcpy(&qos.presentation.access_scope, &INVALID_ENUM_VALUE,
              sizeof qos.presentation.access_scope);
  return qos;
}

DDS::DataWriterQos
Marked_Default_Qos::marked_default_DataWriter_Use_TopicQos()
{
  DDS::DataWriterQos qos = TheServiceParticipant->initial_DataWriterQos();
  std::memcpy(&qos.durability.kind, &INVALID_ENUM_VALUE,
              sizeof qos.durability.kind);
  return qos;
}

DDS::DataReaderQos
Marked_Default_Qos::marked_default_DataReader_Use_TopicQos()
{
  DDS::DataReaderQos qos = TheServiceParticipant->initial_DataReaderQos();
  std::memcpy(&qos.durability.kind, &INVALID_ENUM_VALUE,
              sizeof qos.durability.kind);
  return qos;
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
