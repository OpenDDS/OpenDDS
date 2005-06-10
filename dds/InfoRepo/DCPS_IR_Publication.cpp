#include "DcpsInfo_pch.h"
#include /**/ "DCPS_IR_Publication.h"

#include /**/ "DCPS_IR_Participant.h"
#include /**/ "DCPS_IR_Topic.h"
#include /**/ "DCPS_IR_Subscription.h"
#include /**/ "tao/debug.h"


DCPS_IR_Publication::DCPS_IR_Publication (TAO::DCPS::RepoId id,
                                          DCPS_IR_Participant* participant,
                                          DCPS_IR_Topic* topic,
                                          TAO::DCPS::DataWriterRemote_ptr writer,
                                          ::DDS::DataWriterQos qos,
                                          TAO::DCPS::TransportInterfaceInfo info,
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
  writer_ =  TAO::DCPS::DataWriterRemote::_duplicate( writer );

  incompatibleQosStatus_.total_count = 0;
  incompatibleQosStatus_.count_since_last_send = 0;
}


DCPS_IR_Publication::~DCPS_IR_Publication ()
{
  if (0 != associations_.size() )
    {
      remove_associations();
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
        TAO::DCPS::ReaderAssociationSeq associationSeq(2);
        associationSeq.length(1);
        associationSeq[0].readerTransInfo = sub->get_transportInterfaceInfo ();
        associationSeq[0].readerId = sub->get_id();
        associationSeq[0].subQos = *(sub->get_subscriber_qos());
        associationSeq[0].readerQos = *(sub->get_datareader_qos());

        if (participant_->is_alive())
          {
            ACE_TRY_NEW_ENV
              {
                if (TAO_debug_level > 0)
                 {
                    ACE_DEBUG((LM_DEBUG,
                      ACE_TEXT("(%P|%t) DCPS_IR_Publication::add_associated_subscription:")
                      ACE_TEXT(" calling pub %d with sub %d\n"),
                      id_, sub->get_id() ));
                 }
                writer_->add_associations(id_, associationSeq);
                ACE_TRY_CHECK;
              }
            ACE_CATCHANY
              {
                ACE_PRINT_EXCEPTION (ACE_ANY_EXCEPTION,
                  "ERROR: Exception caught in DCPS_IR_Publication::add_associated_publication:");
                participant_->mark_dead();
                status = -1;
              }
            ACE_ENDTRY;
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
      ACE_ERROR((LM_ERROR, ACE_TEXT("ERROR: DCPS_IR_Publication::add_associated_subscription ")
        ACE_TEXT("Attempted to add existing subscription %X\n"),
        sub));
      break;
    case -1:
      ACE_ERROR((LM_ERROR, ACE_TEXT("ERROR: DCPS_IR_Publication::add_associated_subscription ")
        ACE_TEXT("Unknown error while adding subscription %X\n"),
        sub));
    };


  return status;
}


int DCPS_IR_Publication::remove_associated_subscription (DCPS_IR_Subscription* sub,
                                                         CORBA::Boolean sendNotify)
{
  bool marked_dead = false;

  if (sendNotify)
    {
      TAO::DCPS::ReaderIdSeq idSeq(1);
      idSeq.length(1);
      idSeq[0]= sub->get_id();
      if (participant_->is_alive())
        {
          ACE_TRY_NEW_ENV
            {
              writer_->remove_associations(idSeq);
              ACE_TRY_CHECK;
            }
          ACE_CATCHANY
            {
              if (TAO_debug_level > 0)
                {
                  ACE_PRINT_EXCEPTION (ACE_ANY_EXCEPTION,
                    "ERROR: Exception caught in DCPS_IR_Publication::remove_associated_publication:");
                }
              participant_->mark_dead();
              marked_dead = true;
            }
          ACE_ENDTRY;
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
      ACE_ERROR((LM_ERROR, ACE_TEXT("ERROR: DCPS_IR_Publication::remove_associated_subscription ")
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


int DCPS_IR_Publication::remove_associations ()
{
  int status = 0;
  DCPS_IR_Subscription* sub = 0;
  size_t numAssociations = associations_.size();
  CORBA::Boolean dontSend = 0;
  CORBA::Boolean send = 1;
  long count = 0;

  if (0 < numAssociations)
    {
      TAO::DCPS::ReaderIdSeq idSeq(numAssociations);
      idSeq.length(numAssociations);

      DCPS_IR_Subscription_Set::ITERATOR iter = associations_.begin();
      DCPS_IR_Subscription_Set::ITERATOR end = associations_.end();

      while (iter != end)
        {
          sub = *iter;
          ++iter;

          sub->remove_associated_publication(this, send);
          remove_associated_subscription (sub, dontSend);

          idSeq[count] = sub->get_id();
          ++count;
        }

      if (participant_->is_alive())
        {
          ACE_TRY_NEW_ENV
            {
              writer_->remove_associations(idSeq);
              ACE_TRY_CHECK;
            }
          ACE_CATCHANY
            {
              if (TAO_debug_level > 0)
                {
                  ACE_PRINT_EXCEPTION (ACE_ANY_EXCEPTION,
                    "ERROR: Exception caught in DCPS_IR_Publication::remove_associations:");
                }
              participant_->mark_dead();
              status = -1;
            }
          ACE_ENDTRY;
        }
    }

  return status;
}

void DCPS_IR_Publication::disassociate_participant (TAO::DCPS::RepoId id)
{
  DCPS_IR_Subscription* sub = 0;
  size_t numAssociations = associations_.size();
  CORBA::Boolean send = 1;
  CORBA::Boolean dontSend = 0;
  long count = 0;

  if (0 < numAssociations)
    {
      TAO::DCPS::ReaderIdSeq idSeq(numAssociations);
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
              sub->remove_associated_publication(this, send);
              remove_associated_subscription (sub, dontSend);

              idSeq[count] = sub->get_id();
              ++count;
            }
        }

      if (0 < count)
        {
          idSeq.length(count);

          if (participant_->is_alive())
            {
              ACE_TRY_NEW_ENV
                {
                  writer_->remove_associations(idSeq);
                  ACE_TRY_CHECK;
                }
              ACE_CATCHANY
                {
                  if (TAO_debug_level > 0)
                    {
                      ACE_PRINT_EXCEPTION (ACE_ANY_EXCEPTION,
                        "ERROR: Exception caught in DCPS_IR_Publication::remove_associations:");
                    }
                  participant_->mark_dead();
                }
              ACE_ENDTRY;
            }
        }
    }
}


void DCPS_IR_Publication::disassociate_topic (TAO::DCPS::RepoId id)
{
  DCPS_IR_Subscription* sub = 0;
  size_t numAssociations = associations_.size();
  CORBA::Boolean send = 1;
  CORBA::Boolean dontSend = 0;
  long count = 0;

  if (0 < numAssociations)
    {
      TAO::DCPS::ReaderIdSeq idSeq(numAssociations);
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
              sub->remove_associated_publication(this, send);
              remove_associated_subscription (sub, dontSend);

              idSeq[count] = sub->get_id();
              ++count;
            }
        }

      if (0 < count)
        {
          idSeq.length(count);

          if (participant_->is_alive())
            {
              ACE_TRY_NEW_ENV
                {
                  writer_->remove_associations(idSeq);
                  ACE_TRY_CHECK;
                }
              ACE_CATCHANY
                {
                  if (TAO_debug_level > 0)
                    {
                      ACE_PRINT_EXCEPTION (ACE_ANY_EXCEPTION,
                        "ERROR: Exception caught in DCPS_IR_Publication::remove_associations:");
                    }
                  participant_->mark_dead();
                }
              ACE_ENDTRY;
            }
        }
    }
}


void DCPS_IR_Publication::disassociate_subscription (TAO::DCPS::RepoId id)
{
  DCPS_IR_Subscription* sub = 0;
  size_t numAssociations = associations_.size();
  CORBA::Boolean send = 1;
  CORBA::Boolean dontSend = 0;
  long count = 0;

  if (0 < numAssociations)
    {
      TAO::DCPS::ReaderIdSeq idSeq(numAssociations);
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
              sub->remove_associated_publication(this, send);
              remove_associated_subscription (sub, dontSend);

              idSeq[count] = sub->get_id();
              ++count;
            }
        }

      if (0 < count)
        {
          idSeq.length(count);

          if (participant_->is_alive())
            {
              ACE_TRY_NEW_ENV
                {
                  writer_->remove_associations(idSeq);
                  ACE_TRY_CHECK;
                }
              ACE_CATCHANY
                {
                  if (TAO_debug_level > 0)
                    {
                      ACE_PRINT_EXCEPTION (ACE_ANY_EXCEPTION,
                        "ERROR: Exception caught in DCPS_IR_Publication::remove_associations:");
                    }
                  participant_->mark_dead();
                }
              ACE_ENDTRY;
            }
        }
    }
}


void DCPS_IR_Publication::update_incompatible_qos ()
{
  writer_->update_incompatible_qos(incompatibleQosStatus_);
  incompatibleQosStatus_.count_since_last_send = 0;
}


CORBA::Boolean DCPS_IR_Publication::is_subscription_ignored (TAO::DCPS::RepoId partId,
                                                             TAO::DCPS::RepoId topicId,
                                                             TAO::DCPS::RepoId subId)
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


::DDS::PublisherQos* DCPS_IR_Publication::get_publisher_qos ()
{
  return &publisherQos_;
}


TAO::DCPS::TransportInterfaceId DCPS_IR_Publication::get_transport_id () const
{
  return info_.transport_id;
}

TAO::DCPS::TransportInterfaceInfo DCPS_IR_Publication::get_transportInterfaceInfo () const
{
  TAO::DCPS::TransportInterfaceInfo info = info_;
  return info;
}

TAO::DCPS::IncompatibleQosStatus* DCPS_IR_Publication::get_incompatibleQosStatus ()
{
  return &incompatibleQosStatus_;
}


TAO::DCPS::RepoId DCPS_IR_Publication::get_id ()
{
  return id_;
}

TAO::DCPS::RepoId DCPS_IR_Publication::get_topic_id ()
{
  return topic_->get_id();
}


TAO::DCPS::RepoId DCPS_IR_Publication::get_participant_id ()
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
