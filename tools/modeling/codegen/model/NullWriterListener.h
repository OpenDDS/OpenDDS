// -*- C++ -*-
//
// $Id$
#ifndef NULLWRITERLISTENER_H
#define NULLWRITERLISTENER_H

#include "model_export.h"
#include "dds/DdsDcpsPublicationS.h"
#include "dds/DCPS/Definitions.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

namespace OpenDDS { namespace Model {

  class OpenDDS_Model_Export NullWriterListener
    : public virtual OpenDDS::DCPS::LocalObject<OpenDDS::DCPS::DataWriterListener>
  {
  public:
    NullWriterListener();

    virtual ~NullWriterListener();

    virtual void on_offered_deadline_missed(
      DDS::DataWriter_ptr writer,
      const DDS::OfferedDeadlineMissedStatus& status
    );

    virtual void on_offered_incompatible_qos(
      DDS::DataWriter_ptr writer,
      const DDS::OfferedIncompatibleQosStatus& status
    );

    virtual void on_liveliness_lost(
      DDS::DataWriter_ptr writer,
      const DDS::LivelinessLostStatus& status
    );

    virtual void on_publication_matched(
      DDS::DataWriter_ptr writer,
      const DDS::PublicationMatchedStatus& status
    );

    virtual void on_publication_disconnected(
      DDS::DataWriter_ptr reader,
      const OpenDDS::DCPS::PublicationDisconnectedStatus& status
    );

    virtual void on_publication_reconnected(
      DDS::DataWriter_ptr reader,
      const OpenDDS::DCPS::PublicationReconnectedStatus& status
    );

    virtual void on_publication_lost(
      DDS::DataWriter_ptr writer,
      const OpenDDS::DCPS::PublicationLostStatus& status
    );

    virtual void on_connection_deleted(
      DDS::DataWriter_ptr writer
    );
  };

} } // End of namespace OpenDDS::Model

#endif  /* NULLWRITERLISTENER_H */

