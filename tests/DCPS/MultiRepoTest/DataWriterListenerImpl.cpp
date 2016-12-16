// -*- C++ -*-
//
#include "DataWriterListenerImpl.h"


DataWriterListenerImpl::DataWriterListenerImpl ( OpenDDS::DCPS::Discovery::RepoKey repo)
  : repo_( repo)
{
  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT("(%P|%t) %T DataWriterListenerImpl::DataWriterListenerImpl Repo[ %C]\n"),
    this->repo_.c_str()
  ));
}

DataWriterListenerImpl::~DataWriterListenerImpl (void)
  {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) %T DataWriterListenerImpl::~DataWriterListenerImpl Repo[ %C]\n"),
      this->repo_.c_str()
    ));
  }

void DataWriterListenerImpl::on_offered_deadline_missed (
    ::DDS::DataWriter_ptr writer,
    const ::DDS::OfferedDeadlineMissedStatus & status
  )
  {
    ACE_UNUSED_ARG(writer);
    ACE_UNUSED_ARG(status);

    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) %T DataWriterListenerImpl::on_offered_deadline_missed Repo[ %C]\n"),
      this->repo_.c_str()
    ));
  }

void DataWriterListenerImpl::on_offered_incompatible_qos (
    ::DDS::DataWriter_ptr writer,
    const ::DDS::OfferedIncompatibleQosStatus & status
  )
  {
    ACE_UNUSED_ARG(writer);
    ACE_UNUSED_ARG(status);

    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) %T DataWriterListenerImpl::on_offered_incompatible_qos Repo[ %C]\n"),
      this->repo_.c_str()
    ));
  }

void DataWriterListenerImpl::on_liveliness_lost (
    ::DDS::DataWriter_ptr writer,
    const ::DDS::LivelinessLostStatus & status
  )
  {
    ACE_UNUSED_ARG(writer);
    ACE_UNUSED_ARG(status);

    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) %T DataWriterListenerImpl::on_liveliness_changed Repo[ %C]\n"),
      this->repo_.c_str()
    ));
  }

void DataWriterListenerImpl::on_publication_matched (
    ::DDS::DataWriter_ptr writer,
    const ::DDS::PublicationMatchedStatus & status
  )
  {
    ACE_UNUSED_ARG(writer) ;
    ACE_UNUSED_ARG(status) ;

    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) %T DataWriterListenerImpl::on_publication_matched Repo[ %C] \n"),
      this->repo_.c_str()
    ));
  }

