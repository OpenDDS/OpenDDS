// -*- C++ -*-
//
// $Id$
#ifndef FEDERATORLINKLISTENER_H
#define FEDERATORLINKLISTENER_H

#include "dds/DdsDcpsSubscriptionS.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

namespace OpenDDS { namespace Federator {

class FederatorManager;

class LinkListener
  : public virtual OpenDDS::DCPS::LocalObject<DDS::DataReaderListener>
{
  public:
    /// Default constructor
    LinkListener( FederatorManager& manager);

    /// Virtual destructor
    virtual ~LinkListener();

    virtual void on_requested_deadline_missed (
      ::DDS::DataReader_ptr reader,
      const ::DDS::RequestedDeadlineMissedStatus & status
    )
    ACE_THROW_SPEC ((
      CORBA::SystemException
    ));

   virtual void on_requested_incompatible_qos (
      ::DDS::DataReader_ptr reader,
      const ::DDS::RequestedIncompatibleQosStatus & status
    )
    ACE_THROW_SPEC ((
      CORBA::SystemException
    ));

    virtual void on_liveliness_changed (
      ::DDS::DataReader_ptr reader,
      const ::DDS::LivelinessChangedStatus & status
    )
    ACE_THROW_SPEC ((
      CORBA::SystemException
    ));

    virtual void on_subscription_match (
      ::DDS::DataReader_ptr reader,
      const ::DDS::SubscriptionMatchStatus & status
    )
    ACE_THROW_SPEC ((
      CORBA::SystemException
    ));

    virtual void on_sample_rejected(
      ::DDS::DataReader_ptr reader,
      const DDS::SampleRejectedStatus& status
    )
    ACE_THROW_SPEC ((
      CORBA::SystemException
    ));

    virtual void on_data_available(
      ::DDS::DataReader_ptr reader
    )
    ACE_THROW_SPEC ((
      CORBA::SystemException
    ));

    virtual void on_sample_lost(
      ::DDS::DataReader_ptr reader,
      const DDS::SampleLostStatus& status
    )
    ACE_THROW_SPEC ((
      CORBA::SystemException
    ));

  private:
    /// Manager object to delegate sample processing to.
    FederatorManager& manager_;

};

}} // End of namespace OpenDDS::Federator

#endif /* FEDERATORLINKLISTENER_H  */

