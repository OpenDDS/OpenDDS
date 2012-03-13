/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef DDS_HAS_MINIMUM_BIT

#include "RtpsDiscovery.h"
#include "RtpsInfo.h"

#include "dds/DCPS/Service_Participant.h"
#include "dds/DCPS/InfoRepoUtils.h"
#include "dds/DCPS/ConfigUtils.h"
#include "dds/DCPS/DomainParticipantImpl.h"
#include "dds/DCPS/SubscriberImpl.h"
#include "dds/DCPS/Marked_Default_Qos.h"
#include "dds/DCPS/BuiltInTopicUtils.h"
#include "dds/DCPS/Registered_Data_Types.h"

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
  , servant_(new RtpsInfo(this))
  , resend_period_(30 /*seconds*/) // see RTPS v2.1 9.6.1.4.2
  , pb_(7400) // see RTPS v2.1 9.6.1.3 for PB, DG, PG, D0, D1 defaults
  , dg_(250)
  , pg_(2)
  , d0_(get_default_d0(0))
  , d1_(10)
  , dx_(2)
  , sedp_multicast_(true)
{
  PortableServer::POA_var poa = TheServiceParticipant->the_poa();
  PortableServer::ObjectId_var oid = poa->activate_object(servant_);
  CORBA::Object_var obj = poa->id_to_reference(oid);
  info_ = OpenDDS::DCPS::DCPSInfo::_narrow(obj);
}

RtpsDiscovery::~RtpsDiscovery()
{
  PortableServer::POA_var poa = TheServiceParticipant->the_poa();
  PortableServer::ObjectId_var oid = poa->servant_to_id(servant_);
  poa->deactivate_object(oid);
  delete servant_;
}

DCPS::DCPSInfo_ptr
RtpsDiscovery::get_dcps_info()
{
  return DCPS::DCPSInfo::_duplicate(info_);
}

DCPS::RepoId
RtpsDiscovery::bit_key_to_repo_id(DCPS::DomainParticipantImpl* participant,
                                  const char* bit_topic_name,
                                  const DDS::BuiltinTopicKey_t& key) const
{
  return servant_->bit_key_to_repo_id(participant->get_domain_id(),
                                      participant->get_id(),
                                      bit_topic_name, key);
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
              participant_i, sub, dr, 0 /*remote*/);
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

  servant_->init_bit(participant->get_id(), participant->get_domain_id(), bit_subscriber);

  return bit_subscriber._retn();
}

static const ACE_TCHAR RTPS_SECTION_NAME[] = ACE_TEXT("rtps_discovery");

int
RtpsDiscovery::load_rtps_discovery_configuration(ACE_Configuration_Heap& cf)
{
  const ACE_Configuration_Section_Key &root = cf.root_section();
  ACE_Configuration_Section_Key rtps_sect;

  if (cf.open_section(root, RTPS_SECTION_NAME, 0, rtps_sect) == 0) {

    // Ensure there are no properties in this section
    DCPS::ValueMap vm;
    if (DCPS::pullValues(cf, rtps_sect, vm) > 0) {
      // There are values inside [rtps_discovery]
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("(%P|%t) RtpsDiscovery::load_rtps_discovery_configuration(): ")
                        ACE_TEXT("rtps_discovery sections must have a subsection name\n")),
                       -1);
    }
    // Process the subsections of this section (the individual rtps_discovery/*)
    DCPS::KeyList keys;
    if (DCPS::processSections(cf, rtps_sect, keys) != 0) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("(%P|%t) RtpsDiscovery::load_rtps_discovery_configuration(): ")
                        ACE_TEXT("too many nesting layers in the [rtps] section.\n")),
                       -1);
    }

    // Loop through the [rtps_discovery/*] sections
    for (DCPS::KeyList::const_iterator it = keys.begin();
         it != keys.end(); ++it) {
      const std::string& rtps_name = it->first;

      int resend;
      u_short pb, dg, pg, d0, d1, dx;
      AddrVec spdp_send_addrs;
      bool has_resend = false, has_pb = false, has_dg = false, has_pg = false,
        has_d0 = false, has_d1 = false, has_dx = false, has_sm = false,
        sm = false;

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
              ACE_TEXT("(%P|%t) RtpsDiscovery::load_rtps_discovery_configuration(): ")
              ACE_TEXT("Invalid entry (%C) for ResendPeriod in ")
              ACE_TEXT("[rtps_discovery/%C] section.\n"),
              value.c_str(), rtps_name.c_str()), -1);
          }
        } else if (name == "PB") {
          const std::string& value = it->second;
          has_pb = DCPS::convertToInteger(value, pb);
          if (!has_pb) {
            ACE_ERROR_RETURN((LM_ERROR,
              ACE_TEXT("(%P|%t) RtpsDiscovery::load_rtps_discovery_configuration(): ")
              ACE_TEXT("Invalid entry (%C) for PB in ")
              ACE_TEXT("[rtps_discovery/%C] section.\n"),
              value.c_str(), rtps_name.c_str()), -1);
          }
        } else if (name == "DG") {
          const std::string& value = it->second;
          has_dg = DCPS::convertToInteger(value, dg);
          if (!has_dg) {
            ACE_ERROR_RETURN((LM_ERROR,
              ACE_TEXT("(%P|%t) RtpsDiscovery::load_rtps_discovery_configuration(): ")
              ACE_TEXT("Invalid entry (%C) for DG in ")
              ACE_TEXT("[rtps_discovery/%C] section.\n"),
              value.c_str(), rtps_name.c_str()), -1);
          }
        } else if (name == "PG") {
          const std::string& value = it->second;
          has_pg = DCPS::convertToInteger(value, pg);
          if (!has_pg) {
            ACE_ERROR_RETURN((LM_ERROR,
              ACE_TEXT("(%P|%t) RtpsDiscovery::load_rtps_discovery_configuration(): ")
              ACE_TEXT("Invalid entry (%C) for PG in ")
              ACE_TEXT("[rtps_discovery/%C] section.\n"),
              value.c_str(), rtps_name.c_str()), -1);
          }
        } else if (name == "D0") {
          const std::string& value = it->second;
          has_d0 = DCPS::convertToInteger(value, d0);
          if (!has_d0) {
            ACE_ERROR_RETURN((LM_ERROR,
              ACE_TEXT("(%P|%t) RtpsDiscovery::load_rtps_discovery_configuration(): ")
              ACE_TEXT("Invalid entry (%C) for D0 in ")
              ACE_TEXT("[rtps_discovery/%C] section.\n"),
              value.c_str(), rtps_name.c_str()), -1);
          }
        } else if (name == "D1") {
          const std::string& value = it->second;
          has_d1 = DCPS::convertToInteger(value, d1);
          if (!has_d1) {
            ACE_ERROR_RETURN((LM_ERROR,
              ACE_TEXT("(%P|%t) RtpsDiscovery::load_rtps_discovery_configuration(): ")
              ACE_TEXT("Invalid entry (%C) for D1 in ")
              ACE_TEXT("[rtps_discovery/%C] section.\n"),
              value.c_str(), rtps_name.c_str()), -1);
          }
        } else if (name == "DX") {
          const std::string& value = it->second;
          has_dx = DCPS::convertToInteger(value, dx);
          if (!has_dx) {
            ACE_ERROR_RETURN((LM_ERROR,
               ACE_TEXT("(%P|%t) RtpsDiscovery::load_rtps_discovery_configuration(): ")
               ACE_TEXT("Invalid entry (%C) for DX in ")
               ACE_TEXT("[rtps_discovery/%C] section.\n"),
               value.c_str(), rtps_name.c_str()), -1);
          }
        } else if (name == "SedpMulticast") {
          const std::string& value = it->second;
          int smInt;
          has_sm = DCPS::convertToInteger(value, smInt);
          if (!has_sm) {
            ACE_ERROR_RETURN((LM_ERROR,
               ACE_TEXT("(%P|%t) RtpsDiscovery::load_rtps_discovery_configuration(): ")
               ACE_TEXT("Invalid entry (%C) for SedpMulticast in ")
               ACE_TEXT("[rtps_discovery/%C] section.\n"),
               value.c_str(), rtps_name.c_str()), -1);
          }
          sm = bool(smInt);
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
                            ACE_TEXT("(%P|%t) RtpsDiscovery::load_rtps_discovery_configuration(): ")
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
      if (has_sm) discovery->sedp_multicast(sm);
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

} // namespace DCPS
} // namespace OpenDDS
#endif // DDS_HAS_MINIMUM_BIT
