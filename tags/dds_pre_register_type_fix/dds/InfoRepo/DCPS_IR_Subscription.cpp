#include "DcpsInfo_pch.h"

#include /**/ "DCPS_IR_Subscription.h"

#include /**/ "DCPS_IR_Publication.h"
#include /**/ "DCPS_IR_Participant.h"
#include /**/ "DCPS_IR_Topic_Description.h"
#include /**/ "tao/debug.h"


DCPS_IR_Subscription::DCPS_IR_Subscription (TAO::DCPS::RepoId id,
                                            DCPS_IR_Participant* participant,
                                            DCPS_IR_Topic* topic,
                                            TAO::DCPS::DataReaderRemote_ptr reader,
                                            ::DDS::DataReaderQos qos,
                                            TAO::DCPS::TransportInterfaceInfo info,
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
  reader_ =  TAO::DCPS::DataReaderRemote::_duplicate(reader);

  incompatibleQosStatus_.total_count = 0;
  incompatibleQosStatus_.count_since_last_send = 0;
}


DCPS_IR_Subscription::~DCPS_IR_Subscription ()
{
  if (0 != associations_.size() )
    {
      remove_associations();
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
        TAO::DCPS::WriterAssociationSeq associationSeq(2);
        associationSeq.length(1);
        associationSeq[0].writerTransInfo = pub->get_transportInterfaceInfo ();
        associationSeq[0].writerId = pub->get_id();
        associationSeq[0].pubQos = *(pub->get_publisher_qos());
        associationSeq[0].writerQos = *(pub->get_datawriter_qos());

        if (participant_->is_alive())
          {
            ACE_TRY_NEW_ENV
              {
                if (TAO_debug_level > 0)
                 {
                    ACE_DEBUG((LM_DEBUG,
                      ACE_TEXT("(%P|%t) DCPS_IR_Subscription::add_associated_publication:")
                      ACE_TEXT(" calling sub %d with pub %d\n"),
                      id_, pub->get_id() ));
                 }
                reader_->add_associations(id_, associationSeq);
                ACE_TRY_CHECK;
              }
            ACE_CATCHANY
              {
                ACE_PRINT_EXCEPTION (ACE_ANY_EXCEPTION,
                  "ERROR: Exception caught in DCPS_IR_Subscription::add_associated_publication:");
                participant_->mark_dead();
                status = -1;
              }
            ACE_ENDTRY;
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
      ACE_ERROR((LM_ERROR, ACE_TEXT("ERROR: DCPS_IR_Subscription::add_associated_publication ")
        ACE_TEXT("Attempted to add existing publication %X\n"),
        pub));
      break;
    case -1:
      ACE_ERROR((LM_ERROR, ACE_TEXT("ERROR: DCPS_IR_Subscription::add_associated_publication ")
        ACE_TEXT("Unknown error while adding publication %X\n"),
        pub));
    };


  return status;
}


int DCPS_IR_Subscription::remove_associated_publication (DCPS_IR_Publication* pub,
                                                         CORBA::Boolean sendNotify)
{
  bool marked_dead = false;

  if (sendNotify)
    {
      TAO::DCPS::WriterIdSeq idSeq(5);
      idSeq.length(1);
      idSeq[0] = pub->get_id();
      if (participant_->is_alive())
        {
          ACE_TRY_NEW_ENV
            {
              reader_->remove_associations(idSeq);
              ACE_TRY_CHECK;
            }
          ACE_CATCHANY
            {
              if (TAO_debug_level > 0)
                {
                  ACE_PRINT_EXCEPTION (ACE_ANY_EXCEPTION,
                    "ERROR: Exception caught in DCPS_IR_Subscription::remove_associated_publication:");
                }
              participant_->mark_dead();
              marked_dead = true;
            }
          ACE_ENDTRY;
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
      ACE_ERROR((LM_ERROR, ACE_TEXT("ERROR: DCPS_IR_Subscription::remove_associated_publication ")
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


int DCPS_IR_Subscription::remove_associations ()
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

          pub->remove_associated_subscription (this, send);
          remove_associated_publication (pub, dontSend);
        }
    }

  return status;
}


void DCPS_IR_Subscription::disassociate_participant (TAO::DCPS::RepoId id)
{
  DCPS_IR_Publication* pub = 0;
  size_t numAssociations = associations_.size();
  CORBA::Boolean dontSend = 0;
  CORBA::Boolean send = 1;
  long count = 0;

  if (0 < numAssociations)
    {
      TAO::DCPS::WriterIdSeq idSeq(numAssociations);
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
              pub->remove_associated_subscription (this, send);
              remove_associated_publication (pub, dontSend);

              idSeq[count] = pub->get_id();
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
                  reader_->remove_associations(idSeq);
                  ACE_TRY_CHECK;
                }
              ACE_CATCHANY
                {
                  if (TAO_debug_level > 0)
                    {
                      ACE_PRINT_EXCEPTION (ACE_ANY_EXCEPTION,
                        "ERROR: Exception caught in DCPS_IR_Subscription::remove_associations:");
                    }
                  participant_->mark_dead();
                }
              ACE_ENDTRY;
            }
        }
    }
}


void DCPS_IR_Subscription::disassociate_topic (TAO::DCPS::RepoId id)
{
  DCPS_IR_Publication* pub = 0;
  size_t numAssociations = associations_.size();
  CORBA::Boolean dontSend = 0;
  CORBA::Boolean send = 1;
  long count = 0;

  if (0 < numAssociations)
    {
      TAO::DCPS::WriterIdSeq idSeq(numAssociations);
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
              pub->remove_associated_subscription (this, send);
              remove_associated_publication (pub, dontSend);

              idSeq[count] = pub->get_id();
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
                  reader_->remove_associations(idSeq);
                  ACE_TRY_CHECK;
                }
              ACE_CATCHANY
                {
                  if (TAO_debug_level > 0)
                    {
                      ACE_PRINT_EXCEPTION (ACE_ANY_EXCEPTION,
                        "ERROR: Exception caught in DCPS_IR_Subscription::remove_associations:");
                    }
                  participant_->mark_dead();
                }
              ACE_ENDTRY;
            }
        }
    }
}


void DCPS_IR_Subscription::disassociate_publication (TAO::DCPS::RepoId id)
{
  DCPS_IR_Publication* pub = 0;
  size_t numAssociations = associations_.size();
  CORBA::Boolean dontSend = 0;
  CORBA::Boolean send = 1;
  long count = 0;

  if (0 < numAssociations)
    {
      TAO::DCPS::WriterIdSeq idSeq(numAssociations);
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
              pub->remove_associated_subscription (this, send);
              remove_associated_publication (pub, dontSend);

              idSeq[count] = pub->get_id();
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
                  reader_->remove_associations(idSeq);
                  ACE_TRY_CHECK;
                }
              ACE_CATCHANY
                {
                  if (TAO_debug_level > 0)
                    {
                      ACE_PRINT_EXCEPTION (ACE_ANY_EXCEPTION,
                        "ERROR: Exception caught in DCPS_IR_Subscription::remove_associations:");
                    }
                  participant_->mark_dead();
                }
              ACE_ENDTRY;
            }
        }
    }
}


void DCPS_IR_Subscription::update_incompatible_qos ()
{
  reader_->update_incompatible_qos(incompatibleQosStatus_);
  incompatibleQosStatus_.count_since_last_send = 0;
}


CORBA::Boolean DCPS_IR_Subscription::is_publication_ignored (TAO::DCPS::RepoId partId,
                                                             TAO::DCPS::RepoId topicId,
                                                             TAO::DCPS::RepoId pubId)
{
  CORBA::Boolean ignored;
  ignored = ( participant_->is_participant_ignored(partId) ||
              participant_->is_topic_ignored(topicId) ||
              participant_->is_publication_ignored(pubId) );

  return ignored;
}


TAO::DCPS::TransportInterfaceId DCPS_IR_Subscription::get_transport_id () const
{
  return info_.transport_id;
}

TAO::DCPS::TransportInterfaceInfo DCPS_IR_Subscription::get_transportInterfaceInfo () const
{
  TAO::DCPS::TransportInterfaceInfo info = info_;
  return info;
}


TAO::DCPS::IncompatibleQosStatus* DCPS_IR_Subscription::get_incompatibleQosStatus ()
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


TAO::DCPS::RepoId DCPS_IR_Subscription::get_id ()
{
  return id_;
}


TAO::DCPS::RepoId DCPS_IR_Subscription::get_topic_id ()
{
  return topic_->get_id();
}


TAO::DCPS::RepoId DCPS_IR_Subscription::get_participant_id ()
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
