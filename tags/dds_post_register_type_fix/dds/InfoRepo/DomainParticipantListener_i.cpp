// -*- C++ -*-
//
// $Id$

#include "DcpsInfo_pch.h"


#include "DomainParticipantListener_i.h"

// Implementation skeleton constructor
TAO_DCPS_DomainParticipantListener_i::TAO_DCPS_DomainParticipantListener_i (void)
  {
  }
  
// Implementation skeleton destructor
TAO_DCPS_DomainParticipantListener_i::~TAO_DCPS_DomainParticipantListener_i (void)
  {
  }
  
void TAO_DCPS_DomainParticipantListener_i::on_inconsistent_topic (
    ::DDS::Topic_ptr ,
    const ::DDS::InconsistentTopicStatus & 
    ACE_ENV_ARG_DECL
  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
  ))
  {
    // Add your implementation here
  }
  
void TAO_DCPS_DomainParticipantListener_i::on_data_on_readers (
    ::DDS::Subscriber_ptr 
    ACE_ENV_ARG_DECL
  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
  ))
  {
    // Add your implementation here
  }
  
void TAO_DCPS_DomainParticipantListener_i::on_offered_deadline_missed (
    ::DDS::DataWriter_ptr ,
    const ::DDS::OfferedDeadlineMissedStatus & 
    ACE_ENV_ARG_DECL
  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
  ))
  {
    // Add your implementation here
  }
  
void TAO_DCPS_DomainParticipantListener_i::on_offered_incompatible_qos (
    ::DDS::DataWriter_ptr,
    const ::DDS::OfferedIncompatibleQosStatus & 
    ACE_ENV_ARG_DECL
  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
  ))
  {
    // Add your implementation here
  }
  
void TAO_DCPS_DomainParticipantListener_i::on_liveliness_lost (
    ::DDS::DataWriter_ptr ,
    const ::DDS::LivelinessLostStatus & 
    ACE_ENV_ARG_DECL
  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
  ))
  {
    // Add your implementation here
  }
  
void TAO_DCPS_DomainParticipantListener_i::on_publication_match (
    ::DDS::DataWriter_ptr ,
    const ::DDS::PublicationMatchStatus & 
    ACE_ENV_ARG_DECL
  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
  ))
  {
    // Add your implementation here
  }
  
void TAO_DCPS_DomainParticipantListener_i::on_requested_deadline_missed (
    ::DDS::DataReader_ptr ,
    const ::DDS::RequestedDeadlineMissedStatus & 
    ACE_ENV_ARG_DECL
  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
  ))
  {
    // Add your implementation here
  }
  
void TAO_DCPS_DomainParticipantListener_i::on_requested_incompatible_qos (
    ::DDS::DataReader_ptr ,
    const ::DDS::RequestedIncompatibleQosStatus & 
    ACE_ENV_ARG_DECL
  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
  ))
  {
    // Add your implementation here
  }
  
void TAO_DCPS_DomainParticipantListener_i::on_sample_rejected (
    ::DDS::DataReader_ptr ,
    const ::DDS::SampleRejectedStatus & 
    ACE_ENV_ARG_DECL
  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
  ))
  {
    // Add your implementation here
  }
  
void TAO_DCPS_DomainParticipantListener_i::on_liveliness_changed (
    ::DDS::DataReader_ptr ,
    const ::DDS::LivelinessChangedStatus & 
    ACE_ENV_ARG_DECL
  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
  ))
  {
    // Add your implementation here
  }
  
void TAO_DCPS_DomainParticipantListener_i::on_data_available (
    ::DDS::DataReader_ptr 
    ACE_ENV_ARG_DECL
  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
  ))
  {
    // Add your implementation here
  }
  
void TAO_DCPS_DomainParticipantListener_i::on_subscription_match (
    ::DDS::DataReader_ptr ,
    const ::DDS::SubscriptionMatchStatus & 
    ACE_ENV_ARG_DECL
  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
  ))
  {
    // Add your implementation here
  }
  
void TAO_DCPS_DomainParticipantListener_i::on_sample_lost (
    ::DDS::DataReader_ptr ,
    const ::DDS::SampleLostStatus & 
    ACE_ENV_ARG_DECL
  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
  ))
  {
    // Add your implementation here
  }


