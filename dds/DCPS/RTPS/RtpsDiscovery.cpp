/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "RtpsDiscovery.h"

#include "RtpsDiscoveryConfig.h"

#include <dds/DCPS/BuiltInTopicUtils.h>
#include <dds/DCPS/DomainParticipantImpl.h>
#include <dds/DCPS/LogAddr.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/Registered_Data_Types.h>
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/SubscriberImpl.h>

#include <dds/DCPS/transport/framework/TransportConfig.h>
#include <dds/DCPS/transport/framework/TransportSendStrategy.h>

#include <dds/DdsDcpsInfoUtilsC.h>
#include <dds/OpenDDSConfigWrapper.h>

#include <cstdlib>
#include <limits>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace RTPS {

using DCPS::TimeDuration;

RtpsDiscovery::RtpsDiscovery(const RepoKey& key)
  : key_(key)
  , config_(DCPS::make_rch<RtpsDiscoveryConfig>(key))
{
}

RtpsDiscovery::~RtpsDiscovery()
{
}

int
RtpsDiscovery::Config::discovery_config()
{
  RcHandle<DCPS::ConfigStoreImpl> config_store = TheServiceParticipant->config_store();

  const DCPS::ConfigStoreImpl::StringList sections = config_store->get_section_names("RTPS_DISCOVERY");

  // Loop through the [rtps_discovery/*] sections
  for (DCPS::ConfigStoreImpl::StringList::const_iterator pos = sections.begin(), limit = sections.end();
       pos != limit; ++pos) {
    const String& rtps_name = *pos;

    RtpsDiscovery_rch discovery = OpenDDS::DCPS::make_rch<RtpsDiscovery>(rtps_name);
    RtpsDiscoveryConfig_rch config = discovery->config();

#if OPENDDS_CONFIG_SECURITY
    if (config_store->has(config->config_key("IceTa").c_str())) {
      if (log_level >= DCPS::LogLevel::Warning) {
        ACE_ERROR((LM_WARNING,
                   "(%P|%t) RtpsDiscovery::Config::discovery_config: "
                   "IceTa is deprecated.  Use Ta in [ice]\n"));
      }
      config_store->set("IceTa", config_store->get(config->config_key("IceTa").c_str(), ""));
    }
    if (config_store->has(config->config_key("IceConnectivityCheckTTL").c_str())) {
      if (log_level >= DCPS::LogLevel::Warning) {
        ACE_ERROR((LM_WARNING,
                   "(%P|%t) RtpsDiscovery::Config::discovery_config: "
                   "IceConnectivityCheckTTL is deprecated.  Use ConnectivityCheckTTL in [ice]\n"));
      }
      config_store->set("IceConnectivityCheckTTL", config_store->get(config->config_key("IceConnectivityCheckTTL").c_str(), ""));
    }
    if (config_store->has(config->config_key("IceChecklistPeriod").c_str())) {
      if (log_level >= DCPS::LogLevel::Warning) {
        ACE_ERROR((LM_WARNING,
                   "(%P|%t) RtpsDiscovery::Config::discovery_config: "
                   "IceChecklistPeriod is deprecated.  Use ChecklistPeriod in [ice]\n"));
      }
      config_store->set("IceChecklistPeriod", config_store->get(config->config_key("IceChecklistPeriod").c_str(), ""));
    }
    if (config_store->has(config->config_key("IceIndicationPeriod").c_str())) {
      if (log_level >= DCPS::LogLevel::Warning) {
        ACE_ERROR((LM_WARNING,
                   "(%P|%t) RtpsDiscovery::Config::discovery_config: "
                   "IceIndicationPeriod is deprecated.  Use IndicationPeriod in [ice]\n"));
      }
      config_store->set("IceIndicationPeriod", config_store->get(config->config_key("IceIndicationPeriod").c_str(), ""));
    }
    if (config_store->has(config->config_key("IceNominatedTTL").c_str())) {
      if (log_level >= DCPS::LogLevel::Warning) {
        ACE_ERROR((LM_WARNING,
                   "(%P|%t) RtpsDiscovery::Config::discovery_config: "
                   "IceNominatedTTL is deprecated.  Use NominatedTTL in [ice]\n"));
      }
      config_store->set("IceNominatedTTL", config_store->get(config->config_key("IceNominatedTTL").c_str(), ""));
    }
    if (config_store->has(config->config_key("IceServerReflexiveAddressPeriod").c_str())) {
      if (log_level >= DCPS::LogLevel::Warning) {
        ACE_ERROR((LM_WARNING,
                   "(%P|%t) RtpsDiscovery::Config::discovery_config: "
                   "IceServerReflexiveAddressPeriod is deprecated.  Use ServerReflexiveAddressPeriod in [ice]\n"));
      }
      config_store->set("IceServerReflexiveAddressPeriod", config_store->get(config->config_key("IceServerReflexiveAddressPeriod").c_str(), ""));
    }
    if (config_store->has(config->config_key("IceServerReflexiveIndicationCount").c_str())) {
      if (log_level >= DCPS::LogLevel::Warning) {
        ACE_ERROR((LM_WARNING,
                   "(%P|%t) RtpsDiscovery::Config::discovery_config: "
                   "IceServerReflexiveIndicationCount is deprecated.  Use ServerReflexiveIndicationCount in [ice]\n"));
      }
      config_store->set("IceServerReflexiveIndicationCount", config_store->get(config->config_key("IceServerReflexiveIndicationCount").c_str(), ""));
    }
    if (config_store->has(config->config_key("IceDeferredTriggeredCheckTTL").c_str())) {
      if (log_level >= DCPS::LogLevel::Warning) {
        ACE_ERROR((LM_WARNING,
                   "(%P|%t) RtpsDiscovery::Config::discovery_config: "
                   "IceDeferredTriggeredCheckTTL is deprecated.  Use DeferredTriggeredCheckTTL in [ice]\n"));
      }
      config_store->set("IceDeferredTriggeredCheckTTL", config_store->get(config->config_key("IceDeferredTriggeredCheckTTL").c_str(), ""));
    }
    if (config_store->has(config->config_key("IceChangePasswordPeriod").c_str())) {
      if (log_level >= DCPS::LogLevel::Warning) {
        ACE_ERROR((LM_WARNING,
                   "(%P|%t) RtpsDiscovery::Config::discovery_config: "
                   "IceChangePasswordPeriod is deprecated.  Use ChangePasswordPeriod in [ice]\n"));
      }
      config_store->set("IceChangePasswordPeriod", config_store->get(config->config_key("IceChangePasswordPeriod").c_str(), ""));
    }
#endif

    TheServiceParticipant->add_discovery(discovery);
  }

  // If the default RTPS discovery object has not been configured,
  // instantiate it now.
  TheServiceParticipant->add_discovery(OpenDDS::DCPS::make_rch<RtpsDiscovery>(Discovery::DEFAULT_RTPS));

  return 0;
}

// Participant operations:
OpenDDS::DCPS::GUID_t
RtpsDiscovery::generate_participant_guid()
{
  OpenDDS::DCPS::GUID_t id = GUID_UNKNOWN;
  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, id);
  const OPENDDS_STRING guid_interface = config_->guid_interface();
  if (!guid_interface.empty()) {
    if (guid_gen_.interfaceName(guid_interface.c_str()) != 0) {
      if (DCPS::DCPS_debug_level) {
        ACE_DEBUG((LM_WARNING, "(%P|%t) RtpsDiscovery::generate_participant_guid()"
                   " - attempt to use network interface %C MAC addr for"
                   " GUID generation failed.\n", guid_interface.c_str()));
      }
    }
  }
  guid_gen_.populate(id);
  id.entityId = ENTITYID_PARTICIPANT;
  return id;
}

DCPS::AddDomainStatus
RtpsDiscovery::add_domain_participant(DDS::DomainId_t domain,
                                      const DDS::DomainParticipantQos& qos,
                                      XTypes::TypeLookupService_rch tls)
{
  DCPS::AddDomainStatus ads = {OpenDDS::DCPS::GUID_t(), false /*federated*/};
  {
    ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, ads);
    const OPENDDS_STRING guid_interface = config_->guid_interface();
    if (!guid_interface.empty()) {
      if (guid_gen_.interfaceName(guid_interface.c_str()) != 0) {
        if (DCPS::DCPS_debug_level) {
          ACE_DEBUG((LM_WARNING, "(%P|%t) RtpsDiscovery::add_domain_participant()"
                     " - attempt to use specific network interface %C MAC addr for"
                     " GUID generation failed.\n", guid_interface.c_str()));
        }
      }
    }
    guid_gen_.populate(ads.id);
  }
  ads.id.entityId = ENTITYID_PARTICIPANT;
  try {
    const DCPS::RcHandle<Spdp> spdp(DCPS::make_rch<Spdp>(domain, ref(ads.id), qos, this, tls));
    // ads.id may change during Spdp constructor
    ACE_GUARD_RETURN(ACE_Thread_Mutex, g, participants_lock_, ads);
    participants_[domain][ads.id] = spdp;
  } catch (const std::exception& e) {
    ads.id = GUID_UNKNOWN;
    ACE_ERROR((LM_ERROR, "(%P|%t) RtpsDiscovery::add_domain_participant() - "
      "failed to initialize RTPS Simple Participant Discovery Protocol: %C\n",
      e.what()));
  }
  return ads;
}

#if OPENDDS_CONFIG_SECURITY
DCPS::AddDomainStatus
RtpsDiscovery::add_domain_participant_secure(
  DDS::DomainId_t domain,
  const DDS::DomainParticipantQos& qos,
  XTypes::TypeLookupService_rch tls,
  const OpenDDS::DCPS::GUID_t& guid,
  DDS::Security::IdentityHandle id,
  DDS::Security::PermissionsHandle perm,
  DDS::Security::ParticipantCryptoHandle part_crypto)
{
  DCPS::AddDomainStatus ads = {guid, false /*federated*/};
  ads.id.entityId = ENTITYID_PARTICIPANT;
  try {
    const DCPS::RcHandle<Spdp> spdp(DCPS::make_rch<Spdp>(
      domain, ads.id, qos, this, tls, id, perm, part_crypto));
    ACE_GUARD_RETURN(ACE_Thread_Mutex, g, participants_lock_, ads);
    participants_[domain][ads.id] = spdp;
  } catch (const std::exception& e) {
    ads.id = GUID_UNKNOWN;
    ACE_ERROR((LM_WARNING, "(%P|%t) RtpsDiscovery::add_domain_participant_secure() - "
      "failed to initialize RTPS Simple Participant Discovery Protocol: %C\n",
      e.what()));
  }
  return ads;
}
#endif

void
RtpsDiscovery::signal_liveliness(const DDS::DomainId_t domain_id,
                                 const OpenDDS::DCPS::GUID_t& part_id,
                                 DDS::LivelinessQosPolicyKind kind)
{
  get_part(domain_id, part_id)->signal_liveliness(kind);
}

void
RtpsDiscovery::rtps_relay_only_now(bool after)
{
  RtpsDiscoveryConfig_rch config = get_config();
  config->rtps_relay_only(after);
}

void
RtpsDiscovery::use_rtps_relay_now(bool after)
{
  RtpsDiscoveryConfig_rch config = get_config();
  config->use_rtps_relay(after);
}

#if OPENDDS_CONFIG_SECURITY
void
RtpsDiscovery::use_ice_now(bool after)
{
  RtpsDiscoveryConfig_rch config = get_config();
  config->use_ice(after);
}

DDS::Security::ParticipantCryptoHandle
RtpsDiscovery::get_crypto_handle(DDS::DomainId_t domain,
                                 const DCPS::GUID_t& local_participant,
                                 const DCPS::GUID_t& remote_participant) const
{
  ParticipantHandle p = get_part(domain, local_participant);
  if (p) {
    if (remote_participant == GUID_UNKNOWN || remote_participant == local_participant) {
      return p->crypto_handle();
    } else {
      return p->remote_crypto_handle(remote_participant);
    }
  }

  return DDS::HANDLE_NIL;
}

#endif

RtpsDiscovery::StaticInitializer::StaticInitializer()
{
  TheServiceParticipant->register_discovery_type("rtps_discovery", new Config);
}

u_short
RtpsDiscovery::get_spdp_port(DDS::DomainId_t domain,
                             const DCPS::GUID_t& local_participant) const
{
  ParticipantHandle p = get_part(domain, local_participant);
  if (p) {
    return p->get_spdp_port();
  }

  return 0;
}

u_short
RtpsDiscovery::get_sedp_port(DDS::DomainId_t domain,
                             const DCPS::GUID_t& local_participant) const
{
  ParticipantHandle p = get_part(domain, local_participant);
  if (p) {
    return p->get_sedp_port();
  }

  return 0;
}

#ifdef ACE_HAS_IPV6

u_short
RtpsDiscovery::get_ipv6_spdp_port(DDS::DomainId_t domain,
                                  const DCPS::GUID_t& local_participant) const
{
  ParticipantHandle p = get_part(domain, local_participant);
  if (p) {
    return p->get_ipv6_spdp_port();
  }

  return 0;
}

u_short
RtpsDiscovery::get_ipv6_sedp_port(DDS::DomainId_t domain,
                                  const DCPS::GUID_t& local_participant) const
{
  ParticipantHandle p = get_part(domain, local_participant);
  if (p) {
    return p->get_ipv6_sedp_port();
  }

  return 0;
}
#endif

void
RtpsDiscovery::spdp_rtps_relay_address(const DCPS::NetworkAddress& address)
{
  RtpsDiscoveryConfig_rch config = get_config();
  config->spdp_rtps_relay_address(address);
}

void
RtpsDiscovery::sedp_rtps_relay_address(const DCPS::NetworkAddress& address)
{
  RtpsDiscoveryConfig_rch config = get_config();
  config->sedp_rtps_relay_address(address);
}

void
RtpsDiscovery::spdp_stun_server_address(const DCPS::NetworkAddress& address)
{
  RtpsDiscoveryConfig_rch config = get_config();
  config->spdp_stun_server_address(address);
}

void
RtpsDiscovery::sedp_stun_server_address(const DCPS::NetworkAddress& address)
{
  RtpsDiscoveryConfig_rch config = get_config();
  config->sedp_stun_server_address(address);
}

void
RtpsDiscovery::append_transport_statistics(DDS::DomainId_t domain,
                                           const DCPS::GUID_t& local_participant,
                                           DCPS::TransportStatisticsSequence& seq)
{
  ParticipantHandle p = get_part(domain, local_participant);
  if (p) {
    p->append_transport_statistics(seq);
  }
}

RcHandle<DCPS::BitSubscriber> RtpsDiscovery::init_bit(DCPS::DomainParticipantImpl* participant)
{
  DDS::Subscriber_var bit_subscriber;
#ifndef DDS_HAS_MINIMUM_BIT
  if (!TheServiceParticipant->get_BIT()) {
    DCPS::RcHandle<DCPS::BitSubscriber> bit_subscriber_rch = DCPS::make_rch<DCPS::BitSubscriber>();
    get_part(participant->get_domain_id(), participant->get_id())->init_bit(bit_subscriber_rch);
    return DCPS::RcHandle<DCPS::BitSubscriber>();
  }

  if (create_bit_topics(participant) != DDS::RETCODE_OK) {
    return RcHandle<DCPS::BitSubscriber>();
  }

  bit_subscriber =
    participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT,
                                   DDS::SubscriberListener::_nil(),
                                   DCPS::DEFAULT_STATUS_MASK);
  DCPS::SubscriberImpl* sub = dynamic_cast<DCPS::SubscriberImpl*>(bit_subscriber.in());
  if (sub == 0) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) PeerDiscovery::init_bit")
               ACE_TEXT(" - Could not cast Subscriber to SubscriberImpl\n")));
    return RcHandle<DCPS::BitSubscriber>();
  }

  DDS::DataReaderQos dr_qos;
  sub->get_default_datareader_qos(dr_qos);
  dr_qos.durability.kind = DDS::TRANSIENT_LOCAL_DURABILITY_QOS;

  dr_qos.reader_data_lifecycle.autopurge_nowriter_samples_delay =
    TheServiceParticipant->bit_autopurge_nowriter_samples_delay();
  dr_qos.reader_data_lifecycle.autopurge_disposed_samples_delay =
    TheServiceParticipant->bit_autopurge_disposed_samples_delay();

  DDS::TopicDescription_var bit_part_topic =
    participant->lookup_topicdescription(DCPS::BUILT_IN_PARTICIPANT_TOPIC);
  create_bit_dr(bit_part_topic, DCPS::BUILT_IN_PARTICIPANT_TOPIC_TYPE,
                sub, dr_qos);

  DDS::TopicDescription_var bit_topic_topic =
    participant->lookup_topicdescription(DCPS::BUILT_IN_TOPIC_TOPIC);
  create_bit_dr(bit_topic_topic, DCPS::BUILT_IN_TOPIC_TOPIC_TYPE,
                sub, dr_qos);

  DDS::TopicDescription_var bit_pub_topic =
    participant->lookup_topicdescription(DCPS::BUILT_IN_PUBLICATION_TOPIC);
  create_bit_dr(bit_pub_topic, DCPS::BUILT_IN_PUBLICATION_TOPIC_TYPE,
                sub, dr_qos);

  DDS::TopicDescription_var bit_sub_topic =
    participant->lookup_topicdescription(DCPS::BUILT_IN_SUBSCRIPTION_TOPIC);
  create_bit_dr(bit_sub_topic, DCPS::BUILT_IN_SUBSCRIPTION_TOPIC_TYPE,
                sub, dr_qos);

  DDS::TopicDescription_var bit_part_loc_topic =
    participant->lookup_topicdescription(DCPS::BUILT_IN_PARTICIPANT_LOCATION_TOPIC);
  create_bit_dr(bit_part_loc_topic, DCPS::BUILT_IN_PARTICIPANT_LOCATION_TOPIC_TYPE,
                sub, dr_qos);

  DDS::TopicDescription_var bit_connection_record_topic =
    participant->lookup_topicdescription(DCPS::BUILT_IN_CONNECTION_RECORD_TOPIC);
  create_bit_dr(bit_connection_record_topic, DCPS::BUILT_IN_CONNECTION_RECORD_TOPIC_TYPE,
                sub, dr_qos);

  DDS::TopicDescription_var bit_internal_thread_topic =
    participant->lookup_topicdescription(DCPS::BUILT_IN_INTERNAL_THREAD_TOPIC);
  create_bit_dr(bit_internal_thread_topic, DCPS::BUILT_IN_INTERNAL_THREAD_TOPIC_TYPE,
                sub, dr_qos);

  const DDS::ReturnCode_t ret = bit_subscriber->enable();
  if (ret != DDS::RETCODE_OK) {
    if (DCPS_debug_level) {
      ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) PeerDiscovery::init_bit")
                 ACE_TEXT(" - Error %d enabling subscriber\n"), ret));
    }
    return RcHandle<DCPS::BitSubscriber>();
  }
#endif /* DDS_HAS_MINIMUM_BIT */

  DCPS::RcHandle<DCPS::BitSubscriber> bit_subscriber_rch = DCPS::make_rch<DCPS::BitSubscriber>(bit_subscriber);
  get_part(participant->get_domain_id(), participant->get_id())->init_bit(bit_subscriber_rch);

  return bit_subscriber_rch;
}

void RtpsDiscovery::fini_bit(DCPS::DomainParticipantImpl* participant)
{
  get_part(participant->get_domain_id(), participant->get_id())->fini_bit();
}

bool RtpsDiscovery::attach_participant(
  DDS::DomainId_t /*domainId*/, const GUID_t& /*participantId*/)
{
  return false; // This is just for DCPSInfoRepo?
}

bool RtpsDiscovery::remove_domain_participant(
  DDS::DomainId_t domain_id, const GUID_t& participantId)
{
  // Use reference counting to ensure participant
  // does not get deleted until lock as been released.
  ParticipantHandle participant;
  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, participants_lock_, false);
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
  g.release();

  participant->shutdown();
  return true;
}

bool RtpsDiscovery::ignore_domain_participant(
  DDS::DomainId_t domain, const GUID_t& myParticipantId, const GUID_t& ignoreId)
{
  get_part(domain, myParticipantId)->ignore_domain_participant(ignoreId);
  return true;
}

bool RtpsDiscovery::remove_domain_participant(
  DDS::DomainId_t domain, const GUID_t& myParticipantId, const GUID_t& removeId)
{
  get_part(domain, myParticipantId)->remove_domain_participant(removeId);
  return true;
}

bool RtpsDiscovery::update_domain_participant_qos(
  DDS::DomainId_t domain, const GUID_t& participant, const DDS::DomainParticipantQos& qos)
{
  return get_part(domain, participant)->update_domain_participant_qos(qos);
}

bool RtpsDiscovery::has_domain_participant(DDS::DomainId_t domain, const GUID_t& local, const GUID_t& remote) const
{
  return get_part(domain, local)->has_domain_participant(remote);
}

DCPS::TopicStatus RtpsDiscovery::assert_topic(
  GUID_t& topicId,
  DDS::DomainId_t domainId,
  const GUID_t& participantId,
  const char* topicName,
  const char* dataTypeName,
  const DDS::TopicQos& qos,
  bool hasDcpsKey,
  DCPS::TopicCallbacks* topic_callbacks)
{
  ParticipantHandle part = get_part(domainId, participantId);
  if (part) {
    return part->assert_topic(topicId, topicName,
                              dataTypeName, qos,
                              hasDcpsKey, topic_callbacks);
  }
  return DCPS::INTERNAL_ERROR;
}

DCPS::TopicStatus RtpsDiscovery::find_topic(
  DDS::DomainId_t domainId,
  const GUID_t& participantId,
  const char* topicName,
  CORBA::String_out dataTypeName,
  DDS::TopicQos_out qos,
  GUID_t& topicId)
{
  ParticipantHandle part = get_part(domainId, participantId);
  if (part) {
    return part->find_topic(topicName, dataTypeName, qos, topicId);
  }
  return DCPS::INTERNAL_ERROR;
}

DCPS::TopicStatus RtpsDiscovery::remove_topic(
  DDS::DomainId_t domainId,
  const GUID_t& participantId,
  const GUID_t& topicId)
{
  ParticipantHandle part = get_part(domainId, participantId);
  if (part) {
    return part->remove_topic(topicId);
  }
  return DCPS::INTERNAL_ERROR;
}

bool RtpsDiscovery::ignore_topic(DDS::DomainId_t domainId, const GUID_t& myParticipantId,
                                 const GUID_t& ignoreId)
{
  get_part(domainId, myParticipantId)->ignore_topic(ignoreId);
  return true;
}

bool RtpsDiscovery::update_topic_qos(const GUID_t& topicId, DDS::DomainId_t domainId,
                                    const GUID_t& participantId, const DDS::TopicQos& qos)
{
  ParticipantHandle part = get_part(domainId, participantId);
  if (part) {
    return part->update_topic_qos(topicId, qos);
  }
  return false;
}

bool RtpsDiscovery::add_publication(
  DDS::DomainId_t domainId,
  const GUID_t& participantId,
  const GUID_t& topicId,
  DCPS::DataWriterCallbacks_rch publication,
  const DDS::DataWriterQos& qos,
  const DCPS::TransportLocatorSeq& transInfo,
  const DDS::PublisherQos& publisherQos,
  const XTypes::TypeInformation& type_info)
{
  return get_part(domainId, participantId)->add_publication(topicId,
                                                            publication,
                                                            qos,
                                                            transInfo,
                                                            publisherQos,
                                                            type_info);
}

bool RtpsDiscovery::remove_publication(
  DDS::DomainId_t domainId, const GUID_t& participantId, const GUID_t& publicationId)
{
  get_part(domainId, participantId)->remove_publication(publicationId);
  return true;
}

bool RtpsDiscovery::ignore_publication(
  DDS::DomainId_t domainId, const GUID_t& participantId, const GUID_t& ignoreId)
{
  get_part(domainId, participantId)->ignore_publication(ignoreId);
  return true;
}

bool RtpsDiscovery::update_publication_qos(
  DDS::DomainId_t domainId,
  const GUID_t& partId,
  const GUID_t& dwId,
  const DDS::DataWriterQos& qos,
  const DDS::PublisherQos& publisherQos)
{
  return get_part(domainId, partId)->update_publication_qos(dwId, qos,
                                                            publisherQos);
}

void RtpsDiscovery::update_publication_locators(
  DDS::DomainId_t domainId, const GUID_t& partId, const GUID_t& dwId,
  const DCPS::TransportLocatorSeq& transInfo)
{
  const ParticipantHandle ph = get_part(domainId, partId);
  if (ph) {
    ph->update_publication_locators(dwId, transInfo);
  } else if (log_level >= DCPS::LogLevel::Warning) {
    ACE_ERROR((LM_WARNING,
               "(%P|%t) WARNING: RtpsDiscovery::update_publication_locators: "
               "no participant for domain %d participant %C writer %C\n",
               domainId, LogGuid(partId).c_str(), LogGuid(dwId).c_str()));
  }
}

bool RtpsDiscovery::add_subscription(
  DDS::DomainId_t domainId,
  const GUID_t& participantId,
  const GUID_t& topicId,
  DCPS::DataReaderCallbacks_rch subscription,
  const DDS::DataReaderQos& qos,
  const DCPS::TransportLocatorSeq& transInfo,
  const DDS::SubscriberQos& subscriberQos,
  const char* filterClassName,
  const char* filterExpr,
  const DDS::StringSeq& params,
  const XTypes::TypeInformation& type_info)
{
  return get_part(domainId, participantId)->add_subscription(topicId,
                                                             subscription,
                                                             qos,
                                                             transInfo,
                                                             subscriberQos,
                                                             filterClassName,
                                                             filterExpr,
                                                             params,
                                                             type_info);
}

bool RtpsDiscovery::remove_subscription(
  DDS::DomainId_t domainId, const GUID_t& participantId, const GUID_t& subscriptionId)
{
  get_part(domainId, participantId)->remove_subscription(subscriptionId);
  return true;
}

bool RtpsDiscovery::ignore_subscription(
  DDS::DomainId_t domainId, const GUID_t& participantId, const GUID_t& ignoreId)
{
  get_part(domainId, participantId)->ignore_subscription(ignoreId);
  return true;
}

bool RtpsDiscovery::update_subscription_qos(
  DDS::DomainId_t domainId,
  const GUID_t& partId,
  const GUID_t& drId,
  const DDS::DataReaderQos& qos,
  const DDS::SubscriberQos& subQos)
{
  return get_part(domainId, partId)->update_subscription_qos(drId, qos, subQos);
}

bool RtpsDiscovery::update_subscription_params(
  DDS::DomainId_t domainId, const GUID_t& partId, const GUID_t& subId, const DDS::StringSeq& params)
{
  return get_part(domainId, partId)->update_subscription_params(subId, params);
}

void RtpsDiscovery::update_subscription_locators(
  DDS::DomainId_t domainId, const GUID_t& partId, const GUID_t& subId,
  const DCPS::TransportLocatorSeq& transInfo)
{
  const ParticipantHandle ph = get_part(domainId, partId);
  if (ph) {
    ph->update_subscription_locators(subId, transInfo);
  } else if (log_level >= DCPS::LogLevel::Warning) {
    ACE_ERROR((LM_WARNING,
               "(%P|%t) WARNING: RtpsDiscovery::update_subscription_locators: "
               "no participant for domain %d participant %C reader %C\n",
               domainId, LogGuid(partId).c_str(), LogGuid(subId).c_str()));
  }
}

RcHandle<DCPS::TransportInst> RtpsDiscovery::sedp_transport_inst(DDS::DomainId_t domainId,
                                                                 const GUID_t& partId) const
{
  return get_part(domainId, partId)->sedp_transport_inst();
}

ParticipantHandle RtpsDiscovery::get_part(const DDS::DomainId_t domain_id, const GUID_t& part_id) const
{
  ACE_Guard<ACE_Thread_Mutex> guard(participants_lock_);
  const DomainParticipantMap::const_iterator domain = participants_.find(domain_id);
  if (domain == participants_.end()) {
    return ParticipantHandle();
  }
  const ParticipantMap::const_iterator part = domain->second.find(part_id);
  if (part == domain->second.end()) {
    return ParticipantHandle();
  }
  return part->second;
}

RtpsDiscoveryConfig_rch RtpsDiscovery::get_config() const
{
  ACE_Guard<ACE_Thread_Mutex> guard(lock_);
  return config_;
}

void RtpsDiscovery::create_bit_dr(DDS::TopicDescription_ptr topic,
  const char* type, DCPS::SubscriberImpl* sub, const DDS::DataReaderQos& qos)
{
  DCPS::TopicDescriptionImpl* bit_topic_i =
    dynamic_cast<DCPS::TopicDescriptionImpl*>(topic);
  if (bit_topic_i == 0) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: PeerDiscovery::create_bit_dr: ")
               ACE_TEXT("Could not cast TopicDescription to TopicDescriptionImpl\n")));
    return;
  }

  DDS::DomainParticipant_var participant = sub->get_participant();
  DCPS::DomainParticipantImpl* participant_i =
    dynamic_cast<DCPS::DomainParticipantImpl*>(participant.in());
  if (participant_i == 0) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: PeerDiscovery::create_bit_dr: ")
               ACE_TEXT("Could not cast DomainParticipant to DomainParticipantImpl\n")));
    return;
  }

  DCPS::TypeSupport_var type_support =
    Registered_Data_Types->lookup(participant, type);

  DDS::DataReader_var dr = type_support->create_datareader();
  DCPS::DataReaderImpl* dri = dynamic_cast<DCPS::DataReaderImpl*>(dr.in());
  if (dri == 0) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: PeerDiscovery::create_bit_dr: ")
               ACE_TEXT("Could not cast DataReader to DataReaderImpl\n")));
    return;
  }

  dri->init(bit_topic_i, qos, 0 /*listener*/, 0 /*mask*/, participant_i, sub);
  dri->disable_transport();
  dri->enable();
}

void RtpsDiscovery::request_remote_complete_type_objects(
  DDS::DomainId_t domain, const GUID_t& local_participant,
  const GUID_t& remote_entity, const XTypes::TypeInformation& remote_type_info,
  DCPS::TypeObjReqCond& cond)
{
  ParticipantHandle spdp = get_part(domain, local_participant);
  spdp->request_remote_complete_type_objects(remote_entity, remote_type_info, cond);
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
