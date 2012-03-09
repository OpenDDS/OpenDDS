/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "InfoRepoDiscovery.h"
#include "FailoverListener.h"
#include "Service_Participant.h"
#include "InfoRepoUtils.h"
#include "RepoIdBuilder.h"

#if !defined (DDS_HAS_MINIMUM_BIT)
#include "DomainParticipantImpl.h"
#include "BuiltInTopicUtils.h"
#include "Marked_Default_Qos.h"

#include "transport/framework/TransportRegistry.h"
#include "transport/framework/TransportExceptions.h"

#include "transport/tcp/TcpInst.h"
#include "transport/tcp/TcpInst_rch.h"
#endif

namespace OpenDDS {
namespace DCPS {

InfoRepoDiscovery::InfoRepoDiscovery(const RepoKey& key,
                                     const std::string& ior)
  : Discovery(key),
    ior_(ior),
    bit_transport_port_(0),
    use_local_bit_config_(false),
    failoverListener_(0)
{
}

InfoRepoDiscovery::~InfoRepoDiscovery()
{
  delete this->failoverListener_;
}

DCPSInfo_ptr
InfoRepoDiscovery::get_dcps_info()
{
  if (CORBA::is_nil(this->info_.in())) {
    CORBA::ORB_var orb = TheServiceParticipant->get_ORB();
    try {
      this->info_ = InfoRepoUtils::get_repo(this->ior_.c_str(), orb.in());

      if (CORBA::is_nil(this->info_.in())) {
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("(%P|%t) ERROR: InfoRepoDiscovery::get_repository: ")
                   ACE_TEXT("unable to narrow DCPSInfo (%C) for key %C.\n"),
                   this->ior_.c_str(),
                   this->key().c_str()));
        return DCPSInfo::_nil();
      }

    } catch (const CORBA::Exception& ex) {
      ex._tao_print_exception("ERROR: InfoRepoDiscovery::get_repository: failed to resolve ior - ");
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

TransportConfig_rch
InfoRepoDiscovery::bit_config()
{
#if !defined (DDS_HAS_MINIMUM_BIT)
  if (bit_config_.is_nil()) {
    const std::string cfg_name = TransportRegistry::DEFAULT_INST_PREFIX +
                                 "_BITTransportConfig_" + key();
    bit_config_ = TransportRegistry::instance()->create_config(cfg_name);

    const std::string inst_name = TransportRegistry::DEFAULT_INST_PREFIX +
                                  "_BITTCPTransportInst_" + key();
    TransportInst_rch inst =
      TransportRegistry::instance()->create_inst(inst_name, "tcp");
    bit_config_->instances_.push_back(inst);

    if (!use_local_bit_config_) {
      bit_transport_ip_ = TheServiceParticipant->bit_transport_ip();
      bit_transport_port_ = TheServiceParticipant->bit_transport_port();
    }

    // Use a static cast to avoid dependency on the Tcp library
    TcpInst_rch tcp_inst = static_rchandle_cast<TcpInst>(inst);

    tcp_inst->datalink_release_delay_ = 0;
    if (bit_transport_ip_ == "") {
      tcp_inst->local_address_.set_port_number(bit_transport_port_);
    } else {
      tcp_inst->local_address_ = ACE_INET_Addr(bit_transport_port_,
                                               bit_transport_ip_.c_str());
    }

    std::stringstream out;
    out << bit_transport_ip_ << ':' << bit_transport_port_;
    tcp_inst->local_address_str_ = out.str();
  }
  return bit_config_;
#else
  return 0;
#endif
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
    TransportConfig_rch config = bit_config();
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

    if (participant->federated()) {
      DDS::ParticipantBuiltinTopicDataDataReader* pbit_dr =
        DDS::ParticipantBuiltinTopicDataDataReader::_narrow(dr.in());

      // Create and attach the listener.
      failoverListener_ = new FailoverListener(key());
      pbit_dr->set_listener(failoverListener_, DEFAULT_STATUS_MASK);
    }

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

  } catch (const CORBA::Exception&) {
    ACE_ERROR((LM_ERROR, "(%P|%t) InfoRepoDiscovery::init_bit, "
                         "exception during DataReader initialization\n"));
    return 0;
  }
  return bit_subscriber._retn();
#endif
}

RepoId
InfoRepoDiscovery::bit_key_to_repo_id(DomainParticipantImpl* /*participant*/,
                                      const char* /*bit_topic_name*/,
                                      const DDS::BuiltinTopicKey_t& key) const
{
  RepoId id = RepoIdBuilder::create();
  RepoIdBuilder builder(id);
  builder.federationId(key.value[0]);
  builder.participantId(key.value[1]);
  builder.entityId(key.value[2]);
  return id;
}


} // namespace DCPS
} // namespace OpenDDS
