// -*- C++ -*-
//
// $Id$
#ifndef PUBLISHERLISTENERIMPL_H
#define PUBLISHERLISTENERIMPL_H

#include "dds/DdsDcpsPublicationS.h"
#include "dds/DCPS/PublisherImpl.h"
#include "dds/DCPS/Service_Participant.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */


//Class ForwardingListenerImpl
class DataWriterListenerImpl
  : public virtual OpenDDS::DCPS::LocalObject<DDS::DataWriterListener>
{
public:
  //Constructor
  DataWriterListenerImpl ( OpenDDS::DCPS::Service_Participant::RepoKey repo);

  //Destructor
  virtual ~DataWriterListenerImpl (void);

  virtual void on_offered_deadline_missed (
    ::DDS::DataWriter_ptr writer,
    const ::DDS::OfferedDeadlineMissedStatus & status
  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
  ));

 virtual void on_offered_incompatible_qos (
    ::DDS::DataWriter_ptr writer,
    const ::DDS::OfferedIncompatibleQosStatus & status
  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
  ));

  virtual void on_liveliness_lost (
    ::DDS::DataWriter_ptr writer,
    const ::DDS::LivelinessLostStatus & status
  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
  ));

  virtual void on_publication_match (
    ::DDS::DataWriter_ptr writer,
    const ::DDS::PublicationMatchStatus & status
  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
  ));

  private:
    /// Repository key that we are attached to.
    OpenDDS::DCPS::Service_Participant::RepoKey repo_;

};

#endif /* PUBLISHERLISTENERIMPL_H  */

