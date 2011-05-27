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
  // note: TAO specific extensions
  : public virtual OpenDDS::DCPS::LocalObject<OpenDDS::DCPS::DataWriterListener>
{
public:

  //Constructor
  DataWriterListenerImpl ();

  //Destructor
  virtual ~DataWriterListenerImpl (void);

  virtual void on_offered_deadline_missed (
      ::DDS::DataWriter_ptr writer,
      const ::DDS::OfferedDeadlineMissedStatus & status
    )
    ACE_THROW_SPEC ((
      ::CORBA::SystemException
    ));

  virtual void on_offered_incompatible_qos (
      ::DDS::DataWriter_ptr writer,
      const ::DDS::OfferedIncompatibleQosStatus & status
    )
    ACE_THROW_SPEC ((
      ::CORBA::SystemException
    ));

  virtual void on_liveliness_lost (
      ::DDS::DataWriter_ptr writer,
      const ::DDS::LivelinessLostStatus & status
    )
    ACE_THROW_SPEC ((
      ::CORBA::SystemException
    ));

  virtual void on_publication_match (
      ::DDS::DataWriter_ptr writer,
      const ::DDS::PublicationMatchStatus & status
    )
    ACE_THROW_SPEC ((
      ::CORBA::SystemException
    ));

  virtual void on_publication_disconnected (
    ::DDS::DataWriter_ptr writer,
    const ::OpenDDS::DCPS::PublicationDisconnectedStatus & status
    )
    ACE_THROW_SPEC ((
    ::CORBA::SystemException
    ));

  virtual void on_publication_reconnected (
    ::DDS::DataWriter_ptr writer,
    const ::OpenDDS::DCPS::PublicationReconnectedStatus & status
    )
    ACE_THROW_SPEC ((
    ::CORBA::SystemException
    ));

  virtual void on_publication_lost (
    ::DDS::DataWriter_ptr writer,
    const ::OpenDDS::DCPS::PublicationLostStatus & status
    )
    ACE_THROW_SPEC ((
    ::CORBA::SystemException
    ));

  virtual void on_connection_deleted (
    ::DDS::DataWriter_ptr writer
    )
    ACE_THROW_SPEC ((
    ::CORBA::SystemException
    ));

private:

  DDS::DataWriter_var reader_;
  int                 num_reads_;
};

#endif /* DATAREADER_LISTENER_IMPL  */
