/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "InfoRepoDiscovery.h"
#include "Service_Participant.h"
#include "InfoRepoUtils.h"

#if !defined (DDS_HAS_MINIMUM_BIT)
#include "DomainParticipantImpl.h"
#include "BuiltInTopicUtils.h"
#include "Marked_Default_Qos.h"

#include "transport/framework/TransportRegistry.h"
#include "transport/framework/TransportExceptions.h"
#endif

namespace OpenDDS {
namespace DCPS {

InfoRepoDiscovery::InfoRepoDiscovery(const RepoKey& key,
                                     const std::string& ior)
  : Discovery(key),
    ior_(ior),
    bit_transport_port_(0)
{
}

DCPSInfo_ptr InfoRepoDiscovery::get_dcps_info()
{
  if (CORBA::is_nil(this->info_.in())) {
    CORBA::ORB_var orb = TheServiceParticipant->get_ORB();
    try {
      this->info_ = InfoRepoUtils::get_repo(this->ior_.c_str(), orb.in());

      if (CORBA::is_nil(this->info_.in())) {
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("(%P|%t) ERROR: InfoRepoDiscovery::get_repository: ")
                   ACE_TEXT("unable to narrow DCPSInfo (%C) for key %C. \n"),
                   this->ior_.c_str(),
                   this->key().c_str()));
        return DCPSInfo::_nil();
      }

    } catch (const CORBA::Exception& ex) {
      ex._tao_print_exception(
                              "ERROR: InfoRepoDiscovery::get_repository: failed to resolve ior - ");
      return DCPSInfo::_nil();
    }
  }

  return DCPSInfo::_duplicate(this->info_);
}

std::string
InfoRepoDiscovery::get_stringified_dcps_info_ior()
{
  return this->ior_;
}

DDS::Subscriber_ptr
InfoRepoDiscovery::init_bit(DomainParticipantImpl* participant)
{
#if defined (DDS_HAS_MINIMUM_BIT)
  ACE_UNUSED_ARG(participant);
  return 0;
#else
  if (!TheServiceParticipant->get_BIT()) {
    return 0;
  }

  if (create_bit_topics(participant) != DDS::RETCODE_OK) {
    return 0;
  }

  DDS::Subscriber_var bit_subscriber =
    participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT,
                                   DDS::SubscriberListener::_nil(),
                                   DEFAULT_STATUS_MASK);
  try {
    //TODO: use config details from this class
    TransportConfig_rch config = TheServiceParticipant->bit_transport_config();

    TransportRegistry::instance()->bind_config(config, bit_subscriber);

  } catch (const Transport::Exception&) {
    ACE_ERROR((LM_ERROR, "(%P|%t) InfoRepoDiscovery::init_bit, "
                         "exception during transport initialization\n"));
    return 0;
  }

  // DataReaders
  try {
    DDS::DataReaderQos participantReaderQos;
    bit_subscriber->get_default_datareader_qos(participantReaderQos);
    participantReaderQos.durability.kind = DDS::TRANSIENT_LOCAL_DURABILITY_QOS;

    if (participant->federated()) {
      participantReaderQos.liveliness.lease_duration.nanosec = 0;
      participantReaderQos.liveliness.lease_duration.sec =
        TheServiceParticipant->federation_liveliness();
    }

    DDS::TopicDescription_var bit_part_topic =
      participant->lookup_topicdescription(BUILT_IN_PARTICIPANT_TOPIC);

    DDS::DataReader_var dr =
      bit_subscriber->create_datareader(bit_part_topic,
                                        participantReaderQos,
                                        DDS::DataReaderListener::_nil(),
                                        DEFAULT_STATUS_MASK);
    DDS::DataReaderQos dr_qos;
    bit_subscriber->get_default_datareader_qos(dr_qos);
    dr_qos.durability.kind = DDS::TRANSIENT_LOCAL_DURABILITY_QOS;

    DDS::TopicDescription_var bit_topic_topic =
      participant->lookup_topicdescription(BUILT_IN_TOPIC_TOPIC);

    dr = bit_subscriber->create_datareader(bit_topic_topic,
                                           dr_qos,
                                           DDS::DataReaderListener::_nil(),
                                           DEFAULT_STATUS_MASK);

    DDS::TopicDescription_var bit_pub_topic =
      participant->lookup_topicdescription(BUILT_IN_PUBLICATION_TOPIC);

    dr = bit_subscriber->create_datareader(bit_pub_topic,
                                           dr_qos,
                                           DDS::DataReaderListener::_nil(),
                                           OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    DDS::TopicDescription_var bit_sub_topic =
      participant->lookup_topicdescription(BUILT_IN_SUBSCRIPTION_TOPIC);

    dr = bit_subscriber->create_datareader(bit_sub_topic,
                                           dr_qos,
                                           DDS::DataReaderListener::_nil(),
                                           OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  } catch (const CORBA::Exception& e) {
    ACE_ERROR((LM_ERROR, "(%P|%t) InfoRepoDiscovery::init_bit, "
                         "exception during DataReader initialization\n"));
    return 0;
  }
  return bit_subscriber._retn();
#endif
}

} // namespace DCPS
} // namespace OpenDDS
