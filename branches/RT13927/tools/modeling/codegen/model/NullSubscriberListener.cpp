// -*- C++ -*-
//
// $Id$
#include "NullSubscriberListener.h"
#include <dds/DCPS/debug.h>

OpenDDS::Model::NullSubscriberListener::NullSubscriberListener()
{
  if( OpenDDS::DCPS::DCPS_debug_level > 4) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) NullSubscriberListener::NullSubscriberListener()\n")));
  }
}

OpenDDS::Model::NullSubscriberListener::~NullSubscriberListener()
{
  if( OpenDDS::DCPS::DCPS_debug_level > 4) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) NullSubscriberListener::~NullSubscriberListener()\n")));
  }
}

void 
OpenDDS::Model::NullSubscriberListener::on_data_on_readers(
  DDS::Subscriber_ptr /* subs */
) ACE_THROW_SPEC((CORBA::SystemException))
{  
  if( OpenDDS::DCPS::DCPS_debug_level > 4) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) NullSubscriberListener::on_data_on_readers()\n")));
  }
}

void
OpenDDS::Model::NullSubscriberListener::on_data_available(
  DDS::DataReader_ptr
) ACE_THROW_SPEC((CORBA::SystemException))
{
  if( OpenDDS::DCPS::DCPS_debug_level > 4) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) NullSubscriberListener::on_data_available()\n")));
  }
}

void
OpenDDS::Model::NullSubscriberListener::on_requested_deadline_missed(
  DDS::DataReader_ptr,
  const DDS::RequestedDeadlineMissedStatus&
) ACE_THROW_SPEC((CORBA::SystemException))
{
  if( OpenDDS::DCPS::DCPS_debug_level > 4) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) NullSubscriberListener::on_requested_deadline_missed()\n")));
  }
}

void
OpenDDS::Model::NullSubscriberListener::on_requested_incompatible_qos(
  DDS::DataReader_ptr,
  const DDS::RequestedIncompatibleQosStatus&
) ACE_THROW_SPEC((CORBA::SystemException))
{
  if( OpenDDS::DCPS::DCPS_debug_level > 4) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) NullSubscriberListener::on_requested_incompatible_qos()\n")));
  }
}

void
OpenDDS::Model::NullSubscriberListener::on_liveliness_changed(
  DDS::DataReader_ptr,
  const DDS::LivelinessChangedStatus&
) ACE_THROW_SPEC((CORBA::SystemException))
{
  if( OpenDDS::DCPS::DCPS_debug_level > 4) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) NullSubscriberListener::on_liveliness_changed()\n")));
  }
}

void
OpenDDS::Model::NullSubscriberListener::on_subscription_matched(
  DDS::DataReader_ptr,
  const DDS::SubscriptionMatchedStatus&
) ACE_THROW_SPEC((CORBA::SystemException))
{
  if( OpenDDS::DCPS::DCPS_debug_level > 4) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) NullSubscriberListener::on_subscription_matched()\n")));
  }
}

void
OpenDDS::Model::NullSubscriberListener::on_sample_rejected(
  DDS::DataReader_ptr,
  const DDS::SampleRejectedStatus&
) ACE_THROW_SPEC((CORBA::SystemException))
{
  if( OpenDDS::DCPS::DCPS_debug_level > 4) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) NullSubscriberListener::on_sample_rejected()\n")));
  }
}

void
OpenDDS::Model::NullSubscriberListener::on_sample_lost(
  DDS::DataReader_ptr,
  const DDS::SampleLostStatus&
) ACE_THROW_SPEC((CORBA::SystemException))
{
  if( OpenDDS::DCPS::DCPS_debug_level > 4) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) NullSubscriberListener::on_sample_lost()\n")));
  }
}

