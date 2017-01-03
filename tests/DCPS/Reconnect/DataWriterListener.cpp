// -*- C++ -*-
//
#include "DataWriterListener.h"
#include "MessengerTypeSupportC.h"
#include "MessengerTypeSupportImpl.h"
#include <dds/DCPS/Service_Participant.h>
#include <ace/streams.h>

extern int num_reads_before_crash;
extern int actual_lost_pub_notification;
extern int num_deleted_connections;

DataWriterListenerImpl::DataWriterListenerImpl()
{
}

DataWriterListenerImpl::~DataWriterListenerImpl ()
{
}

void DataWriterListenerImpl::on_offered_deadline_missed (
  ::DDS::DataWriter_ptr,
  const ::DDS::OfferedDeadlineMissedStatus &
  )
{
  ACE_DEBUG ((LM_DEBUG ,
    "(%P|%t) DataWrierListenerImpl::on_offered_deadline_missed\n"));
}


void DataWriterListenerImpl::on_offered_incompatible_qos (
  ::DDS::DataWriter_ptr,
  const ::DDS::OfferedIncompatibleQosStatus &
  )
{
  ACE_DEBUG ((LM_DEBUG ,
    "(%P|%t) DataWriterListenerImpl::on_offered_incompatible_qos\n"));
}


void DataWriterListenerImpl::on_liveliness_lost (
  ::DDS::DataWriter_ptr,
  const ::DDS::LivelinessLostStatus &
  )
{
  ACE_DEBUG ((LM_DEBUG ,
    "(%P|%t) DataWriterListenerImpl::on_liveliness_lost\n"));
}


void DataWriterListenerImpl::on_publication_matched (
  ::DDS::DataWriter_ptr,
  const ::DDS::PublicationMatchedStatus &
  )
{
  ACE_DEBUG ((LM_DEBUG ,
    "(%P|%t) DataWriterListenerImpl::on_publication_matched\n"));
}


void DataWriterListenerImpl::on_publication_disconnected (
  ::DDS::DataWriter_ptr,
  const ::OpenDDS::DCPS::PublicationDisconnectedStatus & status
  )
{
  CORBA::ULong len = status.subscription_handles.length ();
  for (CORBA::ULong i = 0; i < len; ++i)
  {
    cout << "on_publication_disconnected reader " << status.subscription_handles[i] << endl;
    //ACE_DEBUG ((LM_DEBUG ,
    //  "(%P|%t) on_publication_disconnected reader %d \n", status.subscription_handles[i]));
  }
}


void DataWriterListenerImpl::on_publication_reconnected (
  ::DDS::DataWriter_ptr,
  const ::OpenDDS::DCPS::PublicationReconnectedStatus & status
  )
{
  CORBA::ULong len = status.subscription_handles.length ();
  for (CORBA::ULong i = 0; i < len; ++i)
  {
    cout << "on_publication_reconnected reader " << status.subscription_handles[i] << endl;
    //ACE_DEBUG ((LM_DEBUG ,
    //  "(%P|%t) on_publication_reconnected reader %d \n", status.subscription_handles[i]));
  }
}


void DataWriterListenerImpl::on_publication_lost (
  ::DDS::DataWriter_ptr,
  const ::OpenDDS::DCPS::PublicationLostStatus & status
  )
{
  ++ actual_lost_pub_notification;

  CORBA::ULong len = status.subscription_handles.length ();
  for (CORBA::ULong i = 0; i < len; ++i)
  {
    cout << "on_publication_lost reader " << status.subscription_handles[i] << endl;
    //ACE_DEBUG ((LM_DEBUG ,
    //  "(%P|%t) on_publication_lost reader %d \n", status.subscription_handles[i]));
  }
}


void DataWriterListenerImpl::on_connection_deleted (
  ::DDS::DataWriter_ptr
  )
{
  ++ num_deleted_connections;

  ACE_DEBUG ((LM_DEBUG, "(%P|%t) received on_connection_deleted  \n"));
}

