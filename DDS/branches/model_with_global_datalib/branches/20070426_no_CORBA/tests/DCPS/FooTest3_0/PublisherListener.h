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
class PublisherListenerImpl
  : public virtual TAO::DCPS::LocalObject<DDS::PublisherListener>
{
public:
  //Constructor
  PublisherListenerImpl (void);

  //Destructor
  virtual ~PublisherListenerImpl (void);



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
  ));};


#endif /* PUBLISHER_LISTENER_IMPL  */
