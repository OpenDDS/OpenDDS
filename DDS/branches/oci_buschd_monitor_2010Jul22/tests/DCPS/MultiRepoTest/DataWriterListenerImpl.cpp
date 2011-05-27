// -*- C++ -*-
//
// $Id$
#include "DataWriterListenerImpl.h"


// Implementation skeleton constructor
DataWriterListenerImpl::DataWriterListenerImpl ( OpenDDS::DCPS::Service_Participant::RepoKey repo)
  : repo_( repo)
{
  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT("%T (%P|%t) DataWriterListenerImpl::DataWriterListenerImpl Repo[ %d]\n"),
    this->repo_
  ));
}

// Implementation skeleton destructor
DataWriterListenerImpl::~DataWriterListenerImpl (void)
  {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("%T (%P|%t) DataWriterListenerImpl::~DataWriterListenerImpl Repo[ %d] "),
      this->repo_
    ));
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

    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("%T (%P|%t) DataWriterListenerImpl::on_offered_deadline_missed Repo[ %d]\n"),
      this->repo_
    ));
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

    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("%T (%P|%t) DataWriterListenerImpl::on_offered_incompatible_qos Repo[ %d]\n"),
      this->repo_
    ));
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

    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("%T (%P|%t) DataWriterListenerImpl::on_liveliness_changed Repo[ %d]\n"),
      this->repo_
    ));
  }

void DataWriterListenerImpl::on_publication_matched (
    ::DDS::DataWriter_ptr writer,
    const ::DDS::PublicationMatchedStatus & status
  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
  ))
  {
    ACE_UNUSED_ARG(writer) ;
    ACE_UNUSED_ARG(status) ;

    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("%T (%P|%t) DataWriterListenerImpl::on_publication_matched Repo[ %d] \n"),
      this->repo_
    ));
  }

