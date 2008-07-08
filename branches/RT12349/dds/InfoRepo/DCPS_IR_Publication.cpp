#include "DcpsInfo_pch.h"
#include /**/ "DCPS_IR_Publication.h"

#include /**/ "DCPS_IR_Participant.h"
#include /**/ "DCPS_IR_Topic.h"
#include /**/ "DCPS_IR_Subscription.h"
#include /**/ "DCPS_IR_Domain.h"
#include /**/ "dds/DCPS/Qos_Helper.h"
#include /**/ "tao/debug.h"


DCPS_IR_Publication::DCPS_IR_Publication (OpenDDS::DCPS::RepoId id,
                                          DCPS_IR_Participant* participant,
                                          DCPS_IR_Topic* topic,
                                          OpenDDS::DCPS::DataWriterRemote_ptr writer,
                                          ::DDS::DataWriterQos qos,
                                          OpenDDS::DCPS::TransportInterfaceInfo info,
                                          ::DDS::PublisherQos publisherQos)
  : id_(id),
    participant_(participant),
    topic_(topic),
    handle_(0),
    isBIT_(0),
    qos_(qos),
    info_(info),
    publisherQos_(publisherQos)
{
  writer_ =  OpenDDS::DCPS::DataWriterRemote::_duplicate( writer );

  incompatibleQosStatus_.total_count = 0;
  incompatibleQosStatus_.count_since_last_send = 0;
}


DCPS_IR_Publication::~DCPS_IR_Publication ()
{
  if (0 != associations_.size() )
    {
      CORBA::Boolean dont_notify_lost = 0;
      remove_associations(dont_notify_lost);
    }
}


int DCPS_IR_Publication::add_associated_subscription (DCPS_IR_Subscription* sub)
{
  // keep track of the association locally
  int status = associations_.insert(sub);
  switch (status)
    {
    case 0:
      {
        // inform the datawriter about the association
        OpenDDS::DCPS::ReaderAssociationSeq associationSeq(2);
        associationSeq.length(1);
        associationSeq[0].readerTransInfo = sub->get_transportInterfaceInfo ();
        associationSeq[0].readerId = sub->get_id();
        associationSeq[0].subQos = *(sub->get_subscriber_qos());
        associationSeq[0].readerQos = *(sub->get_datareader_qos());

        if (participant_->is_alive())
          {
            try
              {
                if (TAO_debug_level > 0)
                 {
                    ACE_DEBUG((LM_DEBUG,
                      ACE_TEXT("(%P|%t) DCPS_IR_Publication::add_associated_subscription:")
                      ACE_TEXT(" calling pub %d with sub %d\n"),
                      id_, sub->get_id() ));
                 }
                writer_->add_associations(id_, associationSeq);
              }
            catch (const CORBA::Exception& ex)
              {
                ex._tao_print_exception (
                  "ERROR: Exception caught in DCPS_IR_Publication::add_associated_publication:");
                participant_->mark_dead();
                status = -1;
              }
          }

        if (TAO_debug_level > 0)
          {
            ACE_DEBUG((LM_DEBUG, ACE_TEXT("DCPS_IR_Publication::add_associated_subscription ")
              ACE_TEXT("Successfully added subscription %X\n"),
              sub));
          }
      }
      break;
    case 1:
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: DCPS_IR_Publication::add_associated_subscription ")
        ACE_TEXT("Attempted to add existing subscription %X\n"),
        sub));
      break;
    case -1:
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: DCPS_IR_Publication::add_associated_subscription ")
        ACE_TEXT("Unknown error while adding subscription %X\n"),
        sub));
    };


  return status;
}


int DCPS_IR_Publication::remove_associated_subscription (DCPS_IR_Subscription* sub,
                                                         CORBA::Boolean sendNotify,
                                                         CORBA::Boolean notify_lost)
{
  bool marked_dead = false;

  if (sendNotify)
    {
      OpenDDS::DCPS::ReaderIdSeq idSeq(1);
      idSeq.length(1);
      idSeq[0]= sub->get_id();
      if (participant_->is_alive())
        {
          try
            {
              writer_->remove_associations(idSeq, notify_lost);
            }
          catch (const CORBA::Exception& ex)
            {
              if (TAO_debug_level > 0)
                {
                  ex._tao_print_exception (
                    "ERROR: Exception caught in DCPS_IR_Publication::remove_associated_publication:");
                }
              participant_->mark_dead();
              marked_dead = true;
            }
        }
    }

  int status = associations_.remove(sub);
  if (0 == status)
    {
      if (TAO_debug_level > 0)
        {
          ACE_DEBUG((LM_DEBUG, ACE_TEXT("DCPS_IR_Publication::remove_associated_subscription ")
            ACE_TEXT("Removed subscription %X\n"),
            sub));
        }
    }
  else
    {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: DCPS_IR_Publication::remove_associated_subscription ")
        ACE_TEXT("Unable to remove subscription %X\n"),
        sub));
    } // if (0 == status)

  if (marked_dead)
    {
      return -1;
    }
  else
    {
      return status;
    }
}


int DCPS_IR_Publication::remove_associations (CORBA::Boolean notify_lost)
{
  int status = 0;
  DCPS_IR_Subscription* sub = 0;
  size_t numAssociations = associations_.size();
  CORBA::Boolean dontSend = 0;
  CORBA::Boolean send = 1;

  if (0 < numAssociations)
    {
      DCPS_IR_Subscription_Set::ITERATOR iter = associations_.begin();
      DCPS_IR_Subscription_Set::ITERATOR end = associations_.end();

      while (iter != end)
        {
          sub = *iter;
          ++iter;
          sub->remove_associated_publication(this, send, notify_lost);
          remove_associated_subscription (sub, dontSend, notify_lost);
        }
    }

  return status;
}

void DCPS_IR_Publication::disassociate_participant (OpenDDS::DCPS::RepoId id)
{
  DCPS_IR_Subscription* sub = 0;
  size_t numAssociations = associations_.size();
  CORBA::Boolean send = 1;
  CORBA::Boolean dontSend = 0;
  long count = 0;

  if (0 < numAssociations)
    {
      OpenDDS::DCPS::ReaderIdSeq idSeq(numAssociations);
      idSeq.length(numAssociations);

      DCPS_IR_Subscription_Set::ITERATOR iter = associations_.begin();
      DCPS_IR_Subscription_Set::ITERATOR end = associations_.end();

      while (iter != end)
        {
          sub = *iter;
          ++iter;
          if (TAO_debug_level > 0)
            {
              ACE_DEBUG((LM_DEBUG,
                ACE_TEXT("DCPS_IR_Publication::disassociate_participant () ")
                ACE_TEXT("Publication %d testing if sub %d particpant %d = %d\n"),
                id_, sub->get_id(), sub->get_participant_id(), id));
            }

          if (id == sub->get_participant_id() )
            {
              CORBA::Boolean dont_notify_lost = 0;
              sub->remove_associated_publication(this, send, dont_notify_lost);
              remove_associated_subscription (sub, dontSend, dont_notify_lost);

              idSeq[count] = sub->get_id();
              ++count;
            }
        }

      if (0 < count)
        {
          idSeq.length(count);

          if (participant_->is_alive())
            {
              try
                {
                  CORBA::Boolean dont_notify_lost = 0;
                  writer_->remove_associations(idSeq, dont_notify_lost);
                }
              catch (const CORBA::Exception& ex)
                {
                  if (TAO_debug_level > 0)
                    {
                      ex._tao_print_exception (
                        "ERROR: Exception caught in DCPS_IR_Publication::remove_associations:");
                    }
                  participant_->mark_dead();
                }
            }
        }
    }
}


void DCPS_IR_Publication::disassociate_topic (OpenDDS::DCPS::RepoId id)
{
  DCPS_IR_Subscription* sub = 0;
  size_t numAssociations = associations_.size();
  CORBA::Boolean send = 1;
  CORBA::Boolean dontSend = 0;
  long count = 0;

  if (0 < numAssociations)
    {
      OpenDDS::DCPS::ReaderIdSeq idSeq(numAssociations);
      idSeq.length(numAssociations);

      DCPS_IR_Subscription_Set::ITERATOR iter = associations_.begin();
      DCPS_IR_Subscription_Set::ITERATOR end = associations_.end();

      while (iter != end)
        {
          sub = *iter;
          ++iter;
          if (TAO_debug_level > 0)
            {
              ACE_DEBUG((LM_DEBUG,
                ACE_TEXT("DCPS_IR_Publication::disassociate_topic () ")
                ACE_TEXT("Publication %d testing if sub %d topic %d = %d\n"),
                id_, sub->get_id(), sub->get_topic_id(), id));
            }

          if (id == sub->get_topic_id() )
            {
              CORBA::Boolean dont_notify_lost = 0;
              sub->remove_associated_publication(this, send, dont_notify_lost);
              remove_associated_subscription (sub, dontSend, dont_notify_lost);

              idSeq[count] = sub->get_id();
              ++count;
            }
        }

      if (0 < count)
        {
          idSeq.length(count);

          if (participant_->is_alive())
            {
              try
                {
                  CORBA::Boolean dont_notify_lost = 0;
                  writer_->remove_associations(idSeq, dont_notify_lost);
                }
              catch (const CORBA::Exception& ex)
                {
                  if (TAO_debug_level > 0)
                    {
                      ex._tao_print_exception (
                        "ERROR: Exception caught in DCPS_IR_Publication::remove_associations:");
                    }
                  participant_->mark_dead();
                }
            }
        }
    }
}


void DCPS_IR_Publication::disassociate_subscription (OpenDDS::DCPS::RepoId id)
{
  DCPS_IR_Subscription* sub = 0;
  size_t numAssociations = associations_.size();
  CORBA::Boolean send = 1;
  CORBA::Boolean dontSend = 0;
  long count = 0;

  if (0 < numAssociations)
    {
      OpenDDS::DCPS::ReaderIdSeq idSeq(numAssociations);
      idSeq.length(numAssociations);

      DCPS_IR_Subscription_Set::ITERATOR iter = associations_.begin();
      DCPS_IR_Subscription_Set::ITERATOR end = associations_.end();

      while (iter != end)
        {
          sub = *iter;
          ++iter;
          if (TAO_debug_level > 0)
            {
              ACE_DEBUG((LM_DEBUG,
                ACE_TEXT("DCPS_IR_Publication::disassociate_subscription () ")
                ACE_TEXT("Publication %d testing if sub %d = %d\n"),
                id_, sub->get_id(), id));
            }

          if (id == sub->get_id() )
            {
              CORBA::Boolean dont_notify_lost = 0;
              sub->remove_associated_publication(this, send, dont_notify_lost);
              remove_associated_subscription (sub, dontSend, dont_notify_lost);

              idSeq[count] = sub->get_id();
              ++count;
            }
        }

      if (0 < count)
        {
          idSeq.length(count);

          if (participant_->is_alive())
            {
              try
                {
                  CORBA::Boolean dont_notify_lost = 0;
                  writer_->remove_associations(idSeq, dont_notify_lost);
                }
              catch (const CORBA::Exception& ex)
                {
                  if (TAO_debug_level > 0)
                    {
                      ex._tao_print_exception (
                        "ERROR: Exception caught in DCPS_IR_Publication::remove_associations:");
                    }
                  participant_->mark_dead();
                }
            }
        }
    }
}


void DCPS_IR_Publication::update_incompatible_qos ()
{
  writer_->update_incompatible_qos(incompatibleQosStatus_);
  incompatibleQosStatus_.count_since_last_send = 0;
}


CORBA::Boolean DCPS_IR_Publication::is_subscription_ignored (OpenDDS::DCPS::RepoId partId,
                                                             OpenDDS::DCPS::RepoId topicId,
                                                             OpenDDS::DCPS::RepoId subId)
{
  CORBA::Boolean ignored;
  ignored = ( participant_->is_participant_ignored(partId) ||
              participant_->is_topic_ignored(topicId) ||
              participant_->is_subscription_ignored(subId) );

  return ignored;
}


::DDS::DataWriterQos* DCPS_IR_Publication::get_datawriter_qos ()
{
  return &qos_;
}

SpecificQos DCPS_IR_Publication::set_qos (const ::DDS::DataWriterQos & qos,
                                          const ::DDS::PublisherQos & publisherQos)
{
  bool u_dw_qos = ! (qos_ == qos);
  bool u_pub_qos = ! (publisherQos_ == publisherQos);
  if (u_dw_qos)
    qos_ = qos;
  if (u_pub_qos)
    publisherQos_ = publisherQos;
    
  participant_->get_domain_reference()->publish_publication_bit (this);
  return  u_dw_qos ? DataWriterQos : PublisherQos;
}

::DDS::PublisherQos* DCPS_IR_Publication::get_publisher_qos ()
{
  return &publisherQos_;
}


OpenDDS::DCPS::TransportInterfaceId DCPS_IR_Publication::get_transport_id () const
{
  return info_.transport_id;
}

OpenDDS::DCPS::TransportInterfaceInfo DCPS_IR_Publication::get_transportInterfaceInfo () const
{
  OpenDDS::DCPS::TransportInterfaceInfo info = info_;
  return info;
}

OpenDDS::DCPS::IncompatibleQosStatus* DCPS_IR_Publication::get_incompatibleQosStatus ()
{
  return &incompatibleQosStatus_;
}


OpenDDS::DCPS::RepoId DCPS_IR_Publication::get_id ()
{
  return id_;
}

OpenDDS::DCPS::RepoId DCPS_IR_Publication::get_topic_id ()
{
  return topic_->get_id();
}


OpenDDS::DCPS::RepoId DCPS_IR_Publication::get_participant_id ()
{
  return participant_->get_id();
}


DCPS_IR_Topic* DCPS_IR_Publication::get_topic ()
{
  return topic_;
}


DCPS_IR_Topic_Description* DCPS_IR_Publication::get_topic_description()
{
  return topic_->get_topic_description();
}


::DDS::InstanceHandle_t DCPS_IR_Publication::get_handle()
{
  return handle_;
}


void DCPS_IR_Publication::set_handle(::DDS::InstanceHandle_t handle)
{
  handle_ = handle;
}


CORBA::Boolean DCPS_IR_Publication::is_bit ()
{
  return isBIT_;
}


void DCPS_IR_Publication::set_bit_status (CORBA::Boolean isBIT)
{
  isBIT_ = isBIT;
}


#if defined (ACE_HAS_EXPLICIT_TEMPLATE_INSTANTIATION)

template class ACE_Node<DCPS_IR_Subscription*>;
template class ACE_Unbounded_Set<DCPS_IR_Subscription*>;
template class ACE_Unbounded_Set_Iterator<DCPS_IR_Subscription*>;

#elif defined (ACE_HAS_TEMPLATE_INSTANTIATION_PRAGMA)

#pragma instantiate ACE_Node<DCPS_IR_Subscription*>
#pragma instantiate ACE_Unbounded_Set<DCPS_IR_Subscription*>
#pragma instantiate ACE_Unbounded_Set_Iterator<DCPS_IR_Subscription*>

#endif /* ACE_HAS_EXPLICIT_TEMPLATE_INSTANTIATION */
