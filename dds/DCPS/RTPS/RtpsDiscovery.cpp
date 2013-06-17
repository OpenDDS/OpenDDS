/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef DDS_HAS_MINIMUM_BIT

#include "RtpsDiscovery.h"

#include "dds/DCPS/Service_Participant.h"
#include "dds/DCPS/ConfigUtils.h"
#include "dds/DCPS/DomainParticipantImpl.h"
#include "dds/DCPS/SubscriberImpl.h"
#include "dds/DCPS/Marked_Default_Qos.h"
#include "dds/DCPS/BuiltInTopicUtils.h"
#include "dds/DCPS/Registered_Data_Types.h"
#include "dds/DdsDcpsInfoUtilsC.h"

#include "ace/Reactor.h"
#include "ace/Select_Reactor.h"

#include <cstdlib>

namespace {
  u_short get_default_d0(u_short fallback)
  {
#if !defined ACE_LACKS_GETENV && !defined ACE_LACKS_ENV
    const char* from_env = std::getenv("OPENDDS_RTPS_DEFAULT_D0");
    if (from_env) {
      return static_cast<u_short>(std::atoi(from_env));
    }
#endif
    return fallback;
  }
}

namespace OpenDDS {
namespace RTPS {

RtpsDiscovery::RtpsDiscovery(const RepoKey& key)
  : DCPS::Discovery(key)
  , resend_period_(30 /*seconds*/) // see RTPS v2.1 9.6.1.4.2
  , pb_(7400) // see RTPS v2.1 9.6.1.3 for PB, DG, PG, D0, D1 defaults
  , dg_(250)
  , pg_(2)
  , d0_(get_default_d0(0))
  , d1_(10)
  , dx_(2)
  , ttl_(1)
  , sedp_multicast_(true)
  , default_multicast_group_("239.255.0.1")
{
}

RtpsDiscovery::~RtpsDiscovery()
{
  reactor_runner_.end();

}

OpenDDS::DCPS::RepoId
RtpsDiscovery::bit_key_to_repo_id(DCPS::DomainParticipantImpl* participant,
                                  const char* bit_topic_name,
                                  const DDS::BuiltinTopicKey_t& key) const
{
  return get_part(participant->get_domain_id(), participant->get_id())
    ->bit_key_to_repo_id(bit_topic_name, key);
}


namespace {
  void create_bit_dr(DDS::TopicDescription_ptr topic, const char* type,
                     DCPS::SubscriberImpl* sub,
                     const DDS::DataReaderQos& qos)
  {
    using namespace DCPS;
    TopicDescriptionImpl* bit_topic_i =
      dynamic_cast<TopicDescriptionImpl*>(topic);

    DDS::DomainParticipant_var participant = sub->get_participant();
    DomainParticipantImpl* participant_i =
      dynamic_cast<DomainParticipantImpl*>(participant.in());

    TypeSupport_ptr type_support =
      Registered_Data_Types->lookup(participant, type);

    DDS::DataReader_var dr = type_support->create_datareader();
    DataReaderImpl* dri = dynamic_cast<DataReaderImpl*>(dr.in());

    dri->init(bit_topic_i, qos, 0 /*listener*/, 0 /*mask*/,
              participant_i, sub, dr);
    dri->disable_transport();
    dri->enable();
  }
}

DDS::Subscriber_ptr
RtpsDiscovery::init_bit(DCPS::DomainParticipantImpl* participant)
{
  using namespace DCPS;
  if (create_bit_topics(participant) != DDS::RETCODE_OK) {
    return 0;
  }

  DDS::Subscriber_var bit_subscriber =
    participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT,
                                   DDS::SubscriberListener::_nil(),
                                   DEFAULT_STATUS_MASK);
  SubscriberImpl* sub = dynamic_cast<SubscriberImpl*>(bit_subscriber.in());

  DDS::DataReaderQos dr_qos;
  sub->get_default_datareader_qos(dr_qos);
  dr_qos.durability.kind = DDS::TRANSIENT_LOCAL_DURABILITY_QOS;

  DDS::TopicDescription_var bit_part_topic =
    participant->lookup_topicdescription(BUILT_IN_PARTICIPANT_TOPIC);
  create_bit_dr(bit_part_topic, BUILT_IN_PARTICIPANT_TOPIC_TYPE,
                sub, dr_qos);

  DDS::TopicDescription_var bit_topic_topic =
    participant->lookup_topicdescription(BUILT_IN_TOPIC_TOPIC);
  create_bit_dr(bit_topic_topic, BUILT_IN_TOPIC_TOPIC_TYPE,
                sub, dr_qos);

  DDS::TopicDescription_var bit_pub_topic =
    participant->lookup_topicdescription(BUILT_IN_PUBLICATION_TOPIC);
  create_bit_dr(bit_pub_topic, BUILT_IN_PUBLICATION_TOPIC_TYPE,
                sub, dr_qos);

  DDS::TopicDescription_var bit_sub_topic =
    participant->lookup_topicdescription(BUILT_IN_SUBSCRIPTION_TOPIC);
  create_bit_dr(bit_sub_topic, BUILT_IN_SUBSCRIPTION_TOPIC_TYPE,
                sub, dr_qos);

  set_part_bit_subscriber(participant->get_domain_id(), participant->get_id(), bit_subscriber);

  return bit_subscriber._retn();
}

void
RtpsDiscovery::set_part_bit_subscriber(const DDS::DomainId_t domain_id,
                                       const DCPS::RepoId& part_id,
                                       const DDS::Subscriber_var& bit_subscriber)
{
  get_part(domain_id, part_id)->bit_subscriber(bit_subscriber);
}

namespace {
  const ACE_TCHAR RTPS_SECTION_NAME[] = ACE_TEXT("rtps_discovery");
}

int
RtpsDiscovery::Config::discovery_config(ACE_Configuration_Heap& cf)
{
  const ACE_Configuration_Section_Key &root = cf.root_section();
  ACE_Configuration_Section_Key rtps_sect;

  if (cf.open_section(root, RTPS_SECTION_NAME, 0, rtps_sect) == 0) {

    // Ensure there are no properties in this section
    DCPS::ValueMap vm;
    if (DCPS::pullValues(cf, rtps_sect, vm) > 0) {
      // There are values inside [rtps_discovery]
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("(%P|%t) RtpsDiscovery::Config::discovery_config(): ")
                        ACE_TEXT("rtps_discovery sections must have a subsection name\n")),
                       -1);
    }
    // Process the subsections of this section (the individual rtps_discovery/*)
    DCPS::KeyList keys;
    if (DCPS::processSections(cf, rtps_sect, keys) != 0) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("(%P|%t) RtpsDiscovery::Config::discovery_config(): ")
                        ACE_TEXT("too many nesting layers in the [rtps] section.\n")),
                       -1);
    }

    // Loop through the [rtps_discovery/*] sections
    for (DCPS::KeyList::const_iterator it = keys.begin();
         it != keys.end(); ++it) {
      const std::string& rtps_name = it->first;

      int resend;
      u_short pb, dg, pg, d0, d1, dx, ttl;
      AddrVec spdp_send_addrs;
      std::string default_multicast_group = "239.255.0.1" /*RTPS v2.1 9.6.1.4.1*/;
      std::string mi;
      bool has_resend = false, has_pb = false, has_dg = false, has_pg = false,
        has_d0 = false, has_d1 = false, has_dx = false, has_sm = false,
        has_ttl = false, sm = false;

      DCPS::ValueMap values;
      DCPS::pullValues(cf, it->second, values);
      for (DCPS::ValueMap::const_iterator it = values.begin();
           it != values.end(); ++it) {
        const std::string& name = it->first;
        if (name == "ResendPeriod") {
          const std::string& value = it->second;
          has_resend = DCPS::convertToInteger(value, resend);
          if (!has_resend) {
            ACE_ERROR_RETURN((LM_ERROR,
              ACE_TEXT("(%P|%t) RtpsDiscovery::Config::discovery_config(): ")
              ACE_TEXT("Invalid entry (%C) for ResendPeriod in ")
              ACE_TEXT("[rtps_discovery/%C] section.\n"),
              value.c_str(), rtps_name.c_str()), -1);
          }
        } else if (name == "PB") {
          const std::string& value = it->second;
          has_pb = DCPS::convertToInteger(value, pb);
          if (!has_pb) {
            ACE_ERROR_RETURN((LM_ERROR,
              ACE_TEXT("(%P|%t) RtpsDiscovery::Config::discovery_config(): ")
              ACE_TEXT("Invalid entry (%C) for PB in ")
              ACE_TEXT("[rtps_discovery/%C] section.\n"),
              value.c_str(), rtps_name.c_str()), -1);
          }
        } else if (name == "DG") {
          const std::string& value = it->second;
          has_dg = DCPS::convertToInteger(value, dg);
          if (!has_dg) {
            ACE_ERROR_RETURN((LM_ERROR,
              ACE_TEXT("(%P|%t) RtpsDiscovery::Config::discovery_config(): ")
              ACE_TEXT("Invalid entry (%C) for DG in ")
              ACE_TEXT("[rtps_discovery/%C] section.\n"),
              value.c_str(), rtps_name.c_str()), -1);
          }
        } else if (name == "PG") {
          const std::string& value = it->second;
          has_pg = DCPS::convertToInteger(value, pg);
          if (!has_pg) {
            ACE_ERROR_RETURN((LM_ERROR,
              ACE_TEXT("(%P|%t) RtpsDiscovery::Config::discovery_config(): ")
              ACE_TEXT("Invalid entry (%C) for PG in ")
              ACE_TEXT("[rtps_discovery/%C] section.\n"),
              value.c_str(), rtps_name.c_str()), -1);
          }
        } else if (name == "D0") {
          const std::string& value = it->second;
          has_d0 = DCPS::convertToInteger(value, d0);
          if (!has_d0) {
            ACE_ERROR_RETURN((LM_ERROR,
              ACE_TEXT("(%P|%t) RtpsDiscovery::Config::discovery_config(): ")
              ACE_TEXT("Invalid entry (%C) for D0 in ")
              ACE_TEXT("[rtps_discovery/%C] section.\n"),
              value.c_str(), rtps_name.c_str()), -1);
          }
        } else if (name == "D1") {
          const std::string& value = it->second;
          has_d1 = DCPS::convertToInteger(value, d1);
          if (!has_d1) {
            ACE_ERROR_RETURN((LM_ERROR,
              ACE_TEXT("(%P|%t) RtpsDiscovery::Config::discovery_config(): ")
              ACE_TEXT("Invalid entry (%C) for D1 in ")
              ACE_TEXT("[rtps_discovery/%C] section.\n"),
              value.c_str(), rtps_name.c_str()), -1);
          }
        } else if (name == "DX") {
          const std::string& value = it->second;
          has_dx = DCPS::convertToInteger(value, dx);
          if (!has_dx) {
            ACE_ERROR_RETURN((LM_ERROR,
               ACE_TEXT("(%P|%t) RtpsDiscovery::Config::discovery_config(): ")
               ACE_TEXT("Invalid entry (%C) for DX in ")
               ACE_TEXT("[rtps_discovery/%C] section.\n"),
               value.c_str(), rtps_name.c_str()), -1);
          }
        } else if (name == "TTL") {
          const std::string& value = it->second;
          has_ttl = DCPS::convertToInteger(value, ttl);
          if (!has_ttl) {
            ACE_ERROR_RETURN((LM_ERROR,
               ACE_TEXT("(%P|%t) RtpsDiscovery::Config::discovery_config(): ")
               ACE_TEXT("Invalid entry (%C) for TTL in ")
               ACE_TEXT("[rtps_discovery/%C] section.\n"),
               value.c_str(), rtps_name.c_str()), -1);
          }
        } else if (name == "SedpMulticast") {
          const std::string& value = it->second;
          int smInt;
          has_sm = DCPS::convertToInteger(value, smInt);
          if (!has_sm) {
            ACE_ERROR_RETURN((LM_ERROR,
               ACE_TEXT("(%P|%t) RtpsDiscovery::Config::discovery_config ")
               ACE_TEXT("Invalid entry (%C) for SedpMulticast in ")
               ACE_TEXT("[rtps_discovery/%C] section.\n"),
               value.c_str(), rtps_name.c_str()), -1);
          }
          sm = bool(smInt);
        } else if (name == "MulticastInterface") {
          mi = it->second;
        } else if (name == "DefaultMulticastGroup") {
          default_multicast_group = it->second;
        } else if (name == "SpdpSendAddrs") {
          const std::string& value = it->second;
          size_t i = 0;
          do {
            i = value.find_first_not_of(' ', i); // skip spaces
            const size_t n = value.find_first_of(", ", i);
            spdp_send_addrs.push_back(value.substr(i, (n == std::string::npos) ? n : n - i));
            i = value.find(',', i);
          } while (i++ != std::string::npos); // skip past comma if there is one
        } else {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) RtpsDiscovery::Config::discovery_config(): ")
                            ACE_TEXT("Unexpected entry (%C) in [rtps_discovery/%C] section.\n"),
                            name.c_str(), rtps_name.c_str()),
                           -1);
        }
      }

      RtpsDiscovery_rch discovery = new RtpsDiscovery(rtps_name);
      if (has_resend) discovery->resend_period(ACE_Time_Value(resend));
      if (has_pb) discovery->pb(pb);
      if (has_dg) discovery->dg(dg);
      if (has_pg) discovery->pg(pg);
      if (has_d0) discovery->d0(d0);
      if (has_d1) discovery->d1(d1);
      if (has_dx) discovery->dx(dx);
      if (has_ttl) discovery->ttl(ttl);
      if (has_sm) discovery->sedp_multicast(sm);
      discovery->multicast_interface(mi);
      discovery->default_multicast_group( default_multicast_group);
      discovery->spdp_send_addrs().swap(spdp_send_addrs);
      TheServiceParticipant->add_discovery(
        DCPS::static_rchandle_cast<Discovery>(discovery));
    }
  }

  // If the default RTPS discovery object has not been configured,
  // instantiate it now.
  const DCPS::Service_Participant::RepoKeyDiscoveryMap& discoveryMap = TheServiceParticipant->discoveryMap();
  if (discoveryMap.find(Discovery::DEFAULT_RTPS) == discoveryMap.end()) {
    RtpsDiscovery_rch discovery = new RtpsDiscovery(Discovery::DEFAULT_RTPS);
    TheServiceParticipant->add_discovery(
      DCPS::static_rchandle_cast<Discovery>(discovery));
  }

  return 0;
}

ACE_Reactor*
RtpsDiscovery::reactor()
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, reactor_runner_.mtx_, 0);
  if (!reactor_runner_.reactor_) {
    reactor_runner_.reactor_ = new ACE_Reactor(new ACE_Select_Reactor, true);
    reactor_runner_.activate();
  }
  return reactor_runner_.reactor_;
}

int
RtpsDiscovery::ReactorRunner::svc()
{
  reactor_->owner(ACE_Thread_Manager::instance()->thr_self());
  reactor_->run_reactor_event_loop();
  return 0;
}

void
RtpsDiscovery::ReactorRunner::end()
{
  ACE_GUARD(ACE_Thread_Mutex, g, mtx_);
  if (reactor_) {
    reactor_->end_reactor_event_loop();
    wait();
  }
}

RtpsDiscovery::ReactorRunner::~ReactorRunner()
{
  delete reactor_;
}


// Participant operations:

bool
RtpsDiscovery::attach_participant(DDS::DomainId_t /*domainId*/,
                                  const OpenDDS::DCPS::RepoId& /*participantId*/)
{
  return false; // This is just for DCPSInfoRepo?
}

DCPS::AddDomainStatus
RtpsDiscovery::add_domain_participant(DDS::DomainId_t domain,
                                      const DDS::DomainParticipantQos& qos)
{
  DCPS::AddDomainStatus ads = {OpenDDS::DCPS::RepoId(), false /*federated*/};
  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, ads);
  guid_gen_.populate(ads.id);
  ads.id.entityId = ENTITYID_PARTICIPANT;
  try {
    participants_[domain][ads.id] = new Spdp(domain, ads.id, qos, this);
  } catch (const std::exception& e) {
    ads.id = GUID_UNKNOWN;
    ACE_ERROR((LM_ERROR, "(%P|%t) RtpsDiscovery::add_domain_participant() - "
      "failed to initialize RTPS Simple Participant Discovery Protocol: %C\n",
      e.what()));
  }
  return ads;
}

bool
RtpsDiscovery::remove_domain_participant(DDS::DomainId_t domain_id,
                                         const OpenDDS::DCPS::RepoId& participantId)
{
  // Use reference counting to ensure participant
  // does not get deleted until lock as been released.
  DCPS::RcHandle<Spdp> participant;
  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, false);
  DomainParticipantMap::iterator domain = participants_.find(domain_id);
  if (domain == participants_.end()) {
    return false;
  }
  ParticipantMap::iterator part = domain->second.find(participantId);
  if (part == domain->second.end()) {
    return false;
  }
  participant = part->second;
  domain->second.erase(part);
  if (domain->second.empty()) {
    participants_.erase(domain);
  }

  return true;
}

bool
RtpsDiscovery::ignore_domain_participant(DDS::DomainId_t domain,
                                         const OpenDDS::DCPS::RepoId& myParticipantId,
                                         const OpenDDS::DCPS::RepoId& ignoreId)
{
  get_part(domain, myParticipantId)->ignore_domain_participant(ignoreId);
  return true;
}

bool
RtpsDiscovery::update_domain_participant_qos(DDS::DomainId_t domain,
                                             const OpenDDS::DCPS::RepoId& participant,
                                             const DDS::DomainParticipantQos& qos)
{
  return get_part(domain, participant)->update_domain_participant_qos(qos);
}

// Topic operations:

DCPS::TopicStatus
RtpsDiscovery::assert_topic(OpenDDS::DCPS::RepoId_out topicId,
                            DDS::DomainId_t domainId,
                            const OpenDDS::DCPS::RepoId& participantId,
                            const char* topicName,
                            const char* dataTypeName,
                            const DDS::TopicQos& qos,
                            bool hasDcpsKey)
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, DCPS::INTERNAL_ERROR);
  std::map<DDS::DomainId_t,
           std::map<std::string, Sedp::TopicDetails> >::iterator topic_it =
    topics_.find(domainId);
  if (topic_it != topics_.end()) {
    const std::map<std::string, Sedp::TopicDetails>::iterator it =
      topic_it->second.find(topicName);
    if (it != topic_it->second.end()
        && it->second.data_type_ != dataTypeName) {
      topicId = GUID_UNKNOWN;
      return DCPS::CONFLICTING_TYPENAME;
    }
  }

  // Verified its safe to hold lock during call to assert_topic
  const DCPS::TopicStatus stat =
    participants_[domainId][participantId]->assert_topic(topicId, topicName,
                                                         dataTypeName, qos,
                                                         hasDcpsKey);
  if (stat == DCPS::CREATED || stat == DCPS::FOUND) { // qos change (FOUND)
    Sedp::TopicDetails& td = topics_[domainId][topicName];
    td.data_type_ = dataTypeName;
    td.qos_ = qos;
    td.repo_id_ = topicId;
    ++topic_use_[domainId][topicName];
  }
  return stat;
}

DCPS::TopicStatus
RtpsDiscovery::find_topic(DDS::DomainId_t domainId, const char* topicName,
                          CORBA::String_out dataTypeName, DDS::TopicQos_out qos,
                          OpenDDS::DCPS::RepoId_out topicId)
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, DCPS::INTERNAL_ERROR);
  std::map<DDS::DomainId_t,
           std::map<std::string, Sedp::TopicDetails> >::iterator topic_it =
    topics_.find(domainId);
  if (topic_it == topics_.end()) {
    return DCPS::NOT_FOUND;
  }
  std::map<std::string, Sedp::TopicDetails>::iterator iter =
    topic_it->second.find(topicName);
  if (iter == topic_it->second.end()) {
    return DCPS::NOT_FOUND;
  }
  Sedp::TopicDetails& td = iter->second;
  dataTypeName = td.data_type_.c_str();
  qos = new DDS::TopicQos(td.qos_);
  topicId = td.repo_id_;
  ++topic_use_[domainId][topicName];
  return DCPS::FOUND;
}

DCPS::TopicStatus
RtpsDiscovery::remove_topic(DDS::DomainId_t domainId,
                            const OpenDDS::DCPS::RepoId& participantId,
                            const OpenDDS::DCPS::RepoId& topicId)
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, DCPS::INTERNAL_ERROR);
  std::map<DDS::DomainId_t,
           std::map<std::string, Sedp::TopicDetails> >::iterator topic_it =
    topics_.find(domainId);
  if (topic_it == topics_.end()) {
    return DCPS::NOT_FOUND;
  }

  std::string name;
  // Safe to hold lock while calling remove topic
  const DCPS::TopicStatus stat =
    participants_[domainId][participantId]->remove_topic(topicId, name);

  if (stat == DCPS::REMOVED) {
    if (0 == --topic_use_[domainId][name]) {
      topic_use_[domainId].erase(name);
      if (topic_it->second.empty()) {
        topic_use_.erase(domainId);
      }
      topic_it->second.erase(name);
      if (topic_it->second.empty()) {
        topics_.erase(topic_it);
      }
    }
  }
  return stat;
}

bool
RtpsDiscovery::ignore_topic(DDS::DomainId_t domainId, const OpenDDS::DCPS::RepoId& myParticipantId,
                            const OpenDDS::DCPS::RepoId& ignoreId)
{
  get_part(domainId, myParticipantId)->ignore_topic(ignoreId);
  return true;
}

bool
RtpsDiscovery::update_topic_qos(const OpenDDS::DCPS::RepoId& topicId, DDS::DomainId_t domainId,
                                const OpenDDS::DCPS::RepoId& participantId, const DDS::TopicQos& qos)
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, false);
  std::string name;
  // Safe to hold lock while calling update_topic_qos
  if (participants_[domainId][participantId]->update_topic_qos(topicId,
                                                               qos, name)) {
    topics_[domainId][name].qos_ = qos;
    return true;
  }
  return false;
}


// Publication operations:

OpenDDS::DCPS::RepoId
RtpsDiscovery::add_publication(DDS::DomainId_t domainId,
                               const OpenDDS::DCPS::RepoId& participantId,
                               const OpenDDS::DCPS::RepoId& topicId,
                               DCPS::DataWriterCallbacks* publication,
                               const DDS::DataWriterQos& qos,
                               const DCPS::TransportLocatorSeq& transInfo,
                               const DDS::PublisherQos& publisherQos)
{
  return get_part(domainId, participantId)->add_publication(
    topicId, publication, qos, transInfo, publisherQos);
}

bool
RtpsDiscovery::remove_publication(DDS::DomainId_t domainId,
                                  const OpenDDS::DCPS::RepoId& participantId,
                                  const OpenDDS::DCPS::RepoId& publicationId)
{
  get_part(domainId, participantId)->remove_publication(publicationId);
  return true;
}

bool
RtpsDiscovery::ignore_publication(DDS::DomainId_t domainId,
                                  const OpenDDS::DCPS::RepoId& participantId,
                                  const OpenDDS::DCPS::RepoId& ignoreId)
{
  get_part(domainId, participantId)->ignore_publication(ignoreId);
  return true;
}

bool
RtpsDiscovery::update_publication_qos(DDS::DomainId_t domainId,
                                      const OpenDDS::DCPS::RepoId& partId,
                                      const OpenDDS::DCPS::RepoId& dwId,
                                      const DDS::DataWriterQos& qos,
                                      const DDS::PublisherQos& publisherQos)
{
  return get_part(domainId, partId)->update_publication_qos(dwId, qos,
                                                            publisherQos);
}


// Subscription operations:

OpenDDS::DCPS::RepoId
RtpsDiscovery::add_subscription(DDS::DomainId_t domainId,
                                const OpenDDS::DCPS::RepoId& participantId,
                                const OpenDDS::DCPS::RepoId& topicId,
                                DCPS::DataReaderCallbacks* subscription,
                                const DDS::DataReaderQos& qos,
                                const DCPS::TransportLocatorSeq& transInfo,
                                const DDS::SubscriberQos& subscriberQos,
                                const char* filterExpr,
                                const DDS::StringSeq& params)
{
  return get_part(domainId, participantId)->add_subscription(
    topicId, subscription, qos, transInfo, subscriberQos, filterExpr, params);
}

bool
RtpsDiscovery::remove_subscription(DDS::DomainId_t domainId,
                                   const OpenDDS::DCPS::RepoId& participantId,
                                   const OpenDDS::DCPS::RepoId& subscriptionId)
{
  get_part(domainId, participantId)->remove_subscription(subscriptionId);
  return true;
}

bool
RtpsDiscovery::ignore_subscription(DDS::DomainId_t domainId,
                                   const OpenDDS::DCPS::RepoId& participantId,
                                   const OpenDDS::DCPS::RepoId& ignoreId)
{
  get_part(domainId, participantId)->ignore_subscription(ignoreId);
  return true;
}

bool
RtpsDiscovery::update_subscription_qos(DDS::DomainId_t domainId,
                                       const OpenDDS::DCPS::RepoId& partId,
                                       const OpenDDS::DCPS::RepoId& drId,
                                       const DDS::DataReaderQos& qos,
                                       const DDS::SubscriberQos& subQos)
{
  return get_part(domainId, partId)->update_subscription_qos(drId, qos, subQos);
}

bool
RtpsDiscovery::update_subscription_params(DDS::DomainId_t domainId,
                                          const OpenDDS::DCPS::RepoId& partId,
                                          const OpenDDS::DCPS::RepoId& subId,
                                          const DDS::StringSeq& params)

{
  return get_part(domainId, partId)->update_subscription_params(subId, params);
}


// Managing reader/writer associations:

void
RtpsDiscovery::association_complete(DDS::DomainId_t domainId,
                                    const OpenDDS::DCPS::RepoId& participantId,
                                    const OpenDDS::DCPS::RepoId& localId,
                                    const OpenDDS::DCPS::RepoId& remoteId)
{
  get_part(domainId, participantId)->association_complete(localId, remoteId);
}

DCPS::RcHandle<Spdp>
RtpsDiscovery::get_part(const DDS::DomainId_t domain_id,
                        const OpenDDS::DCPS::RepoId& part_id) const
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, DCPS::RcHandle<Spdp>());
  DomainParticipantMap::const_iterator domain = participants_.find(domain_id);
  if (domain == participants_.end()) {
    return DCPS::RcHandle<Spdp>();
  }
  ParticipantMap::const_iterator part = domain->second.find(part_id);
  if (part == domain->second.end()) {
    return DCPS::RcHandle<Spdp>();
  }
  return part->second;
}

RtpsDiscovery::StaticInitializer::StaticInitializer()
{
  TheServiceParticipant->register_discovery_type("rtps_discovery", new Config);
}

} // namespace DCPS
} // namespace OpenDDS
#endif // DDS_HAS_MINIMUM_BIT
