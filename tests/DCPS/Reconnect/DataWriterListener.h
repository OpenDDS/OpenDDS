// -*- C++ -*-
//
// $Id$
#ifndef DATAREADER_LISTENER_IMPL
#define DATAREADER_LISTENER_IMPL

#include <dds/DdsDcpsPublicationS.h>

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */


//Class DataWriterListenerImpl
class DataWriterListenerImpl
  : public virtual POA_TAO::DCPS::DataWriterListener,
    public virtual PortableServer::RefCountServantBase
{
public:
  //Constructor
  DataWriterListenerImpl ();

  //Destructor
  virtual ~DataWriterListenerImpl (void);

  virtual void on_offered_deadline_missed (
      ::DDS::DataWriter_ptr writer,
      const ::DDS::OfferedDeadlineMissedStatus & status
      ACE_ENV_ARG_DECL_WITH_DEFAULTS
    )
    ACE_THROW_SPEC ((
      ::CORBA::SystemException
    ));
  
  virtual void on_offered_incompatible_qos (
      ::DDS::DataWriter_ptr writer,
      const ::DDS::OfferedIncompatibleQosStatus & status
      ACE_ENV_ARG_DECL_WITH_DEFAULTS
    )
    ACE_THROW_SPEC ((
      ::CORBA::SystemException
    ));
  
  virtual void on_liveliness_lost (
      ::DDS::DataWriter_ptr writer,
      const ::DDS::LivelinessLostStatus & status
      ACE_ENV_ARG_DECL_WITH_DEFAULTS
    )
    ACE_THROW_SPEC ((
      ::CORBA::SystemException
    ));
  
  virtual void on_publication_match (
      ::DDS::DataWriter_ptr writer,
      const ::DDS::PublicationMatchStatus & status
      ACE_ENV_ARG_DECL_WITH_DEFAULTS
    )
    ACE_THROW_SPEC ((
      ::CORBA::SystemException
    ));

  virtual void on_publication_disconnected (
    ::DDS::DataWriter_ptr writer,
    const ::TAO::DCPS::PublicationDisconnectedStatus & status
    ACE_ENV_ARG_DECL_WITH_DEFAULTS
    )
    ACE_THROW_SPEC ((
    ::CORBA::SystemException
    ));

  virtual void on_publication_reconnected (
    ::DDS::DataWriter_ptr writer,
    const ::TAO::DCPS::PublicationReconnectedStatus & status
    ACE_ENV_ARG_DECL_WITH_DEFAULTS
    )
    ACE_THROW_SPEC ((
    ::CORBA::SystemException
    ));

  virtual void on_publication_lost (
    ::DDS::DataWriter_ptr writer,
    const ::TAO::DCPS::PublicationLostStatus & status
    ACE_ENV_ARG_DECL_WITH_DEFAULTS
    )
    ACE_THROW_SPEC ((
    ::CORBA::SystemException
    ));

private:

  DDS::DataWriter_var reader_;
  int                 num_reads_;
};

#endif /* DATAREADER_LISTENER_IMPL  */
