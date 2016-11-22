// -*- C++ -*-
//

#include <dds/DCPS/debug.h>
#include "NullParticipantListener.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

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
)
{
  if( OpenDDS::DCPS::DCPS_debug_level > 4) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) NullParticipantListener::on_inconsistent_topic()\n")));
  }
}

void
OpenDDS::Model::NullParticipantListener::on_data_on_readers(
  DDS::Subscriber_ptr
)
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
)
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
)
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
)
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
)
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
)
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
)
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
)
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
)
{
  if( OpenDDS::DCPS::DCPS_debug_level > 4) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) NullParticipantListener::on_liveliness_changed()\n")));
  }
}

void
OpenDDS::Model::NullParticipantListener::on_data_available(
  DDS::DataReader_ptr
)
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
)
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
)
{
  if( OpenDDS::DCPS::DCPS_debug_level > 4) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) NullParticipantListener::on_sample_lost()\n")));
  }
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL

