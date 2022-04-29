/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/

#include "Qos_Helper.h"
#if !defined (__ACE_INLINE__)
#include "Qos_Helper.inl"
#endif /* __ACE_INLINE__ */

#include "debug.h"
#include "Service_Participant.h"

bool Qos_Helper::valid(const DDS::DomainParticipantQos& qos)
{
  if (!valid(qos.user_data)) {
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE,
        "(%P|%t) ERROR: Qos_Helper::valid::DomainParticipantQos, invalid user_data qos.\n"));
    }
    return false;
  }

  if (!valid(qos.entity_factory)) {
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE,
        "(%P|%t) ERROR: Qos_Helper::valid::DomainParticipantQos, invalid entity_factory qos.\n"));
    }
    return false;
  }

  if (!valid(qos.property)) {
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE,
        "(%P|%t) ERROR: Qos_Helper::valid::DomainParticipantQos, invalid property qos.\n"));
    }
    return false;
  }

  return true;
}

bool Qos_Helper::valid(const DDS::TopicQos& qos)
{
  if (!valid(qos.topic_data)) {
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE,
        "(%P|%t) ERROR: Qos_Helper::valid::TopicQos, invalid topic_data qos.\n"));
    }
    return false;
  }

  if (!valid(qos.durability)) {
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE,
        "(%P|%t) ERROR: Qos_Helper::valid::TopicQos, invalid durability qos.\n"));
    }
    return false;
  }

#ifndef OPENDDS_NO_PERSISTENCE_PROFILE
  if (!valid(qos.durability_service)) {
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE,
        "(%P|%t) ERROR: Qos_Helper::valid::TopicQos, invalid durability_service qos.\n"));
    }
    return false;
  }
#endif

  if (!valid(qos.deadline)) {
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE,
        "(%P|%t) ERROR: Qos_Helper::valid::TopicQos, invalid deadline qos.\n"));
    }
    return false;
  }

  if (!valid(qos.latency_budget)) {
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE,
        "(%P|%t) ERROR: Qos_Helper::valid::TopicQos, invalid latency_budget qos.\n"));
    }
    return false;
  }

  if (!valid(qos.liveliness)) {
    ACE_ERROR((LM_NOTICE,
               "(%P|%t) ERROR: Qos_Helper::valid::TopicQos, invalid liveliness qos.\n"));
    return false;
  }

  if (!valid(qos.destination_order)) {
    ACE_ERROR((LM_NOTICE,
               "(%P|%t) ERROR: Qos_Helper::valid::TopicQos, invalid destination_order qos.\n"));
    return false;
  }

  if (!valid(qos.history)) {
    ACE_ERROR((LM_NOTICE,
               "(%P|%t) ERROR: Qos_Helper::valid::TopicQos, invalid history qos.\n"));
    return false;
  }

  if (!valid(qos.resource_limits)) {
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE,
        "(%P|%t) ERROR: Qos_Helper::valid::TopicQos, invalid resource_limits qos.\n"));
    }
    return false;
  }

  if (!valid(qos.transport_priority)) {
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE,
        "(%P|%t) ERROR: Qos_Helper::valid::TopicQos, invalid transport_priority qos.\n"));
    }
    return false;
  }

  if (!valid(qos.lifespan)) {
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE,
        "(%P|%t) ERROR: Qos_Helper::valid::TopicQos, invalid lifespan qos.\n"));
    }
    return false;
  }

  if (!valid(qos.ownership)) {
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE,
        "(%P|%t) ERROR: Qos_Helper::valid::TopicQos, invalid ownership qos.\n"));
    }
    return false;
  }

  if (!valid(qos.representation)) {
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE,
        "(%P|%t) ERROR: Qos_Helper::valid::TopicQos, invalid representation qos.\n"));
    }
    return false;
  }

  return true;
}

bool Qos_Helper::valid(const DDS::DataWriterQos& qos)
{
  if (!valid(qos.durability)) {
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE,
        "(%P|%t) ERROR: Qos_Helper::valid::DataWriterQos, invalid durability qos.\n"));
    }
    return false;
  }

#ifndef OPENDDS_NO_PERSISTENCE_PROFILE
  if (!valid(qos.durability_service)) {
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE,
        "(%P|%t) ERROR: Qos_Helper::valid::DataWriterQos, invalid durability_service qos.\n"));
    }
    return false;
  }
#endif

  if (!valid(qos.deadline)) {
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE,
        "(%P|%t) ERROR: Qos_Helper::valid::DataWriterQos, invalid deadline qos.\n"));
    }
    return false;
  }

  if (!valid(qos.latency_budget)) {
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE,
        "(%P|%t) ERROR: Qos_Helper::valid::DataWriterQos, invalid latency_budget qos.\n"));
    }
    return false;
  }

  if (!valid(qos.liveliness)) {
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE,
        "(%P|%t) ERROR: Qos_Helper::valid::DataWriterQos, invalid liveliness qos.\n"));
    }
    return false;
  }

  if (!valid(qos.destination_order)) {
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE,
        "(%P|%t) ERROR: Qos_Helper::valid::DataWriterQos, invalid destination_order qos.\n"));
    }
    return false;
  }

  if (!valid(qos.history)) {
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE,
        "(%P|%t) ERROR: Qos_Helper::valid::DataWriterQos, invalid history qos.\n"));
    }
    return false;
  }

  if (!valid(qos.resource_limits)) {
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE,
        "(%P|%t) ERROR: Qos_Helper::valid::DataWriterQos, invalid resource_limits qos.\n"));
    }
    return false;
  }

  if (!valid(qos.transport_priority)) {
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE,
        "(%P|%t) ERROR: Qos_Helper::valid::DataWriterQos, invalid transport_priority qos.\n"));
    }
    return false;
  }

  if (!valid(qos.lifespan)) {
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE,
        "(%P|%t) ERROR: Qos_Helper::valid::DataWriterQos, invalid lifespan qos.\n"));
    }
    return false;
  }

  if (!valid(qos.user_data)) {
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE,
        "(%P|%t) ERROR: Qos_Helper::valid::DataWriterQos, invalid user_data qos.\n"));
    }
    return false;
  }

  if (!valid(qos.ownership)) {
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE,
        "(%P|%t) ERROR: Qos_Helper::valid::DataWriterQos, invalid ownership qos.\n"));
    }
    return false;
  }

#ifndef OPENDDS_NO_OWNERSHIP_KIND_EXCLUSIVE
  if (!valid(qos.ownership_strength)) {
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE,
        "(%P|%t) ERROR: Qos_Helper::valid::DataWriterQos, invalid ownership_strength qos.\n"));
    }
    return false;
  }
#endif

  if (!valid(qos.writer_data_lifecycle)) {
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE,
        "(%P|%t) ERROR: Qos_Helper::valid::DataWriterQos, invalid writer_data_lifecycle qos.\n"));
    }
    return false;
  }

  if (!valid(qos.representation)) {
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE,
        "(%P|%t) ERROR: Qos_Helper::valid::DataWriterQos, invalid data representation qos.\n"));
    }
    return false;
  }

  return true;
}

bool Qos_Helper::valid(const DDS::PublisherQos& qos)
{

  if (!valid(qos.presentation)) {
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE,
        "(%P|%t) ERROR: Qos_Helper::valid::PublisherQos, invalid presentation qos.\n"));
    }
    return false;
  }

  if (!valid(qos.partition)) {
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE,
        "(%P|%t) ERROR: Qos_Helper::valid::PublisherQos, invalid partition qos.\n"));
    }
    return false;
  }

  if (!valid(qos.group_data)) {
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE,
        "(%P|%t) ERROR: Qos_Helper::valid::PublisherQos, invalid group_data qos.\n"));
    }
    return false;
  }

  if (!valid(qos.entity_factory)) {
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE,
        "(%P|%t) ERROR: Qos_Helper::valid::PublisherQos, sinvalid entity_factory qos.\n"));
    }
    return false;
  }

  return true;
}

bool Qos_Helper::valid(const DDS::DataReaderQos& qos)
{
  if (!valid(qos.durability)) {
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE,
        "(%P|%t) ERROR: Qos_Helper::valid::DataReaderQos, invalid durability qos.\n"));
    }
    return false;
  }

  if (!valid(qos.deadline)) {
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE,
        "(%P|%t) ERROR: Qos_Helper::valid::DataReaderQos, invalid deadline qos.\n"));
    }
    return false;
  }

  if (!valid(qos.latency_budget)) {
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE,
        "(%P|%t) ERROR: Qos_Helper::valid::DataReaderQos, invalid latency_budget qos.\n"));
    }
    return false;
  }

  if (!valid(qos.liveliness)) {
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE,
        "(%P|%t) ERROR: Qos_Helper::valid::DataReaderQos, invalid liveliness qos.\n"));
    }
    return false;
  }

  if (!valid(qos.reliability)) {
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE,
        "(%P|%t) ERROR: Qos_Helper::valid::DataReaderQos, invalid reliability qos.\n"));
    }
    return false;
  }

  if (!valid(qos.destination_order)) {
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE,
        "(%P|%t) ERROR: Qos_Helper::valid::DataReaderQos, invalid destination_order qos.\n"));
    }
    return false;
  }

  if (!valid(qos.history)) {
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE,
        "(%P|%t) ERROR: Qos_Helper::valid::DataReaderQos, invalid history qos.\n"));
    }
    return false;
  }

  if (!valid(qos.resource_limits)) {
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE,
        "(%P|%t) ERROR: Qos_Helper::valid::DataReaderQos, invalid resource_limits qos.\n"));
    }
    return false;
  }

  if (!valid(qos.user_data)) {
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE,
        "(%P|%t) ERROR: Qos_Helper::valid::DataReaderQos, invalid user_data qos.\n"));
    }
    return false;
  }

  if (!valid(qos.time_based_filter)) {
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE,
        "(%P|%t) ERROR: Qos_Helper::valid::DataReaderQos, invalid time_based_filter qos.\n"));
    }
    return false;
  }

  if (!valid(qos.reader_data_lifecycle)) {
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE,
        "(%P|%t) ERROR: Qos_Helper::valid::DataReaderQos, invalid reader_data_lifecycle qos.\n"));
    }
    return false;
  }

  if (!valid(qos.ownership)) {
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE,
        "(%P|%t) ERROR: Qos_Helper::valid::DataReaderQos, invalid ownership qos.\n"));
    }
    return false;
  }

  if (!valid(qos.representation)) {
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE,
        "(%P|%t) ERROR: Qos_Helper::valid::DataReaderQos, invalid representation qos.\n"));
    }
    return false;
  }

  return true;
}

bool Qos_Helper::valid(const DDS::SubscriberQos& qos)
{
  if (!valid(qos.presentation)) {
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE,
        "(%P|%t) ERROR: Qos_Helper::valid::SubscriberQos, invalid presentation qos.\n"));
    }
    return false;
  }

  if (!valid(qos.partition)) {
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE,
        "(%P|%t) ERROR: Qos_Helper::valid::SubscriberQos, invalid partition qos.\n"));
    }
    return false;
  }

  if (!valid(qos.group_data)) {
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE,
        "(%P|%t) ERROR: Qos_Helper::valid::SubscriberQos, invalid group_data qos.\n"));
    }
    return false;
  }

  if (!valid(qos.entity_factory)) {
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE,
        "(%P|%t) ERROR: Qos_Helper::valid::SubscriberQos, invalid entity_factory qos.\n"));
    }
    return false;
  }

  return true;
}

bool Qos_Helper::valid(const DDS::DomainParticipantFactoryQos& qos)
{
  if (!valid(qos.entity_factory)) {
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE,
        "(%P|%t) ERROR: Qos_Helper::valid::DomainParticipantFactoryQos, invalid entity_factory qos.\n")));
    }
    return false;
  }

  return true;
}
