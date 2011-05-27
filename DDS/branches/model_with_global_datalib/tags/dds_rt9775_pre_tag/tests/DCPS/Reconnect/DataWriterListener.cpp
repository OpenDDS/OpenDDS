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
extern int num_deleted_connections;

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


void DataWriterListenerImpl::on_publication_disconnected (
  ::DDS::DataWriter_ptr,
  const ::TAO::DCPS::PublicationDisconnectedStatus & status
  ACE_ENV_ARG_DECL_WITH_DEFAULTS
  )
  ACE_THROW_SPEC ((
  ::CORBA::SystemException
  ))
{
  CORBA::ULong len = status.subscription_handles.length ();
  for (CORBA::ULong i = 0; i < len; ++i)
  {
    cout << "on_publication_disconnected reader " << status.subscription_handles[i] << endl;
    //ACE_DEBUG ((LM_DEBUG ,
    //  "(%P|%t)on_publication_disconnected reader %d \n", status.subscription_handles[i]));
  }
}


void DataWriterListenerImpl::on_publication_reconnected (
  ::DDS::DataWriter_ptr,
  const ::TAO::DCPS::PublicationReconnectedStatus & status
  ACE_ENV_ARG_DECL_WITH_DEFAULTS
  )
  ACE_THROW_SPEC ((
  ::CORBA::SystemException
  ))
{
  CORBA::ULong len = status.subscription_handles.length ();
  for (CORBA::ULong i = 0; i < len; ++i)
  {
    cout << "on_publication_reconnected reader " << status.subscription_handles[i] << endl;
    //ACE_DEBUG ((LM_DEBUG ,
    //  "(%P|%t)on_publication_reconnected reader %d \n", status.subscription_handles[i]));
  }
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

  CORBA::ULong len = status.subscription_handles.length ();
  for (CORBA::ULong i = 0; i < len; ++i)
  {
    cout << "on_publication_lost reader " << status.subscription_handles[i] << endl;
    //ACE_DEBUG ((LM_DEBUG ,
    //  "(%P|%t)on_publication_lost reader %d \n", status.subscription_handles[i]));
  }
}


void DataWriterListenerImpl::on_connection_deleted (
  ::DDS::DataWriter_ptr
  ACE_ENV_ARG_DECL_WITH_DEFAULTS
  )
  ACE_THROW_SPEC ((
  ::CORBA::SystemException
  ))
{
  ++ num_deleted_connections;

  ACE_DEBUG ((LM_DEBUG, "(%P|%t)received on_connection_deleted  \n"));
}

