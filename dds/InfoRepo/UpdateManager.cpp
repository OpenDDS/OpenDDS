/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DcpsInfo_pch.h"

#include "UpdateManager.h"
#include "Updater.h"
#include "ArrDelAdapter.h"
#include "DCPSInfo_i.h"

#include "tao/CDR.h"

#include <vector>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace Update {

Manager::Manager()
  : info_(0)
{
}

Manager::~Manager()
{
}

void
Manager::add(TAO_DDS_DCPSInfo_i* info)
{
  info_ = info;
}

void
Manager::add(Updater* updater)
{
  // push new element to the back.
  updaters_.insert(updater);
}

void
Manager::remove()
{
  // Clean the refrence to the InfoRepo.
  info_ = 0;
}

void
Manager::remove(const Updater* updater)
{
  // check if the Updaters is part of the list.
  Updaters::iterator iter = updaters_.find(const_cast<Updater*>(updater));

  if (iter == updaters_.end()) {
    return;
  }

  // remove the element
  updaters_.erase(iter);
}

int
Manager::init(int , ACE_TCHAR *[])
{
  return 0;
}

int
Manager::fini()
{
  return 0;
}

void
Manager::requestImage()
{
  for (Updaters::iterator iter = updaters_.begin();
       iter != updaters_.end();
       iter++) {
    (*iter)->requestImage();
  }
}

template <typename T>
class SeqGuard {
public:
  typedef std::vector<T*> Seq;

public:
  ~SeqGuard() {
    for (typename Seq::iterator iter = seq_.begin();
         iter != seq_.end();) {
      typename Seq::iterator current = iter;
      iter++;

      delete(*current);
    }
  };

  Seq& seq() {
    return seq_;
  };

private:
  Seq seq_;
};

void
Manager::pushImage(const DImage& image)
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

  // Participant buckets
  SeqGuard<DDS::DomainParticipantQos> part_qos_guard;
  SeqGuard<DDS::DomainParticipantQos>::Seq& part_qos = part_qos_guard.seq();

  SeqGuard<UParticipant> part_guard;
  SeqGuard<UParticipant>::Seq& parts = part_guard.seq();

  for (DImage::ParticipantSeq::const_iterator iter = image.participants.begin();
       iter != image.participants.end(); iter++) {
    const DParticipant& part = *iter;

    TAO_InputCDR in_cdr(part.participantQos.second.second
                        , part.participantQos.second.first);

    DDS::DomainParticipantQos* qos;
    ACE_NEW_NORETURN(qos, DDS::DomainParticipantQos);
    in_cdr >> *qos;
    part_qos.push_back(qos);

    UParticipant* u_part;
    ACE_NEW_NORETURN(u_part, UParticipant(part.domainId
                                          , part.owner
                                          , part.participantId
                                          , *qos));
    parts.push_back(u_part);

    // push newly created UParticipant into UImage Participant bucket
    u_image.participants.push_back(u_part);
  }

  // Topic buckets
  SeqGuard<DDS::TopicQos> topics_qos_guard;
  SeqGuard<DDS::TopicQos>::Seq& topics_qos = topics_qos_guard.seq();

  SeqGuard<UTopic> topics_guard;
  SeqGuard<UTopic>::Seq& topics = topics_guard.seq();

  for (DImage::TopicSeq::const_iterator iter = image.topics.begin();
       iter != image.topics.end(); iter++) {
    const DTopic& topic = *iter;

    TAO_InputCDR in_cdr(topic.topicQos.second.second
                        , topic.topicQos.second.first);

    DDS::TopicQos* qos;
    ACE_NEW_NORETURN(qos, DDS::TopicQos);
    in_cdr >> *qos;
    topics_qos.push_back(qos);

    UTopic* u_topic;
    ACE_NEW_NORETURN(u_topic
                     , UTopic(topic.domainId, topic.topicId
                              , topic.participantId
                              , topic.name.c_str()
                              , topic.dataType.c_str(), *qos));
    topics.push_back(u_topic);

    // Push newly created UTopic into UImage Topic bucket
    u_image.topics.push_back(u_topic);
  }

  // Actor buckets
  SeqGuard<DDS::PublisherQos> pub_qos_guard;
  SeqGuard<DDS::PublisherQos>::Seq& pub_qos_seq = pub_qos_guard.seq();
  SeqGuard<DDS::DataWriterQos> dw_qos_guard;
  SeqGuard<DDS::DataWriterQos>::Seq& dw_qos_seq = dw_qos_guard.seq();

  SeqGuard<DDS::SubscriberQos> sub_qos_guard;
  SeqGuard<DDS::SubscriberQos>::Seq& sub_qos_seq = sub_qos_guard.seq();
  SeqGuard<DDS::DataReaderQos> dr_qos_guard;
  SeqGuard<DDS::DataReaderQos>::Seq& dr_qos_seq = dr_qos_guard.seq();

  SeqGuard<OpenDDS::DCPS::TransportLocatorSeq> trans_guard;
  SeqGuard<OpenDDS::DCPS::TransportLocatorSeq>::Seq& transports = trans_guard.seq();

  SeqGuard<URActor> reader_guard;
  SeqGuard<URActor>::Seq& readers = reader_guard.seq();
  SeqGuard<UWActor> writer_guard;
  SeqGuard<UWActor>::Seq& writers = writer_guard.seq();

  for (DImage::ReaderSeq::const_iterator iter = image.actors.begin();
       iter != image.actors.end(); iter++) {
    const DActor& actor = *iter;

    TAO_InputCDR in_cdr(actor.transportInterfaceInfo.second
                        , actor.transportInterfaceInfo.first);
    OpenDDS::DCPS::TransportLocatorSeq* trans;
    ACE_NEW_NORETURN(trans, OpenDDS::DCPS::TransportLocatorSeq);
    transports.push_back(trans);
    in_cdr >> *trans;

    DDS::PublisherQos* pub_qos = 0;
    DDS::DataWriterQos* writer_qos = 0;
    DDS::SubscriberQos* sub_qos = 0;
    DDS::DataReaderQos* reader_qos = 0;

    if (actor.type == DataReader) {
      TAO_InputCDR sub_cdr(actor.pubsubQos.second.second
                           , actor.pubsubQos.second.first);
      ACE_NEW_NORETURN(sub_qos, DDS::SubscriberQos);
      sub_qos_seq.push_back(sub_qos);
      sub_cdr >> *sub_qos;

      TAO_InputCDR read_cdr(actor.drdwQos.second.second
                            , actor.drdwQos.second.first);
      ACE_NEW_NORETURN(reader_qos, DDS::DataReaderQos);
      dr_qos_seq.push_back(reader_qos);
      read_cdr >> *reader_qos;

      ContentSubscriptionInfo* csi = 0;
      ACE_NEW_NORETURN(csi, ContentSubscriptionInfo);
      csi->filterClassName = actor.contentSubscriptionProfile.filterClassName.c_str();
      csi->filterExpr = actor.contentSubscriptionProfile.filterExpr.c_str();
      TAO_InputCDR csp_cdr(actor.contentSubscriptionProfile.exprParams.second,
                           actor.contentSubscriptionProfile.exprParams.first);
      csp_cdr >> csi->exprParams;

      URActor* reader;
      ACE_NEW_NORETURN(reader
                       , URActor(actor.domainId, actor.actorId
                                 , actor.topicId, actor.participantId
                                 , actor.type, actor.callback.c_str()
                                 , *sub_qos, *reader_qos
                                 , *trans, *csi));
      readers.push_back(reader);
      u_image.actors.push_back(reader);

    } else if (actor.type == DataWriter) {
      TAO_InputCDR pub_cdr(actor.pubsubQos.second.second
                           , actor.pubsubQos.second.first);
      ACE_NEW_NORETURN(pub_qos, DDS::PublisherQos);
      pub_qos_seq.push_back(pub_qos);
      pub_cdr >> *pub_qos;

      TAO_InputCDR write_cdr(actor.drdwQos.second.second
                             , actor.drdwQos.second.first);
      ACE_NEW_NORETURN(writer_qos, DDS::DataWriterQos);
      dw_qos_seq.push_back(writer_qos);
      write_cdr >> *writer_qos;

      ContentSubscriptionInfo csi; //writers have no info

      UWActor* writer;
      ACE_NEW_NORETURN(writer
                       , UWActor(actor.domainId, actor.actorId
                                 , actor.topicId, actor.participantId
                                 , actor.type, actor.callback.c_str()
                                 , *pub_qos, *writer_qos
                                 , *trans, csi));
      writers.push_back(writer);
      u_image.wActors.push_back(writer);

    } else {
      ACE_ERROR((LM_ERROR, "Update::Manager::pushImage> unknown "
                 "actor type.\n"));
    }
  }

  info_->receive_image(u_image);
}

void
Manager::destroy(const IdPath& id, ItemType type, ActorType actor)
{
  // Invoke remove on each of the iterators.
  for (Updaters::iterator iter = updaters_.begin();
       iter != updaters_.end();
       iter++) {
    (*iter)->destroy(id, type, actor);
  }
}

void
Manager::add(const DTopic& topic)
{
  if (info_ == NULL) {
    return;
  }

  // Demarshal QOS data
  TAO_InputCDR in_cdr(topic.topicQos.second.second
                      , topic.topicQos.second.first);

  DDS::TopicQos qos;
  in_cdr >> qos;

  // Pass topic info to infoRepo.
  info_->add_topic(topic.topicId, topic.domainId
                   , topic.participantId, topic.name.c_str()
                   , topic.dataType.c_str(), qos);
}

void
Manager::add(const DParticipant& participant)
{
  if (info_ == NULL) {
    return;
  }

  // Demarshal QOS data
  TAO_InputCDR in_cdr(participant.participantQos.second.second
                      , participant.participantQos.second.first);

  DDS::DomainParticipantQos qos;
  in_cdr >> qos;

  // Pass participant info to infoRepo.
  info_->add_domain_participant(participant.domainId
                                , participant.participantId
                                , qos);
}

void
Manager::add(const DActor& actor)
{
  if (info_ == NULL) {
    return;
  }

  // Demarshal QOS data
  TAO_InputCDR pubSubCdr(actor.pubsubQos.second.second
                         , actor.pubsubQos.second.first);

  TAO_InputCDR drdwCdr(actor.drdwQos.second.second
                       , actor.drdwQos.second.first);

  std::string callback(actor.callback.c_str());

  TAO_InputCDR transportCdr(actor.transportInterfaceInfo.second
                            , actor.transportInterfaceInfo.first);

  OpenDDS::DCPS::TransportLocatorSeq transport_info;
  transportCdr >> transport_info;

  if (actor.type == DataReader) {
    DDS::SubscriberQos sub_qos;
    DDS::DataReaderQos reader_qos;

    pubSubCdr >> sub_qos;
    drdwCdr >> reader_qos;

    Update::ContentSubscriptionInfo csi;
    csi.filterClassName = actor.contentSubscriptionProfile.filterClassName.c_str();
    csi.filterExpr = actor.contentSubscriptionProfile.filterExpr.c_str();
    TAO_InputCDR cspCdr(actor.contentSubscriptionProfile.exprParams.second,
                        actor.contentSubscriptionProfile.exprParams.first);
    cspCdr >> csi.exprParams;

    // Pass actor to InfoRepo.
    info_->add_subscription(actor.domainId, actor.participantId
                            , actor.topicId, actor.actorId
                            , callback.c_str(), reader_qos
                            , transport_info, sub_qos
                            , csi.filterClassName, csi.filterExpr, csi.exprParams);

  } else if (actor.type == DataWriter) {
    DDS::PublisherQos pub_qos;
    DDS::DataWriterQos writer_qos;

    pubSubCdr >> pub_qos;
    drdwCdr >> writer_qos;

    // Pass actor info to infoRepo.
    info_->add_publication(actor.domainId, actor.participantId
                           , actor.topicId, actor.actorId
                           , callback.c_str(), writer_qos
                           , transport_info, pub_qos);
  }
}

} // namespace Update

int
UpdateManagerSvc_Loader::init()
{
  return ACE_Service_Config::process_directive
         (ace_svc_desc_UpdateManagerSvc);
  return 0;
}

ACE_FACTORY_DEFINE(ACE_Local_Service, UpdateManagerSvc)

ACE_STATIC_SVC_DEFINE(UpdateManagerSvc,
                      ACE_TEXT("UpdateManagerSvc"),
                      ACE_SVC_OBJ_T,
                      &ACE_SVC_NAME(UpdateManagerSvc),
                      ACE_Service_Type::DELETE_THIS
                      | ACE_Service_Type::DELETE_OBJ,
                      0)

OPENDDS_END_VERSIONED_NAMESPACE_DECL
