// -*- C++ -*-
//
// $Id$
#ifndef NULLSUBSCRIBERLISTENER_H
#define NULLSUBSCRIBERLISTENER_H

#include "model_export.h"
#include <dds/DdsDcpsSubscriptionS.h>

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

namespace OpenDDS { namespace Model {

  class OpenDDS_Model_Export NullSubscriberListener
    : public virtual OpenDDS::DCPS::LocalObject<DDS::SubscriberListener> {
    
  public:
    NullSubscriberListener();

    virtual ~NullSubscriberListener();

    virtual void on_data_on_readers(
      DDS::Subscriber_ptr subs
    ) ACE_THROW_SPEC((CORBA::SystemException));

    virtual void on_requested_deadline_missed(
      DDS::DataReader_ptr reader,
      const DDS::RequestedDeadlineMissedStatus& status
    ) ACE_THROW_SPEC((CORBA::SystemException));

    virtual void on_requested_incompatible_qos(
      DDS::DataReader_ptr reader,
      const DDS::RequestedIncompatibleQosStatus& status
    ) ACE_THROW_SPEC((CORBA::SystemException));

    virtual void on_liveliness_changed(
      DDS::DataReader_ptr reader,
      const DDS::LivelinessChangedStatus& status
    ) ACE_THROW_SPEC((CORBA::SystemException));

    virtual void on_subscription_matched(
      DDS::DataReader_ptr reader,
      const DDS::SubscriptionMatchedStatus& status
    ) ACE_THROW_SPEC((CORBA::SystemException));

    virtual void on_sample_rejected(
      DDS::DataReader_ptr reader,
      const DDS::SampleRejectedStatus& status
    ) ACE_THROW_SPEC((CORBA::SystemException));

    virtual void on_data_available(
      DDS::DataReader_ptr reader
    ) ACE_THROW_SPEC((CORBA::SystemException));

    virtual void on_sample_lost(
      DDS::DataReader_ptr reader,
      const DDS::SampleLostStatus& status
    ) ACE_THROW_SPEC((CORBA::SystemException));

  };

} } // End of namespace OpenDDS::Model

#endif /* NULLSUBSCRIBERLISTENER_H  */

