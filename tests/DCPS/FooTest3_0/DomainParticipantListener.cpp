// -*- C++ -*-
//
#include "DomainParticipantListener.h"

extern int offered_incompatible_qos_called_on_dp;

DomainParticipantListenerImpl::DomainParticipantListenerImpl (void)
  {
  }

DomainParticipantListenerImpl::~DomainParticipantListenerImpl (void)
  {
  }

void DomainParticipantListenerImpl::on_inconsistent_topic (
    ::DDS::Topic_ptr the_topic,
    const ::DDS::InconsistentTopicStatus & status
  )
  {
    ACE_UNUSED_ARG(the_topic);
    ACE_UNUSED_ARG(status);
    // Add your implementation here
  }

void DomainParticipantListenerImpl::on_data_on_readers (
    ::DDS::Subscriber_ptr subs
  )
  {
    ACE_UNUSED_ARG(subs);
    // Add your implementation here
  }

void DomainParticipantListenerImpl::on_offered_deadline_missed (
    ::DDS::DataWriter_ptr writer,
    const ::DDS::OfferedDeadlineMissedStatus & status
  )
  {
    ACE_UNUSED_ARG(writer);
    ACE_UNUSED_ARG(status);
    // Add your implementation here
  }

void DomainParticipantListenerImpl::on_offered_incompatible_qos (
    ::DDS::DataWriter_ptr writer,
    const ::DDS::OfferedIncompatibleQosStatus & status
  )
  {
    ACE_UNUSED_ARG(writer);
    ACE_UNUSED_ARG(status);
     offered_incompatible_qos_called_on_dp ++;
  }

void DomainParticipantListenerImpl::on_liveliness_lost (
    ::DDS::DataWriter_ptr writer,
    const ::DDS::LivelinessLostStatus & status
  )
  {
    ACE_UNUSED_ARG(writer);
    ACE_UNUSED_ARG(status);
    // Add your implementation here
  }

void DomainParticipantListenerImpl::on_publication_matched (
    ::DDS::DataWriter_ptr writer,
    const ::DDS::PublicationMatchedStatus & status
  )
  {
    ACE_UNUSED_ARG(writer);
    ACE_UNUSED_ARG(status);
    // Add your implementation here
  }

void DomainParticipantListenerImpl::on_requested_deadline_missed (
    ::DDS::DataReader_ptr reader,
    const ::DDS::RequestedDeadlineMissedStatus & status
  )
  {
    ACE_UNUSED_ARG(reader);
    ACE_UNUSED_ARG(status);
    // Add your implementation here
  }

void DomainParticipantListenerImpl::on_requested_incompatible_qos (
    ::DDS::DataReader_ptr reader,
    const ::DDS::RequestedIncompatibleQosStatus & status
  )
  {
    ACE_UNUSED_ARG(reader);
    ACE_UNUSED_ARG(status);
    // Add your implementation here
  }

void DomainParticipantListenerImpl::on_sample_rejected (
    ::DDS::DataReader_ptr reader,
    const ::DDS::SampleRejectedStatus & status
  )
  {
    ACE_UNUSED_ARG(reader);
    ACE_UNUSED_ARG(status);
    // Add your implementation here
  }

void DomainParticipantListenerImpl::on_liveliness_changed (
    ::DDS::DataReader_ptr reader,
    const ::DDS::LivelinessChangedStatus & status
  )
  {
    ACE_UNUSED_ARG(reader);
    ACE_UNUSED_ARG(status);
    // Add your implementation here
  }

void DomainParticipantListenerImpl::on_data_available (
    ::DDS::DataReader_ptr reader
  )
  {
    ACE_UNUSED_ARG(reader);
    // Add your implementation here
  }

void DomainParticipantListenerImpl::on_subscription_matched (
    ::DDS::DataReader_ptr reader,
    const ::DDS::SubscriptionMatchedStatus & status
  )
  {
    ACE_UNUSED_ARG(reader);
    ACE_UNUSED_ARG(status);
    // Add your implementation here
  }

void DomainParticipantListenerImpl::on_sample_lost (
    ::DDS::DataReader_ptr reader,
    const ::DDS::SampleLostStatus & status
  )
  {
    ACE_UNUSED_ARG(reader);
    ACE_UNUSED_ARG(status);
    // Add your implementation here
  }


