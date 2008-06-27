// -*- C++ -*-
//
// $Id$
#ifndef FEDERATORPUBLICATIONS_H
#define FEDERATORPUBLICATIONS_H

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "federator_export.h"
#include "dds/DCPS/PublisherImpl.h"

#include "FederatorTypeSupportC.h"
#include "FederatorTypeSupportImpl.h"

#include <string>

namespace OpenDDS { namespace Federator {

/**
 * @class Publications
 *
 * @brief Manages the publications to Topics in a remote repository.
 *
 * This class manages the Update Topic publications for
 * data published to a remote repository.
 *
 * All managed publications are created in the local repository.  They
 * are also in the <self> DDS domain.  The supplied publisher is
 * expected to have a PARTITION value of "<self>-<remote>" which is
 * intended to restrict the data to only that data destined for the
 * remote repository.
 */
class OpenDDS_Federator_Export Publications  {
  public:
    /// Default constructor
    Publications();

    /// Virtual destructor.
    virtual ~Publications();

    /**
     * @brief Lazy initialization.
     *
     * @param participant the DomainParticipant in which to create the Publisher
     * @param publisher   the Publisher for all the publication Topics
     */
    void initialize(
           ::DDS::DomainParticipant_ptr participant,
           ::DDS::Publisher_ptr         publisher
         );

  private:
    ::DDS::DomainParticipant_var participant_;
    ::DDS::Publisher_var         publisher_;

    //
    // Keep the narrow-ed versions of the writers to avoid the need to
    // perform that operation each time we need to publish data.
    //

    /// ParticipantUpdate specific DataWriter.
    ParticipantUpdateDataWriter_var participantWriter_;

    /// TopicUpdate specific DataWriter.
    TopicUpdateDataWriter_var topicWriter_;

    /// PublicationUpdate specific DataWriter.
    PublicationUpdateDataWriter_var publicationWriter_;

    /// SubscriptionUpdate specific DataWriter.
    SubscriptionUpdateDataWriter_var subscriptionWriter_;

};

}} // End namespace OpenDDS::Federator

#if defined (__ACE_INLINE__)
# include "FederatorPublications.inl"
#endif  /* __ACE_INLINE__ */

#endif // FEDERATORPUBLICATIONS_H

