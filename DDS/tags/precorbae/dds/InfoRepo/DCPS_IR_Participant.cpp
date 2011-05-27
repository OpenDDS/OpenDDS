#include "DcpsInfo_pch.h"
#include /**/ "DCPS_IR_Participant.h"

#include /**/ "DCPS_IR_Domain.h"
#include /**/ "DCPS_IR_Subscription.h"
#include /**/ "DCPS_IR_Publication.h"
#include /**/ "DCPS_IR_Topic.h"
#include /**/ "DCPS_IR_Topic_Description.h"
#include /**/ "tao/debug.h"

DCPS_IR_Participant::DCPS_IR_Participant (TAO::DCPS::RepoId id,
                                          DCPS_IR_Domain* domain,
                                          ::DDS::DomainParticipantQos qos)
: id_(id),
  domain_(domain),
  qos_(qos),
  aliveStatus_(1),
  handle_(0),
  isBIT_(0)
{
}



DCPS_IR_Participant::~DCPS_IR_Participant()
{
  if (0 != subscriptions_.current_size())
    {
      DCPS_IR_Subscription* subscription = 0;
      DCPS_IR_Subscription_Map::ITERATOR iter = subscriptions_.begin();
      DCPS_IR_Subscription_Map::ITERATOR end = subscriptions_.end();

      ACE_ERROR((LM_ERROR, 
                 ACE_TEXT("ERROR: DCPS_IR_Participant::~DCPS_IR_Participant () ")
                 ACE_TEXT("domain %d  id  %d\n"),
                 domain_, id_ ));

      while (iter != end)
        {
          subscription = (*iter).int_id_;
          ++iter;
          ACE_ERROR((LM_ERROR, 
                     ACE_TEXT("\tERROR: Removing subscription id %d that was still held!\n"),
                     subscription->get_id()
                     ));
          remove_subscription(subscription->get_id());
        }
    }

  if (0 != publications_.current_size())
    {
      DCPS_IR_Publication* publication = 0;
      DCPS_IR_Publication_Map::ITERATOR iter = publications_.begin();
      DCPS_IR_Publication_Map::ITERATOR end = publications_.end();

      ACE_ERROR((LM_ERROR, 
                 ACE_TEXT("ERROR: DCPS_IR_Participant::~DCPS_IR_Participant () ")
                 ACE_TEXT("domain %d  id  %d\n"),
                 domain_, id_ ));

      while (iter != end)
        {
          publication = (*iter).int_id_;
          ++iter;
          ACE_ERROR((LM_ERROR, 
                     ACE_TEXT("\tERROR: Removing publication id %d that was still held!\n"),
                     publication->get_id()
                     ));
          remove_publication(publication->get_id());
        }
    }

  if (0 != topicRefs_.current_size())
    {
      TAO::DCPS::RepoId topicId = 0;
      DCPS_IR_Topic_Map::ITERATOR iter = topicRefs_.begin();
      DCPS_IR_Topic_Map::ITERATOR end = topicRefs_.end();

      ACE_ERROR((LM_ERROR, 
                 ACE_TEXT("ERROR: DCPS_IR_Participant::~DCPS_IR_Participant () ")
                 ACE_TEXT("domain %d  id  %d\n"),
                 domain_, id_ ));

      while (iter != end)
        {
          topicId = (*iter).ext_id_;
          ++iter;
          ACE_ERROR((LM_ERROR, 
                     ACE_TEXT("\tERROR: Topic id %d still held!\n"),
                     topicId
                     ));
        }
    }

}



int DCPS_IR_Participant::add_publication (DCPS_IR_Publication* pub)
{
  TAO::DCPS::RepoId pubId = pub->get_id();
  int status = publications_.bind(pubId, pub);

  switch (status)
    {
    case 0:
      if (TAO_debug_level > 0)
        {
          ACE_DEBUG((LM_DEBUG, ACE_TEXT("DCPS_IR_Participant::add_publication ")
            ACE_TEXT("Participant id %d added publication %X id: %d\n"),
            id_,
            pub, pubId));
        }
      break;
    case 1:
      ACE_ERROR((LM_ERROR, ACE_TEXT("ERROR: DCPS_IR_Participant::add_publication ")
        ACE_TEXT("Attempted to add existing publication id %d\n"),
        pubId));
      break;
    case -1:
      ACE_ERROR((LM_ERROR, ACE_TEXT("ERROR: DCPS_IR_Participant::add_publication ")
        ACE_TEXT("Unknown error while adding publication id %d\n"),
        pubId));
    };

  return status;
}


int DCPS_IR_Participant::remove_publication (long pubId)
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
        ACE_ERROR((LM_ERROR, ACE_TEXT("ERROR: DCPS_IR_Participant::remove_publication ")
          ACE_TEXT("Error removing associations from publication id: %d\n"),
          pubId));
        return status;
      }

      if (TAO_debug_level > 0)
        {
          ACE_DEBUG((LM_DEBUG, ACE_TEXT("DCPS_IR_Participant::remove_publication ")
            ACE_TEXT("Removed publication id: %d\n"),
            pubId));
        } // if (TAO_debug_level > 0)

      // Dispose the BIT information
      domain_->dispose_publication_bit(pub);

      delete pub;
    }
  else
    {
      ACE_ERROR((LM_ERROR, ACE_TEXT("ERROR: DCPS_IR_Participant::remove_publication ")
        ACE_TEXT("Error removing publication %X id: %d\n"),
        pub, pubId));
    } // if (0 == status)

  return status;
}



int DCPS_IR_Participant::add_subscription (DCPS_IR_Subscription* sub)
{
  TAO::DCPS::RepoId subId = sub->get_id();
  int status = subscriptions_.bind(subId, sub);

  switch (status)
    {
    case 0:
      if (TAO_debug_level > 0)
        {
          ACE_DEBUG((LM_DEBUG, ACE_TEXT("DCPS_IR_Participant::add_subscription ")
            ACE_TEXT("Successfully added subscription %X id: %d\n"),
            sub, subId));
        }
      break;
    case 1:
      ACE_ERROR((LM_ERROR, ACE_TEXT("ERROR: DCPS_IR_Participant::add_subscription ")
        ACE_TEXT("Attempted to add existing subscription id %d\n"),
        subId));
      break;
    case -1:
      ACE_ERROR((LM_ERROR, ACE_TEXT("ERROR: DCPS_IR_Participant::add_subscription ")
        ACE_TEXT("Unknown error while adding subscription id %d\n"),
        subId));
    };

  return status;
}


int DCPS_IR_Participant::remove_subscription (long subId)
{
  DCPS_IR_Subscription* sub = 0;

  int status = subscriptions_.unbind (subId, sub);

  if (0 == status)
    {
      DCPS_IR_Topic_Description* desc = sub->get_topic_description ();
      desc->remove_subscription_reference(sub);

      CORBA::Boolean dont_notify_lost = 0;
      status = sub->remove_associations(dont_notify_lost);
      if (0 != status)
      {
        ACE_ERROR((LM_ERROR, ACE_TEXT("ERROR: DCPS_IR_Participant::remove_subscription ")
          ACE_TEXT("Error removing all associations for subscription id: %d\n"),
          subId));
        return status;
      }

      if (TAO_debug_level > 0)
        {
          ACE_DEBUG((LM_DEBUG, ACE_TEXT("DCPS_IR_Participant::remove_subscription ")
            ACE_TEXT("Removed subscription id: %d\n"),
            subId));
        }

      // Dispose the BIT information
      domain_->dispose_subscription_bit(sub);

      delete sub;
    }
  else
    {
      ACE_ERROR((LM_ERROR, ACE_TEXT("ERROR: DCPS_IR_Participant::remove_subscription ")
        ACE_TEXT("Error removing subscription %X id: %d\n"),
        sub, subId));
    } // if (0 == status)

  return status;
}


int DCPS_IR_Participant::add_topic_reference (DCPS_IR_Topic* topic)
{
  TAO::DCPS::RepoId topicId = topic->get_id();
  int status = topicRefs_.bind(topicId, topic);

  switch (status)
    {
    case 0:
      if (TAO_debug_level > 0)
        {
          ACE_DEBUG((LM_DEBUG, ACE_TEXT("DCPS_IR_Participant::add_topic_reference ")
            ACE_TEXT("Successfully added topic reference %X id: %d\n"),
            topic, topicId));
        }
      break;
    case 1:
      ACE_ERROR((LM_ERROR, ACE_TEXT("ERROR: DCPS_IR_Participant::add_topic_reference ")
        ACE_TEXT("Attempted to add existing topic reference id %d\n"),
        topicId));
      break;
    case -1:
      ACE_ERROR((LM_ERROR, ACE_TEXT("ERROR: DCPS_IR_Participant::add_topic_reference ")
        ACE_TEXT("Unknown error while adding topic reference id %d\n"),
        topicId));
    };

  return status;
}


int DCPS_IR_Participant::remove_topic_reference (long topicId,
                                                 DCPS_IR_Topic*& topic)
{
  int status = topicRefs_.unbind (topicId, topic);

  if (0 == status)
    {
      if (TAO_debug_level > 0)
        {
          ACE_DEBUG((LM_DEBUG, ACE_TEXT("DCPS_IR_Participant::remove_topic_reference ")
            ACE_TEXT("Removed topic reference %X id: %d\n"),
            topic, topicId));
        }
    }
  else
    {
      ACE_ERROR((LM_ERROR, ACE_TEXT("ERROR: DCPS_IR_Participant::remove_topic_reference ")
        ACE_TEXT("Error removing topic reference %X id: %d\n"),
        topic, topicId));
    } // if (0 == status)

  return status;
}


int DCPS_IR_Participant::find_topic_reference (long topicId,
                                               DCPS_IR_Topic*& topic)
{
  int status = topicRefs_.find (topicId, topic);

  if (0 == status)
    {
      if (TAO_debug_level > 0)
        {
          ACE_DEBUG((LM_DEBUG, ACE_TEXT("DCPS_IR_Participant::find_topic_reference ")
            ACE_TEXT("Found topic reference %X id: %d\n"),
            topic, topicId));
        }
    }
  else
    {
      ACE_ERROR((LM_ERROR, ACE_TEXT("ERROR: DCPS_IR_Participant::find_topic_reference ")
        ACE_TEXT("Error finding topic reference %X id: %d\n"),
        topic, topicId));
    } // if (0 == status)

  return status;
}


void DCPS_IR_Participant::remove_all_dependents (CORBA::Boolean notify_lost)
{
  DCPS_IR_Topic* topic = 0;
  DCPS_IR_Topic_Description* subDesc = 0;

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

  // remove all the subscriptions assocaitions
  DCPS_IR_Subscription* sub = 0;
  DCPS_IR_Subscription_Map::ITERATOR subIter = subscriptions_.begin();
  DCPS_IR_Subscription_Map::ITERATOR subEnd = subscriptions_.end();

  while (subIter != subEnd)
    {
      sub = (*subIter).int_id_;
      ++subIter;

      subDesc = sub->get_topic_description ();
      subDesc->remove_subscription_reference(sub);
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
      delete pub;
    }
  publications_.unbind_all();

  // delete all the subscriptions
  subIter = subscriptions_.begin();
  while (subIter != subEnd)
    {
      sub = (*subIter).int_id_;
      ++subIter;
      delete sub;
    }
  subscriptions_.unbind_all();
}


void DCPS_IR_Participant::mark_dead ()
{
  aliveStatus_ = 0;
  domain_->add_dead_participant(this);
}


TAO::DCPS::RepoId DCPS_IR_Participant::get_id ()
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



void DCPS_IR_Participant::ignore_participant (TAO::DCPS::RepoId id)
{
  if (TAO_debug_level > 0)
    {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("DCPS_IR_Participant::ignore_participant () ")
        ACE_TEXT("Participant %d now ignoring participant %d\n"),
        id_, id));
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


void DCPS_IR_Participant::ignore_topic (TAO::DCPS::RepoId id)
{
  if (TAO_debug_level > 0)
    {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("DCPS_IR_Participant::ignore_topic () ")
        ACE_TEXT("Participant %d now ignoring topic %d\n"),
        id_, id));
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


void DCPS_IR_Participant::ignore_publication (TAO::DCPS::RepoId id)
{
    if (TAO_debug_level > 0)
    {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("DCPS_IR_Participant::ignore_publication () ")
        ACE_TEXT("Participant %d now ignoring publication %d\n"),
        id_, id));
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


void DCPS_IR_Participant::ignore_subscription (TAO::DCPS::RepoId id)
{
    if (TAO_debug_level > 0)
    {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("DCPS_IR_Participant::ignore_subscription () ")
        ACE_TEXT("Participant %d now ignoring subscription %d\n"),
        id_, id));
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



CORBA::Boolean DCPS_IR_Participant::is_participant_ignored (TAO::DCPS::RepoId id)
{
  return (0 == ignoredParticipants_.find(id));
}


CORBA::Boolean DCPS_IR_Participant::is_topic_ignored (TAO::DCPS::RepoId id)
{
  return (0 == ignoredTopics_.find(id));
}


CORBA::Boolean DCPS_IR_Participant::is_publication_ignored (TAO::DCPS::RepoId id)
{
  return (0 == ignoredPublications_.find(id));
}


CORBA::Boolean DCPS_IR_Participant::is_subscription_ignored (TAO::DCPS::RepoId id)
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


CORBA::Boolean DCPS_IR_Participant::is_bit ()
{
  return isBIT_;
}


void DCPS_IR_Participant::set_bit_status (CORBA::Boolean isBIT)
{
  isBIT_ = isBIT;
}



#if defined (ACE_HAS_EXPLICIT_TEMPLATE_INSTANTIATION)

template class ACE_Map_Entry<TAO::DCPS::RepoId,DCPS_IR_Publication*>;
template class ACE_Map_Manager<TAO::DCPS::RepoId,DCPS_IR_Publication*,ACE_Null_Mutex>;
template class ACE_Map_Iterator_Base<TAO::DCPS::RepoId,DCPS_IR_Publication*,ACE_Null_Mutex>;
template class ACE_Map_Iterator<TAO::DCPS::RepoId,DCPS_IR_Publication*,ACE_Null_Mutex>;
template class ACE_Map_Reverse_Iterator<TAO::DCPS::RepoId,DCPS_IR_Publication*,ACE_Null_Mutex>;

template class ACE_Map_Entry<TAO::DCPS::RepoId,DCPS_IR_Subscription*>;
template class ACE_Map_Manager<TAO::DCPS::RepoId,DCPS_IR_Subscription*,ACE_Null_Mutex>;
template class ACE_Map_Iterator_Base<TAO::DCPS::RepoId,DCPS_IR_Subscription*,ACE_Null_Mutex>;
template class ACE_Map_Iterator<TAO::DCPS::RepoId,DCPS_IR_Subscription*,ACE_Null_Mutex>;
template class ACE_Map_Reverse_Iterator<TAO::DCPS::RepoId,DCPS_IR_Subscription*,ACE_Null_Mutex>;

template class ACE_Node<TAO::DCPS::RepoId>;
template class ACE_Unbounded_Set<TAO::DCPS::RepoId>;
template class ACE_Unbounded_Set_Iterator<TAO::DCPS::RepoId>;

#elif defined(ACE_HAS_TEMPLATE_INSTANTIATION_PRAGMA)

#pragma instantiate ACE_Map_Entry<TAO::DCPS::RepoId,DCPS_IR_Publication*>
#pragma instantiate ACE_Map_Manager<TAO::DCPS::RepoId,DCPS_IR_Publication*,ACE_Null_Mutex>
#pragma instantiate ACE_Map_Iterator_Base<TAO::DCPS::RepoId,DCPS_IR_Publication*,ACE_Null_Mutex>
#pragma instantiate ACE_Map_Iterator<TAO::DCPS::RepoId,DCPS_IR_Publication*,ACE_Null_Mutex>
#pragma instantiate ACE_Map_Reverse_Iterator<TAO::DCPS::RepoId,DCPS_IR_Publication*,ACE_Null_Mutex>

#pragma instantiate ACE_Map_Entry<TAO::DCPS::RepoId,DCPS_IR_Subscription*>
#pragma instantiate ACE_Map_Manager<TAO::DCPS::RepoId,DCPS_IR_Subscription*,ACE_Null_Mutex>
#pragma instantiate ACE_Map_Iterator_Base<TAO::DCPS::RepoId,DCPS_IR_Subscription*,ACE_Null_Mutex>
#pragma instantiate ACE_Map_Iterator<TAO::DCPS::RepoId,DCPS_IR_Subscription*,ACE_Null_Mutex>
#pragma instantiate ACE_Map_Reverse_Iterator<TAO::DCPS::RepoId,DCPS_IR_Subscription*,ACE_Null_Mutex>

#pragma instantiate ACE_Node<TAO::DCPS::RepoId>
#pragma instantiate ACE_Unbounded_Set<TAO::DCPS::RepoId>
#pragma instantiate ACE_Unbounded_Set_Iterator<TAO::DCPS::RepoId>

#endif /* ACE_HAS_EXPLICIT_TEMPLATE_INSTANTIATION */

