// -*- C++ -*-
//
#include "DataWriterListener.h"

extern int offered_incompatible_qos_called_on_dw;

DataWriterListenerImpl::DataWriterListenerImpl (void)
  {
  }

DataWriterListenerImpl::~DataWriterListenerImpl (void)
  {
  }

void DataWriterListenerImpl::on_offered_deadline_missed (
    ::DDS::DataWriter_ptr writer,
    const ::DDS::OfferedDeadlineMissedStatus & status
  )
  {
    ACE_UNUSED_ARG(writer);
    ACE_UNUSED_ARG(status);
    // Add your implementation here
  }

void DataWriterListenerImpl::on_offered_incompatible_qos (
    ::DDS::DataWriter_ptr writer,
    const ::DDS::OfferedIncompatibleQosStatus & status
  )
  {
    ACE_UNUSED_ARG(writer);
    ACE_UNUSED_ARG(status);
    offered_incompatible_qos_called_on_dw ++;
  }

void DataWriterListenerImpl::on_liveliness_lost (
    ::DDS::DataWriter_ptr writer,
    const ::DDS::LivelinessLostStatus & status
  )
  {
    ACE_UNUSED_ARG(writer);
    ACE_UNUSED_ARG(status);
    // Add your implementation here
  }

void DataWriterListenerImpl::on_publication_matched (
    ::DDS::DataWriter_ptr writer,
    const ::DDS::PublicationMatchedStatus & status
  )
  {
    ACE_UNUSED_ARG(writer);
    ACE_UNUSED_ARG(status);
    // Add your implementation here
  }

void DataWriterListenerImpl::on_publication_disconnected (
    ::DDS::DataWriter_ptr writer,
    const ::OpenDDS::DCPS::PublicationDisconnectedStatus & status
  )
  {
    ACE_UNUSED_ARG(writer);
    ACE_UNUSED_ARG(status);
    // Add your implementation here
  }

void DataWriterListenerImpl::on_publication_reconnected (
    ::DDS::DataWriter_ptr writer,
    const ::OpenDDS::DCPS::PublicationReconnectedStatus & status
  )
  {
    ACE_UNUSED_ARG(writer);
    ACE_UNUSED_ARG(status);
    // Add your implementation here
  }

void DataWriterListenerImpl::on_publication_lost (
    ::DDS::DataWriter_ptr writer,
    const ::OpenDDS::DCPS::PublicationLostStatus & status
  )
  {
    ACE_UNUSED_ARG(writer);
    ACE_UNUSED_ARG(status);
    // Add your implementation here
  }

void DataWriterListenerImpl::on_connection_deleted (
    ::DDS::DataWriter_ptr writer
  )
  {
    ACE_UNUSED_ARG(writer) ;

    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) DataWriterListenerImpl::on_connection_deleted \n")));
  }
