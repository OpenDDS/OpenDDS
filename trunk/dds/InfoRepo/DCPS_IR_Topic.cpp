#include "DcpsInfo_pch.h"

#include /**/ "DCPS_IR_Topic.h"
#include /**/ "DCPS_IR_Domain.h"

#include /**/ "DCPS_IR_Subscription.h"
#include /**/ "DCPS_IR_Publication.h"
#include /**/ "DCPS_IR_Participant.h"
#include /**/ "DCPS_IR_Topic_Description.h"

#include /**/ "DCPS_Utils.h"
#include /**/ "tao/debug.h"


DCPS_IR_Topic::DCPS_IR_Topic(TAO::DCPS::RepoId id,
                             ::DDS::TopicQos qos,
                             DCPS_IR_Domain* domain,
                             DCPS_IR_Participant* creator,
                             DCPS_IR_Topic_Description* description)
: id_(id),
  qos_(qos),
  domain_(domain),
  participant_(creator),
  description_(description),
  handle_(0),
  isBIT_(0)
{
}


DCPS_IR_Topic::~DCPS_IR_Topic ()
{
  // check for remaining publication references
  if (0 != publicationRefs_.size())
    {
      DCPS_IR_Publication* pub = 0;
      DCPS_IR_Publication_Set::ITERATOR iter = publicationRefs_.begin();
      DCPS_IR_Publication_Set::ITERATOR end = publicationRefs_.end();

      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("ERROR: DCPS_IR_Topic::~DCPS_IR_Topic () ")
                 ACE_TEXT("id %d\n"),
                 id_ ));

      while (iter != end)
        {
          pub = *iter;
          ++iter;
          ACE_ERROR((LM_ERROR,
                     ACE_TEXT("\tERROR: Publication id %d still held!\n"),
                     pub->get_id()
                     ));
        }
    }
}


int DCPS_IR_Topic::add_publication_reference (DCPS_IR_Publication* publication
                                              , bool associate)
{
  int status = publicationRefs_.insert(publication);
  switch (status)
    {
    case 0:

      if (associate)
        {
          // Publish the BIT information
          domain_->publish_publication_bit (publication);

          description_->try_associate_publication(publication);
          // Do not check incompatible qos here.  The check is done
          // in the DCPS_IR_Topic_Description::try_associate_publication method
        }

      if (TAO_debug_level > 0)
        {
          ACE_DEBUG((LM_DEBUG, ACE_TEXT("DCPS_IR_Topic::add_publication_reference ")
            ACE_TEXT("Successfully added publication reference %X\n"),
            publication));
        }
      break;
    case 1:
      ACE_ERROR((LM_ERROR, ACE_TEXT("ERROR: DCPS_IR_Topic::add_publication_reference ")
        ACE_TEXT("Attempted to add existing publication reference %X\n"),
        publication));
      break;
    case -1:
      ACE_ERROR((LM_ERROR, ACE_TEXT("ERROR: DCPS_IR_Topic::add_publication_reference ")
        ACE_TEXT("Unknown error while adding publication reference %X\n"),
        publication));
    };

  return status;
}


int DCPS_IR_Topic::remove_publication_reference (DCPS_IR_Publication* publication)
{
  int status = publicationRefs_.remove(publication);
  if (0 == status)
    {
      if (TAO_debug_level > 0)
        {
          ACE_DEBUG((LM_DEBUG, ACE_TEXT("DCPS_IR_Topic::remove_publication_reference ")
            ACE_TEXT("Removed publication reference %X\n"),
            publication));
        }
    }
  else
    {
      ACE_ERROR((LM_ERROR, ACE_TEXT("ERROR: DCPS_IR_Topic::remove_publication_reference ")
        ACE_TEXT("Unable to remove publication reference %X\n"),
        publication));
    }
  return status;
}


TAO::DCPS::RepoId DCPS_IR_Topic::get_id () const
{
  return id_;
}


TAO::DCPS::RepoId DCPS_IR_Topic::get_participant_id () const
{
  return participant_->get_id();
}


::DDS::TopicQos * DCPS_IR_Topic::get_topic_qos ()
{
  return &qos_;
}


void DCPS_IR_Topic::try_associate (DCPS_IR_Subscription* subscription)
{
  // check if we should ignore this subscription
  if ( participant_->is_subscription_ignored (subscription->get_id()) ||
       participant_->is_participant_ignored (subscription->get_participant_id()) ||
       participant_->is_topic_ignored (subscription->get_topic_id()) )
    {
      if (TAO_debug_level > 0)
        {
          ACE_DEBUG((LM_DEBUG,
            ACE_TEXT("DCPS_IR_Topic::try_associate () Topic %d ")
            ACE_TEXT("ignoring subscription %d"),
            id_, subscription->get_id() ));
        }
    }
  else
    {
      // check all publications for compatibility
      DCPS_IR_Publication* pub = 0;
      TAO::DCPS::IncompatibleQosStatus* qosStatus = 0;

      DCPS_IR_Publication_Set::ITERATOR iter = publicationRefs_.begin ();
      DCPS_IR_Publication_Set::ITERATOR end = publicationRefs_.end();

      while (iter != end)
        {
          pub = *iter;
          ++iter;
          description_->try_associate(pub, subscription);
          // Check the publications QOS status
          qosStatus = pub->get_incompatibleQosStatus();
          if (0 < qosStatus->count_since_last_send)
            {
              pub->update_incompatible_qos();
            }
        } /* while (iter != end) */

      // The subscription QOS is not checked because
      // we don't know if the subscription is finished cycling
      // through topics.
    }
}


DCPS_IR_Topic_Description* DCPS_IR_Topic::get_topic_description ()
{
  return description_;
}



::DDS::InstanceHandle_t DCPS_IR_Topic::get_handle()
{
  return handle_;
}


void DCPS_IR_Topic::set_handle(::DDS::InstanceHandle_t handle)
{
  handle_ = handle;
}


CORBA::Boolean DCPS_IR_Topic::is_bit ()
{
  return isBIT_;
}


void DCPS_IR_Topic::set_bit_status (CORBA::Boolean isBIT)
{
  isBIT_ = isBIT;
}


#if defined (ACE_HAS_EXPLICIT_TEMPLATE_INSTANTIATION)

template class ACE_Node<DCPS_IR_Publication*>;
template class ACE_Unbounded_Set<DCPS_IR_Publication*>;
template class ACE_Unbounded_Set_Iterator<DCPS_IR_Publication*>;

#elif defined (ACE_HAS_TEMPLATE_INSTANTIATION_PRAGMA)

#pragma instantiate ACE_Node<DCPS_IR_Publication*>
#pragma instantiate ACE_Unbounded_Set<DCPS_IR_Publication*>
#pragma instantiate ACE_Unbounded_Set_Iterator<DCPS_IR_Publication*>

#endif /* ACE_HAS_EXPLICIT_TEMPLATE_INSTANTIATION */
