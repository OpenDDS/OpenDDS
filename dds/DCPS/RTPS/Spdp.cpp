/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "Spdp.h"
#include "BaseMessageTypes.h"
#include "MessageTypes.h"
#include "RtpsBaseMessageTypesTypeSupportImpl.h"
#include "RtpsMessageTypesTypeSupportImpl.h"
#include "ParameterListConverter.h"
#include "RtpsDiscovery.h"

#include "dds/DdsDcpsGuidC.h"
#include "dds/DdsDcpsInfrastructureTypeSupportImpl.h"

#include "dds/DCPS/Service_Participant.h"
#include "dds/DCPS/BuiltInTopicUtils.h"
#include "dds/DCPS/GuidConverter.h"
#include "dds/DCPS/Qos_Helper.h"

#include "ace/Reactor.h"

#include <cstring>
#include <stdexcept>

namespace OpenDDS {
namespace RTPS {
using DCPS::RepoId;

namespace {
  // Multiplier for resend period -> lease duration conversion,
  // if a remote discovery misses this many resends from us it will consider
  // us offline / unreachable.
  const int LEASE_MULT = 10;
  const CORBA::ULong encap_LE = 0x00000300; // {options, PL_CDR_LE} in LE
  const CORBA::ULong encap_BE = 0x00000200; // {options, PL_CDR_BE} in LE

  bool disposed(const ParameterList& inlineQos)
  {
    for (CORBA::ULong i = 0; i < inlineQos.length(); ++i) {
      if (inlineQos[i]._d() == PID_STATUS_INFO) {
        return inlineQos[i].status_info().value[3] & 1;
      }
    }
    return false;
  }
}


Spdp::Spdp(DDS::DomainId_t domain, const RepoId& guid,
           const DDS::DomainParticipantQos& qos, RtpsDiscovery* disco)
  : disco_(disco), domain_(domain), guid_(guid), qos_(qos)
  , tport_(new SpdpTransport(this)), eh_(tport_), eh_shutdown_(false)
  , shutdown_cond_(lock_), sedp_(guid, *this)
{
  ACE_GUARD(ACE_Thread_Mutex, g, lock_);
  sedp_.ignore(guid);
}

Spdp::~Spdp()
{
  tport_->close();
  eh_.reset();
  ACE_GUARD(ACE_Thread_Mutex, g, lock_);
  while (!eh_shutdown_) {
    shutdown_cond_.wait();
  }
}

void
Spdp::ignore_domain_participant(const RepoId& ignoreId)
{
  ACE_GUARD(ACE_Thread_Mutex, g, lock_);
  sedp_.ignore(ignoreId);
}

bool
Spdp::update_domain_participant_qos(const DDS::DomainParticipantQos& qos)
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, false);
  qos_ = qos;
  return true;
}

void
Spdp::data_received(const Header& header, const DataSubmessage& data,
                    const ParameterList& plist)
{
  const ACE_Time_Value time = ACE_OS::gettimeofday();
  SPDPdiscoveredParticipantData pdata;
  if (ParameterListConverter::from_param_list(plist, pdata) < 0) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR Spdp::data_received - ")
      ACE_TEXT("failed to convert from ParameterList to ")
      ACE_TEXT("SPDPdiscoveredParticipantData\n")));
    return;
  }

  DCPS::RepoId guid;
  std::memcpy(guid.guidPrefix, pdata.participantProxy.guidPrefix,
              sizeof(guid.guidPrefix));
  guid.entityId = OpenDDS::DCPS::ENTITYID_PARTICIPANT;

  ACE_GUARD(ACE_Thread_Mutex, g, lock_);
  if (sedp_.ignoring(guid)) {
    // Ignore, this is our domain participant
    return;
  }

  const ParticipantIter iter = participants_.find(guid);

  if (iter == participants_.end()) {
    // copy guid prefix (octet[12]) into BIT key (long[3])
    std::memcpy(pdata.ddsParticipantData.key.value,
                pdata.participantProxy.guidPrefix,
                sizeof(pdata.ddsParticipantData.key.value));

    if (DCPS::DCPS_debug_level) {
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("Data received called part key = %x %x %x\n"),
                 pdata.ddsParticipantData.key.value[0],
                 pdata.ddsParticipantData.key.value[1],
                 pdata.ddsParticipantData.key.value[2]));
    }

    // add a new participant
    participants_[guid] = ParticipantDetails(pdata, time);
    participants_[guid].bit_ih_ =
      part_bit()->store_synthetic_data(pdata.ddsParticipantData,
                                       DDS::NEW_VIEW_STATE);
    //TODO: inform SEDP
//     sedp_->associate(pdata);

  } else if (data.inlineQos.length() && disposed(data.inlineQos)) {
    remove_discovered_participant(iter);

  } else {
    // update an existing participant
    pdata.ddsParticipantData.key = iter->second.pdata_.ddsParticipantData.key;
    using OpenDDS::DCPS::operator!=;
    if (iter->second.pdata_.ddsParticipantData.user_data !=
        pdata.ddsParticipantData.user_data) {
      part_bit()->store_synthetic_data(pdata.ddsParticipantData,
                                       DDS::NOT_NEW_VIEW_STATE);
    }
    iter->second.pdata_ = pdata;
    iter->second.last_seen_ = time;
  }
}

void
Spdp::remove_discovered_participant(ParticipantIter iter)
{
  //TODO: inform SEDP
//  sedp_->disassociate(iter->second.pdata_);
  part_bit()->set_instance_state(iter->second.bit_ih_,
                                 DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE);
  participants_.erase(iter);
}

void
Spdp::remove_expired_participants()
{
  // Find and remove any expired discovered participant
  ACE_GUARD (ACE_Thread_Mutex, g, lock_);
  for (ParticipantIter it = participants_.begin();
       it != participants_.end();) {
    if (it->second.last_seen_ <
        ACE_OS::gettimeofday() - it->second.pdata_.leaseDuration.seconds) {
      if (DCPS::DCPS_debug_level > 1) {
        DCPS::GuidConverter conv(it->first);
        ACE_DEBUG((LM_WARNING,
          ACE_TEXT("(%P|%t) Spdp::SpdpTransport::handle_timeout() - ")
          ACE_TEXT("participant %C exceeded lease duration, removing\n"),
          std::string(conv).c_str()));
      }
      remove_discovered_participant(it++);
    } else {
      ++it;
    }
  }
}

void
Spdp::bit_subscriber(const DDS::Subscriber_var& bit_subscriber)
{
  bit_subscriber_ = bit_subscriber;
  tport_->open();
}

DDS::ParticipantBuiltinTopicDataDataReaderImpl*
Spdp::part_bit()
{
  DDS::DataReader_var d =
    bit_subscriber_->lookup_datareader(DCPS::BUILT_IN_PARTICIPANT_TOPIC);
  return dynamic_cast<DDS::ParticipantBuiltinTopicDataDataReaderImpl*>(d.in());
}

ACE_Reactor*
Spdp::reactor() const
{
  return TheServiceParticipant->discovery_reactor();
}

Spdp::SpdpTransport::SpdpTransport(Spdp* outer)
  : outer_(outer), lease_duration_(outer_->disco_->resend_period() * LEASE_MULT)
  , buff_(64 * 1024)
{
  hdr_.prefix[0] = 'R';
  hdr_.prefix[1] = 'T';
  hdr_.prefix[2] = 'P';
  hdr_.prefix[3] = 'S';
  hdr_.version = PROTOCOLVERSION;
  hdr_.vendorId = VENDORID_OPENDDS;
  std::memcpy(hdr_.guidPrefix, outer_->guid_.guidPrefix, sizeof(GuidPrefix_t));
  data_.smHeader.submessageId = DATA;
  data_.smHeader.flags = 1 /*FLAG_E*/ | 4 /*FLAG_D*/;
  data_.smHeader.submessageLength = 0; // last submessage in the Message
  data_.extraFlags = 0;
  data_.octetsToInlineQos = DATA_OCTETS_TO_IQOS;
  data_.readerId = ENTITYID_UNKNOWN;
  data_.writerId = ENTITYID_SPDP_BUILTIN_PARTICIPANT_WRITER;
  data_.writerSN.high = 0;
  data_.writerSN.low = 0;

  const u_short participantId = (hdr_.guidPrefix[10] << 8)
                                | hdr_.guidPrefix[11];

  // Ports are set by the formulas in RTPS v2.1 Table 9.8
  const u_short port_common = outer_->disco_->pb() +
                              (outer_->disco_->dg() * outer_->domain_),
    uni_port = port_common + outer_->disco_->d1() +
               (outer_->disco_->pg() * participantId),
    mc_port = port_common + outer_->disco_->d0();

  ACE_INET_Addr local_addr;
  if (0 != local_addr.set(uni_port)) {
    if (DCPS::DCPS_debug_level) {
      ACE_DEBUG((LM_ERROR, "(%P|%t) Spdp::SpdpTransport::SpdpTransport() - "
        "failed setting unicast local_addr to port %hd %p\n",
        uni_port, ACE_TEXT("ACE_INET_Addr::set")));
    }
    throw std::runtime_error("failed to set unicast local address");
  }

  if (0 != unicast_socket_.open(local_addr)) {
    if (DCPS::DCPS_debug_level) {
      ACE_DEBUG((LM_ERROR, "(%P|%t) Spdp::SpdpTransport::SpdpTransport() - "
        "failed to open unicast socket on port %hd %p\n",
        uni_port, ACE_TEXT("ACE_SOCK_Dgram::open")));
    }
    throw std::runtime_error("failed to open unicast socket");
  }

  const char mc_addr[] = "239.255.0.1" /*RTPS v2.1 9.6.1.4.1*/;
  ACE_INET_Addr default_multicast;
  if (0 != default_multicast.set(mc_port, mc_addr)) {
    if (DCPS::DCPS_debug_level) {
      ACE_DEBUG((LM_ERROR, "(%P|%t) Spdp::SpdpTransport::SpdpTransport() - "
        "failed setting default_multicast address %C:%hd %p\n",
        mc_addr, mc_port, ACE_TEXT("ACE_INET_Addr::set")));
    }
    throw std::runtime_error("failed to set default_multicast address");
  }

  if (0 != multicast_socket_.join(default_multicast)) {
    if (DCPS::DCPS_debug_level) {
      ACE_DEBUG((LM_ERROR, "(%P|%t) Spdp::SpdpTransport::SpdpTransport() - "
        "failed to join multicast group %C:%hd %p\n",
        mc_addr, mc_port, ACE_TEXT("ACE_SOCK_Dgram_Mcast::join")));
    }
    throw std::runtime_error("failed to join multicast group");
  }

  send_addrs_.insert(default_multicast);

  typedef RtpsDiscovery::AddrVec::iterator iter;
  for (iter it = outer_->disco_->spdp_send_addrs().begin(),
       end = outer_->disco_->spdp_send_addrs().end(); it != end; ++it) {
    send_addrs_.insert(ACE_INET_Addr(it->c_str()));
  }

  reference_counting_policy().value(Reference_Counting_Policy::ENABLED);
}

void
Spdp::SpdpTransport::open()
{
  if (outer_->reactor()->register_handler(unicast_socket_.get_handle(),
        this, ACE_Event_Handler::READ_MASK) != 0) {
    throw std::runtime_error("failed to register unicast input handler");
  }

  if (outer_->reactor()->register_handler(multicast_socket_.get_handle(),
        this, ACE_Event_Handler::READ_MASK) != 0) {
    throw std::runtime_error("failed to register multicast input handler");
  }

  const ACE_Time_Value per = outer_->disco_->resend_period();
  if (-1 == outer_->reactor()->schedule_timer(this, 0,
                                              ACE_Time_Value(0), per)) {
    throw std::runtime_error("failed to schedule timer with reactor");
  }
}

Spdp::SpdpTransport::~SpdpTransport()
{
  dispose_unregister();
  {
    ACE_GUARD(ACE_Thread_Mutex, g, outer_->lock_);
    outer_->eh_shutdown_ = true;
  }
  outer_->shutdown_cond_.signal();
  unicast_socket_.close();
  multicast_socket_.close();
}

void
Spdp::SpdpTransport::dispose_unregister()
{
  // Send the dispose/unregister SPDP sample
  data_.writerSN.high = seq_.getHigh();
  data_.writerSN.low = seq_.getLow();
  data_.smHeader.flags = 1 /*FLAG_E*/ | 2 /*FLAG_Q*/; // note no FLAG_D
  data_.inlineQos.length(1);
  static const StatusInfo_t dispose_unregister = { {0, 0, 0, 3} };
  data_.inlineQos[0].status_info(dispose_unregister);
  buff_.reset();
  DCPS::Serializer ser(&buff_, false, DCPS::Serializer::ALIGN_CDR);
  if (!(ser << hdr_) || !(ser << data_)) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR Spdp::SpdpTransport::dispose_unregister() - ")
      ACE_TEXT("failed to serialize headers for dispose/unregister\n")));
    return;
  }
  typedef std::set<ACE_INET_Addr>::const_iterator iter_t;
  for (iter_t iter = send_addrs_.begin(); iter != send_addrs_.end(); ++iter) {
    const ssize_t res =
      unicast_socket_.send(buff_.rd_ptr(), buff_.length(), *iter);
    if (res < 0) {
      ACE_TCHAR addr_buff[256] = {};
      iter->addr_to_string(addr_buff, 256, 0);
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR Spdp::SpdpTransport::dispose_unregister() - ")
        ACE_TEXT("destination %s failed %p\n"), addr_buff, ACE_TEXT("send")));
    }
  }
}

void
Spdp::SpdpTransport::close()
{
  outer_->reactor()->cancel_timer(this);
  const ACE_Reactor_Mask mask =
    ACE_Event_Handler::READ_MASK | ACE_Event_Handler::DONT_CALL;
  outer_->reactor()->remove_handler(multicast_socket_.get_handle(), mask);
  outer_->reactor()->remove_handler(unicast_socket_.get_handle(), mask);
}

void
Spdp::SpdpTransport::write()
{
  static const LocatorSeq emptyList;
  static const BuiltinEndpointSet_t availableBuiltinEndpoints =
    DISC_BUILTIN_ENDPOINT_PARTICIPANT_ANNOUNCER |
    DISC_BUILTIN_ENDPOINT_PARTICIPANT_DETECTOR |
    DISC_BUILTIN_ENDPOINT_PUBLICATION_ANNOUNCER |
    DISC_BUILTIN_ENDPOINT_PUBLICATION_DETECTOR |
    DISC_BUILTIN_ENDPOINT_SUBSCRIPTION_ANNOUNCER |
    DISC_BUILTIN_ENDPOINT_SUBSCRIPTION_DETECTOR;
  // The RTPS spec has no constants for the builtinTopics{Writer,Reader}

  data_.writerSN.high = seq_.getHigh();
  data_.writerSN.low = seq_.getLow();
  ++seq_;

  ACE_GUARD(ACE_Thread_Mutex, g, outer_->lock_);
  const GuidPrefix_t& gp = outer_->guid_.guidPrefix;

  const SPDPdiscoveredParticipantData pdata = {
    { // ParticipantBuiltinTopicData
      DDS::BuiltinTopicKey_t() /*ignored*/,
      outer_->qos_.user_data
    },
    { // ParticipantProxy_t
      PROTOCOLVERSION,
      {gp[0], gp[1], gp[2], gp[3], gp[4], gp[5],
       gp[6], gp[7], gp[8], gp[9], gp[10], gp[11]},
      VENDORID_OPENDDS,
      false /*expectsIQoS*/,
      availableBuiltinEndpoints,
      outer_->sedp_unicast_,
      outer_->sedp_multicast_,
      emptyList /*defaultMulticastLocatorList*/,
      emptyList /*defaultUnicastLocatorList*/,
      {0 /*manualLivelinessCount*/}   //FUTURE: implement manual liveliness
    },
    { // Duration_t (leaseDuration)
      static_cast<CORBA::Long>(lease_duration_.sec()),
      0 // we are not supporting fractional seconds in the lease duration
    }
  };

  ParameterList plist;
  ParameterListConverter::to_param_list(pdata, plist);

  buff_.reset();
  DCPS::Serializer ser(&buff_, false, DCPS::Serializer::ALIGN_CDR);
  if (!(ser << hdr_) || !(ser << data_) || !(ser << encap_LE) ||
      !(ser << plist)) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR Spdp::SpdpTransport::write() - ")
      ACE_TEXT("failed to serialize headers for SPDP\n")));
    return;
  }

  typedef std::set<ACE_INET_Addr>::const_iterator iter_t;
  for (iter_t iter = send_addrs_.begin(); iter != send_addrs_.end(); ++iter) {
    const ssize_t res =
      unicast_socket_.send(buff_.rd_ptr(), buff_.length(), *iter);
    if (res < 0) {
      ACE_TCHAR addr_buff[256] = {};
      iter->addr_to_string(addr_buff, 256, 0);
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR Spdp::SpdpTransport::write() - ")
        ACE_TEXT("destination %s failed %p\n"), addr_buff, ACE_TEXT("send")));
    }
  }
}

int
Spdp::SpdpTransport::handle_timeout(const ACE_Time_Value&, const void*)
{
  write();
  outer_->remove_expired_participants();
  return 0;
}

int
Spdp::SpdpTransport::handle_input(ACE_HANDLE h)
{
  const ACE_SOCK_Dgram& socket = (h == unicast_socket_.get_handle())
                                 ? unicast_socket_ : multicast_socket_;
  ACE_INET_Addr remote;
  buff_.reset();
  const ssize_t bytes = socket.recv(buff_.wr_ptr(), buff_.space(), remote);

  if (bytes > 0) {
    buff_.wr_ptr(bytes);
  } else if (bytes == 0) {
    return -1;
  } else {
    if (DCPS::DCPS_debug_level) {
      ACE_DEBUG((LM_ERROR, "(%P|%t) Spdp::SpdpTransport::handle_input() - "
        "error reading from %C socket %p\n", (h == unicast_socket_.get_handle())
        ? "unicast" : "multicast", ACE_TEXT("ACE_SOCK_Dgram::recv")));
    }
    return -1;
  }

  DCPS::Serializer ser(&buff_, false, DCPS::Serializer::ALIGN_CDR);
  Header header;
  if (!(ser >> header)) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR Spdp::SpdpTransport::handle_input() - ")
      ACE_TEXT("failed to deserialize RTPS header for SPDP\n")));
    return 0;
  }

  while (buff_.length() > 3) {
    const char subm = buff_.rd_ptr()[0], flags = buff_.rd_ptr()[1];
    ser.swap_bytes((flags & 1 /*FLAG_E*/) != ACE_CDR_BYTE_ORDER);
    const size_t start = buff_.length();
    CORBA::UShort submessageLength = 0;
    switch (subm) {
    case DATA: {
      DataSubmessage data;
      if (!(ser >> data)) {
        ACE_ERROR((LM_ERROR,
          ACE_TEXT("(%P|%t) ERROR Spdp::SpdpTransport::handle_input() - ")
          ACE_TEXT("failed to deserialize DATA header for SPDP\n")));
        return 0;
      }
      submessageLength = data.smHeader.submessageLength;
      ParameterList plist;
      if (data.smHeader.flags & (4 /*FLAG_D*/ | 8 /*FLAG_K*/)) {
        ser.swap_bytes(!ACE_CDR_BYTE_ORDER); // read "encap" itself in LE
        CORBA::ULong encap;
        if (!(ser >> encap) || (encap != encap_LE && encap != encap_BE)) {
          ACE_ERROR((LM_ERROR,
            ACE_TEXT("(%P|%t) ERROR Spdp::SpdpTransport::handle_input() - ")
            ACE_TEXT("failed to deserialize encapsulation header for SPDP\n")));
          return 0;
        }
        // bit 8 in encap is on if it's PL_CDR_LE
        ser.swap_bytes(((encap & 0x100) >> 8) != ACE_CDR_BYTE_ORDER);
        if (!(ser >> plist)) {
          ACE_ERROR((LM_ERROR,
            ACE_TEXT("(%P|%t) ERROR Spdp::SpdpTransport::handle_input() - ")
            ACE_TEXT("failed to deserialize data payload for SPDP\n")));
          return 0;
        }
      }
      if (data.writerId == ENTITYID_SPDP_BUILTIN_PARTICIPANT_WRITER) {
        outer_->data_received(header, data, plist);
      }
      break;
    }
    default:
      if (subm != INFO_TS && DCPS::DCPS_debug_level) {
        ACE_DEBUG((LM_WARNING, "(%P|%t) Spdp::SpdpTransport::handle_input() - "
                   "ignored submessage type: %x, DATA is %x\n", int(subm), int(DATA)));
      }
      break;
    }
    if (submessageLength && buff_.length()) {
      const size_t read = start - buff_.length();
      if (read < static_cast<size_t>(submessageLength + SMHDR_SZ)) {
        ser.skip(static_cast<CORBA::UShort>(submessageLength + SMHDR_SZ - read));
      }
    }
  }

  return 0;
}


// start of methods that just forward to sedp_

DCPS::TopicStatus
Spdp::assert_topic(DCPS::RepoId_out topicId, const char* topicName,
                   const char* dataTypeName, const DDS::TopicQos& qos)
{
  return sedp_.assert_topic(topicId, topicName, dataTypeName, qos);
}

DCPS::TopicStatus
Spdp::remove_topic(const RepoId& topicId, std::string& name)
{
  return sedp_.remove_topic(topicId, name);
}

void
Spdp::ignore_topic(const RepoId& ignoreId)
{
  sedp_.ignore(ignoreId);
}

bool
Spdp::update_topic_qos(const RepoId& topicId, const DDS::TopicQos& qos,
                       std::string& name)
{
  return sedp_.update_topic_qos(topicId, qos, name);
}

RepoId
Spdp::add_publication(const RepoId& topicId,
                      DCPS::DataWriterRemote_ptr publication,
                      const DDS::DataWriterQos& qos,
                      const DCPS::TransportLocatorSeq& transInfo,
                      const DDS::PublisherQos& publisherQos)
{
  return sedp_.add_publication(topicId, publication, qos,
                               transInfo, publisherQos);
}

void
Spdp::remove_publication(const RepoId& publicationId)
{
  sedp_.remove_publication(publicationId);
}

void
Spdp::ignore_publication(const RepoId& ignoreId)
{
  return sedp_.ignore(ignoreId);
}

bool
Spdp::update_publication_qos(const RepoId& publicationId,
                             const DDS::DataWriterQos& qos,
                             const DDS::PublisherQos& publisherQos)
{
  return sedp_.update_publication_qos(publicationId, qos, publisherQos);
}

RepoId
Spdp::add_subscription(const RepoId& topicId,
                       DCPS::DataReaderRemote_ptr subscription,
                       const DDS::DataReaderQos& qos,
                       const DCPS::TransportLocatorSeq& transInfo,
                       const DDS::SubscriberQos& subscriberQos,
                       const char* filterExpr,
                       const DDS::StringSeq& params)
{
  return sedp_.add_subscription(topicId, subscription, qos, transInfo,
                                subscriberQos, filterExpr, params);
}

void
Spdp::remove_subscription(const RepoId& subscriptionId)
{
  sedp_.remove_subscription(subscriptionId);
}

void
Spdp::ignore_subscription(const RepoId& ignoreId)
{
  return sedp_.ignore(ignoreId);
}

bool
Spdp::update_subscription_qos(const RepoId& subscriptionId,
                              const DDS::DataReaderQos& qos,
                              const DDS::SubscriberQos& subscriberQos)
{
  return sedp_.update_subscription_qos(subscriptionId, qos, subscriberQos);
}

bool
Spdp::update_subscription_params(const RepoId& subId,
                                 const DDS::StringSeq& params)
{
  return sedp_.update_subscription_params(subId, params);
}


// Managing reader/writer associations

void
Spdp::association_complete(const RepoId& localId, const RepoId& remoteId)
{
  sedp_.association_complete(localId, remoteId);
}

void
Spdp::disassociate_participant(const RepoId& remoteId)
{
  sedp_.disassociate_participant(remoteId);
}

void
Spdp::disassociate_publication(const RepoId& localId, const RepoId& remoteId)
{
  sedp_.disassociate_publication(localId, remoteId);
}

void
Spdp::disassociate_subscription(const RepoId& localId, const RepoId& remoteId)
{
  sedp_.disassociate_subscription(localId, remoteId);
}

}
}
