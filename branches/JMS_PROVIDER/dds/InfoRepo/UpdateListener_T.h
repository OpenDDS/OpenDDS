// -*- C++ -*-
//
// $Id$
#ifndef UPDATELISTENER_T_H
#define UPDATELISTENER_T_H

#include "dds/DCPS/SubscriberImpl.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

namespace OpenDDS { namespace Federator {

class ManagerImpl;

/**
 * @class UpdateMarker
 *
 * @brief Mark the udpate listeners to enable containment.
 *
 * This Marker class is used to enable the individual UpdateListener<>
 * instantiations to e contained in a single container.  The only use we
 * have for this is to allow the deletion of the contained listeners, so
 * not interfaces are included.
 */
class UpdateMarker { };

/// @class UpdateListener< DataType, ReaderType>
template< class DataType, class ReaderType>
class UpdateListener
  : public virtual ::OpenDDS::DCPS::LocalObject< ::DDS::DataReaderListener>,
    public virtual UpdateMarker
{
  public:
    /// Default constructor
    UpdateListener( ManagerImpl& manager);

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

  private:
    /// Manager object to delegate sample processing to.
    ManagerImpl& manager_;

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

