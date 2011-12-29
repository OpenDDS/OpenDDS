/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_RTPS_SEDP_H
#define OPENDDS_RTPS_SEDP_H

#include "dds/DdsDcpsInfrastructureC.h"
#include "dds/DdsDcpsInfoUtilsC.h"
#include "dds/DdsDcpsInfoC.h"

#include "dds/DCPS/RTPS/RtpsMessageTypesTypeSupportImpl.h"
#include "dds/DCPS/RTPS/BaseMessageTypes.h"
#include "dds/DCPS/RTPS/BaseMessageUtils.h"

#include "dds/DCPS/RcHandle_T.h"
#include "dds/DCPS/GuidUtils.h"
#include "dds/DCPS/Definitions.h"

#include "dds/DCPS/transport/framework/TransportRegistry.h"
#include "dds/DCPS/transport/framework/TransportSendListener.h"
#include "dds/DCPS/transport/framework/TransportClient.h"
#include "dds/DCPS/transport/framework/TransportInst_rch.h"

#include <map>
#include <set>
#include <string>


#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

namespace DDS {
  class TopicBuiltinTopicDataDataReaderImpl;
  class PublicationBuiltinTopicDataDataReaderImpl;
  class SubscriptionBuiltinTopicDataDataReaderImpl;
}

namespace OpenDDS {
namespace RTPS {

class RtpsDiscovery;
class Spdp;

class Sedp {
public:
  Sedp(const DCPS::RepoId& participant_id, Spdp& owner)
    : participant_id_(participant_id)
    , spdp_(owner)
    , publications_writer_(make_id(participant_id,
                                   ENTITYID_SEDP_BUILTIN_PUBLICATIONS_WRITER),
                           owner)
    , subscriptions_writer_(make_id(participant_id,
                                    ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_WRITER),
                            owner)
    , publications_reader_(make_id(participant_id,
                                   ENTITYID_SEDP_BUILTIN_PUBLICATIONS_READER),
                           owner)
    , subscriptions_reader_(make_id(participant_id,
                                    ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_READER),
                            owner)
    , publication_counter_(0), subscription_counter_(0), topic_counter_(0)
  {}

  DDS::ReturnCode_t init(const DCPS::RepoId& guid, 
                         RtpsDiscovery& disco, 
                         DDS::DomainId_t domainId);

  void ignore(const DCPS::RepoId& to_ignore) {
    ignored_guids_.insert(to_ignore);
  }
  bool ignoring(const DCPS::RepoId& guid) const {
    return ignored_guids_.count(guid);
  }

  // Topic
  DCPS::TopicStatus assert_topic(DCPS::RepoId_out topicId,
                                 const char* topicName,
                                 const char* dataTypeName,
                                 const DDS::TopicQos& qos,
                                 bool hasDcpsKey);
  DCPS::TopicStatus remove_topic(const DCPS::RepoId& topicId,
                                 std::string& name);
  bool update_topic_qos(const DCPS::RepoId& topicId, const DDS::TopicQos& qos,
                        std::string& name);
  struct TopicDetails {
    std::string data_type_;
    DDS::TopicQos qos_;
    DCPS::RepoId repo_id_;
    bool has_dcps_key_;
  };

  // Publication
  DCPS::RepoId add_publication(const DCPS::RepoId& topicId,
                               DCPS::DataWriterRemote_ptr publication,
                               const DDS::DataWriterQos& qos,
                               const DCPS::TransportLocatorSeq& transInfo,
                               const DDS::PublisherQos& publisherQos);
  void remove_publication(const DCPS::RepoId& publicationId);
  bool update_publication_qos(const DCPS::RepoId& publicationId,
                              const DDS::DataWriterQos& qos,
                              const DDS::PublisherQos& publisherQos);

  // Subscription
  DCPS::RepoId add_subscription(const DCPS::RepoId& topicId,
                                DCPS::DataReaderRemote_ptr subscription,
                                const DDS::DataReaderQos& qos,
                                const DCPS::TransportLocatorSeq& transInfo,
                                const DDS::SubscriberQos& subscriberQos,
                                const char* filterExpr,
                                const DDS::StringSeq& params);
  void remove_subscription(const DCPS::RepoId& subscriptionId);
  bool update_subscription_qos(const DCPS::RepoId& subscriptionId,
                               const DDS::DataReaderQos& qos,
                               const DDS::SubscriberQos& subscriberQos);
  bool update_subscription_params(const DCPS::RepoId& subId,
                                  const DDS::StringSeq& params);

  // Managing reader/writer associations
  void association_complete(const DCPS::RepoId& localId,
                            const DCPS::RepoId& remoteId);
  void disassociate_participant(const DCPS::RepoId& remoteId);
  void disassociate_publication(const DCPS::RepoId& localId,
                                const DCPS::RepoId& remoteId);
  void disassociate_subscription(const DCPS::RepoId& localId,
                                 const DCPS::RepoId& remoteId);

private:
  static DCPS::RepoId
  make_id(const DCPS::RepoId& participant_id, const EntityId_t& entity)
  {
    DCPS::RepoId id = participant_id;
    id.entityId = entity;
    return id;
  }

  DCPS::RepoId participant_id_;
  Spdp& spdp_;

  class Endpoint : public DCPS::TransportClient {
  public:
    Endpoint(const DCPS::RepoId& repo_id, Spdp& spdp)
      : repo_id_(repo_id)
      , spdp_(spdp)
    {}

    virtual ~Endpoint();

    // Implementing TransportClient
    bool check_transport_qos(const DCPS::TransportInst&)
      { return true; }
    const DCPS::RepoId& get_repo_id() const
      { return repo_id_; }
    CORBA::Long get_priority_value(const DCPS::AssociationData&) const
      { return 0; }

  protected:
    using DCPS::TransportClient::enable_transport;
    using DCPS::TransportClient::disassociate;
    using DCPS::TransportClient::send;
    using DCPS::TransportClient::send_control;

    DCPS::RepoId repo_id_;
    Spdp& spdp_;
  };

  class Writer : public DCPS::TransportSendListener, public Endpoint {
  public:
    Writer(const DCPS::RepoId& pub_id, Spdp& spdp)
      : Endpoint(pub_id, spdp)
    {}

    virtual ~Writer();

    bool assoc(const DCPS::AssociationData& subscription);

    // Implementing TransportSendListener
    void data_delivered(const DCPS::DataSampleListElement*);

    void data_dropped(const DCPS::DataSampleListElement*, bool by_transport);

    void control_delivered(ACE_Message_Block* /*sample*/);

    void control_dropped(ACE_Message_Block* /*sample*/,
                         bool /*dropped_by_transport*/);

    void notify_publication_disconnected(const DCPS::ReaderIdSeq&) {}
    void notify_publication_reconnected(const DCPS::ReaderIdSeq&) {}
    void notify_publication_lost(const DCPS::ReaderIdSeq&) {}
    void notify_connection_deleted() {}
    void remove_associations(const DCPS::ReaderIdSeq&, bool) {}
    void retrieve_inline_qos_data(InlineQosData&) const {}

  } publications_writer_, subscriptions_writer_;

  class Reader : public DCPS::TransportReceiveListener, public Endpoint {
  public:
    Reader(const DCPS::RepoId& sub_id, Spdp& spdp)
      : Endpoint(sub_id, spdp)
    {}

    virtual ~Reader();

    bool assoc(const DCPS::AssociationData& publication);

    // Implementing TransportReceiveListener

    void data_received(const DCPS::ReceivedDataSample& sample);

    void notify_subscription_disconnected(const DCPS::WriterIdSeq&) {}
    void notify_subscription_reconnected(const DCPS::WriterIdSeq&) {}
    void notify_subscription_lost(const DCPS::WriterIdSeq&) {}
    void notify_connection_deleted() {}
    void remove_associations(const DCPS::WriterIdSeq&, bool) {}

  } publications_reader_, subscriptions_reader_;

  // Transport
  OpenDDS::DCPS::TransportInst_rch transport_;

  DDS::TopicBuiltinTopicDataDataReaderImpl* topic_bit();
  DDS::PublicationBuiltinTopicDataDataReaderImpl* pub_bit();
  DDS::SubscriptionBuiltinTopicDataDataReaderImpl* sub_bit();

  struct LocalPublication {
    DCPS::RepoId topic_id_;
    DCPS::DataWriterRemote_ptr publication_;
    DDS::DataWriterQos qos_;
    DCPS::TransportLocatorSeq trans_info_;
    DDS::PublisherQos publisher_qos_;
  };

  typedef std::map<DCPS::RepoId, LocalPublication,
                   DCPS::GUID_tKeyLessThan> LocalPublicationMap;
  typedef LocalPublicationMap::iterator LocalPublicationIter;
  LocalPublicationMap local_publications_;

  struct LocalSubscription {
    DCPS::RepoId topic_id_;
    DCPS::DataReaderRemote_ptr subscription_;
    DDS::DataReaderQos qos_;
    DCPS::TransportLocatorSeq trans_info_;
    DDS::SubscriberQos subscriber_qos_;
    DDS::StringSeq params_;
  };

  typedef std::map<DCPS::RepoId, LocalSubscription,
                   DCPS::GUID_tKeyLessThan> LocalSubscriptionMap;
  typedef LocalSubscriptionMap::iterator LocalSubscriptionIter;
  LocalSubscriptionMap local_subscriptions_;

  unsigned int publication_counter_, subscription_counter_;

  std::set<DCPS::RepoId, DCPS::GUID_tKeyLessThan> ignored_guids_;

  // Topic:
  std::map<std::string, TopicDetails> topics_;
  std::map<DCPS::RepoId, std::string, DCPS::GUID_tKeyLessThan> topic_names_;
  unsigned int topic_counter_;
  bool has_dcps_key(const DCPS::RepoId& topicId) const;
};

}
}

#endif // OPENDDS_RTPS_SEDP_H

