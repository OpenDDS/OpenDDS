// -*- C++ -*-
//
// $Id$
#include "DataWriterListener.h"
#include "MessageTypeSupportC.h"
#include "MessageTypeSupportImpl.h"
#include <dds/DCPS/Service_Participant.h>
#include <ace/streams.h>

extern int num_reads_before_crash;

// Implementation skeleton constructor
DataWriterListenerImpl::DataWriterListenerImpl()
{
}

// Implementation skeleton destructor
DataWriterListenerImpl::~DataWriterListenerImpl ()
{
}

void DataWriterListenerImpl::on_offered_deadline_missed (
  ::DDS::DataWriter_ptr writer,
  const ::DDS::OfferedDeadlineMissedStatus & status
  ACE_ENV_ARG_DECL_WITH_DEFAULTS
  )
  ACE_THROW_SPEC ((
  ::CORBA::SystemException
  ))
{
  ACE_DEBUG ((LM_DEBUG , 
    "(%P|%t)DataWriterListenerImpl::on_offered_deadline_missed\n"));
}


void DataWriterListenerImpl::on_offered_incompatible_qos (
  ::DDS::DataWriter_ptr writer,
  const ::DDS::OfferedIncompatibleQosStatus & status
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
  ::DDS::DataWriter_ptr writer,
  const ::DDS::LivelinessLostStatus & status
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
  ::DDS::DataWriter_ptr writer,
  const ::DDS::PublicationMatchStatus & status
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
  ::DDS::DataWriter_ptr writer,
  const ::TAO::DCPS::PublicationLostStatus & status
  ACE_ENV_ARG_DECL_WITH_DEFAULTS
  )
  ACE_THROW_SPEC ((
  ::CORBA::SystemException
  ))
{
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

