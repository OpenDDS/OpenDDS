// -*- C++ -*-
//
// $Id$
#include "DataWriterListener.h"

extern int offered_incompatible_qos_called_on_dw;

// Implementation skeleton constructor
DataWriterListenerImpl::DataWriterListenerImpl (void)
  {
  }

// Implementation skeleton destructor
DataWriterListenerImpl::~DataWriterListenerImpl (void)
  {
  }

void DataWriterListenerImpl::on_offered_deadline_missed (
    ::DDS::DataWriter_ptr writer,
    const ::DDS::OfferedDeadlineMissedStatus & status
  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
  ))
  {
    ACE_UNUSED_ARG(writer);
    ACE_UNUSED_ARG(status);
    // Add your implementation here
  }

void DataWriterListenerImpl::on_offered_incompatible_qos (
    ::DDS::DataWriter_ptr writer,
    const ::DDS::OfferedIncompatibleQosStatus & status
  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
  ))
  {
    ACE_UNUSED_ARG(writer);
    ACE_UNUSED_ARG(status);
    offered_incompatible_qos_called_on_dw ++;
  }

void DataWriterListenerImpl::on_liveliness_lost (
    ::DDS::DataWriter_ptr writer,
    const ::DDS::LivelinessLostStatus & status
  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
  ))
  {
    ACE_UNUSED_ARG(writer);
    ACE_UNUSED_ARG(status);
    // Add your implementation here
  }

void DataWriterListenerImpl::on_publication_match (
    ::DDS::DataWriter_ptr writer,
    const ::DDS::PublicationMatchStatus & status
  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
  ))
  {
    ACE_UNUSED_ARG(writer);
    ACE_UNUSED_ARG(status);
    // Add your implementation here
  }

void DataWriterListenerImpl::on_publication_disconnected (
    ::DDS::DataWriter_ptr writer,
    const ::OpenDDS::DCPS::PublicationDisconnectedStatus & status
  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
  ))
  {
    ACE_UNUSED_ARG(writer);
    ACE_UNUSED_ARG(status);
    // Add your implementation here
  }

void DataWriterListenerImpl::on_publication_reconnected (
    ::DDS::DataWriter_ptr writer,
    const ::OpenDDS::DCPS::PublicationReconnectedStatus & status
  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
  ))
  {
    ACE_UNUSED_ARG(writer);
    ACE_UNUSED_ARG(status);
    // Add your implementation here
  }

void DataWriterListenerImpl::on_publication_lost (
    ::DDS::DataWriter_ptr writer,
    const ::OpenDDS::DCPS::PublicationLostStatus & status
  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
  ))
  {
    ACE_UNUSED_ARG(writer);
    ACE_UNUSED_ARG(status);
    // Add your implementation here
  }

void DataWriterListenerImpl::on_connection_deleted (
    ::DDS::DataWriter_ptr writer
  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
  ))
  {
    ACE_UNUSED_ARG(writer) ;

    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) DataWriterListenerImpl::on_connection_deleted \n")));
  }
