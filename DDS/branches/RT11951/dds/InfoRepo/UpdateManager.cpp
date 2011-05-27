#include "DcpsInfo_pch.h"

#include "UpdateManager.h"
#include "Updater.h"
#include "ArrDelAdapter.h"
#include "DCPSInfo_i.h"

#include "tao/CDR.h"

#include <vector>

UpdateManager::UpdateManager (void)
  : info_ (0)
{
}

UpdateManager::~UpdateManager (void)
{
}

void
UpdateManager::add (TAO_DDS_DCPSInfo_i* info)
{
  info_ = info;
}

void
UpdateManager::add (Updater* updater)
{
  // push new element to the back.
  updaters_.insert (updater);
}

void
UpdateManager::remove ()
{
  // Clean the refrence to the InfoRepo.
  info_ = 0;
}

void
UpdateManager::remove (const Updater* updater)
{
  // check if the Updaters is part of the list.
  Updaters::iterator iter = updaters_.find(const_cast<Updater*>(updater));
  if (iter == updaters_.end()) {
    return;
  }

  // remove the element
  updaters_.erase (iter);
}

int
UpdateManager::init (int , ACE_TCHAR *[])
{
  return 0;
}

int
UpdateManager::fini (void)
{
  return 0;
}

void
UpdateManager::requestImage (void)
{
  for (Updaters::iterator iter = updaters_.begin();
       iter != updaters_.end();
       iter++) {
    (*iter)->requestImage ();
  }
}

template <typename T>
class SeqGuard
{
public:
  typedef std::vector<T*> Seq;

public:
  ~SeqGuard (void)
  {
    for (typename Seq::iterator iter = seq_.begin ();
         iter != seq_.end(); )
      {
        typename Seq::iterator current = iter; iter++;

        delete (*current);
      }
  };

  Seq& seq (void)
  {
    return seq_;
  };

private:
  Seq seq_;
};


void
UpdateManager::pushImage (const DImage& image)
{
  if (info_ == NULL) {
    return;
  }

  // image to be propagated.
  UImage u_image;

  /***************************
  // The downstream image needs to be converted to a
  // format compatible with the upstream layers (Dimage -> UImage)
  // The Uimage cotains a lot of refrences to the complex data
  // types. These are collecetd in several buckets (see below) and
  // passed by reference. Usage of a custom guard class 'SeqGuard'
  // automates memory cleanup.
  ***************************/

  // Topic buckets
  SeqGuard< ::DDS::TopicQos> topics_qos_guard;
  SeqGuard< ::DDS::TopicQos>::Seq& topics_qos = topics_qos_guard.seq ();

  SeqGuard<UTopic> topics_guard;
  SeqGuard<UTopic>::Seq& topics = topics_guard.seq ();

  for (DImage::TopicSeq::const_iterator iter = image.topics.begin();
       iter != image.topics.end(); iter++)
    {
      const DTopic& topic = *iter;

      TAO_InputCDR in_cdr (topic.topicQos.second.second
                          , topic.topicQos.second.first);

      ::DDS::TopicQos* qos;
      ACE_NEW_NORETURN (qos, ::DDS::TopicQos);
      in_cdr >> *qos;
      topics_qos.push_back (qos);

      UTopic* u_topic;
      ACE_NEW_NORETURN (u_topic
                        , UTopic (topic.domainId, topic.topicId
                                  , topic.participantId
                                  , topic.name.c_str()
                                  , topic.dataType.c_str(), *qos));
      topics.push_back (u_topic);

      // Push newly created UTopic into UImage Topic bucket
      u_image.topics.push_back (u_topic);
    }


  // Participant buckets
  SeqGuard< ::DDS::DomainParticipantQos> part_qos_guard;
  SeqGuard< ::DDS::DomainParticipantQos>::Seq& part_qos = part_qos_guard.seq ();

  SeqGuard<UParticipant> part_guard;
  SeqGuard< UParticipant>::Seq& parts = part_guard.seq ();

  for (DImage::ParticipantSeq::const_iterator iter = image.participants.begin();
       iter != image.participants.end(); iter++)
    {
      const DParticipant& part = *iter;

      TAO_InputCDR in_cdr (part.participantQos.second.second
                          , part.participantQos.second.first);

      ::DDS::DomainParticipantQos* qos;
      ACE_NEW_NORETURN (qos, ::DDS::DomainParticipantQos);
      in_cdr >> *qos;
      part_qos.push_back (qos);

      UParticipant* u_part;
      ACE_NEW_NORETURN (u_part, UParticipant (part.domainId
                                              , part.participantId
                                              , *qos));
      parts.push_back (u_part);

      // push newly created UParticipant into UImage Participant bucket
      u_image.participants.push_back (u_part);
    }

  // Actor buckets
  SeqGuard< ::DDS::PublisherQos> pub_qos_guard;
  SeqGuard< ::DDS::PublisherQos>::Seq& pub_qos_seq = pub_qos_guard.seq ();
  SeqGuard< ::DDS::DataWriterQos> dw_qos_guard;
  SeqGuard< ::DDS::DataWriterQos>::Seq& dw_qos_seq = dw_qos_guard.seq ();

  SeqGuard< ::DDS::SubscriberQos> sub_qos_guard;
  SeqGuard< ::DDS::SubscriberQos>::Seq& sub_qos_seq = sub_qos_guard.seq ();
  SeqGuard< ::DDS::DataReaderQos> dr_qos_guard;
  SeqGuard< ::DDS::DataReaderQos>::Seq& dr_qos_seq = dr_qos_guard.seq ();

  SeqGuard<OpenDDS::DCPS::TransportInterfaceInfo> trans_guard;
  SeqGuard<OpenDDS::DCPS::TransportInterfaceInfo>::Seq& transports = trans_guard.seq ();

  SeqGuard<URActor> reader_guard;
  SeqGuard<URActor>::Seq& readers = reader_guard.seq ();
  SeqGuard<UWActor> writer_guard;
  SeqGuard<UWActor>::Seq& writers = writer_guard.seq ();

  for (DImage::ReaderSeq::const_iterator iter = image.actors.begin();
       iter != image.actors.end(); iter++)
    {
      const DActor& actor = *iter;

      TAO_InputCDR in_cdr (actor.transportInterfaceInfo.second
                          , actor.transportInterfaceInfo.first);
      OpenDDS::DCPS::TransportInterfaceInfo* trans;
      ACE_NEW_NORETURN (trans, OpenDDS::DCPS::TransportInterfaceInfo);
      transports.push_back (trans);
      in_cdr >> *trans;

      ::DDS::PublisherQos* pub_qos = 0;
      ::DDS::DataWriterQos* writer_qos = 0;
      ::DDS::SubscriberQos* sub_qos = 0;
      ::DDS::DataReaderQos* reader_qos = 0;

      if (actor.type == DataReader)
        {
          TAO_InputCDR sub_cdr (actor.pubsubQos.second.second
                               , actor.pubsubQos.second.first);
          ACE_NEW_NORETURN (sub_qos, ::DDS::SubscriberQos);
          sub_qos_seq.push_back (sub_qos);
          sub_cdr >> *sub_qos;

          TAO_InputCDR read_cdr (actor.drdwQos.second.second
                                 , actor.drdwQos.second.first);
          ACE_NEW_NORETURN (reader_qos, ::DDS::DataReaderQos);
          dr_qos_seq.push_back (reader_qos);
          read_cdr >> *reader_qos;

          URActor* reader;
          ACE_NEW_NORETURN (reader
                            , URActor (actor.domainId, actor.actorId
                                       , actor.topicId, actor.participantId
                                       , actor.type, actor.callback.c_str()
                                       , *sub_qos, *reader_qos
                                       , *trans));
          readers.push_back (reader);
          u_image.actors.push_back (reader);
        }
      else if (actor.type == DataWriter)
        {
          TAO_InputCDR pub_cdr (actor.pubsubQos.second.second
                                , actor.pubsubQos.second.first);
          ACE_NEW_NORETURN (pub_qos, ::DDS::PublisherQos);
          pub_qos_seq.push_back (pub_qos);
          pub_cdr >> *pub_qos;

          TAO_InputCDR write_cdr (actor.drdwQos.second.second
                                  , actor.drdwQos.second.first);
          ACE_NEW_NORETURN (writer_qos, ::DDS::DataWriterQos);
          dw_qos_seq.push_back (writer_qos);
          write_cdr >> *writer_qos;

          UWActor* writer;
          ACE_NEW_NORETURN (writer
                            , UWActor (actor.domainId, actor.actorId
                                       , actor.topicId, actor.participantId
                                       , actor.type, actor.callback.c_str()
                                       , *pub_qos, *writer_qos
                                       , *trans));
          writers.push_back (writer);
          u_image.wActors.push_back (writer);
        }
      else {
        ACE_ERROR ((LM_ERROR, "UpdateManager::pushImage> unknown "
                    "actor type.\n"));
      }
    }

  info_->receive_image (u_image);
}

void
UpdateManager::add (const UTopic& topic)
{
  if (updaters_.empty()) {
    return;
  }

  // serialize the Topic QOS
  TAO_OutputCDR outCdr;
  outCdr << topic.topicQos;

  size_t len = outCdr.total_length();
  char *buf;
  ACE_NEW_NORETURN (buf, char[len]);
  ArrDelAdapter<char> guard (buf);
  if (buf == 0) {
    ACE_ERROR ((LM_ERROR, "UpdateManager::add> Allocation failed.\n"));
    return;
  }

  ACE_Message_Block dst;
  ACE_CDR::consolidate (&dst, outCdr.begin ());
  ACE_OS::memcpy (buf, dst.base(), len);

  BinSeq qos_bin (len, buf);

  QosSeq p (TopicQos, qos_bin);
  DTopic topic_data (topic.domainId, topic.topicId, topic.participantId
                     , topic.name.c_str(), topic.dataType.c_str(), p);

  // Invoke add on each of the iterators.
  for (Updaters::iterator iter = updaters_.begin();
       iter != updaters_.end();
       iter++) {
    (*iter)->add (topic_data);
  }
}

void
UpdateManager::add(const UParticipant& participant)
{
  if (updaters_.empty()) {
    return;
  }

  // serialize the Topic QOS
  TAO_OutputCDR outCdr;
  outCdr << participant.participantQos;

  size_t len = outCdr.total_length();
  char *buf;
  ACE_NEW_NORETURN (buf, char[len]);
  ArrDelAdapter<char> guard (buf);
  if (buf == 0) {
    ACE_ERROR ((LM_ERROR, "UpdateManager::add2> Allocation failed.\n"));
    return;
  }

  ACE_Message_Block dst;
  ACE_CDR::consolidate (&dst, outCdr.begin ());
  ACE_OS::memcpy (buf, dst.base(), len);

  BinSeq qos_bin (len, buf);

  QosSeq p (ParticipantQos, qos_bin);
  DParticipant paticipant_data
    (participant.domainId, participant.participantId, p);

  // Invoke add on each of the iterators.
  for (Updaters::iterator iter = updaters_.begin();
       iter != updaters_.end();
       iter++) {
    (*iter)->add (paticipant_data);
  }
}

void
UpdateManager::remove (ItemType type, const IdType& id)
{
  // Invoke remove on each of the iterators.
  for (Updaters::iterator iter = updaters_.begin();
       iter != updaters_.end();
       iter++) {
    (*iter)->remove (type, id);
  }
}

void
UpdateManager::updateQos(const ItemType& itemType, const IdType& id
			 , const QosSeq& qos)
{
  // Invoke updateQos on each of the iterators.
  for (Updaters::iterator iter = updaters_.begin();
       iter != updaters_.end();
       iter++) {
    (*iter)->updateQos (itemType, id, qos);
  }
}


void
UpdateManager::add (const DTopic& topic)
{
  if (info_ == NULL) {
    return;
  }

  // Demarshal QOS data
  TAO_InputCDR in_cdr (topic.topicQos.second.second
                      , topic.topicQos.second.first);

  ::DDS::TopicQos qos;
  in_cdr >> qos;

  // Pass topic info to infoRepo.
  info_->add_topic (topic.topicId, topic.domainId
                    , topic.participantId, topic.name.c_str()
                    , topic.dataType.c_str(), qos);
}

void
UpdateManager::add (const DParticipant& participant)
{
  if (info_ == NULL) {
    return;
  }

  // Demarshal QOS data
  TAO_InputCDR in_cdr (participant.participantQos.second.second
                      , participant.participantQos.second.first);

  ::DDS::DomainParticipantQos qos;
  in_cdr >> qos;

  // Pass participant info to infoRepo.
  info_->add_domain_participant (participant.domainId
                                 , participant.participantId
                                 , qos);
}

void
UpdateManager::add (const DActor& actor)
{
  if (info_ == NULL) {
    return;
  }

  // Demarshal QOS data
  TAO_InputCDR pubSubCdr (actor.pubsubQos.second.second
                          , actor.pubsubQos.second.first);

  TAO_InputCDR drdwCdr (actor.drdwQos.second.second
                        , actor.drdwQos.second.first);

  std::string callback (actor.callback.c_str());

  TAO_InputCDR transportCdr (actor.transportInterfaceInfo.second
                             , actor.transportInterfaceInfo.first);

  OpenDDS::DCPS::TransportInterfaceInfo transport_info;
  transportCdr >> transport_info;

  if (actor.type == DataReader)
    {
      ::DDS::SubscriberQos sub_qos;
      ::DDS::DataReaderQos reader_qos;

      pubSubCdr >> sub_qos;
      drdwCdr >> reader_qos;

      // Pass actor to InfoRepo.
      info_->add_subscription (actor.domainId, actor.participantId
                               , actor.topicId, actor.actorId
                               , callback.c_str(), reader_qos
                               , transport_info, sub_qos);
    }
  else if (actor.type == DataWriter)
    {
      ::DDS::PublisherQos pub_qos;
      ::DDS::DataWriterQos writer_qos;

      pubSubCdr >> pub_qos;
      drdwCdr >> writer_qos;

      // Pass actor info to infoRepo.
      info_->add_publication (actor.domainId, actor.participantId
                              , actor.topicId, actor.actorId
                              , callback.c_str(), writer_qos
                              , transport_info, pub_qos);
    }
}

void
UpdateManager::add (Updater* updater, const DActor& actor)
{
  updater->add (actor);
}

int
UpdateManager_Loader::init (void)
{
  return ACE_Service_Config::process_directive
    (ace_svc_desc_UpdateManager);
  return 0;
}

ACE_FACTORY_DEFINE (ACE_Local_Service, UpdateManager)

ACE_STATIC_SVC_DEFINE (UpdateManager,
                         ACE_TEXT ("UpdateManager"),
                         ACE_SVC_OBJ_T,
                         &ACE_SVC_NAME (UpdateManager),
                         ACE_Service_Type::DELETE_THIS
                         | ACE_Service_Type::DELETE_OBJ,
                         0)

  //ACE_STATIC_SVC_REQUIRE (UpdateManager)
