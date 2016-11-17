/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DcpsInfo_pch.h"
#include /**/ "DCPS_IR_Participant.h"
#include "FederationId.h"
#include "UpdateManager.h"

#include /**/ "DCPS_IR_Domain.h"
#include /**/ "DCPS_IR_Subscription.h"
#include /**/ "DCPS_IR_Publication.h"
#include /**/ "DCPS_IR_Topic.h"
#include /**/ "DCPS_IR_Topic_Description.h"

#include /**/ "dds/DCPS/RepoIdConverter.h"
#include <sstream>

#include /**/ "tao/debug.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

DCPS_IR_Participant::DCPS_IR_Participant(const TAO_DDS_DCPSFederationId& federationId,
                                         OpenDDS::DCPS::RepoId id,
                                         DCPS_IR_Domain* domain,
                                         DDS::DomainParticipantQos qos,
                                         Update::Manager* um)
  : id_(id),
    domain_(domain),
    qos_(qos),
    aliveStatus_(1),
    handle_(0),
    isBIT_(0),
    federationId_(federationId),
    owner_(federationId.overridden() ? OWNER_NONE : federationId.id()),
    topicIdGenerator_(
      federationId.id(),
      OpenDDS::DCPS::RepoIdConverter(id).participantId(),
      OpenDDS::DCPS::KIND_TOPIC),
    publicationIdGenerator_(
      federationId.id(),
      OpenDDS::DCPS::RepoIdConverter(id).participantId(),
      OpenDDS::DCPS::KIND_WRITER),
    subscriptionIdGenerator_(
      federationId.id(),
      OpenDDS::DCPS::RepoIdConverter(id).participantId(),
      OpenDDS::DCPS::KIND_READER),
    um_(um),
    isBitPublisher_(false)
{
}

DCPS_IR_Participant::~DCPS_IR_Participant()
{
  for (DCPS_IR_Subscription_Map::const_iterator current = this->subscriptions_.begin();
       current != this->subscriptions_.end();
       ++current) {
    OpenDDS::DCPS::RepoIdConverter part_converter(id_);
    OpenDDS::DCPS::RepoIdConverter sub_converter(current->first);
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: DCPS_IR_Participant::~DCPS_IR_Participant: ")
               ACE_TEXT("domain %d participant %C removing subscription %C.\n"),
               this->domain_->get_id(),
               std::string(part_converter).c_str(),
               std::string(sub_converter).c_str()));
    remove_subscription(current->first);
  }

  for (DCPS_IR_Publication_Map::const_iterator current = this->publications_.begin();
       current != this->publications_.end();
       ++current) {
    OpenDDS::DCPS::RepoIdConverter part_converter(id_);
    OpenDDS::DCPS::RepoIdConverter pub_converter(current->first);
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: DCPS_IR_Participant::~DCPS_IR_Participant: ")
               ACE_TEXT("domain %d participant %C removing publication %C.\n"),
               this->domain_->get_id(),
               std::string(part_converter).c_str(),
               std::string(pub_converter).c_str()));
    remove_publication(current->first);
  }

  for (DCPS_IR_Topic_Map::const_iterator current = this->topicRefs_.begin();
       current != this->topicRefs_.end();
       ++current) {
    OpenDDS::DCPS::RepoIdConverter part_converter(id_);
    OpenDDS::DCPS::RepoIdConverter topic_converter(current->first);
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: DCPS_IR_Participant::~DCPS_IR_Participant: ")
               ACE_TEXT("domain %d participant %C retained topic %C.\n"),
               this->domain_->get_id(),
               std::string(part_converter).c_str(),
               std::string(topic_converter).c_str()));
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
  if (this->um_ && (this->isBitPublisher() == false)) {
    this->um_->create(
      Update::OwnershipData(
        this->domain_->get_id(),
        this->id_,
        this->federationId_.id()));

    if (OpenDDS::DCPS::DCPS_debug_level > 4) {
      OpenDDS::DCPS::RepoIdConverter converter(id_);
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) DCPS_IR_Participant::take_ownership: ")
                 ACE_TEXT("pushing ownership %C in domain %d.\n"),
                 std::string(converter).c_str(),
                 this->domain_->get_id()));
    }
  }

  // And now handle our internal ownership processing.
  this->changeOwner(this->federationId_.id(), this->federationId_.id());
}

void
DCPS_IR_Participant::changeOwner(long sender, long owner)
{
  {
    ACE_GUARD(ACE_SYNCH_MUTEX, guard, this->ownerLock_);

    if ((owner == OWNER_NONE)
        && (this->isOwner() || (this->owner_ != sender))) {
      // Do not eliminate ownership if we are the owner or if the update
      // does not come from the current owner.
      return;
    }

    // Finally.  Change the value.
    this->owner_ = owner;

  } // End of lock scope.

  if (this->isOwner()) {
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
  return this->owner_ == this->federationId_.id();
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

int DCPS_IR_Participant::add_publication(DCPS_IR_Publication* pub)
{
  OpenDDS::DCPS::RepoId pubId = pub->get_id();
  DCPS_IR_Publication_Map::iterator where = this->publications_.find(pubId);

  if (where == this->publications_.end()) {
    this->publications_.insert(
      where, DCPS_IR_Publication_Map::value_type(pubId, pub));

    if (isBitPublisher_) {
      pub->set_bit_status(isBitPublisher_);
    }

    if (OpenDDS::DCPS::DCPS_debug_level > 0) {
      OpenDDS::DCPS::RepoIdConverter part_converter(id_);
      OpenDDS::DCPS::RepoIdConverter pub_converter(pubId);
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) DCPS_IR_Participant::add_publication: ")
                 ACE_TEXT("participant %C successfully added publication %C at 0x%x.\n"),
                 std::string(part_converter).c_str(),
                 std::string(pub_converter).c_str(),
                 pub));
    }

    return 0;

  } else {
    if (OpenDDS::DCPS::DCPS_debug_level > 0) {
      OpenDDS::DCPS::RepoIdConverter part_converter(id_);
      OpenDDS::DCPS::RepoIdConverter pub_converter(pubId);
      ACE_ERROR((LM_NOTICE,
                 ACE_TEXT("(%P|%t) NOTICE: DCPS_IR_Participant::add_publication: ")
                 ACE_TEXT("participant %C attempted to add existing publication %C.\n"),
                 std::string(part_converter).c_str(),
                 std::string(pub_converter).c_str()));
    }

    return 1;
  }
}

int DCPS_IR_Participant::find_publication_reference(OpenDDS::DCPS::RepoId pubId,
                                                    DCPS_IR_Publication* & pub)
{
  DCPS_IR_Publication_Map::iterator where = this->publications_.find(pubId);

  if (where != this->publications_.end()) {
    pub = where->second;

    if (OpenDDS::DCPS::DCPS_debug_level > 0) {
      OpenDDS::DCPS::RepoIdConverter part_converter(id_);
      OpenDDS::DCPS::RepoIdConverter pub_converter(pubId);
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) DCPS_IR_Participant::find_publication_reference: ")
                 ACE_TEXT("participant %C found publication %C at 0x%x.\n"),
                 std::string(part_converter).c_str(),
                 std::string(pub_converter).c_str(),
                 pub));
    }

    return 0;

  } else {
    if (OpenDDS::DCPS::DCPS_debug_level > 0) {
      OpenDDS::DCPS::RepoIdConverter part_converter(id_);
      OpenDDS::DCPS::RepoIdConverter pub_converter(pubId);
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) DCPS_IR_Participant::find_publication_reference: ")
                 ACE_TEXT("participant %C could not find publication %C.\n"),
                 std::string(part_converter).c_str(),
                 std::string(pub_converter).c_str()));
    }
    pub = 0;
    return -1;
  }
}

int DCPS_IR_Participant::remove_publication(OpenDDS::DCPS::RepoId pubId)
{
  DCPS_IR_Publication_Map::iterator where = this->publications_.find(pubId);

  if (where != this->publications_.end()) {
    DCPS_IR_Topic* topic = where->second->get_topic();
    topic->remove_publication_reference(where->second);

    if (0 != where->second->remove_associations(false)) {
      // N.B. As written today, this branch will never be taken.
      OpenDDS::DCPS::RepoIdConverter part_converter(id_);
      OpenDDS::DCPS::RepoIdConverter pub_converter(pubId);
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: DCPS_IR_Participant::remove_publication: ")
                 ACE_TEXT("participant %C unable to remove associations from publication %C\n"),
                 std::string(part_converter).c_str(),
                 std::string(pub_converter).c_str()));
      return -1;
    }

    this->domain_->dispose_publication_bit(where->second);
    delete where->second;
    topic->release(false);
    this->publications_.erase(where);

    if (OpenDDS::DCPS::DCPS_debug_level > 0) {
      OpenDDS::DCPS::RepoIdConverter part_converter(id_);
      OpenDDS::DCPS::RepoIdConverter pub_converter(pubId);
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) DCPS_IR_Participant::remove_publication: ")
                 ACE_TEXT("participant %C removed publication %C.\n"),
                 std::string(part_converter).c_str(),
                 std::string(pub_converter).c_str()));
    }

    return 0;

  } else {
    OpenDDS::DCPS::RepoIdConverter part_converter(id_);
    OpenDDS::DCPS::RepoIdConverter pub_converter(pubId);
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: DCPS_IR_Participant::remove_publication: ")
               ACE_TEXT("participant %C unable to remove publication %C.\n"),
               std::string(part_converter).c_str(),
               std::string(pub_converter).c_str()));
    return -1;
  }
}

int DCPS_IR_Participant::add_subscription(DCPS_IR_Subscription* sub)
{
  OpenDDS::DCPS::RepoId subId = sub->get_id();
  DCPS_IR_Subscription_Map::iterator where = this->subscriptions_.find(subId);

  if (where == this->subscriptions_.end()) {
    this->subscriptions_.insert(
      where, DCPS_IR_Subscription_Map::value_type(subId, sub));

    if (isBitPublisher_) {
      sub->set_bit_status(isBitPublisher_);
    }

    if (OpenDDS::DCPS::DCPS_debug_level > 0) {
      OpenDDS::DCPS::RepoIdConverter part_converter(id_);
      OpenDDS::DCPS::RepoIdConverter sub_converter(subId);
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) DCPS_IR_Participant::add_subscription: ")
                 ACE_TEXT("participant %C successfully added subscription %C at 0x%x.\n"),
                 std::string(part_converter).c_str(),
                 std::string(sub_converter).c_str(),
                 sub));
    }

    return 0;

  } else {
    if (OpenDDS::DCPS::DCPS_debug_level > 0) {
      OpenDDS::DCPS::RepoIdConverter part_converter(id_);
      OpenDDS::DCPS::RepoIdConverter sub_converter(subId);
      ACE_ERROR((LM_NOTICE,
                 ACE_TEXT("(%P|%t) NOTICE: DCPS_IR_Participant::add_subscription: ")
                 ACE_TEXT("participant %C attempted to add existing subscription %C.\n"),
                 std::string(part_converter).c_str(),
                 std::string(sub_converter).c_str()));
    }

    return 1;
  }
}

int DCPS_IR_Participant::find_subscription_reference(OpenDDS::DCPS::RepoId subId,
                                                     DCPS_IR_Subscription*& sub)
{
  DCPS_IR_Subscription_Map::iterator where = this->subscriptions_.find(subId);

  if (where != this->subscriptions_.end()) {
    sub = where->second;

    if (OpenDDS::DCPS::DCPS_debug_level > 0) {
      OpenDDS::DCPS::RepoIdConverter part_converter(id_);
      OpenDDS::DCPS::RepoIdConverter sub_converter(subId);
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) DCPS_IR_Participant::find_subscription_reference: ")
                 ACE_TEXT("participant %C found subscription %C at 0x%x.\n"),
                 std::string(part_converter).c_str(),
                 std::string(sub_converter).c_str(),
                 sub));
    }

    return 0;

  } else {
    if (OpenDDS::DCPS::DCPS_debug_level > 0) {
      OpenDDS::DCPS::RepoIdConverter part_converter(id_);
      OpenDDS::DCPS::RepoIdConverter sub_converter(subId);
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) DCPS_IR_Participant::find_subscription_reference: ")
                 ACE_TEXT("participant %C could not find subscription %C.\n"),
                 std::string(part_converter).c_str(),
                 std::string(sub_converter).c_str()));
    }
    sub = 0;
    return -1;
  }
}

int DCPS_IR_Participant::remove_subscription(OpenDDS::DCPS::RepoId subId)
{
  DCPS_IR_Subscription_Map::iterator where = this->subscriptions_.find(subId);

  if (where != this->subscriptions_.end()) {
    DCPS_IR_Topic* topic = where->second->get_topic();
    topic->remove_subscription_reference(where->second);

    if (0 != where->second->remove_associations(false)) {
      // N.B. As written today, this branch will never be taken.
      OpenDDS::DCPS::RepoIdConverter part_converter(id_);
      OpenDDS::DCPS::RepoIdConverter sub_converter(subId);
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: DCPS_IR_Participant::remove_subscription: ")
                 ACE_TEXT("participant %C unable to remove associations from subscription %C\n"),
                 std::string(part_converter).c_str(),
                 std::string(sub_converter).c_str()));
      return -1;
    }

    this->domain_->dispose_subscription_bit(where->second);
    delete where->second;
    topic->release(false);
    this->subscriptions_.erase(where);

    if (OpenDDS::DCPS::DCPS_debug_level > 0) {
      OpenDDS::DCPS::RepoIdConverter part_converter(id_);
      OpenDDS::DCPS::RepoIdConverter sub_converter(subId);
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) DCPS_IR_Participant::remove_subscription: ")
                 ACE_TEXT("participant %C removed subscription %C.\n"),
                 std::string(part_converter).c_str(),
                 std::string(sub_converter).c_str()));
    }

    return 0;

  } else {
    OpenDDS::DCPS::RepoIdConverter part_converter(id_);
    OpenDDS::DCPS::RepoIdConverter sub_converter(subId);
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: DCPS_IR_Participant::remove_subscription: ")
               ACE_TEXT("participant %C unable to remove subscription %C.\n"),
               std::string(part_converter).c_str(),
               std::string(sub_converter).c_str()));
    return -1;
  }
}

int DCPS_IR_Participant::add_topic_reference(DCPS_IR_Topic* topic)
{
  OpenDDS::DCPS::RepoId topicId = topic->get_id();
  DCPS_IR_Topic_Map::iterator where = this->topicRefs_.find(topicId);

  if (where == this->topicRefs_.end()) {
    this->topicRefs_.insert(
      where, DCPS_IR_Topic_Map::value_type(topicId, topic));

    if (isBitPublisher_) {
      topic->set_bit_status(isBitPublisher_);
    }

    if (OpenDDS::DCPS::DCPS_debug_level > 0) {
      OpenDDS::DCPS::RepoIdConverter part_converter(id_);
      OpenDDS::DCPS::RepoIdConverter topic_converter(topicId);
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) DCPS_IR_Participant::add_topic_reference: ")
                 ACE_TEXT("participant %C successfully added topic %C at 0x%x.\n"),
                 std::string(part_converter).c_str(),
                 std::string(topic_converter).c_str(),
                 topic));
    }

    return 0;

  } else {
    if (OpenDDS::DCPS::DCPS_debug_level > 0) {
      OpenDDS::DCPS::RepoIdConverter part_converter(id_);
      OpenDDS::DCPS::RepoIdConverter topic_converter(topicId);
      ACE_DEBUG((LM_NOTICE,
                 ACE_TEXT("(%P|%t) NOTICE: DCPS_IR_Participant::add_topic_reference: ")
                 ACE_TEXT("participant %C attempted to add existing topic %C.\n"),
                 std::string(part_converter).c_str(),
                 std::string(topic_converter).c_str()));
    }

    return 1;
  }
}

int DCPS_IR_Participant::remove_topic_reference(OpenDDS::DCPS::RepoId topicId,
                                                DCPS_IR_Topic*& topic)
{
  DCPS_IR_Topic_Map::iterator where = this->topicRefs_.find(topicId);

  if (where != this->topicRefs_.end()) {
    topic = where->second;
    this->topicRefs_.erase(where);

    if (OpenDDS::DCPS::DCPS_debug_level > 0) {
      OpenDDS::DCPS::RepoIdConverter part_converter(id_);
      OpenDDS::DCPS::RepoIdConverter topic_converter(topicId);
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) DCPS_IR_Participant::remove_topic_reference: ")
                 ACE_TEXT("participant %C removed topic %C at 0x%x.\n"),
                 std::string(part_converter).c_str(),
                 std::string(topic_converter).c_str(),
                 topic));
    }

    return 0;

  } else {
    if (OpenDDS::DCPS::DCPS_debug_level > 0) {
      OpenDDS::DCPS::RepoIdConverter part_converter(id_);
      OpenDDS::DCPS::RepoIdConverter topic_converter(topicId);
      ACE_ERROR((LM_WARNING,
                 ACE_TEXT("(%P|%t) WARNING: DCPS_IR_Participant::remove_topic_reference: ")
                 ACE_TEXT("participant %C unable to find topic %C for removal.\n"),
                 std::string(part_converter).c_str(),
                 std::string(topic_converter).c_str()));
    }

    return -1;
  }
}

int DCPS_IR_Participant::find_topic_reference(OpenDDS::DCPS::RepoId topicId,
                                              DCPS_IR_Topic*& topic)
{
  DCPS_IR_Topic_Map::iterator where = this->topicRefs_.find(topicId);

  if (where != this->topicRefs_.end()) {
    topic = where->second;

    if (OpenDDS::DCPS::DCPS_debug_level > 0) {
      OpenDDS::DCPS::RepoIdConverter part_converter(id_);
      OpenDDS::DCPS::RepoIdConverter topic_converter(topicId);
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) DCPS_IR_Participant::find_topic_reference: ")
                 ACE_TEXT("participant %C found topic %C at %x.\n"),
                 std::string(part_converter).c_str(),
                 std::string(topic_converter).c_str(),
                 topic));
    }

    return 0;

  } else {
    OpenDDS::DCPS::RepoIdConverter part_converter(id_);
    OpenDDS::DCPS::RepoIdConverter topic_converter(topicId);
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: DCPS_IR_Participant::find_topic_reference: ")
               ACE_TEXT("participant %C unable to find topic %C.\n"),
               std::string(part_converter).c_str(),
               std::string(topic_converter).c_str()));
    topic = 0;
    return -1;
  }
}

void DCPS_IR_Participant::remove_all_dependents(CORBA::Boolean notify_lost)
{
  // remove all the publications associations
  {
    DCPS_IR_Publication_Map::const_iterator next = this->publications_.begin();

    while (next != this->publications_.end()) {
      DCPS_IR_Publication_Map::const_iterator current = next;
      ++ next;
      DCPS_IR_Topic* topic = current->second->get_topic();
      topic->remove_publication_reference(current->second);

      if (0 != current->second->remove_associations(notify_lost)) {
        return;
      }

      topic->release(false);
    }
  }

  {
    DCPS_IR_Subscription_Map::const_iterator next = this->subscriptions_.begin();

    while (next != this->subscriptions_.end()) {
      DCPS_IR_Subscription_Map::const_iterator current = next;
      ++ next;
      DCPS_IR_Topic* topic = current->second->get_topic();
      topic->remove_subscription_reference(current->second);

      if (0 != current->second->remove_associations(notify_lost)) {
        return;
      }

      topic->release(false);
    }
  }

  {
    DCPS_IR_Topic_Map::const_iterator next = this->topicRefs_.begin();

    while (next != this->topicRefs_.end()) {
      DCPS_IR_Topic_Map::const_iterator current = next;
      ++ next;

      // Notify the federation to remove the topic.
      if (this->um_ && (this->isBitPublisher() == false)) {
        Update::IdPath path(
          this->domain_->get_id(),
          this->get_id(),
          current->second->get_id());
        this->um_->destroy(path, Update::Topic);

        if (OpenDDS::DCPS::DCPS_debug_level > 4) {
          OpenDDS::DCPS::RepoId id = current->second->get_id();
          OpenDDS::DCPS::RepoIdConverter converter(id);
          ACE_DEBUG((LM_DEBUG,
                     ACE_TEXT("(%P|%t) DCPS_IR_Participant::remove_all_dependents: ")
                     ACE_TEXT("pushing deletion of topic %C in domain %d.\n"),
                     std::string(converter).c_str(),
                     this->domain_->get_id()));
        }

        // Remove the topic ourselves.
        // N.B. This call sets the second (reference) argument to 0, so when
        //      clear() is called below, no destructor is (re)called.

        // Get the topic id and topic point before remove_topic since it
        // invalidates the iterator. Accessing after removal got SEGV.
        OpenDDS::DCPS::RepoId id = current->first;
        DCPS_IR_Topic* topic = current->second;

        this->domain_->remove_topic(this, topic);

        if (OpenDDS::DCPS::DCPS_debug_level > 9) {
          OpenDDS::DCPS::RepoIdConverter part_converter(id_);
          OpenDDS::DCPS::RepoIdConverter topic_converter(id);
          ACE_DEBUG((LM_DEBUG,
                     ACE_TEXT("(%P|%t) DCPS_IR_Participant::remove_all_dependents: ")
                     ACE_TEXT("domain %d participant %C removed topic %C.\n"),
                     this->domain_->get_id(),
                     std::string(part_converter).c_str(),
                     std::string(topic_converter).c_str()));
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
  for (DCPS_IR_Publication_Map::const_iterator current = this->publications_.begin();
       current != this->publications_.end();
       ++current) {
    // Notify the federation to destroy the publication.
    if (this->um_ && (this->isBitPublisher() == false)) {
      Update::IdPath path(
        this->domain_->get_id(),
        this->get_id(),
        current->second->get_id());
      this->um_->destroy(path, Update::Actor, Update::DataWriter);

      if (OpenDDS::DCPS::DCPS_debug_level > 4) {
        OpenDDS::DCPS::RepoId id = current->second->get_id();
        OpenDDS::DCPS::RepoIdConverter converter(id);
        ACE_DEBUG((LM_DEBUG,
                   ACE_TEXT("(%P|%t) DCPS_IR_Participant::remove_all_dependents: ")
                   ACE_TEXT("pushing deletion of publication %C in domain %d.\n"),
                   std::string(converter).c_str(),
                   this->domain_->get_id()));
      }
    }
    // delete the publication
    delete current->second;
  }

  // Clear the container.
  this->publications_.clear();

  // delete all the subscriptions
  for (DCPS_IR_Subscription_Map::const_iterator current
       = this->subscriptions_.begin();
       current != this->subscriptions_.end();
       ++current) {
    // Notify the federation to destroy the subscription.
    if (this->um_ && (this->isBitPublisher() == false)) {
      Update::IdPath path(
        this->domain_->get_id(),
        this->get_id(),
        current->second->get_id());
      this->um_->destroy(path, Update::Actor, Update::DataReader);

      if (OpenDDS::DCPS::DCPS_debug_level > 4) {
        OpenDDS::DCPS::RepoId id = current->second->get_id();
        OpenDDS::DCPS::RepoIdConverter converter(id);
        ACE_DEBUG((LM_DEBUG,
                   ACE_TEXT("(%P|%t) DCPS_IR_Participant::remove_all_dependents: ")
                   ACE_TEXT("pushing deletion of subscription %C in domain %d.\n"),
                   std::string(converter).c_str(),
                   this->domain_->get_id()));
      }
    }
    // delete the subscription
    delete current->second;
  }

  // Clear the container.
  this->subscriptions_.clear();
}

void DCPS_IR_Participant::mark_dead()
{
  aliveStatus_ = 0;
  domain_->add_dead_participant(this);
}

OpenDDS::DCPS::RepoId DCPS_IR_Participant::get_id()
{
  return id_;
}

CORBA::Boolean DCPS_IR_Participant::is_alive()
{
  return aliveStatus_;
}

void DCPS_IR_Participant::set_alive(CORBA::Boolean alive)
{
  aliveStatus_ = alive;
}

void DCPS_IR_Participant::ignore_participant(OpenDDS::DCPS::RepoId id)
{
  if (OpenDDS::DCPS::DCPS_debug_level > 0) {
    OpenDDS::DCPS::RepoIdConverter part_converter(id_);
    OpenDDS::DCPS::RepoIdConverter ignore_converter(id);
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) DCPS_IR_Participant::ignore_participant: ")
               ACE_TEXT("participant %C now ignoring participant %C.\n"),
               std::string(part_converter).c_str(),
               std::string(ignore_converter).c_str()));
  }

  ignoredParticipants_.insert(id);

  // disassociate any publications
  for (DCPS_IR_Publication_Map::const_iterator current = this->publications_.begin();
       current != this->publications_.end();
       ++current) {
    current->second->disassociate_participant(id);
  }

  // disassociate any subscriptions
  for (DCPS_IR_Subscription_Map::const_iterator current = this->subscriptions_.begin();
       current != this->subscriptions_.end();
       ++current) {
    current->second->disassociate_participant(id);
  }
}

void DCPS_IR_Participant::ignore_topic(OpenDDS::DCPS::RepoId id)
{
  if (OpenDDS::DCPS::DCPS_debug_level > 0) {
    OpenDDS::DCPS::RepoIdConverter part_converter(id_);
    OpenDDS::DCPS::RepoIdConverter ignore_converter(id);
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) DCPS_IR_Participant::ignore_topic: ")
               ACE_TEXT("participant %C now ignoring topic %C.\n"),
               std::string(part_converter).c_str(),
               std::string(ignore_converter).c_str()));
  }

  ignoredTopics_.insert(id);

  // disassociate any publications
  for (DCPS_IR_Publication_Map::const_iterator current = this->publications_.begin();
       current != this->publications_.end();
       ++current) {
    current->second->disassociate_topic(id);
  }

  // disassociate any subscriptions
  for (DCPS_IR_Subscription_Map::const_iterator current = this->subscriptions_.begin();
       current != this->subscriptions_.end();
       ++current) {
    current->second->disassociate_topic(id);
  }
}

void DCPS_IR_Participant::ignore_publication(OpenDDS::DCPS::RepoId id)
{
  if (OpenDDS::DCPS::DCPS_debug_level > 0) {
    OpenDDS::DCPS::RepoIdConverter part_converter(id_);
    OpenDDS::DCPS::RepoIdConverter ignore_converter(id);
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) DCPS_IR_Participant::ignore_publication: ")
               ACE_TEXT("participant %C now ignoring publication %C.\n"),
               std::string(part_converter).c_str(),
               std::string(ignore_converter).c_str()));
  }

  ignoredPublications_.insert(id);

  // disassociate any subscriptions
  for (DCPS_IR_Subscription_Map::const_iterator current = this->subscriptions_.begin();
       current != this->subscriptions_.end();
       ++current) {
    current->second->disassociate_publication(id);
  }
}

void DCPS_IR_Participant::ignore_subscription(OpenDDS::DCPS::RepoId id)
{
  if (OpenDDS::DCPS::DCPS_debug_level > 0) {
    OpenDDS::DCPS::RepoIdConverter part_converter(id_);
    OpenDDS::DCPS::RepoIdConverter ignore_converter(id);
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) DCPS_IR_Participant::ignore_subscription: ")
               ACE_TEXT("participant %C now ignoring subscription %C.\n"),
               std::string(part_converter).c_str(),
               std::string(ignore_converter).c_str()));
  }

  ignoredSubscriptions_.insert(id);

  // disassociate any publications
  for (DCPS_IR_Publication_Map::const_iterator current = this->publications_.begin();
       current != this->publications_.end();
       ++current) {
    current->second->disassociate_subscription(id);
  }
}

CORBA::Boolean DCPS_IR_Participant::is_participant_ignored(OpenDDS::DCPS::RepoId id)
{
  return (0 == ignoredParticipants_.find(id));
}

CORBA::Boolean DCPS_IR_Participant::is_topic_ignored(OpenDDS::DCPS::RepoId id)
{
  return (0 == ignoredTopics_.find(id));
}

CORBA::Boolean DCPS_IR_Participant::is_publication_ignored(OpenDDS::DCPS::RepoId id)
{
  return (0 == ignoredPublications_.find(id));
}

CORBA::Boolean DCPS_IR_Participant::is_subscription_ignored(OpenDDS::DCPS::RepoId id)
{
  return (0 == ignoredSubscriptions_.find(id));
}

DDS::InstanceHandle_t DCPS_IR_Participant::get_handle()
{
  return handle_;
}

void DCPS_IR_Participant::set_handle(DDS::InstanceHandle_t handle)
{
  handle_ = handle;
}

const DDS::DomainParticipantQos* DCPS_IR_Participant::get_qos()
{
  return &qos_;
}

bool DCPS_IR_Participant::set_qos(const DDS::DomainParticipantQos & qos)
{
  // Do not need re-evaluate compatibility and associations when
  // DomainParticipantQos changes since only datareader and datawriter
  // QoS are evaludated during normal associations establishment.

  // Do not need publish the QoS change to topics or datareader or
  // datawriter BIT as they are independent.
  qos_ = qos;
  this->domain_->publish_participant_bit(this);

  return true;
}

CORBA::Boolean DCPS_IR_Participant::is_bit()
{
  return isBIT_;
}

void DCPS_IR_Participant::set_bit_status(CORBA::Boolean isBIT)
{
  isBIT_ = isBIT;
}

DCPS_IR_Domain* DCPS_IR_Participant::get_domain_reference() const
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
DCPS_IR_Participant::last_topic_key(long key)
{
  return this->topicIdGenerator_.last(key);
}

void
DCPS_IR_Participant::last_publication_key(long key)
{
  return this->publicationIdGenerator_.last(key);
}

void
DCPS_IR_Participant::last_subscription_key(long key)
{
  return this->subscriptionIdGenerator_.last(key);
}

std::string
DCPS_IR_Participant::dump_to_string(const std::string& prefix, int depth) const
{
  std::string str;
#if !defined (OPENDDS_INFOREPO_REDUCED_FOOTPRINT)
  OpenDDS::DCPS::RepoIdConverter local_converter(id_);

  for (int i=0; i < depth; i++)
    str += prefix;
  std::string indent = str + prefix;
  str += "DCPS_IR_Participant[";
  str += std::string(local_converter);
  str += "]";
  if (isBIT_)
    str += " (BIT)";
  std::ostringstream os;
  os << "federation id[" << federationId_.id();
  if (federationId_.overridden())
    os << "(federated)";

  os << "]  owner[" << owner_ << "]";
  str += os.str();
  str += aliveStatus_ ? " (alive)" : " (not alive)";
  str += "\n";

  str += indent + "Topics:\n";
  for (DCPS_IR_Topic_Map::const_iterator tm = topicRefs_.begin();
       tm != topicRefs_.end();
       tm++)
  {
    str += tm->second->dump_to_string(prefix, depth+1);
  }

  str += indent + "Publications:\n";
  for (DCPS_IR_Publication_Map::const_iterator pm = publications_.begin();
       pm != publications_.end();
       pm++)
  {
    str += pm->second->dump_to_string(prefix, depth+1);
  }

  str += indent + "Subscriptions:\n";
  for (DCPS_IR_Subscription_Map::const_iterator sm = subscriptions_.begin();
       sm != subscriptions_.end();
       sm++)
  {
    str += sm->second->dump_to_string(prefix, depth+1);
  }

  str += indent + "ignored Participants [ ";
  for (TAO_DDS_RepoId_Set::const_iterator ipart = ignoredParticipants_.begin();
       ipart != ignoredParticipants_.end();
       ipart++)
  {
    OpenDDS::DCPS::RepoIdConverter ipart_converter(*ipart);
    str += std::string(ipart_converter);
    str += " ";
  }
  str += "]\n";
  str += indent + "ignored Topics [ ";

  for (TAO_DDS_RepoId_Set::const_iterator itop = ignoredTopics_.begin();
       itop != ignoredTopics_.end();
       itop++)
  {
    OpenDDS::DCPS::RepoIdConverter itop_converter(*itop);
    str += std::string(itop_converter);
    str += " ";
  }
  str += "]\n";
  str += indent + "ignored Publications [ ";

  for (TAO_DDS_RepoId_Set::const_iterator ipub = ignoredPublications_.begin();
       ipub != ignoredPublications_.end();
       ipub++)
  {
    OpenDDS::DCPS::RepoIdConverter ipub_converter(*ipub);
    str += std::string(ipub_converter);
    str += " ";
  }
  str += "]\n";
  str += indent + "ignored Subscriptions [ ";

  for (TAO_DDS_RepoId_Set::const_iterator isub = ignoredSubscriptions_.begin();
       isub != ignoredSubscriptions_.end();
       isub++)
  {
    OpenDDS::DCPS::RepoIdConverter isub_converter(*isub);
    str += std::string(isub_converter);
    str += " ";
  }
  str += "]\n";

#endif // !defined (OPENDDS_INFOREPO_REDUCED_FOOTPRINT)
  return str;
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL

