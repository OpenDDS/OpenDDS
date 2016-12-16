// -*- C++ -*-
//

#ifndef DATA_WRITER_LISTENER_H
#define DATA_WRITER_LISTENER_H

#include "dds/DdsDcpsPublicationC.h"
#include "dds/DCPS/Definitions.h"
#include "dds/DCPS/LocalObject.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */


namespace Test
{

  class DataWriterListener
    : public virtual OpenDDS::DCPS::LocalObject<OpenDDS::DCPS::DataWriterListener>
  {
  public:
    DataWriterListener (long expected_matches);

    virtual void on_offered_deadline_missed (
        ::DDS::DataWriter_ptr writer,
        const ::DDS::OfferedDeadlineMissedStatus & status);

    virtual void on_offered_incompatible_qos (
        ::DDS::DataWriter_ptr writer,
        const ::DDS::OfferedIncompatibleQosStatus & status);

    virtual void on_liveliness_lost (
        ::DDS::DataWriter_ptr writer,
        const ::DDS::LivelinessLostStatus & status);

    virtual void on_publication_matched (
        ::DDS::DataWriter_ptr writer,
        const ::DDS::PublicationMatchedStatus & status);

    virtual void on_publication_disconnected (
        ::DDS::DataWriter_ptr reader,
        const ::OpenDDS::DCPS::PublicationDisconnectedStatus & status);

    virtual void on_publication_reconnected (
        ::DDS::DataWriter_ptr reader,
        const ::OpenDDS::DCPS::PublicationReconnectedStatus & status);

    virtual void on_publication_lost (
        ::DDS::DataWriter_ptr writer,
        const ::OpenDDS::DCPS::PublicationLostStatus & status);

    virtual void on_connection_deleted (::DDS::DataWriter_ptr writer);

  protected:

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
