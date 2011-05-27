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
                                          Update::Manager* um)
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
  um_ (um),
  isBitPublisher_( false)
{
}



DCPS_IR_Participant::~DCPS_IR_Participant()
{
  for( DCPS_IR_Subscription_Map::const_iterator current = this->subscriptions_.begin();
       current != this->subscriptions_.end();
       ++current
     ) {
      std::stringstream buffer;
      std::stringstream subscriptionBuffer;

      long key = ::OpenDDS::DCPS::GuidConverter( this->id_);
      buffer << this->id_ << "(" << std::hex << key << ")";

      key = OpenDDS::DCPS::GuidConverter(
              const_cast< OpenDDS::DCPS::RepoId*>( &current->first)
            );
      subscriptionBuffer << current->first << "(" << std::hex << key << ")";

      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: DCPS_IR_Participant::~DCPS_IR_Participant: ")
        ACE_TEXT("domain %d participant %s removing subscription %s.\n"),
        this->domain_->get_id(),
        buffer.str().c_str(),
        subscriptionBuffer.str().c_str()
      ));
      remove_subscription( current->first);
  }

  for( DCPS_IR_Publication_Map::const_iterator current = this->publications_.begin();
       current != this->publications_.end();
       ++current
     ) {
      std::stringstream buffer;
      std::stringstream publicationBuffer;

      long key = ::OpenDDS::DCPS::GuidConverter( this->id_);
      buffer << this->id_ << "(" << std::hex << key << ")";

      key = OpenDDS::DCPS::GuidConverter(
              const_cast< OpenDDS::DCPS::RepoId*>( &current->first)
            );
      publicationBuffer << current->first << "(" << std::hex << key << ")";

      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: DCPS_IR_Participant::~DCPS_IR_Participant: ")
        ACE_TEXT("domain %d participant %s removing publication %s.\n"),
        this->domain_->get_id(),
        buffer.str().c_str(),
        publicationBuffer.str().c_str()
      ));
      remove_publication( current->first);
  }

  for( DCPS_IR_Topic_Map::const_iterator current = this->topicRefs_.begin();
       current != this->topicRefs_.end();
       ++current
     ) {
      std::stringstream buffer;
      std::stringstream topicBuffer;

      long key = ::OpenDDS::DCPS::GuidConverter( this->id_);
      buffer << this->id_ << "(" << std::hex << key << ")";

      key = OpenDDS::DCPS::GuidConverter(
              const_cast< OpenDDS::DCPS::RepoId*>( &current->first)
            );
      topicBuffer << current->first << "(" << std::hex << key << ")";

      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: DCPS_IR_Participant::~DCPS_IR_Participant: ")
        ACE_TEXT("domain %d participant %s retained topic %s.\n"),
        this->domain_->get_id(),
        buffer.str().c_str(),
        topicBuffer.str().c_str()
      ));
  }
}

const DCPS_IR_Publication_Map&
DCPS_IR_Participant::publications() const
{
  return this->publications_;
}

const DCPS_IR_Subscription_Map&
DCPS_IR_Participant::subscriptions() const
{
  return this->subscriptions_;
}

const DCPS_IR_Topic_Map&
DCPS_IR_Participant::topics() const
{
  return this->topicRefs_;
}

void
DCPS_IR_Participant::takeOwnership()
{
  /// Publish an update with our ownership.
  if( this->um_ && (this->isBitPublisher() == false)) {
    this->um_->create(
      Update::OwnershipData(
        this->domain_->get_id(),
        this->id_,
        this->federationId_
    ));
    if( ::OpenDDS::DCPS::DCPS_debug_level > 4) {
      std::stringstream buffer;
      long key = ::OpenDDS::DCPS::GuidConverter(
                   const_cast< OpenDDS::DCPS::RepoId*>( &this->id_)
                 );
      buffer << this->id_ << "(" << std::hex << key << ")";
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) DCPS_IR_Participant::take_ownership: ")
        ACE_TEXT("pushing ownership %s in domain %d.\n"),
        buffer.str().c_str(),
        this->domain_->get_id()
      ));
    }
  }

  // And now handle our internal ownership processing.
  this->changeOwner( this->federationId_, this->federationId_);
}

void
DCPS_IR_Participant::changeOwner( long sender, long owner)
{
  { ACE_GUARD( ACE_SYNCH_MUTEX, guard, this->ownerLock_);

    if( (owner == OWNER_NONE)
     && ( this->isOwner() || (this->owner_ != sender)
        )
      ) {
      // Do not eliminate ownership if we are the owner or if the update
      // does not come from the current owner.
      return;
    }

    // Finally.  Change the value.
    this->owner_ = owner;

  } // End of lock scope.

  if( this->isOwner()) {
    /// @TODO: Ensure that any stalled callbacks are made.
  }
}

long
DCPS_IR_Participant::owner() const
{
  return this->owner_;
}

bool
DCPS_IR_Participant::isOwner() const
{
  return this->owner_ == this->federationId_;
}

bool&
DCPS_IR_Participant::isBitPublisher()
{
  return this->isBitPublisher_;
}

bool
DCPS_IR_Participant::isBitPublisher() const
{
  return this->isBitPublisher_;
}

int DCPS_IR_Participant::add_publication (DCPS_IR_Publication* pub)
{
  OpenDDS::DCPS::RepoId pubId = pub->get_id();
  DCPS_IR_Publication_Map::iterator where = this->publications_.find( pubId);
  if( where == this->publications_.end()) {
    this->publications_.insert(
      where, DCPS_IR_Publication_Map::value_type( pubId, pub)
    );
    if (::OpenDDS::DCPS::DCPS_debug_level > 0) {
      std::stringstream buffer;
      std::stringstream publicationBuffer;

      long key = OpenDDS::DCPS::GuidConverter( this->id_);
      buffer << this->get_id() << "(" << std::hex << key << ")";

      key = ::OpenDDS::DCPS::GuidConverter( pubId);
      publicationBuffer << pubId << "(" << std::hex << key << ")";

      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) DCPS_IR_Participant::add_publication: ")
        ACE_TEXT("participant %s successfully added publication %s at 0x%x.\n"),
        buffer.str().c_str(),
        publicationBuffer.str().c_str(),
        pub
      ));
    }
    return 0;

  } else {
    if (::OpenDDS::DCPS::DCPS_debug_level > 0) {
      std::stringstream buffer;
      std::stringstream publicationBuffer;

      long key = OpenDDS::DCPS::GuidConverter( this->id_);
      buffer << this->get_id() << "(" << std::hex << key << ")";

      key = ::OpenDDS::DCPS::GuidConverter( pubId);
      publicationBuffer << pubId << "(" << std::hex << key << ")";

      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) WARNING: DCPS_IR_Participant::add_publication: ")
        ACE_TEXT("participant %s attempted to add existing publication %s.\n"),
        buffer.str().c_str(),
        publicationBuffer.str().c_str()
      ));
    }
    return 1;
  }
}


int DCPS_IR_Participant::find_publication_reference (OpenDDS::DCPS::RepoId pubId,
                                                     DCPS_IR_Publication* & pub)
{
  DCPS_IR_Publication_Map::iterator where = this->publications_.find( pubId);
  if( where != this->publications_.end()) {
    pub = where->second;
    if (::OpenDDS::DCPS::DCPS_debug_level > 0) {
      std::stringstream buffer;
      std::stringstream publicationBuffer;

      long key = OpenDDS::DCPS::GuidConverter( this->id_);
      buffer << this->get_id() << "(" << std::hex << key << ")";

      key = ::OpenDDS::DCPS::GuidConverter( pubId);
      publicationBuffer << pubId << "(" << std::hex << key << ")";

      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) DCPS_IR_Participant::find_publication_reference: ")
        ACE_TEXT("participant %s found publication %s at 0x%x.\n"),
        buffer.str().c_str(),
        publicationBuffer.str().c_str(),
        pub
      ));
    }
    return 0;

  } else {
    std::stringstream buffer;
    std::stringstream publicationBuffer;

    long key = OpenDDS::DCPS::GuidConverter( this->id_);
    buffer << this->get_id() << "(" << std::hex << key << ")";

    key = ::OpenDDS::DCPS::GuidConverter( pubId);
    publicationBuffer << pubId << "(" << std::hex << key << ")";

    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: DCPS_IR_Participant::find_publication_reference: ")
      ACE_TEXT("participant %s could not find publication %s.\n"),
      buffer.str().c_str(),
      publicationBuffer.str().c_str()
    ));
    pub = 0;
    return -1;
  }
}


int DCPS_IR_Participant::remove_publication (OpenDDS::DCPS::RepoId pubId)
{
  DCPS_IR_Publication_Map::iterator where = this->publications_.find( pubId);
  if( where != this->publications_.end()) {
    DCPS_IR_Topic* topic = where->second->get_topic();
    topic->remove_publication_reference( where->second);

    if( 0 != where->second->remove_associations( false)) {
      // N.B. As written today, this branch will never be taken.
      std::stringstream buffer;
      std::stringstream publicationBuffer;

      long key = OpenDDS::DCPS::GuidConverter( this->id_);
      buffer << this->get_id() << "(" << std::hex << key << ")";

      key = ::OpenDDS::DCPS::GuidConverter( pubId);
      publicationBuffer << pubId << "(" << std::hex << key << ")";

      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: DCPS_IR_Participant::remove_publication: ")
        ACE_TEXT("participant %s unable to remove associations from publication %s\n"),
        buffer.str().c_str(),
        publicationBuffer.str().c_str()
      ));
      return -1;
    }

    this->domain_->dispose_publication_bit( where->second);
    delete where->second;
    topic->release (false);
    this->publications_.erase( where);

    if (::OpenDDS::DCPS::DCPS_debug_level > 0) {
      std::stringstream buffer;
      std::stringstream publicationBuffer;

      long key = OpenDDS::DCPS::GuidConverter( this->id_);
      buffer << this->get_id() << "(" << std::hex << key << ")";

      key = ::OpenDDS::DCPS::GuidConverter( pubId);
      publicationBuffer << pubId << "(" << std::hex << key << ")";

      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) DCPS_IR_Participant::remove_publication: ")
        ACE_TEXT("participant %s removed publication %s.\n"),
        buffer.str().c_str(),
        publicationBuffer.str().c_str()
      ));
    }
    return 0;

  } else {
    std::stringstream buffer;
    std::stringstream publicationBuffer;

    long key = OpenDDS::DCPS::GuidConverter( this->id_);
    buffer << this->get_id() << "(" << std::hex << key << ")";

    key = ::OpenDDS::DCPS::GuidConverter( pubId);
    publicationBuffer << pubId << "(" << std::hex << key << ")";

    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: DCPS_IR_Participant::remove_publication: ")
      ACE_TEXT("participant %s unable to remove publication %s.\n"),
      buffer.str().c_str(),
      publicationBuffer.str().c_str()
    ));
    return -1;
  }
}



int DCPS_IR_Participant::add_subscription (DCPS_IR_Subscription* sub)
{
  OpenDDS::DCPS::RepoId subId = sub->get_id();
  DCPS_IR_Subscription_Map::iterator where = this->subscriptions_.find( subId);
  if( where == this->subscriptions_.end()) {
    this->subscriptions_.insert(
      where, DCPS_IR_Subscription_Map::value_type( subId, sub)
    );
    if (::OpenDDS::DCPS::DCPS_debug_level > 0) {
      std::stringstream buffer;
      std::stringstream subscriptionBuffer;

      long key = OpenDDS::DCPS::GuidConverter( this->id_);
      buffer << this->get_id() << "(" << std::hex << key << ")";

      key = ::OpenDDS::DCPS::GuidConverter( subId);
      subscriptionBuffer << subId << "(" << std::hex << key << ")";

      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) DCPS_IR_Participant::add_subscription: ")
        ACE_TEXT("participant %s successfully added subscription %s at 0x%x.\n"),
        buffer.str().c_str(),
        subscriptionBuffer.str().c_str(),
        sub
      ));
    }
    return 0;

  } else {
    if (::OpenDDS::DCPS::DCPS_debug_level > 0) {
      std::stringstream buffer;
      std::stringstream subscriptionBuffer;

      long key = OpenDDS::DCPS::GuidConverter( this->id_);
      buffer << this->get_id() << "(" << std::hex << key << ")";

      key = ::OpenDDS::DCPS::GuidConverter( subId);
      subscriptionBuffer << subId << "(" << std::hex << key << ")";

      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) WARNING: DCPS_IR_Participant::add_subscription: ")
        ACE_TEXT("participant %s attempted to add existing subscription %s.\n"),
        buffer.str().c_str(),
        subscriptionBuffer.str().c_str()
      ));
    }
    return 1;
  }
}


 int DCPS_IR_Participant::find_subscription_reference (OpenDDS::DCPS::RepoId subId,
                                                       DCPS_IR_Subscription*& sub)
{
  DCPS_IR_Subscription_Map::iterator where = this->subscriptions_.find( subId);
  if( where != this->subscriptions_.end()) {
    sub = where->second;
    if (::OpenDDS::DCPS::DCPS_debug_level > 0) {
      std::stringstream buffer;
      std::stringstream subscriptionBuffer;

      long key = OpenDDS::DCPS::GuidConverter( this->id_);
      buffer << this->get_id() << "(" << std::hex << key << ")";

      key = ::OpenDDS::DCPS::GuidConverter( subId);
      subscriptionBuffer << subId << "(" << std::hex << key << ")";

      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) DCPS_IR_Participant::find_subscription_reference: ")
        ACE_TEXT("participant %s found subscription %s at 0x%x.\n"),
        buffer.str().c_str(),
        subscriptionBuffer.str().c_str(),
        sub
      ));
    }
    return 0;

  } else {
    std::stringstream buffer;
    std::stringstream subscriptionBuffer;

    long key = OpenDDS::DCPS::GuidConverter( this->id_);
    buffer << this->get_id() << "(" << std::hex << key << ")";

    key = ::OpenDDS::DCPS::GuidConverter( subId);
    subscriptionBuffer << subId << "(" << std::hex << key << ")";

    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: DCPS_IR_Participant::find_subscription_reference: ")
      ACE_TEXT("participant %s could not find subscription %s.\n"),
      buffer.str().c_str(),
      subscriptionBuffer.str().c_str()
    ));
    sub = 0;
    return -1;
  }
}

int DCPS_IR_Participant::remove_subscription (OpenDDS::DCPS::RepoId subId)
{
  DCPS_IR_Subscription_Map::iterator where = this->subscriptions_.find( subId);
  if( where != this->subscriptions_.end()) {
    DCPS_IR_Topic* topic = where->second->get_topic();
    topic->remove_subscription_reference( where->second);

    if( 0 != where->second->remove_associations( false)) {
      // N.B. As written today, this branch will never be taken.
      std::stringstream buffer;
      std::stringstream subscriptionBuffer;

      long key = OpenDDS::DCPS::GuidConverter( this->id_);
      buffer << this->get_id() << "(" << std::hex << key << ")";

      key = ::OpenDDS::DCPS::GuidConverter( subId);
      subscriptionBuffer << subId << "(" << std::hex << key << ")";

      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: DCPS_IR_Participant::remove_subscription: ")
        ACE_TEXT("participant %s unable to remove associations from subscription %s\n"),
        buffer.str().c_str(),
        subscriptionBuffer.str().c_str()
      ));
      return -1;
    }

    this->domain_->dispose_subscription_bit( where->second);
    delete where->second;
    topic->release (false);
    this->subscriptions_.erase( where);

    if (::OpenDDS::DCPS::DCPS_debug_level > 0) {
      std::stringstream buffer;
      std::stringstream subscriptionBuffer;

      long key = OpenDDS::DCPS::GuidConverter( this->id_);
      buffer << this->get_id() << "(" << std::hex << key << ")";

      key = ::OpenDDS::DCPS::GuidConverter( subId);
      subscriptionBuffer << subId << "(" << std::hex << key << ")";

      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) DCPS_IR_Participant::remove_subscription: ")
        ACE_TEXT("participant %s removed subscription %s.\n"),
        buffer.str().c_str(),
        subscriptionBuffer.str().c_str()
      ));
    }
    return 0;

  } else {
    std::stringstream buffer;
    std::stringstream subscriptionBuffer;

    long key = OpenDDS::DCPS::GuidConverter( this->id_);
    buffer << this->get_id() << "(" << std::hex << key << ")";

    key = ::OpenDDS::DCPS::GuidConverter( subId);
    subscriptionBuffer << subId << "(" << std::hex << key << ")";

    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: DCPS_IR_Participant::remove_subscription: ")
      ACE_TEXT("participant %s unable to remove subscription %s.\n"),
      buffer.str().c_str(),
      subscriptionBuffer.str().c_str()
    ));
    return -1;
  }
}


int DCPS_IR_Participant::add_topic_reference (DCPS_IR_Topic* topic)
{
  OpenDDS::DCPS::RepoId topicId = topic->get_id();
  DCPS_IR_Topic_Map::iterator where = this->topicRefs_.find( topicId);
  if( where == this->topicRefs_.end()) {
    this->topicRefs_.insert(
      where, DCPS_IR_Topic_Map::value_type( topicId, topic)
    );
    if (::OpenDDS::DCPS::DCPS_debug_level > 0) {
      std::stringstream buffer;
      std::stringstream topicBuffer;

      long key = OpenDDS::DCPS::GuidConverter( this->id_);
      buffer << this->get_id() << "(" << std::hex << key << ")";

      key = ::OpenDDS::DCPS::GuidConverter( topicId);
      topicBuffer << topicId << "(" << std::hex << key << ")";

      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) DCPS_IR_Participant::add_topic_reference: ")
        ACE_TEXT("participant %s successfully added topic %s at 0x%x.\n"),
        buffer.str().c_str(),
        topicBuffer.str().c_str(),
        topic
      ));
    }
    return 0;

  } else {
    if (::OpenDDS::DCPS::DCPS_debug_level > 0) {
      std::stringstream buffer;
      std::stringstream topicBuffer;

      long key = OpenDDS::DCPS::GuidConverter( this->id_);
      buffer << this->get_id() << "(" << std::hex << key << ")";

      key = ::OpenDDS::DCPS::GuidConverter( topicId);
      topicBuffer << topicId << "(" << std::hex << key << ")";

      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) WARNING: DCPS_IR_Participant::add_topic_reference: ")
        ACE_TEXT("participant %s attempted to add existing topic %s.\n"),
        buffer.str().c_str(),
        topicBuffer.str().c_str()
      ));
    }
    return 1;
  }
}


int DCPS_IR_Participant::remove_topic_reference (OpenDDS::DCPS::RepoId topicId,
                                                 DCPS_IR_Topic*& topic)
{
  DCPS_IR_Topic_Map::iterator where = this->topicRefs_.find( topicId);
  if( where != this->topicRefs_.end()) {
    topic = where->second;
    this->topicRefs_.erase( where);
    if (::OpenDDS::DCPS::DCPS_debug_level > 0) {
      std::stringstream buffer;
      std::stringstream topicBuffer;

      long key = OpenDDS::DCPS::GuidConverter( this->id_);
      buffer << this->get_id() << "(" << std::hex << key << ")";

      key = ::OpenDDS::DCPS::GuidConverter( topicId);
      topicBuffer << topicId << "(" << std::hex << key << ")";

      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) DCPS_IR_Participant::remove_topic_reference: ")
        ACE_TEXT("participant %s removed topic %s at 0x%x.\n"),
        buffer.str().c_str(),
        topicBuffer.str().c_str(),
        topic
      ));
    }
    return 0;

  } else {
    if (::OpenDDS::DCPS::DCPS_debug_level > 0) {
      std::stringstream buffer;
      std::stringstream topicBuffer;

      long key = OpenDDS::DCPS::GuidConverter( this->id_);
      buffer << this->get_id() << "(" << std::hex << key << ")";

      key = ::OpenDDS::DCPS::GuidConverter( topicId);
      topicBuffer << topicId << "(" << std::hex << key << ")";

      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) WARNING: DCPS_IR_Participant::remove_topic_reference: ")
        ACE_TEXT("participant %s unable to find topic %s for removal.\n"),
        buffer.str().c_str(),
        topicBuffer.str().c_str()
      ));
    }
    return -1;
  }
}


int DCPS_IR_Participant::find_topic_reference (OpenDDS::DCPS::RepoId topicId,
                                               DCPS_IR_Topic*& topic)
{
  DCPS_IR_Topic_Map::iterator where = this->topicRefs_.find( topicId);
  if( where != this->topicRefs_.end()) {
    topic = where->second;
    if (::OpenDDS::DCPS::DCPS_debug_level > 0) {
      std::stringstream buffer;
      std::stringstream topicBuffer;

      long key = OpenDDS::DCPS::GuidConverter( this->id_);
      buffer << this->get_id() << "(" << std::hex << key << ")";

      key = ::OpenDDS::DCPS::GuidConverter( topicId);
      topicBuffer << topicId << "(" << std::hex << key << ")";

      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) DCPS_IR_Participant::find_topic_reference: ")
        ACE_TEXT("participant %s found topic %s at %x.\n"),
        buffer.str().c_str(),
        topicBuffer.str().c_str(),
        topic
      ));
    }
    return 0;

  } else {
    std::stringstream buffer;
    std::stringstream topicBuffer;

    long key = OpenDDS::DCPS::GuidConverter( this->id_);
    buffer << this->get_id() << "(" << std::hex << key << ")";

    key = ::OpenDDS::DCPS::GuidConverter( topicId);
    topicBuffer << topicId << "(" << std::hex << key << ")";

    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: DCPS_IR_Participant::find_topic_reference: ")
      ACE_TEXT("participant %s unable to find topic %s.\n"),
      buffer.str().c_str(),
      topicBuffer.str().c_str()
    ));
    topic = 0;
    return -1;
  }
}


void DCPS_IR_Participant::remove_all_dependents (CORBA::Boolean notify_lost)
{
  // remove all the publications associations
  {
    DCPS_IR_Publication_Map::const_iterator next = this->publications_.begin();
    while (next != this->publications_.end())
    {
      DCPS_IR_Publication_Map::const_iterator current = next;
      ++ next;
      DCPS_IR_Topic* topic = current->second->get_topic();
      topic->remove_publication_reference( current->second);

      if( 0 != current->second->remove_associations( notify_lost)) {
        return;
      }
      topic->release (false);
    }
  }

  {
    DCPS_IR_Subscription_Map::const_iterator next = this->subscriptions_.begin();
    while (next != this->subscriptions_.end())
    {
      DCPS_IR_Subscription_Map::const_iterator current = next;
      ++ next;
      DCPS_IR_Topic* topic = current->second->get_topic();
      topic->remove_subscription_reference( current->second);

      if( 0 != current->second->remove_associations( notify_lost)) {
        return;
      }
      topic->release (false);
    }
  }

  {
    DCPS_IR_Topic_Map::const_iterator next = this->topicRefs_.begin();
    while (next != this->topicRefs_.end())
    {
      DCPS_IR_Topic_Map::const_iterator current = next;
      ++ next;
      // Notify the federation to remove the topic.
      if( this->um_ && (this->isBitPublisher() == false)) {
        Update::IdPath path(
          this->domain_->get_id(),
          this->get_id(),
          current->second->get_id()
          );
        this->um_->destroy( path, Update::Topic);
        if( ::OpenDDS::DCPS::DCPS_debug_level > 4) {
          ::OpenDDS::DCPS::RepoId id = current->second->get_id();
          std::stringstream buffer;
          long key = ::OpenDDS::DCPS::GuidConverter( id);
          buffer << id << "(" << std::hex << key << ")";
          ACE_DEBUG((LM_DEBUG,
            ACE_TEXT("(%P|%t) DCPS_IR_Participant::remove_all_dependents: ")
            ACE_TEXT("pushing deletion of topic %s in domain %d.\n"),
            buffer.str().c_str(),
            this->domain_->get_id()
            ));
        }


        // Remove the topic ourselves.
        // N.B. This call sets the second (reference) argument to 0, so when
        //      clear() is called below, no destructor is (re)called.

        // Get the topic id and topic point before remove_topic since it 
        // invalidates the iterator. Accessing after removal got SEGV.
        OpenDDS::DCPS::RepoId id = current->first;
        DCPS_IR_Topic* topic = current->second;

        this->domain_->remove_topic( this, topic);

        if (::OpenDDS::DCPS::DCPS_debug_level > 9) {
          std::stringstream buffer;
          std::stringstream topicBuffer;

          long key = ::OpenDDS::DCPS::GuidConverter( this->id_);
          buffer << this->id_ << "(" << std::hex << key << ")";

          key = OpenDDS::DCPS::GuidConverter(
            const_cast< OpenDDS::DCPS::RepoId*>( &id)
            );
          topicBuffer << id << "(" << std::hex << key << ")";

          ACE_DEBUG((LM_DEBUG,
            ACE_TEXT("(%P|%t) DCPS_IR_Participant::remove_all_dependents: ")
            ACE_TEXT("domain %d participant %s removed topic %s.\n"),
            this->domain_->get_id(),
            buffer.str().c_str(),
            topicBuffer.str().c_str()
            ));
        }
      }
    }
  }

  // Clear the Topic container of null pointers.
  this->topicRefs_.clear();

  // The publications and subscriptions can NOT be deleted until after all
  // the associations have been removed.  Otherwise an access violation
  // can occur because a publication and subscription of this participant
  // could be associated.

  // delete all the publications
  for( DCPS_IR_Publication_Map::const_iterator current = this->publications_.begin();
       current != this->publications_.end();
       ++current
     ) {
    // Notify the federation to destroy the publication.
    if( this->um_ && (this->isBitPublisher() == false)) {
      Update::IdPath path(
        this->domain_->get_id(),
        this->get_id(),
        current->second->get_id()
      );
      this->um_->destroy( path, Update::Actor, Update::DataWriter);
      if( ::OpenDDS::DCPS::DCPS_debug_level > 4) {
        ::OpenDDS::DCPS::RepoId id = current->second->get_id();
        std::stringstream buffer;
        long key = ::OpenDDS::DCPS::GuidConverter( id);
        buffer << id << "(" << std::hex << key << ")";
        ACE_DEBUG((LM_DEBUG,
          ACE_TEXT("(%P|%t) DCPS_IR_Participant::remove_all_dependents: ")
          ACE_TEXT("pushing deletion of publication %s in domain %d.\n"),
          buffer.str().c_str(),
          this->domain_->get_id()
        ));
      }
    }
  }

  // Clear the container.  This will call the destructors as well.
  this->publications_.clear();

  // delete all the subscriptions
  for( DCPS_IR_Subscription_Map::const_iterator current
         = this->subscriptions_.begin();
       current != this->subscriptions_.end();
       ++current
     ) {
    // Notify the federation to destroy the subscription.
    if( this->um_ && (this->isBitPublisher() == false)) {
      Update::IdPath path(
        this->domain_->get_id(),
        this->get_id(),
        current->second->get_id()
      );
      this->um_->destroy( path, Update::Actor, Update::DataReader);
      if( ::OpenDDS::DCPS::DCPS_debug_level > 4) {
        ::OpenDDS::DCPS::RepoId id = current->second->get_id();
        std::stringstream buffer;
        long key = ::OpenDDS::DCPS::GuidConverter( id);
        buffer << id << "(" << std::hex << key << ")";
        ACE_DEBUG((LM_DEBUG,
          ACE_TEXT("(%P|%t) DCPS_IR_Participant::remove_all_dependents: ")
          ACE_TEXT("pushing deletion of subscription %s in domain %d.\n"),
          buffer.str().c_str(),
          this->domain_->get_id()
        ));
      }
    }
  }

  // Clear the container.  This will call the destructors as well.
  this->subscriptions_.clear();
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

  // disassociate any publications
  for( DCPS_IR_Publication_Map::const_iterator current = this->publications_.begin();
       current != this->publications_.end();
       ++current
     ) {
    current->second->disassociate_participant( id);
  }

  // disassociate any subscriptions
  for( DCPS_IR_Subscription_Map::const_iterator current = this->subscriptions_.begin();
       current != this->subscriptions_.end();
       ++current
     ) {
    current->second->disassociate_participant( id);
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

  // disassociate any publications
  for( DCPS_IR_Publication_Map::const_iterator current = this->publications_.begin();
       current != this->publications_.end();
       ++current
     ) {
    current->second->disassociate_topic( id);
  }

  // disassociate any subscriptions
  for( DCPS_IR_Subscription_Map::const_iterator current = this->subscriptions_.begin();
       current != this->subscriptions_.end();
       ++current
     ) {
    current->second->disassociate_topic( id);
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

  // disassociate any subscriptions
  for( DCPS_IR_Subscription_Map::const_iterator current = this->subscriptions_.begin();
       current != this->subscriptions_.end();
       ++current
     ) {
    current->second->disassociate_publication( id);
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

  // disassociate any publications
  for( DCPS_IR_Publication_Map::const_iterator current = this->publications_.begin();
       current != this->publications_.end();
       ++current
     ) {
    current->second->disassociate_subscription( id);
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
