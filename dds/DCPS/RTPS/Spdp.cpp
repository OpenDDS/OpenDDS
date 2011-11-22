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

#include "dds/DCPS/Service_Participant.h"

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

  void assign(DCPS::EntityKey_t& lhs, unsigned int rhs)
  {
    lhs[0] = static_cast<CORBA::Octet>(rhs);
    lhs[1] = static_cast<CORBA::Octet>(rhs >> 8);
    lhs[2] = static_cast<CORBA::Octet>(rhs >> 16);
  }
}


Spdp::Spdp(DDS::DomainId_t domain, const RepoId& guid,
           const DDS::DomainParticipantQos& qos, RtpsDiscovery* disco)
  : disco_(disco), domain_(domain), guid_(guid), qos_(qos)
  , tport_(new SpdpTransport(this)), eh_(tport_), eh_shutdown_(false)
  , shutdown_cond_(lock_), topic_counter_(0)
{
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
  //TODO
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
                    const ParameterList& plist, const Time_t& timestamp)
{
  //TODO
}

void
Spdp::bit_subscriber(const DDS::Subscriber_var& bit_subscriber)
{
  bit_subscriber_ = bit_subscriber;
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
  //TODO: allow user-configured addresses to be added here

  reference_counting_policy().value(Reference_Counting_Policy::ENABLED);

  if (outer_->reactor()->register_handler(unicast_socket_.get_handle(),
        this, ACE_Event_Handler::READ_MASK) != 0) {
    throw std::runtime_error("failed to register unicast input handler");
  }

  if (outer_->reactor()->register_handler(multicast_socket_.get_handle(),
        this, ACE_Event_Handler::READ_MASK) != 0) {
    throw std::runtime_error("failed to register multicast input handler");
  }

  const ACE_Time_Value per = outer_->disco_->resend_period();
  if (-1 == outer_->reactor()->schedule_timer(this, 0, per, per)) {
    throw std::runtime_error("failed to schedule timer with reactor");
  }
}

Spdp::SpdpTransport::~SpdpTransport()
{
  {
    ACE_GUARD(ACE_Thread_Mutex, g, outer_->lock_);
    outer_->eh_shutdown_ = true;
  }
  outer_->shutdown_cond_.signal();
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
    //TODO: error
  }

  typedef std::set<ACE_INET_Addr>::const_iterator iter_t;
  for (iter_t iter = send_addrs_.begin(); iter != send_addrs_.end(); ++iter) {
    ssize_t res =
      unicast_socket_.send(buff_.rd_ptr(), buff_.length(), *iter);
    //TODO: check
  }
}

int
Spdp::SpdpTransport::handle_timeout(const ACE_Time_Value&, const void*)
{
  write();
  //TODO: expire stale discovered participants
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
    //TODO
  }

  Time_t timestamp = TIME_INVALID;

  while (buff_.length() > 3) {
    const char subm = buff_.rd_ptr()[0], flags = buff_.rd_ptr()[1];
    ser.swap_bytes((flags & 1 /*FLAG_E*/) != ACE_CDR_BYTE_ORDER);
    const size_t start = buff_.length();
    CORBA::UShort submessageLength = 0;
    switch (subm) {
    case INFO_TS: {
      InfoTimestampSubmessage it;
      if (!(ser >> it)) {
        //TODO
      }
      submessageLength = it.smHeader.submessageLength;
      if (!(it.smHeader.flags & 2 /*FLAG_I*/)) {
        timestamp = it.timestamp;
      } else {
        timestamp = TIME_INVALID;
      }
    }
    case DATA: {
      DataSubmessage data;
      if (!(ser >> data)) {
        //TODO
      }
      submessageLength = data.smHeader.submessageLength;
      ParameterList plist;
      if (data.smHeader.flags & (4 /*FLAG_D*/ | 8 /*FLAG_K*/)) {
        ser.swap_bytes(!ACE_CDR_BYTE_ORDER); // read "encap" itself in LE
        CORBA::ULong encap;
        if (!(ser >> encap) || (encap != encap_LE && encap != encap_BE)) {
          //TODO
        }
        // bit 8 in encap is on if it's PL_CDR_LE
        ser.swap_bytes(((encap & 0x100) >> 8) != ACE_CDR_BYTE_ORDER);
        if (!(ser >> plist)) {
          //TODO
        }
      }
      outer_->data_received(header, data, plist, timestamp);
    }
    default:
      if (DCPS::DCPS_debug_level) {
        ACE_DEBUG((LM_WARNING, "(%P|%t) Spdp::SpdpTransport::handle_input() - "
          "unrecognized submessage type: %d\n", int(subm)));
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


DCPS::TopicStatus
Spdp::assert_topic(DCPS::RepoId_out topicId, const char* topicName,
                   const char* dataTypeName, const DDS::TopicQos& qos)
{
  if (topics_.count(topicName)) { // types must match, RtpsInfo checked for us
    topics_[topicName].qos_ = qos;
    //TODO: write to transport if QoS changed
    topicId = topics_[topicName].repo_id_;
    return DCPS::FOUND;
  }

  TopicDetails& td = topics_[topicName];
  td.data_type_ = dataTypeName;
  td.qos_ = qos;
  td.repo_id_ = guid_;
  td.repo_id_.entityId.entityKind = DCPS::ENTITYKIND_OPENDDS_TOPIC;
  assign(td.repo_id_.entityId.entityKey, topic_counter_++);
  topic_names_[td.repo_id_] = topicName;
  topicId = td.repo_id_;

  if (topic_counter_ == 0x1000000) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: Spdp::assert_topic: ")
               ACE_TEXT("Exceeded Maximum number of topic entity keys!")
               ACE_TEXT("Next key will be a duplicate!\n")));
    topic_counter_ = 0;
  }

  //TODO: write to transport
  return DCPS::CREATED;
}

DCPS::TopicStatus
Spdp::remove_topic(const RepoId& topicId, std::string& name)
{
  name = topic_names_[topicId];
  topics_.erase(name);
  topic_names_.erase(topicId);

  //TODO: write to transport (dispose/unregister?)
  return DCPS::REMOVED;
}

void
Spdp::ignore_topic(const RepoId& ignoreId)
{
  //TODO
}

bool
Spdp::update_topic_qos(const RepoId& topicId, const DDS::TopicQos& qos,
                       std::string& name)
{
  if (topic_names_.count(topicId)) {
    name = topic_names_[topicId];
    topics_[name].qos_ = qos;
    //TODO: write to transport
    return true;
  }
  return false;
}

RepoId
Spdp::add_publication(const RepoId& topicId,
                      DCPS::DataWriterRemote_ptr publication,
                      const DDS::DataWriterQos& qos,
                      const DCPS::TransportLocatorSeq& transInfo,
                      const DDS::PublisherQos& publisherQos)
{
  return RepoId();
}

void
Spdp::remove_publication(const RepoId& publicationId)
{
}

void
Spdp::ignore_publication(const RepoId& ignoreId)
{
}

bool
Spdp::update_publication_qos(const RepoId& publicationId,
                             const DDS::DataWriterQos& qos,
                             const DDS::PublisherQos& publisherQos)
{
  return false;
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
  return RepoId();
}

void
Spdp::remove_subscription(const RepoId& subscriptionId)
{
}

void
Spdp::ignore_subscription(const RepoId& ignoreId)
{
}

bool
Spdp::update_subscription_qos(const RepoId& subscriptionId,
                              const DDS::DataReaderQos& qos,
                              const DDS::SubscriberQos& subscriberQos)
{
  return false;
}

bool
Spdp::update_subscription_params(const RepoId& subId,
                                 const DDS::StringSeq& params)
{
  return false;
}

void
Spdp::association_complete(const RepoId& localId, const RepoId& remoteId)
{
}

void
Spdp::disassociate_participant(const RepoId& remoteId)
{
}

void
Spdp::disassociate_publication(const RepoId& localId, const RepoId& remoteId)
{
}

void
Spdp::disassociate_subscription(const RepoId& localId, const RepoId& remoteId)
{
}

}
}
