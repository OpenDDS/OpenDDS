// -*- C++ -*-
//
#ifndef NULLPARTICIPANTLISTENER_H
#define NULLPARTICIPANTLISTENER_H

#include "model_export.h"
#include "dds/DdsDcpsDomainC.h"
#include "dds/DCPS/Definitions.h"
#include "dds/DCPS/LocalObject.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS { namespace Model {

  class OpenDDS_Model_Export NullParticipantListener
    : public virtual OpenDDS::DCPS::LocalObject<DDS::DomainParticipantListener>
  {
  public:
    NullParticipantListener();

    virtual ~NullParticipantListener();

    virtual void on_inconsistent_topic (
      DDS::Topic_ptr the_topic,
      const DDS::InconsistentTopicStatus& status
    );

    virtual void on_data_on_readers (
      DDS::Subscriber_ptr subs
    );

    virtual void on_offered_deadline_missed (
      DDS::DataWriter_ptr writer,
      const DDS::OfferedDeadlineMissedStatus& status
    );

    virtual void on_offered_incompatible_qos (
      DDS::DataWriter_ptr writer,
      const DDS::OfferedIncompatibleQosStatus& status
    );

    virtual void on_liveliness_lost (
      DDS::DataWriter_ptr writer,
      const DDS::LivelinessLostStatus& status
    );

    virtual void on_publication_matched (
      DDS::DataWriter_ptr writer,
      const DDS::PublicationMatchedStatus& status
    );

    virtual void on_requested_deadline_missed (
      DDS::DataReader_ptr reader,
      const DDS::RequestedDeadlineMissedStatus& status
    );

    virtual void on_requested_incompatible_qos (
      DDS::DataReader_ptr reader,
      const DDS::RequestedIncompatibleQosStatus& status
    );

    virtual void on_sample_rejected (
      DDS::DataReader_ptr reader,
      const DDS::SampleRejectedStatus& status
    );

    virtual void on_liveliness_changed (
      DDS::DataReader_ptr reader,
      const DDS::LivelinessChangedStatus& status
    );

    virtual void on_data_available (
      DDS::DataReader_ptr reader
    );

    virtual void on_subscription_matched (
      DDS::DataReader_ptr reader,
      const DDS::SubscriptionMatchedStatus& status
    );

    virtual void on_sample_lost (
      DDS::DataReader_ptr reader,
      const DDS::SampleLostStatus& status
    );

  };

} } // End of namespace OpenDDS::Model

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* NULLPARTICIPANTLISTENER_H  */
