// -*- C++ -*-
//
// $Id$
#include "DCPS/DdsDcps_pch.h"
#include  "Marked_Default_Qos.h"
#include  "Service_Participant.h"


namespace TAO
{
  namespace DCPS
  {
    int INVALID_ENUM_VALUE = 999;

    ::DDS::DomainParticipantQos
    Marked_Default_Qos::marked_default_DomainParticipantQos ()
    {
      ::DDS::DomainParticipantQos qos;
      TheParticipantFactory->get_default_participant_qos(qos);
      return qos;
    }

    ::DDS::TopicQos
    Marked_Default_Qos::marked_default_TopicQos ()
    {
      ::DDS::TopicQos qos = TheServiceParticipant->initial_TopicQos();
      qos.liveliness.kind 
        = ACE_static_cast(::DDS::LivelinessQosPolicyKind, 
                          INVALID_ENUM_VALUE);
      return qos;
    }

    ::DDS::DataWriterQos
    Marked_Default_Qos::marked_default_DataWriterQos ()
    {
      ::DDS::DataWriterQos qos = TheServiceParticipant->initial_DataWriterQos();
      qos.liveliness.kind 
        = ACE_static_cast(::DDS::LivelinessQosPolicyKind, 
                          INVALID_ENUM_VALUE);
      return qos;
    }

    ::DDS::PublisherQos
    Marked_Default_Qos::marked_default_PublisherQos ()
    {
      ::DDS::PublisherQos qos = TheServiceParticipant->initial_PublisherQos();
      qos.presentation.access_scope 
        = ACE_static_cast(::DDS::PresentationQosPolicyAccessScopeKind, 
                          INVALID_ENUM_VALUE);
      return qos;
    }

    ::DDS::DataReaderQos
    Marked_Default_Qos::marked_default_DataReaderQos ()
    {
      ::DDS::DataReaderQos qos = TheServiceParticipant->initial_DataReaderQos();
      qos.liveliness.kind 
        = ACE_static_cast(::DDS::LivelinessQosPolicyKind, 
                          INVALID_ENUM_VALUE);
      return qos;
    }

    ::DDS::SubscriberQos
    Marked_Default_Qos::marked_default_SubscriberQos ()
    {
      ::DDS::SubscriberQos qos = TheServiceParticipant->initial_SubscriberQos();
      qos.presentation.access_scope 
        = ACE_static_cast(::DDS::PresentationQosPolicyAccessScopeKind, 
                          INVALID_ENUM_VALUE);
      return qos;
    }

    ::DDS::DataWriterQos         
    Marked_Default_Qos::marked_default_DataWriter_Use_TopicQos ()
    {
      ::DDS::DataWriterQos qos = TheServiceParticipant->initial_DataWriterQos();
      qos.durability.kind 
        = ACE_static_cast(::DDS::DurabilityQosPolicyKind, 
                          INVALID_ENUM_VALUE);
      return qos;
    }

    ::DDS::DataReaderQos         
    Marked_Default_Qos::marked_default_DataReader_Use_TopicQos ()
    {
      ::DDS::DataReaderQos qos = TheServiceParticipant->initial_DataReaderQos();
      qos.durability.kind 
        = ACE_static_cast(::DDS::DurabilityQosPolicyKind, 
                          INVALID_ENUM_VALUE);
      return qos;
    }

  } // namespace DCPS
} // namespace TAO

