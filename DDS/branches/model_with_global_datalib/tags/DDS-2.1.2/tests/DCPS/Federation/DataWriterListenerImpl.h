#ifndef DATAWRITER_LISTENER_IMPL
#define DATAWRITER_LISTENER_IMPL

#include "dds/DdsDcpsPublicationS.h"
#include "ace/Condition_T.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */


//Class DataWriterListenerImpl
class DataWriterListenerImpl
  : public virtual OpenDDS::DCPS::LocalObject<OpenDDS::DCPS::DataWriterListener>
{
public:
  DataWriterListenerImpl (void);

  virtual ~DataWriterListenerImpl (void);

  void wait_for_completion();

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
  
  virtual void on_publication_matched (
      ::DDS::DataWriter_ptr writer,
      const ::DDS::PublicationMatchedStatus & status
    )
    ACE_THROW_SPEC ((
      ::CORBA::SystemException
    ));

  virtual void on_publication_disconnected (
      ::DDS::DataWriter_ptr reader,
      const ::OpenDDS::DCPS::PublicationDisconnectedStatus & status
    )
    ACE_THROW_SPEC ((
      ::CORBA::SystemException
    ));

  virtual void on_publication_reconnected (
      ::DDS::DataWriter_ptr reader,
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
  // Lock for signaling.
  ACE_SYNCH_MUTEX lock_;

  // Condition to signal.
  ACE_Condition<ACE_SYNCH_MUTEX> condition_;

  // Flag to manage.
  bool complete_;
};

#endif /* DATAWRITER_LISTENER_IMPL  */
