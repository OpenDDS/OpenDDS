/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

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

namespace OpenDDS {
namespace RTPS {

RtpsDiscovery::RtpsDiscovery(const RepoKey& key)
  : DCPS::Discovery(key)
  , servant_(new RtpsInfo)
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

DCPS::DCPSInfo_ptr RtpsDiscovery::get_dcps_info()
{
  return DCPS::DCPSInfo::_duplicate(info_);
}

namespace {
  void create_bit_dr(DDS::TopicDescription_ptr topic, const char* type,
                     DCPS::SubscriberImpl* sub, const DDS::DataReaderQos& qos,
                     const DCPS::DataReaderQosExt& ext_qos)
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

    dri->init(bit_topic_i, qos, ext_qos, 0 /*listener*/, 0 /*mask*/,
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
  DataReaderQosExt ext_qos;
  sub->get_default_datareader_qos_ext(ext_qos);

  DDS::TopicDescription_var bit_part_topic =
    participant->lookup_topicdescription(BUILT_IN_PARTICIPANT_TOPIC);
  create_bit_dr(bit_part_topic, BUILT_IN_PARTICIPANT_TOPIC_TYPE,
                sub, dr_qos, ext_qos);

  DDS::TopicDescription_var bit_topic_topic =
    participant->lookup_topicdescription(BUILT_IN_TOPIC_TOPIC);
  create_bit_dr(bit_topic_topic, BUILT_IN_TOPIC_TOPIC_TYPE,
                sub, dr_qos, ext_qos);

  DDS::TopicDescription_var bit_pub_topic =
    participant->lookup_topicdescription(BUILT_IN_PUBLICATION_TOPIC);
  create_bit_dr(bit_pub_topic, BUILT_IN_PUBLICATION_TOPIC_TYPE,
                sub, dr_qos, ext_qos);

  DDS::TopicDescription_var bit_sub_topic =
    participant->lookup_topicdescription(BUILT_IN_SUBSCRIPTION_TOPIC);
  create_bit_dr(bit_sub_topic, BUILT_IN_SUBSCRIPTION_TOPIC_TYPE,
                sub, dr_qos, ext_qos);

  servant_->init_bit(bit_subscriber);

  return bit_subscriber._retn();
}

static const ACE_TCHAR RTPS_SECTION_NAME[] = ACE_TEXT("rtps_discovery");

int RtpsDiscovery::load_rtps_discovery_configuration(ACE_Configuration_Heap& cf)
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
    if (DCPS::processSections( cf, rtps_sect, keys ) != 0) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("(%P|%t) RtpsDiscovery::load_rtps_discovery_configuration(): ")
                        ACE_TEXT("too many nesting layers in the [rtps] section.\n")),
                       -1);
    }

    // Loop through the [rtps/*] sections
    for (DCPS::KeyList::const_iterator it=keys.begin(); it != keys.end(); ++it) {
      std::string rtps_name = it->first;

      DCPS::ValueMap values;
      DCPS::pullValues( cf, it->second, values );
      for (DCPS::ValueMap::const_iterator it=values.begin(); it != values.end(); ++it) {
        std::string name = it->first;
        if (name == "???") {
          // TODO: Implement other RTPS Discovery config parameters (including those
          // required by the spec.
          std::string value = it->second;

        } else {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) RtpsDiscovery::load_rtps_discovery_configuration(): ")
                            ACE_TEXT("Unexpected entry (%s) in [rtps_discovery/%s] section.\n"),
                            name.c_str(), rtps_name.c_str()),
                           -1);
        }
      }

      RtpsDiscovery_rch discovery = new RtpsDiscovery(rtps_name);
      TheServiceParticipant->add_discovery(
        DCPS::dynamic_rchandle_cast<Discovery>(discovery));
    }
  }

  // If the default RTPS discovery object has not been configured,
  // instantiate it now.
  const DCPS::Service_Participant::RepoKeyDiscoveryMap& discoveryMap = TheServiceParticipant->discoveryMap();
  if (discoveryMap.find(Discovery::DEFAULT_RTPS) == discoveryMap.end()) {
    RtpsDiscovery_rch discovery = new RtpsDiscovery(Discovery::DEFAULT_RTPS);
    TheServiceParticipant->add_discovery(
      DCPS::dynamic_rchandle_cast<Discovery>(discovery));
  }

  return 0;
}

RtpsDiscovery::StaticInitializer RtpsDiscovery::dummy_;


} // namespace DCPS
} // namespace OpenDDS
