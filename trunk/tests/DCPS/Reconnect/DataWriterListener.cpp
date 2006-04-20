// -*- C++ -*-
//
// $Id$
#include "DataWriterListener.h"
#include "MessageTypeSupportC.h"
#include "MessageTypeSupportImpl.h"
#include <dds/DCPS/Service_Participant.h>
#include <ace/streams.h>

extern int num_reads_before_crash;
extern int actual_lost_pub_notification;

// Implementation skeleton constructor
DataWriterListenerImpl::DataWriterListenerImpl()
{
}

// Implementation skeleton destructor
DataWriterListenerImpl::~DataWriterListenerImpl ()
{
}

void DataWriterListenerImpl::on_offered_deadline_missed (
  ::DDS::DataWriter_ptr,
  const ::DDS::OfferedDeadlineMissedStatus &
  ACE_ENV_ARG_DECL_WITH_DEFAULTS
  )
  ACE_THROW_SPEC ((
  ::CORBA::SystemException
  ))
{
  ACE_DEBUG ((LM_DEBUG ,
    "(%P|%t)DataWrierListenerImpl::on_offered_deadline_missed\n"));
}


void DataWriterListenerImpl::on_offered_incompatible_qos (
  ::DDS::DataWriter_ptr,
  const ::DDS::OfferedIncompatibleQosStatus &
  ACE_ENV_ARG_DECL_WITH_DEFAULTS
  )
  ACE_THROW_SPEC ((
  ::CORBA::SystemException
  ))
{
  ACE_DEBUG ((LM_DEBUG ,
    "(%P|%t)DataWriterListenerImpl::on_offered_incompatible_qos\n"));
}


void DataWriterListenerImpl::on_liveliness_lost (
  ::DDS::DataWriter_ptr,
  const ::DDS::LivelinessLostStatus &
  ACE_ENV_ARG_DECL_WITH_DEFAULTS
  )
  ACE_THROW_SPEC ((
  ::CORBA::SystemException
  ))
{
  ACE_DEBUG ((LM_DEBUG ,
    "(%P|%t)DataWriterListenerImpl::on_liveliness_lost\n"));
}


void DataWriterListenerImpl::on_publication_match (
  ::DDS::DataWriter_ptr,
  const ::DDS::PublicationMatchStatus &
  ACE_ENV_ARG_DECL_WITH_DEFAULTS
  )
  ACE_THROW_SPEC ((
  ::CORBA::SystemException
  ))
{
  ACE_DEBUG ((LM_DEBUG ,
    "(%P|%t)DataWriterListenerImpl::on_publication_match\n"));
}


void DataWriterListenerImpl::on_publication_lost (
  ::DDS::DataWriter_ptr,
  const ::TAO::DCPS::PublicationLostStatus & status
  ACE_ENV_ARG_DECL_WITH_DEFAULTS
  )
  ACE_THROW_SPEC ((
  ::CORBA::SystemException
  ))
{
  ++ actual_lost_pub_notification;
  ACE_DEBUG ((LM_DEBUG ,
    "(%P|%t)DataWriterListenerImpl::on_publication_lost "
    "total_count=%d total_count_change=%d \n",
    status.total_count, status.total_count_change));

  for (CORBA::Long i = 0; i < status.total_count; ++i)
  {
    ACE_DEBUG ((LM_DEBUG ,
      "(%P|%t)Lost publication to reader %d \n", status.subscription_handles[i]));
  }
}
