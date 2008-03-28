// -*- C++ -*-
//
// $Id$
#ifndef FEDERATORSUSCRIPTIONS_H
#define FEDERATORSUSCRIPTIONS_H

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "federator_export.h"
#include "dds/DCPS/SubscriberImpl.h"

#include <vector>

namespace OpenDDS { namespace Federator {

class FederatorManager;

/**
 * @class Subscriptions
 *
 * @brief Manages the subscriptions to Topics in a remote repository.
 *
 * This class manages the Update and LinkState Topic subscriptions for
 * data published in a remote repository.  An (stateless) UpdateListener
 * is created and installed into each subscription for the Topics:
 *
 *   ParticipantUpdate
 *   TopicUpdate
 *   PublicationUpdate
 *   SubscriptionUpdate
 *
 * A LinkStateListener is created and installed into a subscription for
 * the Topic:
 *
 *   LinkState
 *
 * All managed subscriptions are created in the remote repository.  They
 * are also in the <remote> DDS domain.  The supplied subscriber is
 * expected to have a PARTITION value of "<remote>-<self>" which is
 * intended to restrict the data to only that data published from
 * the remote repository.
 */
class OpenDDS_Federator_Export Subscriptions  {
  public:
    /// Default constructor
    Subscriptions();

    /// Virtual destructor.
    virtual ~Subscriptions();

    /**
     * @brief Lazy initialization.
     *
     * @param subscriber  the Subscriber in which to create the subscriptions
     * @param participant the DomainParticipant in which to create the Topics
     * @param manager     an object to be called back by the listeners
     *
     * This will take ownership of the passed subscriber and install the
     * callback object into the listeners for each subscription created.
     */
    void initialize(
      ::DDS::Subscriber_ptr        subscriber,
      ::DDS::DomainParticipant_ptr participant,
      FederatorManager*            manager
    );

    /// Subscribe to the Update Topics from the remote repository.
    void subscribeToUpdates( ::DDS::DomainParticipant_ptr participant);

    /// Unsubscribe from the Update Topics from the remote repository.
    void unsubscribeFromUpdates( ::DDS::DomainParticipant_ptr participant);

  private:
    /// Federator::Manager to callback on events.
    FederatorManager* manager_;

    /// Subscriber in the remote repository/domain.
    ::DDS::Subscriber_var subscriber_;

    /// LinkState Topic reader.
    ::DDS::DataReader_var linkReader_;

    //
    // By convention (but not required) the indices indicate:
    //
    //   Index          Topic
    //     0    ParticipantUpdate
    //     1    TopicUpdate
    //     2    PublicationUpdate
    //     3    SubscriptionUpdate
    //

    /// Readers in the remote repository/domain.
    std::vector< ::DDS::DataReader_var> readers_;

};

}} // End namespace OpenDDS::Federator

#if defined (__ACE_INLINE__)
# include "FederatorSubscriptions.inl"
#endif  /* __ACE_INLINE__ */

#endif // FEDERATORSUSCRIPTIONS_H

