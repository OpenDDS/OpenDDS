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
#include "dds/DCPS/DataSampleList.h"

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

enum RtpsFlags { FLAG_E = 1, FLAG_Q = 2, FLAG_D = 4 };

class RtpsDiscovery;
class Spdp;

class Sedp {
public:
  Sedp(const DCPS::RepoId& participant_id, Spdp& owner);

  DDS::ReturnCode_t init(const DCPS::RepoId& guid, 
                         RtpsDiscovery& disco, 
                         DDS::DomainId_t domainId);

  // @brief return the ip address we have bound to.  
  // Valid after init() call
  const ACE_INET_Addr& local_address() const;
  const ACE_INET_Addr& multicast_group() const;

  void ignore(const DCPS::RepoId& to_ignore);

  bool ignoring(const DCPS::RepoId& guid) const {
    return ignored_guids_.count(guid);
  }

  void associate(const SPDPdiscoveredParticipantData& pdata);
  void disassociate(const SPDPdiscoveredParticipantData& pdata);

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

  static const bool host_is_bigendian_;
private:
  DCPS::RepoId participant_id_;
  Spdp& spdp_;
  struct LocalPublication;

  class Endpoint : public DCPS::TransportClient {
  public:
    Endpoint(const DCPS::RepoId& repo_id, Sedp& sedp)
      : repo_id_(repo_id)
      , sedp_(sedp)
    {}

    virtual ~Endpoint();

    // Implementing TransportClient
    bool check_transport_qos(const DCPS::TransportInst&)
      { return true; }
    const DCPS::RepoId& get_repo_id() const
      { return repo_id_; }
    CORBA::Long get_priority_value(const DCPS::AssociationData&) const
      { return 0; }

    using DCPS::TransportClient::enable_transport;
    using DCPS::TransportClient::disassociate;

  protected:
    DCPS::RepoId repo_id_;
    Sedp& sedp_;
  };

  class Writer : public DCPS::TransportSendListener, public Endpoint {
  public:
    Writer(const DCPS::RepoId& pub_id, Sedp& sedp)
      : Endpoint(pub_id, sedp),
        alloc_(2, sizeof(DCPS::TransportSendElementAllocator))
    {
      header_.prefix[0] = 'R';
      header_.prefix[1] = 'T';
      header_.prefix[2] = 'P';
      header_.prefix[3] = 'S';
      header_.version = PROTOCOLVERSION;
      header_.vendorId = VENDORID_OPENDDS;
      header_.guidPrefix[0] = pub_id.guidPrefix[0];
      header_.guidPrefix[1] = pub_id.guidPrefix[1],
      header_.guidPrefix[2] = pub_id.guidPrefix[2];
      header_.guidPrefix[3] = pub_id.guidPrefix[3];
      header_.guidPrefix[4] = pub_id.guidPrefix[4];
      header_.guidPrefix[5] = pub_id.guidPrefix[5];
      header_.guidPrefix[6] = pub_id.guidPrefix[6];
      header_.guidPrefix[7] = pub_id.guidPrefix[7];
      header_.guidPrefix[8] = pub_id.guidPrefix[8];
      header_.guidPrefix[9] = pub_id.guidPrefix[9];
      header_.guidPrefix[10] = pub_id.guidPrefix[10];
      header_.guidPrefix[11] = pub_id.guidPrefix[11];
    }

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

    DDS::ReturnCode_t publish_sample(const DiscoveredWriterData& dwd);
  private:
    DCPS::TransportSendElementAllocator alloc_;
    Header header_;
    SequenceNumber_t seq_;

    DDS::ReturnCode_t build_message(const DiscoveredWriterData& dwd,
                                    ACE_Message_Block& payload);

    void publish_sample(ACE_Message_Block& payload, size_t size);

  } publications_writer_, subscriptions_writer_;

  class Reader : public DCPS::TransportReceiveListener, public Endpoint {
  public:
    Reader(const DCPS::RepoId& sub_id, Sedp& sedp)
      : Endpoint(sub_id, sedp)
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
  DCPS::TransportInst_rch transport_inst_;
  DCPS::TransportConfig_rch transport_cfg_;

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

  DDS::ReturnCode_t populate_discovered_writer_msg(
      DiscoveredWriterData& dwd,
      const DCPS::RepoId& publication_id,
      const LocalPublication& pub);

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

  void data_received(char message_id, const DiscoveredWriterData& wdata);
  void data_received(char message_id, const DiscoveredReaderData& rdata);

  struct DiscoveredPublication {
    DiscoveredPublication() {}
    explicit DiscoveredPublication(const DiscoveredWriterData& w)
      : writer_data_(w) {}
    DiscoveredWriterData writer_data_;
    DDS::InstanceHandle_t bit_ih_;
  };
  typedef std::map<DCPS::RepoId, DiscoveredPublication,
                   DCPS::GUID_tKeyLessThan> DiscoveredPublicationMap;
  typedef DiscoveredPublicationMap::iterator DiscoveredPublicationIter;
  DiscoveredPublicationMap discovered_publications_;

  struct DiscoveredSubscription {
    DiscoveredSubscription() {}
    explicit DiscoveredSubscription(const DiscoveredReaderData& r)
      : reader_data_(r) {}
    DiscoveredReaderData reader_data_;
    DDS::InstanceHandle_t bit_ih_;
  };
  typedef std::map<DCPS::RepoId, DiscoveredSubscription,
                   DCPS::GUID_tKeyLessThan> DiscoveredSubscriptionMap;
  typedef DiscoveredSubscriptionMap::iterator DiscoveredSubscriptionIter;
  DiscoveredSubscriptionMap discovered_subscriptions_;

  template<typename Map>
  void remove_entities_belonging_to(Map& m, const DCPS::RepoId& participant);
  void remove_from_bit(const DiscoveredPublication& pub);
  void remove_from_bit(const DiscoveredSubscription& sub);

  bool qosChanged(DDS::PublicationBuiltinTopicData& dest,
                  const DDS::PublicationBuiltinTopicData& src);
  bool qosChanged(DDS::SubscriptionBuiltinTopicData& dest,
                  const DDS::SubscriptionBuiltinTopicData& src);

  std::set<DCPS::RepoId, DCPS::GUID_tKeyLessThan> ignored_guids_;

  // Topic:
  std::map<std::string, TopicDetails> topics_;
  std::map<DCPS::RepoId, std::string, DCPS::GUID_tKeyLessThan> topic_names_;
  unsigned int topic_counter_;
  bool has_dcps_key(const DCPS::RepoId& topicId) const;

  static DCPS::RepoId
  make_id(const DCPS::RepoId& participant_id, const EntityId_t& entity);
};

}
}

#endif // OPENDDS_RTPS_SEDP_H

