// -*- C++ -*-
//
#include "NullTopicListener.h"
#include <dds/DCPS/debug.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

OpenDDS::Model::NullTopicListener::NullTopicListener()
{
  if( OpenDDS::DCPS::DCPS_debug_level > 4) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) NullTopicListener::NullTopicListener()\n")));
  }
}

OpenDDS::Model::NullTopicListener::~NullTopicListener()
{
  if( OpenDDS::DCPS::DCPS_debug_level > 4) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) NullTopicListener::~NullTopicListener()\n")));
  }
}

void
OpenDDS::Model::NullTopicListener::on_inconsistent_topic(
  DDS::Topic_ptr,
  const DDS::InconsistentTopicStatus&
)
{
  if( OpenDDS::DCPS::DCPS_debug_level > 4) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) NullTopicListener::on_inconsistent_topic()\n")));
  }
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
