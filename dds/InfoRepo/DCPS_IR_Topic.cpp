#include "DcpsInfo_pch.h"

#include /**/ "DCPS_IR_Topic.h"
#include /**/ "DCPS_IR_Domain.h"

#include /**/ "DCPS_IR_Subscription.h"
#include /**/ "DCPS_IR_Publication.h"
#include /**/ "DCPS_IR_Participant.h"
#include /**/ "DCPS_IR_Topic_Description.h"

#include /**/ "DCPS_Utils.h"
#include /**/ "dds/DCPS/Qos_Helper.h"
#include /**/ "tao/debug.h"

#include <sstream>

DCPS_IR_Topic::DCPS_IR_Topic(OpenDDS::DCPS::RepoId id,
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

      std::stringstream buffer;
      long handle;
      handle = ::OpenDDS::DCPS::GuidConverter( this->id_);
      buffer << this->id_ << "(" << std::hex << handle << ")";

      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: DCPS_IR_Topic::~DCPS_IR_Topic: ")
        ACE_TEXT("id %s has retained publications.\n"),
        buffer.str().c_str()
      ));

      while (iter != end)
        {
          pub = *iter;
          ++iter;

          std::stringstream publicationBuffer;
          long handle;
          ::OpenDDS::DCPS::RepoId publicationId = pub->get_id();
          handle = ::OpenDDS::DCPS::GuidConverter( publicationId);
          publicationBuffer << publicationId << "(" << std::hex << handle << ")";

          ACE_ERROR((LM_ERROR,
            ACE_TEXT("(%P|%t) ERROR: DCPS_IR_Topic::~DCPS_IR_Topic: ")
            ACE_TEXT("topic %s retains publication id %s.\n"),
            buffer.str().c_str(),
            publicationBuffer.str().c_str()
          ));
        }
    }

  if (0 != subscriptionRefs_.size())
    {
      DCPS_IR_Subscription* sub = 0;
      DCPS_IR_Subscription_Set::ITERATOR iter = subscriptionRefs_.begin();
      DCPS_IR_Subscription_Set::ITERATOR end = subscriptionRefs_.end();

      std::stringstream buffer;
      long handle;
      handle = ::OpenDDS::DCPS::GuidConverter( this->id_);
      buffer << this->id_ << "(" << std::hex << handle << ")";

      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: DCPS_IR_Topic::~DCPS_IR_Topic: ")
        ACE_TEXT("id %s has retained subscriptions.\n"),
        buffer.str().c_str()
      ));

      while (iter != end)
        {
          sub = *iter;
          ++iter;

          std::stringstream subscriptionBuffer;
          long handle;
          ::OpenDDS::DCPS::RepoId subscriptionId = sub->get_id();
          handle = ::OpenDDS::DCPS::GuidConverter( subscriptionId);
          subscriptionBuffer << subscriptionId << "(" << std::hex << handle << ")";

          ACE_ERROR((LM_ERROR,
            ACE_TEXT("(%P|%t) ERROR: DCPS_IR_Topic::~DCPS_IR_Topic: ")
            ACE_TEXT("topic %s retains subscription id %s.\n"),
            buffer.str().c_str(),
            subscriptionBuffer.str().c_str()
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

      // Publish the BIT information
      domain_->publish_publication_bit (publication);

      if (associate)
        {
          description_->try_associate_publication(publication);
          // Do not check incompatible qos here.  The check is done
          // in the DCPS_IR_Topic_Description::try_associate_publication method
        }
      if (::OpenDDS::DCPS::DCPS_debug_level > 0)
        {
          std::stringstream buffer;
          long handle;
          handle = ::OpenDDS::DCPS::GuidConverter( this->id_);
          buffer << this->id_ << "(" << std::hex << handle << ")";

          std::stringstream publicationBuffer;
          ::OpenDDS::DCPS::RepoId publicationId = publication->get_id();
          handle = ::OpenDDS::DCPS::GuidConverter( publicationId);
          publicationBuffer << publicationId << "(" << std::hex << handle << ")";

          ACE_DEBUG((LM_DEBUG,
            ACE_TEXT("(%P|%t) DCPS_IR_Topic::add_publication_reference: ")
            ACE_TEXT("topic %s added publication %s at %x\n"),
            buffer.str().c_str(),
            publicationBuffer.str().c_str(),
            publication
          ));
        }
      break;

    case 1:
      if( ::OpenDDS::DCPS::DCPS_debug_level > 0) {
        std::stringstream buffer;
        long handle;
        handle = ::OpenDDS::DCPS::GuidConverter( this->id_);
        buffer << this->id_ << "(" << std::hex << handle << ")";

        std::stringstream publicationBuffer;
        ::OpenDDS::DCPS::RepoId publicationId = publication->get_id();
        handle = ::OpenDDS::DCPS::GuidConverter( publicationId);
        publicationBuffer << publicationId << "(" << std::hex << handle << ")";

        ACE_DEBUG((LM_DEBUG,
          ACE_TEXT("(%P|%t) WARNING: DCPS_IR_Topic::add_publication_reference: ")
          ACE_TEXT("topic %s attempt to re-add publication %s.\n"),
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
        ::OpenDDS::DCPS::RepoId publicationId = publication->get_id();
        handle = ::OpenDDS::DCPS::GuidConverter( publicationId);
        publicationBuffer << publicationId << "(" << std::hex << handle << ")";

        ACE_ERROR((LM_ERROR,
          ACE_TEXT("(%P|%t) ERROR: DCPS_IR_Topic::add_publication_reference: ")
          ACE_TEXT("topic %s failed to add publication %s\n"),
          buffer.str().c_str(),
          publicationBuffer.str().c_str()
        ));
      }
    };

  return status;
}


int DCPS_IR_Topic::remove_publication_reference (DCPS_IR_Publication* publication)
{
  int status = publicationRefs_.remove(publication);
  if (0 == status)
    {
      if (::OpenDDS::DCPS::DCPS_debug_level > 0)
        {
          std::stringstream buffer;
          long handle;
          handle = ::OpenDDS::DCPS::GuidConverter( this->id_);
          buffer << this->id_ << "(" << std::hex << handle << ")";

          std::stringstream publicationBuffer;
          ::OpenDDS::DCPS::RepoId publicationId = publication->get_id();
          handle = ::OpenDDS::DCPS::GuidConverter( publicationId);
          publicationBuffer << publicationId << "(" << std::hex << handle << ")";

          ACE_DEBUG((LM_DEBUG,
            ACE_TEXT("(%P|%t) DCPS_IR_Topic::remove_publication_reference: ")
            ACE_TEXT("topic %s removed publication %s.\n"),
            buffer.str().c_str(),
            publicationBuffer.str().c_str()
          ));
        }
    }
  else
    {
      std::stringstream buffer;
      long handle;
      handle = ::OpenDDS::DCPS::GuidConverter( this->id_);
      buffer << this->id_ << "(" << std::hex << handle << ")";

      std::stringstream publicationBuffer;
      ::OpenDDS::DCPS::RepoId publicationId = publication->get_id();
      handle = ::OpenDDS::DCPS::GuidConverter( publicationId);
      publicationBuffer << publicationId << "(" << std::hex << handle << ")";

      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: DCPS_IR_Topic::remove_publication_reference: ")
        ACE_TEXT("topic %s failed to remove publication %s.\n"),
        buffer.str().c_str(),
        publicationBuffer.str().c_str()
      ));
    }
  return status;
}


int DCPS_IR_Topic::add_subscription_reference (DCPS_IR_Subscription* subscription
                                                           , bool associate)
{
  int status = subscriptionRefs_.insert(subscription);

  switch (status)
    {
    case 0:
      if( ::OpenDDS::DCPS::DCPS_debug_level > 0) {
        std::stringstream buffer;
        long handle;
        handle = ::OpenDDS::DCPS::GuidConverter( this->id_);
        buffer << this->id_ << "(" << std::hex << handle << ")";

        std::stringstream subscriptionBuffer;
        ::OpenDDS::DCPS::RepoId subscriptionId = subscription->get_id();
        handle = ::OpenDDS::DCPS::GuidConverter( subscriptionId);
        subscriptionBuffer << subscriptionId << "(" << std::hex << handle << ")";

        ACE_DEBUG((LM_DEBUG,
          ACE_TEXT("(%P|%t) DCPS_IR_Topic::add_subscription_reference: ")
          ACE_TEXT("topic %s added subscription %s at %x.\n"),
          buffer.str().c_str(),
          subscriptionBuffer.str().c_str(),
          subscription
        ));
      }
      status = this->description_->add_subscription_reference (subscription, associate);
      break;

    case 1:
      {
        std::stringstream buffer;
        long handle;
        handle = ::OpenDDS::DCPS::GuidConverter( this->id_);
        buffer << this->id_ << "(" << std::hex << handle << ")";

        std::stringstream subscriptionBuffer;
        ::OpenDDS::DCPS::RepoId subscriptionId = subscription->get_id();
        handle = ::OpenDDS::DCPS::GuidConverter( subscriptionId);
        subscriptionBuffer << subscriptionId << "(" << std::hex << handle << ")";

        ACE_ERROR((LM_ERROR,
          ACE_TEXT("(%P|%t) ERROR: DCPS_IR_Topic::add_subscription_reference: ")
          ACE_TEXT("topic %s attempt to re-add subscription %s.\n"),
          buffer.str().c_str(),
          subscriptionBuffer.str().c_str()
        ));
      }
      break;

    case -1:
      {
        std::stringstream buffer;
        long handle;
        handle = ::OpenDDS::DCPS::GuidConverter( this->id_);
        buffer << this->id_ << "(" << std::hex << handle << ")";

        std::stringstream subscriptionBuffer;
        ::OpenDDS::DCPS::RepoId subscriptionId = subscription->get_id();
        handle = ::OpenDDS::DCPS::GuidConverter( subscriptionId);
        subscriptionBuffer << subscriptionId << "(" << std::hex << handle << ")";

        ACE_ERROR((LM_ERROR,
          ACE_TEXT("(%P|%t) ERROR: DCPS_IR_Topic::add_subscription_reference: ")
          ACE_TEXT("topic %s failed to add subscription %s.\n"),
          buffer.str().c_str(),
          subscriptionBuffer.str().c_str()
        ));
      }
    };

  return status;
}


int DCPS_IR_Topic::remove_subscription_reference (DCPS_IR_Subscription* subscription)
{
  int status = subscriptionRefs_.remove(subscription);
  if (0 == status)
    {
      if (::OpenDDS::DCPS::DCPS_debug_level > 0)
        {
          std::stringstream buffer;
          long handle;
          handle = ::OpenDDS::DCPS::GuidConverter( this->id_);
          buffer << this->id_ << "(" << std::hex << handle << ")";

          std::stringstream subscriptionBuffer;
          ::OpenDDS::DCPS::RepoId subscriptionId = subscription->get_id();
          handle = ::OpenDDS::DCPS::GuidConverter( subscriptionId);
          subscriptionBuffer << subscriptionId << "(" << std::hex << handle << ")";

          ACE_DEBUG((LM_DEBUG,
            ACE_TEXT("(%P|%t) DCPS_IR_Topic::remove_subscription_reference: ")
            ACE_TEXT("topic %s removed subscription %s.\n"),
            buffer.str().c_str(),
            subscriptionBuffer.str().c_str()
          ));
        }
        
      this->description_->remove_subscription_reference (subscription);
    }
  else
    {
      std::stringstream buffer;
      long handle;
      handle = ::OpenDDS::DCPS::GuidConverter( this->id_);
      buffer << this->id_ << "(" << std::hex << handle << ")";

      std::stringstream subscriptionBuffer;
      ::OpenDDS::DCPS::RepoId subscriptionId = subscription->get_id();
      handle = ::OpenDDS::DCPS::GuidConverter( subscriptionId);
      subscriptionBuffer << subscriptionId << "(" << std::hex << handle << ")";

      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: DCPS_IR_Topic::remove_subscription_reference: ")
        ACE_TEXT("topic %s failed to remove subscription %s.\n"),
        buffer.str().c_str(),
        subscriptionBuffer.str().c_str()
      ));
    } // if (0 == status)

  return status;
}



OpenDDS::DCPS::RepoId DCPS_IR_Topic::get_id () const
{
  return id_;
}


OpenDDS::DCPS::RepoId DCPS_IR_Topic::get_participant_id () const
{
  return participant_->get_id();
}


::DDS::TopicQos * DCPS_IR_Topic::get_topic_qos ()
{
  return &qos_;
}


bool DCPS_IR_Topic::set_topic_qos (const ::DDS::TopicQos& qos)
{
  // Do not need re-evaluate compatibility and associations when
  // TopicQos changes since only datareader and datawriter QoS 
  // are evaludated during normal associations establishment.

  bool pub_to_rd_wr = ! (qos.topic_data == qos_.topic_data);

  qos_ = qos;
  domain_->publish_topic_bit (this);
 
  if (! pub_to_rd_wr)
    return true;
  
  // The only changeable TopicQos used by DataWriter and DataReader
  // is topic_data so we need publish it to DW/DR BIT to make they
  // are consistent.

  // Update qos in datawriter BIT for associated datawriters.

  {
    DCPS_IR_Publication_Set::ITERATOR iter = publicationRefs_.begin ();
    DCPS_IR_Publication_Set::ITERATOR end = publicationRefs_.end();

    while (iter != end)
    {
      domain_->publish_publication_bit (*iter);
      ++iter;
    }
  }

  // Update qos in datareader BIT for associated datareader.

  {
    DCPS_IR_Subscription_Set::ITERATOR iter = subscriptionRefs_.begin ();
    DCPS_IR_Subscription_Set::ITERATOR end = subscriptionRefs_.end();

    while (iter != end)
    {
      domain_->publish_subscription_bit (*iter);
      ++iter;
    }
  }

  return true;
}



void DCPS_IR_Topic::try_associate (DCPS_IR_Subscription* subscription)
{
  // check if we should ignore this subscription
  if ( participant_->is_subscription_ignored (subscription->get_id()) ||
       participant_->is_participant_ignored (subscription->get_participant_id()) ||
       participant_->is_topic_ignored (subscription->get_topic_id()) )
    {
      if (::OpenDDS::DCPS::DCPS_debug_level > 0)
        {
          std::stringstream buffer;
          long handle;
          handle = ::OpenDDS::DCPS::GuidConverter( this->id_);
          buffer << this->id_ << "(" << std::hex << handle << ")";

          std::stringstream subscriptionBuffer;
          ::OpenDDS::DCPS::RepoId subscriptionId = subscription->get_id();
          handle = ::OpenDDS::DCPS::GuidConverter( subscriptionId);
          subscriptionBuffer << subscriptionId << "(" << std::hex << handle << ")";

          ACE_DEBUG((LM_DEBUG,
            ACE_TEXT("(%P|%t) DCPS_IR_Topic::try_associate: ")
            ACE_TEXT("topic %s ignoring subscription %s.\n"),
            buffer.str().c_str(),
            subscriptionBuffer.str().c_str()
          ));
        }
    }
  else
    {
      // check all publications for compatibility
      DCPS_IR_Publication* pub = 0;
      OpenDDS::DCPS::IncompatibleQosStatus* qosStatus = 0;

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


void DCPS_IR_Topic::reevaluate_associations (DCPS_IR_Subscription* subscription)
{
  DCPS_IR_Publication * pub = 0;
  DCPS_IR_Publication_Set::ITERATOR iter = publicationRefs_.begin ();
  DCPS_IR_Publication_Set::ITERATOR end = publicationRefs_.end();

  while (iter != end)
  {
    pub = *iter;
    ++iter;

    subscription->reevaluate_association (pub);
    pub->reevaluate_association (subscription);
  }
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
