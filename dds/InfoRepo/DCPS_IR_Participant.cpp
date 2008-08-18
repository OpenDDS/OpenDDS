#include "DcpsInfo_pch.h"
#include /**/ "DCPS_IR_Participant.h"
#include "UpdateManager.h"

#include /**/ "DCPS_IR_Domain.h"
#include /**/ "DCPS_IR_Subscription.h"
#include /**/ "DCPS_IR_Publication.h"
#include /**/ "DCPS_IR_Topic.h"
#include /**/ "DCPS_IR_Topic_Description.h"
#include /**/ "tao/debug.h"

#include <sstream>

DCPS_IR_Participant::DCPS_IR_Participant (long federationId,
                                          OpenDDS::DCPS::RepoId id,
                                          DCPS_IR_Domain* domain,
                                          ::DDS::DomainParticipantQos qos,
                                          UpdateManager* um)
: id_(id),
  domain_(domain),
  qos_(qos),
  aliveStatus_(1),
  handle_(0),
  isBIT_(0),
  federationId_( federationId),
  owner_( OWNER_NONE),
  topicIdGenerator_(
    federationId,
    OpenDDS::DCPS::GuidConverter(id).participantId(),
    OpenDDS::DCPS::KIND_TOPIC
  ),
  publicationIdGenerator_(
    federationId,
    OpenDDS::DCPS::GuidConverter(id).participantId(),
    OpenDDS::DCPS::KIND_WRITER
  ),
  subscriptionIdGenerator_(
    federationId,
    OpenDDS::DCPS::GuidConverter(id).participantId(),
    OpenDDS::DCPS::KIND_READER
  ),
  um_ (um)
{
}



DCPS_IR_Participant::~DCPS_IR_Participant()
{
  if (0 != subscriptions_.current_size())
    {
      DCPS_IR_Subscription* subscription = 0;
      DCPS_IR_Subscription_Map::ITERATOR iter = subscriptions_.begin();
      DCPS_IR_Subscription_Map::ITERATOR end = subscriptions_.end();

      std::stringstream buffer;
      long handle;
      handle = ::OpenDDS::DCPS::GuidConverter( this->id_);
      buffer << this->id_ << "(" << std::hex << handle << ")";

      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: DCPS_IR_Participant::~DCPS_IR_Participant: ")
        ACE_TEXT("domain %d id %s has retained subscriptions.\n"),
        domain_, buffer.str().c_str()
      ));

      while (iter != end)
        {
          subscription = (*iter).int_id_;
          ++iter;

          std::stringstream subscriptionBuffer;
          long handle;
          ::OpenDDS::DCPS::RepoId subscriptionId = subscription->get_id();
          handle = ::OpenDDS::DCPS::GuidConverter( subscriptionId);
          subscriptionBuffer << subscriptionId << "(" << std::hex << handle << ")";

          ACE_ERROR((LM_ERROR,
            ACE_TEXT("(%P|%t) ERROR: DCPS_IR_Participant::~DCPS_IR_Participant: ")
            ACE_TEXT("removing subscription id %s.\n"),
            subscriptionBuffer.str().c_str()
          ));
          remove_subscription( subscriptionId);
        }
    }

  if (0 != publications_.current_size())
    {
      DCPS_IR_Publication* publication = 0;
      DCPS_IR_Publication_Map::ITERATOR iter = publications_.begin();
      DCPS_IR_Publication_Map::ITERATOR end = publications_.end();

      std::stringstream buffer;
      long handle;
      handle = ::OpenDDS::DCPS::GuidConverter( this->id_);
      buffer << this->id_ << "(" << std::hex << handle << ")";

      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: DCPS_IR_Participant::~DCPS_IR_Participant: ")
        ACE_TEXT("domain %d id %s has retained publications.\n"),
        domain_, buffer.str().c_str()
      ));

      while (iter != end)
        {
          publication = (*iter).int_id_;
          ++iter;

          std::stringstream publicationBuffer;
          long handle;
          ::OpenDDS::DCPS::RepoId publicationId = publication->get_id();
          handle = ::OpenDDS::DCPS::GuidConverter( publicationId);
          publicationBuffer << publicationId << "(" << std::hex << handle << ")";

          ACE_ERROR((LM_ERROR,
            ACE_TEXT("(%P|%t) \tERROR: DCPS_IR_Participant::~DCPS_IR_Participant: ")
            ACE_TEXT("removing publication id %s.\n"),
            publicationBuffer.str().c_str()
          ));
          remove_publication( publicationId);
        }
    }

  if (0 != topicRefs_.current_size())
    {
      OpenDDS::DCPS::RepoId topicId = OpenDDS::DCPS::GUID_UNKNOWN;
      DCPS_IR_Topic_Map::ITERATOR iter = topicRefs_.begin();
      DCPS_IR_Topic_Map::ITERATOR end = topicRefs_.end();

      std::stringstream buffer;
      long handle;
      handle = ::OpenDDS::DCPS::GuidConverter( this->id_);
      buffer << this->id_ << "(" << std::hex << handle << ")";

      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: DCPS_IR_Participant::~DCPS_IR_Participant () ")
        ACE_TEXT("domain %d id %s is retaining topics.\n"),
        domain_, buffer.str().c_str()
      ));

      while (iter != end)
        {
          topicId = (*iter).ext_id_;
          ++iter;

          std::stringstream topicBuffer;
          long handle;
          handle = ::OpenDDS::DCPS::GuidConverter( topicId);
          topicBuffer << topicId << "(" << std::hex << handle << ")";

          ACE_ERROR((LM_ERROR,
            ACE_TEXT("(%P|%t) ERROR: DCPS_IR_Participant::~DCPS_IR_Participant: ")
            ACE_TEXT("retained topic id %s.\n"),
            topicBuffer.str().c_str()
          ));
        }
    }

}


void
DCPS_IR_Participant::takeOwnership()
{
  /// Publish an update with our ownership.
  if( this->um_) {
    this->um_->add( this->domain_->get_id(), this->id_, this->federationId_);
  }

  // And now handle our internal ownership processing.
  this->changeOwner( this->federationId_, this->federationId_);
}

void
DCPS_IR_Participant::changeOwner( long sender, long owner)
{
  { ACE_GUARD( ACE_SYNCH_MUTEX, guard, this->ownerLock_);

    if( (owner == OWNER_NONE)
     && ( this->owner() || (this->owner_ != sender)
        )
      ) {
      // Do not eliminate ownership if we are the owner or if the update
      // does not come from the current owner.
      return;
    }

    // Finally.  Change the value.
    this->owner_ = owner;

  } // End of lock scope.

  if( this->owner()) {
    /// @TODO: Ensure that any stalled callbacks are made.
  }
}

bool
DCPS_IR_Participant::owner() const
{
  return this->owner_ == this->federationId_;
}

int DCPS_IR_Participant::add_publication (DCPS_IR_Publication* pub)
{
  OpenDDS::DCPS::RepoId pubId = pub->get_id();
  int status = publications_.bind(pubId, pub);

  switch (status)
    {
    case 0:
      if (::OpenDDS::DCPS::DCPS_debug_level > 0) {
        std::stringstream buffer;
        long handle;
        handle = ::OpenDDS::DCPS::GuidConverter( this->id_);
        buffer << this->id_ << "(" << std::hex << handle << ")";

        std::stringstream publicationBuffer;
        handle = ::OpenDDS::DCPS::GuidConverter( pubId);
        publicationBuffer << pubId << "(" << std::hex << handle << ")";

        ACE_DEBUG((LM_DEBUG,
          ACE_TEXT("(%P|%t) DCPS_IR_Participant::add_publication: ")
          ACE_TEXT("participant %s added publication %s at %x.\n"),
          buffer.str().c_str(),
          publicationBuffer.str().c_str(),
          pub
        ));
      }
      break;

    case 1:
      {
        std::stringstream buffer;
        long handle;
        handle = ::OpenDDS::DCPS::GuidConverter( this->id_);
        buffer << this->id_ << "(" << std::hex << handle << ")";

        std::stringstream publicationBuffer;
        handle = ::OpenDDS::DCPS::GuidConverter( pubId);
        publicationBuffer << pubId << "(" << std::hex << handle << ")";

        ACE_ERROR((LM_ERROR,
          ACE_TEXT("(%P|%t) ERROR: DCPS_IR_Participant::add_publication: ")
          ACE_TEXT("participant %s attempt to re-add publication %s\n"),
          buffer.str().c_str(),
          publicationBuffer.str().c_str()
        ));
      }
      break;

    case -1:
      {
        std::stringstream buffer;
        long handle;
        handle = ::OpenDDS::DCPS::GuidConverter( this->id_);
        buffer << this->id_ << "(" << std::hex << handle << ")";

        std::stringstream publicationBuffer;
        handle = ::OpenDDS::DCPS::GuidConverter( pubId);
        publicationBuffer << pubId << "(" << std::hex << handle << ")";

        ACE_ERROR((LM_ERROR,
          ACE_TEXT("(%P|%t) ERROR: DCPS_IR_Participant::add_publication: ")
          ACE_TEXT("participant %s failed to add publication %s\n"),
          buffer.str().c_str(),
          publicationBuffer.str().c_str()
        ));
      }
    };

  return status;
}


int DCPS_IR_Participant::find_publication_reference (OpenDDS::DCPS::RepoId pubId,
                                                     DCPS_IR_Publication* & pub)
{
  int status = publications_.find (pubId, pub);
  if (0 == status)
  {
    if (::OpenDDS::DCPS::DCPS_debug_level > 0)
    {
      std::stringstream buffer;
      long handle;
      handle = ::OpenDDS::DCPS::GuidConverter( pubId);
      buffer << pubId << "(" << std::hex << handle << ")";

      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) DCPS_IR_Participant::find_publication_reference: ")
        ACE_TEXT("found datawriter reference %x id: %s\n"),
        pub,
        buffer.str().c_str()
      ));
    }
  }
  else
  {
    std::stringstream buffer;
    long handle;
    handle = ::OpenDDS::DCPS::GuidConverter( pubId);
    buffer << pubId << "(" << std::hex << handle << ")";

    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: DCPS_IR_Participant::find_publication_reference: ")
      ACE_TEXT("could not find datawriter reference %x id: %s\n"),
      pub,
      buffer.str().c_str()
    ));
  } // if (0 == status)

  return status;
}


int DCPS_IR_Participant::remove_publication (OpenDDS::DCPS::RepoId pubId)
{
  DCPS_IR_Publication* pub = 0;

  int status = publications_.unbind (pubId, pub);

  if (0 == status)
    {
      DCPS_IR_Topic* topic = pub->get_topic ();
      topic->remove_publication_reference(pub);

      CORBA::Boolean dont_notify_lost = 0;
      status = pub->remove_associations(dont_notify_lost);
      if (0 != status)
      {
        std::stringstream buffer;
        long handle;
        handle = ::OpenDDS::DCPS::GuidConverter( pubId);
        buffer << pubId << "(" << std::hex << handle << ")";

        ACE_ERROR((LM_ERROR,
          ACE_TEXT("(%P|%t) ERROR: DCPS_IR_Participant::remove_publication: ")
          ACE_TEXT("unable to remove associations from publication id: %s\n"),
          buffer.str().c_str()
        ));
        return status;
      }

      if (::OpenDDS::DCPS::DCPS_debug_level > 0)
        {
          std::stringstream buffer;
          long handle;
          handle = ::OpenDDS::DCPS::GuidConverter( pubId);
          buffer << pubId << "(" << std::hex << handle << ")";

          ACE_DEBUG((LM_DEBUG,
            ACE_TEXT("(%P|%t) DCPS_IR_Participant::remove_publication: ")
            ACE_TEXT("removed publication id: %s\n"),
            buffer.str().c_str()
          ));
        } // if (::OpenDDS::DCPS::DCPS_debug_level > 0)

      // Dispose the BIT information
      domain_->dispose_publication_bit(pub);

      delete pub;
    }
  else
    {
      std::stringstream buffer;
      long handle;
      handle = ::OpenDDS::DCPS::GuidConverter( pubId);
      buffer << pubId << "(" << std::hex << handle << ")";

      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: DCPS_IR_Participant::remove_publication: ")
        ACE_TEXT("failed to remove publication %x id: %s\n"),
        pub,
        buffer.str().c_str()
      ));
    } // if (0 == status)

  return status;
}



int DCPS_IR_Participant::add_subscription (DCPS_IR_Subscription* sub)
{
  OpenDDS::DCPS::RepoId subId = sub->get_id();
  int status = subscriptions_.bind(subId, sub);

  switch (status)
    {
    case 0:
      if (::OpenDDS::DCPS::DCPS_debug_level > 0)
        {
          std::stringstream buffer;
          long handle;
          handle = ::OpenDDS::DCPS::GuidConverter( subId);
          buffer << subId << "(" << std::hex << handle << ")";

          ACE_DEBUG((LM_DEBUG,
            ACE_TEXT("(%P|%t) DCPS_IR_Participant::add_subscription: ")
            ACE_TEXT("successfully added subscription %x id: %s.\n"),
            sub,
            buffer.str().c_str()
          ));
        }
      break;
    case 1:
      {
        std::stringstream buffer;
        long handle;
        handle = ::OpenDDS::DCPS::GuidConverter( subId);
        buffer << subId << "(" << std::hex << handle << ")";

        ACE_ERROR((LM_ERROR,
          ACE_TEXT("(%P|%t) ERROR: DCPS_IR_Participant::add_subscription: ")
          ACE_TEXT("attempted to re-add existing subscription id %s.\n"),
          buffer.str().c_str()
        ));
      }
      break;
    case -1:
      {
        std::stringstream buffer;
        long handle;
        handle = ::OpenDDS::DCPS::GuidConverter( subId);
        buffer << subId << "(" << std::hex << handle << ")";

        ACE_ERROR((LM_ERROR,
          ACE_TEXT("(%P|%t) ERROR: DCPS_IR_Participant::add_subscription: ")
          ACE_TEXT("unknown error while adding subscription id %s.\n"),
          buffer.str().c_str()
        ));
      }
    };

  return status;
}


 int DCPS_IR_Participant::find_subscription_reference (OpenDDS::DCPS::RepoId subId,
                                                       DCPS_IR_Subscription*& sub)
{
  int status = subscriptions_.find (subId, sub);
  if (0 == status)
  {
    if (::OpenDDS::DCPS::DCPS_debug_level > 0)
    {
      std::stringstream buffer;
      long handle;
      handle = ::OpenDDS::DCPS::GuidConverter( subId);
      buffer << subId << "(" << std::hex << handle << ")";

      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) DCPS_IR_Participant::find_subscription_reference: ")
        ACE_TEXT("found datareader reference %x id: %s.\n"),
        sub,
        buffer.str().c_str()
      ));
    }
  }
  else
  {
    std::stringstream buffer;
    long handle;
    handle = ::OpenDDS::DCPS::GuidConverter( subId);
    buffer << subId << "(" << std::hex << handle << ")";

    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: DCPS_IR_Participant::find_subscription_reference: ")
      ACE_TEXT("could not find datareader reference %x id: %s.\n"),
      sub,
      buffer.str().c_str()
    ));
  } // if (0 == status)

  return status;
}

int DCPS_IR_Participant::remove_subscription (OpenDDS::DCPS::RepoId subId)
{
  DCPS_IR_Subscription* sub = 0;

  int status = subscriptions_.unbind (subId, sub);

  if (0 == status)
    {
      DCPS_IR_Topic* topic = sub->get_topic ();
      topic->remove_subscription_reference(sub);
       
      CORBA::Boolean dont_notify_lost = 0;
      status = sub->remove_associations(dont_notify_lost);
      if (0 != status)
      {
        std::stringstream buffer;
        long handle;
        handle = ::OpenDDS::DCPS::GuidConverter( subId);
        buffer << subId << "(" << std::hex << handle << ")";

        ACE_ERROR((LM_ERROR,
          ACE_TEXT("(%P|%t) ERROR: DCPS_IR_Participant::remove_subscription: ")
          ACE_TEXT("could not remove all associations for subscription id: %s.\n"),
          buffer.str().c_str()
        ));
        return status;
      }

      if (::OpenDDS::DCPS::DCPS_debug_level > 0)
        {
          std::stringstream buffer;
          long handle;
          handle = ::OpenDDS::DCPS::GuidConverter( subId);
          buffer << subId << "(" << std::hex << handle << ")";

          ACE_DEBUG((LM_DEBUG,
            ACE_TEXT("(%P|%t) DCPS_IR_Participant::remove_subscription: ")
            ACE_TEXT("removed subscription id: %s.\n"),
            buffer.str().c_str()
          ));
        }

      // Dispose the BIT information
      domain_->dispose_subscription_bit(sub);

      delete sub;
    }
  else
    {
      std::stringstream buffer;
      long handle;
      handle = ::OpenDDS::DCPS::GuidConverter( subId);
      buffer << subId << "(" << std::hex << handle << ")";

      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: DCPS_IR_Participant::remove_subscription: ")
        ACE_TEXT("failed to remove subscription %x id: %s.\n"),
        sub,
        buffer.str().c_str()
      ));
    } // if (0 == status)

  return status;
}


int DCPS_IR_Participant::add_topic_reference (DCPS_IR_Topic* topic)
{
  OpenDDS::DCPS::RepoId topicId = topic->get_id();
  int status = topicRefs_.bind(topicId, topic);

  switch (status)
    {
    case 0:
      if (::OpenDDS::DCPS::DCPS_debug_level > 0) {
        std::stringstream buffer;
        long handle;
        handle = ::OpenDDS::DCPS::GuidConverter( topicId);
        buffer << topicId << "(" << std::hex << handle << ")";

        ACE_DEBUG((LM_DEBUG,
          ACE_TEXT("(%P|%t) DCPS_IR_Participant::add_topic_reference: ")
          ACE_TEXT("successfully added topic reference %x id: %s.\n"),
          topic,
          buffer.str().c_str()
        ));
      }
      break;

    case 1:
      {
        std::stringstream buffer;
        long handle;
        handle = ::OpenDDS::DCPS::GuidConverter( topicId);
        buffer << topicId << "(" << std::hex << handle << ")";

        ACE_ERROR((LM_ERROR,
          ACE_TEXT("(%P|%t) ERROR: DCPS_IR_Participant::add_topic_reference: ")
          ACE_TEXT("Attempted to add existing topic reference id %s.\n"),
          buffer.str().c_str()
        ));
      }
      break;

    case -1:
      {
        std::stringstream buffer;
        long handle;
        handle = ::OpenDDS::DCPS::GuidConverter( topicId);
        buffer << topicId << "(" << std::hex << handle << ")";

        ACE_ERROR((LM_ERROR,
          ACE_TEXT("(%P|%t) ERROR: DCPS_IR_Participant::add_topic_reference: ")
          ACE_TEXT("unknown error while adding topic reference id %s.\n"),
          buffer.str().c_str()
        ));
      }
    };

  return status;
}


int DCPS_IR_Participant::remove_topic_reference (OpenDDS::DCPS::RepoId topicId,
                                                 DCPS_IR_Topic*& topic)
{
  int status = topicRefs_.unbind (topicId, topic);

  if (0 == status)
    {
      if (::OpenDDS::DCPS::DCPS_debug_level > 0) {
        std::stringstream buffer;
        long handle;
        handle = ::OpenDDS::DCPS::GuidConverter( topicId);
        buffer << topicId << "(" << std::hex << handle << ")";

        ACE_DEBUG((LM_DEBUG,
          ACE_TEXT("(%P|%t) DCPS_IR_Participant::remove_topic_reference: ")
          ACE_TEXT("removed topic reference %x id: %s.\n"),
          topic,
          buffer.str().c_str()
        ));
      }
    }
  else
    {
      std::stringstream buffer;
      long handle;
      handle = ::OpenDDS::DCPS::GuidConverter( topicId);
      buffer << topicId << "(" << std::hex << handle << ")";

      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: DCPS_IR_Participant::remove_topic_reference: ")
        ACE_TEXT("failed to remove topic reference %x id: %s.\n"),
        topic,
        buffer.str().c_str()
      ));
    } // if (0 == status)

  return status;
}


int DCPS_IR_Participant::find_topic_reference (OpenDDS::DCPS::RepoId topicId,
                                               DCPS_IR_Topic*& topic)
{
  int status = topicRefs_.find (topicId, topic);

  if (0 == status)
    {
      if (::OpenDDS::DCPS::DCPS_debug_level > 0) {
        std::stringstream buffer;
        long handle;
        handle = ::OpenDDS::DCPS::GuidConverter( topicId);
        buffer << topicId << "(" << std::hex << handle << ")";

        ACE_DEBUG((LM_DEBUG,
          ACE_TEXT("(%P|%t) DCPS_IR_Participant::find_topic_reference: ")
          ACE_TEXT("found topic reference %x id: %s.\n"),
          topic,
          buffer.str().c_str()
        ));
      }
    }
  else
    {
      std::stringstream buffer;
      long handle;
      handle = ::OpenDDS::DCPS::GuidConverter( topicId);
      buffer << topicId << "(" << std::hex << handle << ")";

      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: DCPS_IR_Participant::find_topic_reference: ")
        ACE_TEXT("failed to find topic reference %x id: %s.\n"),
        topic,
        buffer.str().c_str()
      ));
    } // if (0 == status)

  return status;
}


void DCPS_IR_Participant::remove_all_dependents (CORBA::Boolean notify_lost)
{
  DCPS_IR_Topic* topic = 0;

  // remove all the publications associations
  DCPS_IR_Publication* pub = 0;
  DCPS_IR_Publication_Map::ITERATOR pubIter = publications_.begin();
  DCPS_IR_Publication_Map::ITERATOR pubEnd = publications_.end();

  while (pubIter != pubEnd)
    {
      pub = (*pubIter).int_id_;
      ++pubIter;

      topic = pub->get_topic ();
      topic->remove_publication_reference(pub);
      if (0 != pub->remove_associations(notify_lost))
        {
          return;
        }
    }

  // remove all the subscriptions associations
  DCPS_IR_Subscription* sub = 0;
  DCPS_IR_Subscription_Map::ITERATOR subIter = subscriptions_.begin();
  DCPS_IR_Subscription_Map::ITERATOR subEnd = subscriptions_.end();

  while (subIter != subEnd)
    {
      sub = (*subIter).int_id_;
      ++subIter;

      topic = sub->get_topic ();
      topic->remove_subscription_reference(sub);

      if (0 != sub->remove_associations(notify_lost))
        {
          return;
        }
    }

  // remove all the topics
  topic = 0;
  DCPS_IR_Topic_Map::ITERATOR topicIter = topicRefs_.begin();
  DCPS_IR_Topic_Map::ITERATOR topicEnd = topicRefs_.end();

  while (topicIter != topicEnd)
    {
      topic = (*topicIter).int_id_;
      ++topicIter;

      if (um_) {
        um_->remove (Topic, topic->get_id());
      }
      domain_->remove_topic(this, topic);
    }
  topicRefs_.unbind_all();

  // The publications and subscriptions
  // can NOT be deleted until after all
  // the associations have been removed.
  // Otherwise an access violation can
  // occur because a publication and
  // subscription of this participant
  // could be associated.

  // delete all the publications
  pubIter = publications_.begin();
  while (pubIter != pubEnd)
    {
      pub = (*pubIter).int_id_;
      ++pubIter;
      if (um_) {
        um_->remove (Actor, pub->get_id());
      }

      delete pub;
    }
  publications_.unbind_all();

  // delete all the subscriptions
  subIter = subscriptions_.begin();
  while (subIter != subEnd)
    {
      sub = (*subIter).int_id_;
      ++subIter;
      if (um_) {
        um_->remove (Actor, sub->get_id());
      }

      delete sub;
    }
  subscriptions_.unbind_all();

  if (um_) {
    um_->remove (Participant, this->get_id());
  }
}


void DCPS_IR_Participant::mark_dead ()
{
  aliveStatus_ = 0;
  domain_->add_dead_participant(this);
}


OpenDDS::DCPS::RepoId DCPS_IR_Participant::get_id ()
{
  return id_;
}


CORBA::Boolean DCPS_IR_Participant::is_alive ()
{
  return aliveStatus_;
}


void DCPS_IR_Participant::set_alive (CORBA::Boolean alive)
{
  aliveStatus_ = alive;
}



void DCPS_IR_Participant::ignore_participant (OpenDDS::DCPS::RepoId id)
{
  if (::OpenDDS::DCPS::DCPS_debug_level > 0)
    {
      std::stringstream buffer;
      long handle;
      handle = ::OpenDDS::DCPS::GuidConverter( this->id_);
      buffer << this->id_ << "(" << std::hex << handle << ")";

      std::stringstream ignoreBuffer;
      handle = ::OpenDDS::DCPS::GuidConverter( id);
      ignoreBuffer << id << "(" << std::hex << handle << ")";

      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) DCPS_IR_Participant::ignore_participant: ")
        ACE_TEXT("participant %s now ignoring participant %s.\n"),
        buffer.str().c_str(),
        ignoreBuffer.str().c_str()
      ));
    }

  ignoredParticipants_.insert(id);

  // check for any subscriptions or publications to disassociate
  DCPS_IR_Publication* pub = 0;
  DCPS_IR_Publication_Map::ITERATOR pubIter = publications_.begin();
  DCPS_IR_Publication_Map::ITERATOR pubEnd = publications_.end();

  while (pubIter != pubEnd)
    {
      pub = (*pubIter).int_id_;
      ++pubIter;
      pub->disassociate_participant(id);
    }

  DCPS_IR_Subscription* sub = 0;
  DCPS_IR_Subscription_Map::ITERATOR subIter = subscriptions_.begin();
  DCPS_IR_Subscription_Map::ITERATOR subEnd = subscriptions_.end();

  while (subIter != subEnd)
    {
      sub = (*subIter).int_id_;
      ++subIter;
      sub->disassociate_participant(id);
    }
}


void DCPS_IR_Participant::ignore_topic (OpenDDS::DCPS::RepoId id)
{
  if (::OpenDDS::DCPS::DCPS_debug_level > 0)
    {
      std::stringstream buffer;
      long handle;
      handle = ::OpenDDS::DCPS::GuidConverter( this->id_);
      buffer << this->id_ << "(" << std::hex << handle << ")";

      std::stringstream ignoreBuffer;
      handle = ::OpenDDS::DCPS::GuidConverter( id);
      ignoreBuffer << id << "(" << std::hex << handle << ")";

      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) DCPS_IR_Participant::ignore_topic: ")
        ACE_TEXT("participant %s now ignoring topic %s.\n"),
        buffer.str().c_str(),
        ignoreBuffer.str().c_str()
      ));
    }

  ignoredTopics_.insert(id);

  // check for any subscriptions or publications to disassociate
  DCPS_IR_Publication* pub = 0;
  DCPS_IR_Publication_Map::ITERATOR pubIter = publications_.begin();
  DCPS_IR_Publication_Map::ITERATOR pubEnd = publications_.end();

  while (pubIter != pubEnd)
    {
      pub = (*pubIter).int_id_;
      ++pubIter;
      pub->disassociate_topic(id);
    }

  DCPS_IR_Subscription* sub = 0;
  DCPS_IR_Subscription_Map::ITERATOR subIter = subscriptions_.begin();
  DCPS_IR_Subscription_Map::ITERATOR subEnd = subscriptions_.end();

  while (subIter != subEnd)
    {
      sub = (*subIter).int_id_;
      ++subIter;
      sub->disassociate_topic(id);
    }
}


void DCPS_IR_Participant::ignore_publication (OpenDDS::DCPS::RepoId id)
{
    if (::OpenDDS::DCPS::DCPS_debug_level > 0)
    {
      std::stringstream buffer;
      long handle;
      handle = ::OpenDDS::DCPS::GuidConverter( this->id_);
      buffer << this->id_ << "(" << std::hex << handle << ")";

      std::stringstream ignoreBuffer;
      handle = ::OpenDDS::DCPS::GuidConverter( id);
      ignoreBuffer << id << "(" << std::hex << handle << ")";

      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) DCPS_IR_Participant::ignore_publication: ")
        ACE_TEXT("participant %s now ignoring publication %s.\n"),
        buffer.str().c_str(),
        ignoreBuffer.str().c_str()
      ));
    }

  ignoredPublications_.insert(id);

  // check for any subscriptions to disassociate
  DCPS_IR_Subscription* sub = 0;
  DCPS_IR_Subscription_Map::ITERATOR subIter = subscriptions_.begin();
  DCPS_IR_Subscription_Map::ITERATOR subEnd = subscriptions_.end();

  while (subIter != subEnd)
    {
      sub = (*subIter).int_id_;
      ++subIter;
      sub->disassociate_publication(id);
    }
}


void DCPS_IR_Participant::ignore_subscription (OpenDDS::DCPS::RepoId id)
{
    if (::OpenDDS::DCPS::DCPS_debug_level > 0)
    {
      std::stringstream buffer;
      long handle;
      handle = ::OpenDDS::DCPS::GuidConverter( this->id_);
      buffer << this->id_ << "(" << std::hex << handle << ")";

      std::stringstream ignoreBuffer;
      handle = ::OpenDDS::DCPS::GuidConverter( id);
      ignoreBuffer << id << "(" << std::hex << handle << ")";

      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) DCPS_IR_Participant::ignore_subscription: ")
        ACE_TEXT("participant %s now ignoring subscription %s.\n"),
        buffer.str().c_str(),
        ignoreBuffer.str().c_str()
      ));
    }
  ignoredSubscriptions_.insert(id);

  // check for any subscriptions or publications to disassociate
  DCPS_IR_Publication* pub = 0;
  DCPS_IR_Publication_Map::ITERATOR pubIter = publications_.begin();
  DCPS_IR_Publication_Map::ITERATOR pubEnd = publications_.end();

  while (pubIter != pubEnd)
    {
      pub = (*pubIter).int_id_;
      ++pubIter;
      pub->disassociate_subscription(id);
    }
}



CORBA::Boolean DCPS_IR_Participant::is_participant_ignored (OpenDDS::DCPS::RepoId id)
{
  return (0 == ignoredParticipants_.find(id));
}


CORBA::Boolean DCPS_IR_Participant::is_topic_ignored (OpenDDS::DCPS::RepoId id)
{
  return (0 == ignoredTopics_.find(id));
}


CORBA::Boolean DCPS_IR_Participant::is_publication_ignored (OpenDDS::DCPS::RepoId id)
{
  return (0 == ignoredPublications_.find(id));
}


CORBA::Boolean DCPS_IR_Participant::is_subscription_ignored (OpenDDS::DCPS::RepoId id)
{
  return (0 == ignoredSubscriptions_.find(id));
}


::DDS::InstanceHandle_t DCPS_IR_Participant::get_handle()
{
  return handle_;
}


void DCPS_IR_Participant::set_handle(::DDS::InstanceHandle_t handle)
{
  handle_ = handle;
}


const ::DDS::DomainParticipantQos* DCPS_IR_Participant::get_qos ()
{
  return &qos_;
}


bool DCPS_IR_Participant::set_qos (const ::DDS::DomainParticipantQos & qos)
{
  // Do not need re-evaluate compatibility and associations when
  // DomainParticipantQos changes since only datareader and datawriter
  // QoS are evaludated during normal associations establishment.

  // Do not need publish the QoS change to topics or datareader or 
  // datawriter BIT as they are independent.
  qos_ = qos;
  this->domain_->publish_participant_bit (this);

  return true;
}


CORBA::Boolean DCPS_IR_Participant::is_bit ()
{
  return isBIT_;
}


void DCPS_IR_Participant::set_bit_status (CORBA::Boolean isBIT)
{
  isBIT_ = isBIT;
}


DCPS_IR_Domain* DCPS_IR_Participant::get_domain_reference () const
{
  return domain_;
}

OpenDDS::DCPS::RepoId
DCPS_IR_Participant::get_next_topic_id()
{
  return this->topicIdGenerator_.next();
}

OpenDDS::DCPS::RepoId
DCPS_IR_Participant::get_next_publication_id()
{
  return this->publicationIdGenerator_.next();
}

OpenDDS::DCPS::RepoId
DCPS_IR_Participant::get_next_subscription_id()
{
  return this->subscriptionIdGenerator_.next();
}

void
DCPS_IR_Participant::last_topic_key( long key)
{
  return this->topicIdGenerator_.last( key);
}

void
DCPS_IR_Participant::last_publication_key( long key)
{
  return this->publicationIdGenerator_.last( key);
}

void
DCPS_IR_Participant::last_subscription_key( long key)
{
  return this->subscriptionIdGenerator_.last( key);
}

#if defined (ACE_HAS_EXPLICIT_TEMPLATE_INSTANTIATION)

template class ACE_Map_Entry<OpenDDS::DCPS::RepoId,DCPS_IR_Publication*>;
template class ACE_Map_Manager<OpenDDS::DCPS::RepoId,DCPS_IR_Publication*,ACE_Null_Mutex>;
template class ACE_Map_Iterator_Base<OpenDDS::DCPS::RepoId,DCPS_IR_Publication*,ACE_Null_Mutex>;
template class ACE_Map_Iterator<OpenDDS::DCPS::RepoId,DCPS_IR_Publication*,ACE_Null_Mutex>;
template class ACE_Map_Reverse_Iterator<OpenDDS::DCPS::RepoId,DCPS_IR_Publication*,ACE_Null_Mutex>;

template class ACE_Map_Entry<OpenDDS::DCPS::RepoId,DCPS_IR_Subscription*>;
template class ACE_Map_Manager<OpenDDS::DCPS::RepoId,DCPS_IR_Subscription*,ACE_Null_Mutex>;
template class ACE_Map_Iterator_Base<OpenDDS::DCPS::RepoId,DCPS_IR_Subscription*,ACE_Null_Mutex>;
template class ACE_Map_Iterator<OpenDDS::DCPS::RepoId,DCPS_IR_Subscription*,ACE_Null_Mutex>;
template class ACE_Map_Reverse_Iterator<OpenDDS::DCPS::RepoId,DCPS_IR_Subscription*,ACE_Null_Mutex>;

template class ACE_Node<OpenDDS::DCPS::RepoId>;
template class ACE_Unbounded_Set<OpenDDS::DCPS::RepoId>;
template class ACE_Unbounded_Set_Iterator<OpenDDS::DCPS::RepoId>;

#elif defined(ACE_HAS_TEMPLATE_INSTANTIATION_PRAGMA)

#pragma instantiate ACE_Map_Entry<OpenDDS::DCPS::RepoId,DCPS_IR_Publication*>
#pragma instantiate ACE_Map_Manager<OpenDDS::DCPS::RepoId,DCPS_IR_Publication*,ACE_Null_Mutex>
#pragma instantiate ACE_Map_Iterator_Base<OpenDDS::DCPS::RepoId,DCPS_IR_Publication*,ACE_Null_Mutex>
#pragma instantiate ACE_Map_Iterator<OpenDDS::DCPS::RepoId,DCPS_IR_Publication*,ACE_Null_Mutex>
#pragma instantiate ACE_Map_Reverse_Iterator<OpenDDS::DCPS::RepoId,DCPS_IR_Publication*,ACE_Null_Mutex>

#pragma instantiate ACE_Map_Entry<OpenDDS::DCPS::RepoId,DCPS_IR_Subscription*>
#pragma instantiate ACE_Map_Manager<OpenDDS::DCPS::RepoId,DCPS_IR_Subscription*,ACE_Null_Mutex>
#pragma instantiate ACE_Map_Iterator_Base<OpenDDS::DCPS::RepoId,DCPS_IR_Subscription*,ACE_Null_Mutex>
#pragma instantiate ACE_Map_Iterator<OpenDDS::DCPS::RepoId,DCPS_IR_Subscription*,ACE_Null_Mutex>
#pragma instantiate ACE_Map_Reverse_Iterator<OpenDDS::DCPS::RepoId,DCPS_IR_Subscription*,ACE_Null_Mutex>

#pragma instantiate ACE_Node<OpenDDS::DCPS::RepoId>
#pragma instantiate ACE_Unbounded_Set<OpenDDS::DCPS::RepoId>
#pragma instantiate ACE_Unbounded_Set_Iterator<OpenDDS::DCPS::RepoId>

#endif /* ACE_HAS_EXPLICIT_TEMPLATE_INSTANTIATION */
