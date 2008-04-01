#include "DcpsInfo_pch.h"

#include /**/ "DCPS_IR_Subscription.h"

#include /**/ "DCPS_IR_Publication.h"
#include /**/ "DCPS_IR_Participant.h"
#include /**/ "DCPS_IR_Topic_Description.h"
#include /**/ "DCPS_IR_Domain.h"
#include /**/ "dds/DCPS/Qos_Helper.h"
#include /**/ "tao/debug.h"


DCPS_IR_Subscription::DCPS_IR_Subscription (OpenDDS::DCPS::RepoId id,
                                            DCPS_IR_Participant* participant,
                                            DCPS_IR_Topic* topic,
                                            OpenDDS::DCPS::DataReaderRemote_ptr reader,
                                            ::DDS::DataReaderQos qos,
                                            OpenDDS::DCPS::TransportInterfaceInfo info,
                                            ::DDS::SubscriberQos subscriberQos)
  : id_(id),
    participant_(participant),
    topic_(topic),
    handle_(0),
    isBIT_(0),
    qos_(qos),
    info_(info),
    subscriberQos_(subscriberQos)
{
  reader_ =  OpenDDS::DCPS::DataReaderRemote::_duplicate(reader);

  incompatibleQosStatus_.total_count = 0;
  incompatibleQosStatus_.count_since_last_send = 0;
}


DCPS_IR_Subscription::~DCPS_IR_Subscription ()
{
  if (0 != associations_.size() )
    {
      CORBA::Boolean dont_notify_lost = 0;
      remove_associations(dont_notify_lost);
    }
}


int DCPS_IR_Subscription::add_associated_publication (DCPS_IR_Publication* pub)
{
  // keep track of the association locally
  int status = associations_.insert(pub);
  switch (status)
    {
    case 0:
      {
        // inform the datareader about the association
        OpenDDS::DCPS::WriterAssociationSeq associationSeq(2);
        associationSeq.length(1);
        associationSeq[0].writerTransInfo = pub->get_transportInterfaceInfo ();
        associationSeq[0].writerId = pub->get_id();
        associationSeq[0].pubQos = *(pub->get_publisher_qos());
        associationSeq[0].writerQos = *(pub->get_datawriter_qos());

        if (participant_->is_alive())
          {
            try
              {
                if (TAO_debug_level > 0)
                 {
                    ACE_DEBUG((LM_DEBUG,
                      ACE_TEXT("(%P|%t) DCPS_IR_Subscription::add_associated_publication:")
                      ACE_TEXT(" calling sub %d with pub %d\n"),
                      id_, pub->get_id() ));
                 }
                reader_->add_associations(id_, associationSeq);
              }
            catch (const CORBA::Exception& ex)
              {
                ex._tao_print_exception (
                  "ERROR: Exception caught in DCPS_IR_Subscription::add_associated_publication:");
                participant_->mark_dead();
                status = -1;
              }
          }

        if (TAO_debug_level > 0)
          {
            ACE_DEBUG((LM_DEBUG, ACE_TEXT("DCPS_IR_Subscription::add_associated_publication ")
              ACE_TEXT("Successfully added publication %X\n"),
              pub));
          }
      }
      break;
    case 1:
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: DCPS_IR_Subscription::add_associated_publication ")
        ACE_TEXT("Attempted to add existing publication %X\n"),
        pub));
      break;
    case -1:
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: DCPS_IR_Subscription::add_associated_publication ")
        ACE_TEXT("Unknown error while adding publication %X\n"),
        pub));
    };


  return status;
}


int DCPS_IR_Subscription::remove_associated_publication (DCPS_IR_Publication* pub,
                                                         CORBA::Boolean sendNotify,
                                                         CORBA::Boolean notify_lost)
{
  bool marked_dead = false;

  if (sendNotify)
    {
      OpenDDS::DCPS::WriterIdSeq idSeq(5);
      idSeq.length(1);
      idSeq[0] = pub->get_id();
      if (participant_->is_alive())
        {
          try
            {
              reader_->remove_associations(idSeq, notify_lost);
            }
          catch (const CORBA::Exception& ex)
            {
              if (TAO_debug_level > 0)
                {
                  ex._tao_print_exception (
                    "ERROR: Exception caught in DCPS_IR_Subscription::remove_associated_publication:");
                }
              participant_->mark_dead();
              marked_dead = true;
            }
        }
    }

  int status = associations_.remove(pub);
  if (0 == status)
    {
      if (TAO_debug_level > 0)
        {
          ACE_DEBUG((LM_DEBUG, ACE_TEXT("DCPS_IR_Subscription::remove_associated_publication ")
            ACE_TEXT("Removed publication %X\n"),
            pub));
        }
    }
  else
    {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: DCPS_IR_Subscription::remove_associated_publication ")
        ACE_TEXT("Unable to remove publication %X\n"),
        pub));
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


int DCPS_IR_Subscription::remove_associations (CORBA::Boolean notify_lost)
{
  int status = 0;
  DCPS_IR_Publication* pub = 0;
  size_t numAssociations = associations_.size();
  CORBA::Boolean dontSend = 0;
  CORBA::Boolean send = 1;

  if (0 < numAssociations)
    {
      DCPS_IR_Publication_Set::ITERATOR iter = associations_.begin();
      DCPS_IR_Publication_Set::ITERATOR end = associations_.end();

      while (iter != end)
        {
          pub = *iter;
          ++iter;

          pub->remove_associated_subscription (this, send, notify_lost);
          CORBA::Boolean dont_notify_lost = 0;
          remove_associated_publication (pub, dontSend, dont_notify_lost);
        }
    }

  return status;
}


void DCPS_IR_Subscription::disassociate_participant (OpenDDS::DCPS::RepoId id)
{
  DCPS_IR_Publication* pub = 0;
  size_t numAssociations = associations_.size();
  CORBA::Boolean dontSend = 0;
  CORBA::Boolean send = 1;
  long count = 0;

  if (0 < numAssociations)
    {
      OpenDDS::DCPS::WriterIdSeq idSeq(numAssociations);
      idSeq.length(numAssociations);

      DCPS_IR_Publication_Set::ITERATOR iter = associations_.begin();
      DCPS_IR_Publication_Set::ITERATOR end = associations_.end();

      while (iter != end)
        {
          pub = *iter;
          ++iter;
          if (TAO_debug_level > 0)
            {
              ACE_DEBUG((LM_DEBUG,
                ACE_TEXT("DCPS_IR_Subscription::disassociate_participant () ")
                ACE_TEXT("Subscription %d testing if pub %d particpant %d = %d\n"),
                id_, pub->get_id(), pub->get_participant_id(), id));
            }
          if (id == pub->get_participant_id() )
            {
              CORBA::Boolean dont_notify_lost = 0;
              pub->remove_associated_subscription (this, send, dont_notify_lost);
              remove_associated_publication (pub, dontSend, dont_notify_lost);

              idSeq[count] = pub->get_id();
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
                  reader_->remove_associations(idSeq, dont_notify_lost);
                }
              catch (const CORBA::Exception& ex)
                {
                  if (TAO_debug_level > 0)
                    {
                      ex._tao_print_exception (
                        "ERROR: Exception caught in DCPS_IR_Subscription::remove_associations:");
                    }
                  participant_->mark_dead();
                }
            }
        }
    }
}


void DCPS_IR_Subscription::disassociate_topic (OpenDDS::DCPS::RepoId id)
{
  DCPS_IR_Publication* pub = 0;
  size_t numAssociations = associations_.size();
  CORBA::Boolean dontSend = 0;
  CORBA::Boolean send = 1;
  long count = 0;

  if (0 < numAssociations)
    {
      OpenDDS::DCPS::WriterIdSeq idSeq(numAssociations);
      idSeq.length(numAssociations);

      DCPS_IR_Publication_Set::ITERATOR iter = associations_.begin();
      DCPS_IR_Publication_Set::ITERATOR end = associations_.end();

      while (iter != end)
        {
          pub = *iter;
          ++iter;
          if (TAO_debug_level > 0)
            {
              ACE_DEBUG((LM_DEBUG,
                ACE_TEXT("DCPS_IR_Subscription::disassociate_topic () ")
                ACE_TEXT("Subscription %d testing if pub %d topic %d = %d\n"),
                id_, pub->get_id(), pub->get_topic_id(), id));
            }
          if (id == pub->get_topic_id() )
            {
              CORBA::Boolean dont_notify_lost = 0;
              pub->remove_associated_subscription (this, send, dont_notify_lost);
              remove_associated_publication (pub, dontSend, dont_notify_lost);

              idSeq[count] = pub->get_id();
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
                  reader_->remove_associations(idSeq, dont_notify_lost);
                }
              catch (const CORBA::Exception& ex)
                {
                  if (TAO_debug_level > 0)
                    {
                      ex._tao_print_exception (
                        "ERROR: Exception caught in DCPS_IR_Subscription::remove_associations:");
                    }
                  participant_->mark_dead();
                }
            }
        }
    }
}


void DCPS_IR_Subscription::disassociate_publication (OpenDDS::DCPS::RepoId id)
{
  DCPS_IR_Publication* pub = 0;
  size_t numAssociations = associations_.size();
  CORBA::Boolean dontSend = 0;
  CORBA::Boolean send = 1;
  long count = 0;

  if (0 < numAssociations)
    {
      OpenDDS::DCPS::WriterIdSeq idSeq(numAssociations);
      idSeq.length(numAssociations);

      DCPS_IR_Publication_Set::ITERATOR iter = associations_.begin();
      DCPS_IR_Publication_Set::ITERATOR end = associations_.end();

      while (iter != end)
        {
          pub = *iter;
          ++iter;
          if (TAO_debug_level > 0)
            {
              ACE_DEBUG((LM_DEBUG,
                ACE_TEXT("DCPS_IR_Subscription::disassociate_publication () ")
                ACE_TEXT("Subscription %d testing if pub %d = %d\n"),
                id_, pub->get_id(), id));
            }
          if (id == pub->get_id() )
            {
              CORBA::Boolean dont_notify_lost = 0;
              pub->remove_associated_subscription (this, send, dont_notify_lost);
              remove_associated_publication (pub, dontSend, dont_notify_lost);

              idSeq[count] = pub->get_id();
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
                  reader_->remove_associations(idSeq, dont_notify_lost);
                }
              catch (const CORBA::Exception& ex)
                {
                  if (TAO_debug_level > 0)
                    {
                      ex._tao_print_exception (
                        "ERROR: Exception caught in DCPS_IR_Subscription::remove_associations:");
                    }
                  participant_->mark_dead();
                }
            }
        }
    }
}


void DCPS_IR_Subscription::update_incompatible_qos ()
{
  reader_->update_incompatible_qos(incompatibleQosStatus_);
  incompatibleQosStatus_.count_since_last_send = 0;
}


CORBA::Boolean DCPS_IR_Subscription::is_publication_ignored (OpenDDS::DCPS::RepoId partId,
                                                             OpenDDS::DCPS::RepoId topicId,
                                                             OpenDDS::DCPS::RepoId pubId)
{
  CORBA::Boolean ignored;
  ignored = ( participant_->is_participant_ignored(partId) ||
              participant_->is_topic_ignored(topicId) ||
              participant_->is_publication_ignored(pubId) );

  return ignored;
}


OpenDDS::DCPS::TransportInterfaceId DCPS_IR_Subscription::get_transport_id () const
{
  return info_.transport_id;
}

OpenDDS::DCPS::TransportInterfaceInfo DCPS_IR_Subscription::get_transportInterfaceInfo () const
{
  OpenDDS::DCPS::TransportInterfaceInfo info = info_;
  return info;
}


OpenDDS::DCPS::IncompatibleQosStatus* DCPS_IR_Subscription::get_incompatibleQosStatus ()
{
  return &incompatibleQosStatus_;
}


const ::DDS::DataReaderQos* DCPS_IR_Subscription::get_datareader_qos ()
{
  return &qos_;
}

const ::DDS::SubscriberQos* DCPS_IR_Subscription::get_subscriber_qos ()
{
  return &subscriberQos_;
}


SpecificQos DCPS_IR_Subscription::set_qos (const ::DDS::DataReaderQos & qos,
                                   const ::DDS::SubscriberQos & subscriberQos)
{
  bool u_dr_qos = ! (qos_ == qos);
  bool u_sub_qos = ! (subscriberQos_ == subscriberQos);
  if (u_dr_qos)
    qos_ = qos;
  if (u_sub_qos)
    subscriberQos_ = subscriberQos;
  
  participant_->get_domain_reference()->publish_subscription_bit (this);
  return  u_dr_qos ? DataReaderQos : SubscriberQos;
}


OpenDDS::DCPS::RepoId DCPS_IR_Subscription::get_id ()
{
  return id_;
}


OpenDDS::DCPS::RepoId DCPS_IR_Subscription::get_topic_id ()
{
  return topic_->get_id();
}


OpenDDS::DCPS::RepoId DCPS_IR_Subscription::get_participant_id ()
{
  return participant_->get_id();
}


DCPS_IR_Topic_Description* DCPS_IR_Subscription::get_topic_description()
{
  return topic_->get_topic_description();
}


DCPS_IR_Topic* DCPS_IR_Subscription::get_topic ()
{
  return topic_;
}


::DDS::InstanceHandle_t DCPS_IR_Subscription::get_handle()
{
  return handle_;
}


void DCPS_IR_Subscription::set_handle(::DDS::InstanceHandle_t handle)
{
  handle_ = handle;
}


CORBA::Boolean DCPS_IR_Subscription::is_bit ()
{
  return isBIT_;
}


void DCPS_IR_Subscription::set_bit_status (CORBA::Boolean isBIT)
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
