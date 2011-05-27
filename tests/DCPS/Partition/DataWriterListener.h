// -*- C++ -*-
//
// $Id$

#ifndef DATA_WRITER_LISTENER_H
#define DATA_WRITER_LISTENER_H

#include "dds/DdsDcpsPublicationS.h"
#include "dds/DCPS/Definitions.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */


namespace Test
{

  class DataWriterListener
    : public virtual OpenDDS::DCPS::LocalObject<OpenDDS::DCPS::DataWriterListener>
  {
  public:
    //Constructor
    DataWriterListener (long expected_matches);

    virtual void on_offered_deadline_missed (
        ::DDS::DataWriter_ptr writer,
        const ::DDS::OfferedDeadlineMissedStatus & status)
      ACE_THROW_SPEC ((::CORBA::SystemException));

    virtual void on_offered_incompatible_qos (
        ::DDS::DataWriter_ptr writer,
        const ::DDS::OfferedIncompatibleQosStatus & status)
      ACE_THROW_SPEC ((::CORBA::SystemException));

    virtual void on_liveliness_lost (
        ::DDS::DataWriter_ptr writer,
        const ::DDS::LivelinessLostStatus & status)
      ACE_THROW_SPEC ((::CORBA::SystemException));

    virtual void on_publication_matched (
        ::DDS::DataWriter_ptr writer,
        const ::DDS::PublicationMatchedStatus & status)
      ACE_THROW_SPEC ((::CORBA::SystemException));

    virtual void on_publication_disconnected (
        ::DDS::DataWriter_ptr reader,
        const ::OpenDDS::DCPS::PublicationDisconnectedStatus & status)
      ACE_THROW_SPEC ((::CORBA::SystemException));

    virtual void on_publication_reconnected (
        ::DDS::DataWriter_ptr reader,
        const ::OpenDDS::DCPS::PublicationReconnectedStatus & status)
      ACE_THROW_SPEC ((::CORBA::SystemException));

    virtual void on_publication_lost (
        ::DDS::DataWriter_ptr writer,
        const ::OpenDDS::DCPS::PublicationLostStatus & status)
      ACE_THROW_SPEC ((::CORBA::SystemException));

    virtual void on_connection_deleted (::DDS::DataWriter_ptr writer)
      ACE_THROW_SPEC ((::CORBA::SystemException));

  protected:

    // Destructor
    virtual ~DataWriterListener (void);

  private:

    void display_partitions (DDS::DataWriter_ptr writer) const;

  private:

    /// The number of expected publication matches.
    long const expected_matches_;

    /// The actual number of publication matches.
    ACE_Atomic_Op<ACE_Thread_Mutex, long> publication_matches_;

  };

}

#endif  /* DATA_WRITER_LISTENER_H */
