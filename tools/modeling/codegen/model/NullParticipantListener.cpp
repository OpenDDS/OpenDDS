// -*- C++ -*-
//
// $Id$

#include <dds/DCPS/debug.h>
#include "NullParticipantListener.h"

OpenDDS::Model::NullParticipantListener::NullParticipantListener()
{
  if( OpenDDS::DCPS::DCPS_debug_level > 4) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) NullParticipantListener::NullParticipantListener()\n")));
  }
}

OpenDDS::Model::NullParticipantListener::~NullParticipantListener()
{
  if( OpenDDS::DCPS::DCPS_debug_level > 4) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) NullParticipantListener::~NullParticipantListener()\n")));
  }
}

void
OpenDDS::Model::NullParticipantListener::on_inconsistent_topic(
  DDS::Topic_ptr ,
  const DDS::InconsistentTopicStatus&
) ACE_THROW_SPEC((CORBA::SystemException))
{
  if( OpenDDS::DCPS::DCPS_debug_level > 4) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) NullParticipantListener::on_inconsistent_topic()\n")));
  }
}

void
OpenDDS::Model::NullParticipantListener::on_data_on_readers(
  DDS::Subscriber_ptr
) ACE_THROW_SPEC((CORBA::SystemException))
{
  if( OpenDDS::DCPS::DCPS_debug_level > 4) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) NullParticipantListener::on_data_on_readers()\n")));
  }
}

void
OpenDDS::Model::NullParticipantListener::on_offered_deadline_missed(
  DDS::DataWriter_ptr ,
  const DDS::OfferedDeadlineMissedStatus&
) ACE_THROW_SPEC((CORBA::SystemException))
{
  if( OpenDDS::DCPS::DCPS_debug_level > 4) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) NullParticipantListener::on_offered_deadline_missed()\n")));
  }
}

void
OpenDDS::Model::NullParticipantListener::on_offered_incompatible_qos(
  DDS::DataWriter_ptr,
  const DDS::OfferedIncompatibleQosStatus&
) ACE_THROW_SPEC((CORBA::SystemException))
{
  if( OpenDDS::DCPS::DCPS_debug_level > 4) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) NullParticipantListener::on_offered_incompatible_qos()\n")));
  }
}

void
OpenDDS::Model::NullParticipantListener::on_liveliness_lost(
  DDS::DataWriter_ptr ,
  const DDS::LivelinessLostStatus&
) ACE_THROW_SPEC((CORBA::SystemException))
{
  if( OpenDDS::DCPS::DCPS_debug_level > 4) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) NullParticipantListener::on_livliness_lost()\n")));
  }
}

void
OpenDDS::Model::NullParticipantListener::on_publication_matched(
  DDS::DataWriter_ptr ,
  const DDS::PublicationMatchedStatus&
) ACE_THROW_SPEC((CORBA::SystemException))
{
  if( OpenDDS::DCPS::DCPS_debug_level > 4) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) NullParticipantListener::on_publication_matched()\n")));
  }
}

void
OpenDDS::Model::NullParticipantListener::on_requested_deadline_missed(
  DDS::DataReader_ptr ,
  const DDS::RequestedDeadlineMissedStatus&
) ACE_THROW_SPEC((CORBA::SystemException))
{
  if( OpenDDS::DCPS::DCPS_debug_level > 4) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) NullParticipantListener::on_requested_deadline_missed()\n")));
  }
}

void
OpenDDS::Model::NullParticipantListener::on_requested_incompatible_qos(
  DDS::DataReader_ptr ,
  const DDS::RequestedIncompatibleQosStatus&
) ACE_THROW_SPEC((CORBA::SystemException))
{
  if( OpenDDS::DCPS::DCPS_debug_level > 4) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) NullParticipantListener::on_requested_incompatible_qos()\n")));
  }
}

void
OpenDDS::Model::NullParticipantListener::on_sample_rejected(
  DDS::DataReader_ptr ,
  const DDS::SampleRejectedStatus&
) ACE_THROW_SPEC((CORBA::SystemException))
{
  if( OpenDDS::DCPS::DCPS_debug_level > 4) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) NullParticipantListener::on_sample_rejected()\n")));
  }
}

void
OpenDDS::Model::NullParticipantListener::on_liveliness_changed(
  DDS::DataReader_ptr ,
  const DDS::LivelinessChangedStatus&
) ACE_THROW_SPEC((CORBA::SystemException))
{
  if( OpenDDS::DCPS::DCPS_debug_level > 4) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) NullParticipantListener::on_liveliness_changed()\n")));
  }
}

void
OpenDDS::Model::NullParticipantListener::on_data_available(
  DDS::DataReader_ptr
) ACE_THROW_SPEC((CORBA::SystemException))
{
  if( OpenDDS::DCPS::DCPS_debug_level > 4) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) NullParticipantListener::on_data_available()\n")));
  }
}

void
OpenDDS::Model::NullParticipantListener::on_subscription_matched(
  DDS::DataReader_ptr ,
  const DDS::SubscriptionMatchedStatus&
) ACE_THROW_SPEC((CORBA::SystemException))
{
  if( OpenDDS::DCPS::DCPS_debug_level > 4) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) NullParticipantListener::on_subscription_matched()\n")));
  }
}

void
OpenDDS::Model::NullParticipantListener::on_sample_lost(
  DDS::DataReader_ptr ,
  const DDS::SampleLostStatus&
) ACE_THROW_SPEC((CORBA::SystemException))
{
  if( OpenDDS::DCPS::DCPS_debug_level > 4) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) NullParticipantListener::on_sample_lost()\n")));
  }
}

