// -*- C++ -*-
//

#ifndef DATAWRITER_LISTENER_IMPL
#define DATAWRITER_LISTENER_IMPL

#include "dds/DdsDcpsPublicationC.h"
#include "dds/DCPS/Definitions.h"
#include "dds/DCPS/LocalObject.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */


class DataWriterListenerImpl
  : public virtual OpenDDS::DCPS::LocalObject<OpenDDS::DCPS::DataWriterListener>
{
public:

  DataWriterListenerImpl();

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

  virtual ~DataWriterListenerImpl (void);

  OpenDDS::DCPS::Atomic<bool> publication_matched_;

};

#endif /* DATAWRITER_LISTENER_IMPL  */
