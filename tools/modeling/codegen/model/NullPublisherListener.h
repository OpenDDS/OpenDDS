// -*- C++ -*-
//
// $Id$
#ifndef NULLPUBLISHERLISTENER_H
#define NULLPUBLISHERLISTENER_H

#include "model_export.h"
#include "dds/DdsDcpsPublicationS.h"
#include "dds/DCPS/Definitions.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

namespace OpenDDS { namespace Model {

  class OpenDDS_Model_Export NullPublisherListener
    : public virtual OpenDDS::DCPS::LocalObject<DDS::PublisherListener>
  {
  public:
    NullPublisherListener();

    virtual ~NullPublisherListener();

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
    );};

} } // End of namespace OpenDDS::Model

#endif /* NULLPUBLISHERLISTENER_H  */

