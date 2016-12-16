// -*- C++ -*-
//
#ifndef PUBLISHERLISTENERIMPL_H
#define PUBLISHERLISTENERIMPL_H

#include "dds/DdsDcpsPublicationC.h"
#include "dds/DCPS/PublisherImpl.h"
#include "dds/DCPS/Service_Participant.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

class DataWriterListenerImpl
  : public virtual OpenDDS::DCPS::LocalObject<DDS::DataWriterListener>
{
public:
  DataWriterListenerImpl ( OpenDDS::DCPS::Discovery::RepoKey repo);

  virtual ~DataWriterListenerImpl (void);

  virtual void on_offered_deadline_missed (
    ::DDS::DataWriter_ptr writer,
    const ::DDS::OfferedDeadlineMissedStatus & status
  );

 virtual void on_offered_incompatible_qos (
    ::DDS::DataWriter_ptr writer,
    const ::DDS::OfferedIncompatibleQosStatus & status
  );

  virtual void on_liveliness_lost (
    ::DDS::DataWriter_ptr writer,
    const ::DDS::LivelinessLostStatus & status
  );

  virtual void on_publication_matched (
    ::DDS::DataWriter_ptr writer,
    const ::DDS::PublicationMatchedStatus & status
  );

  private:
    /// Repository key that we are attached to.
    OpenDDS::DCPS::Discovery::RepoKey repo_;

};

#endif /* PUBLISHERLISTENERIMPL_H  */

