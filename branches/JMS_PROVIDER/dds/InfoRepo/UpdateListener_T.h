// -*- C++ -*-
//
// $Id$
#ifndef UPDATELISTENER_T_H
#define UPDATELISTENER_T_H

#include "dds/DCPS/SubscriberImpl.h"
#include "UpdateReceiver_T.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

namespace OpenDDS { namespace Federator {

/// @class UpdateListener< DataType, ReaderType>
template< class DataType, class ReaderType>
class UpdateListener
  : public virtual ::OpenDDS::DCPS::LocalObject< ::DDS::DataReaderListener>
{
  public:
    /// Default constructor
    UpdateListener( UpdateProcessor< DataType>& processor);

    /// Virtual destructor
    virtual ~UpdateListener();

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

    /// Access our Federation Id value.
    RepoKey& federationId();
    RepoKey  federationId() const;

    void stop ();
    void join ();

  private:
    /// Our Federation Id value.
    RepoKey federationId_;

    /// Manager object to delegate sample processing to.
    UpdateReceiver< DataType> receiver_;

};

}} // End of namespace OpenDDS::Federator

#if defined (ACE_TEMPLATES_REQUIRE_SOURCE)
#include "UpdateListener_T.cpp"
#endif /* ACE_TEMPLATES_REQUIRE_SOURCE */

#if defined (ACE_TEMPLATES_REQUIRE_PRAGMA)
#pragma message ("UpdateListener_T.cpp template inst")
#pragma implementation ("UpdateListener_T.cpp")
#endif /* ACE_TEMPLATES_REQUIRE_PRAGMA */

#endif /* UPDATELISTENER_T_H  */

