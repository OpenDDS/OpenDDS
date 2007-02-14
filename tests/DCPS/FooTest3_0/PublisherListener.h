// -*- C++ -*-
//
// $Id$
#ifndef PUBLISHER_LISTENER_IMPL
#define PUBLISHER_LISTENER_IMPL

#include "dds/DdsDcpsPublicationS.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */


//Class PublisherListenerImpl
class PublisherListenerImpl : public virtual POA_DDS::PublisherListener
{
public:
  //Constructor
  PublisherListenerImpl (void);

  //Destructor
  virtual ~PublisherListenerImpl (void);



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

virtual void on_publication_match (
    ::DDS::DataWriter_ptr writer,
    const ::DDS::PublicationMatchStatus & status
  );

};
#endif /* PUBLISHER_LISTENER_IMPL  */
