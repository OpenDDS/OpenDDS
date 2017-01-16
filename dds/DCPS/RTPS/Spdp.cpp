/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "Spdp.h"
#include "BaseMessageTypes.h"
#include "MessageTypes.h"
#include "RtpsCoreTypeSupportImpl.h"
#include "ParameterListConverter.h"
#include "RtpsDiscovery.h"

#include "dds/DdsDcpsGuidC.h"

#include "dds/DCPS/Service_Participant.h"
#include "dds/DCPS/BuiltInTopicUtils.h"
#include "dds/DCPS/GuidConverter.h"
#include "dds/DCPS/Qos_Helper.h"

#include "ace/Reactor.h"
#include "ace/OS_NS_sys_socket.h" // For setsockopt()

#include <cstring>
#include <stdexcept>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace RTPS {
using DCPS::RepoId;

namespace {
  // Multiplier for resend period -> lease duration conversion,
  // if a remote discovery misses this many resends from us it will consider
  // us offline / unreachable.
  const int LEASE_MULT = 10;
  const CORBA::UShort encap_LE = 0x0300; // {PL_CDR_LE} in LE
  const CORBA::UShort encap_BE = 0x0200; // {PL_CDR_BE} in LE

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


Spdp::Spdp(DDS::DomainId_t domain, RepoId& guid,
           const DDS::DomainParticipantQos& qos, RtpsDiscovery* disco)
  : OpenDDS::DCPS::LocalParticipant<Sedp>(qos)
  , disco_(disco), domain_(domain), guid_(guid)
  , tport_(new SpdpTransport(this)), eh_(tport_), eh_shutdown_(false)
  , shutdown_cond_(lock_), shutdown_flag_(0), sedp_(guid_, *this, lock_)
{
  ACE_GUARD(ACE_Thread_Mutex, g, lock_);
  guid = guid_; // may have changed in SpdpTransport constructor
  sedp_.ignore(guid);
  sedp_.init(guid_, *disco, domain_);

  // Append metatraffic unicast locator
  sedp_.unicast_locators(sedp_unicast_);

  if (disco->sedp_multicast()) { // Append metatraffic multicast locator
    const ACE_INET_Addr& mc_addr = sedp_.multicast_group();
    OpenDDS::DCPS::Locator_t mc_locator;
    mc_locator.kind = address_to_kind(mc_addr);
    mc_locator.port = mc_addr.get_port_number();
    address_to_bytes(mc_locator.address, mc_addr);
    sedp_multicast_.length(1);
    sedp_multicast_[0] = mc_locator;
  }
}

Spdp::~Spdp()
{
  shutdown_flag_ = 1;
  {
    ACE_GUARD(ACE_Thread_Mutex, g, lock_);
    if (DCPS::DCPS_debug_level > 3) {
      ACE_DEBUG((LM_INFO,
                 ACE_TEXT("(%P|%t) Spdp::~Spdp ")
                 ACE_TEXT("remove discovered participants\n")));
    }
    // Iterate through a copy of the repo Ids, rather than the map
    //   as it gets unlocked in remove_discovered_participant()
    DCPS::RepoIdSet participant_ids;
    get_discovered_participant_ids(participant_ids);
    for (DCPS::RepoIdSet::iterator participant_id = participant_ids.begin();
         participant_id != participant_ids.end();
         ++participant_id)
    {
      DiscoveredParticipantIter part = participants_.find(*participant_id);
      if (part != participants_.end()) {
        remove_discovered_participant(part);
      }
    }
  }

  // ensure sedp's task queue is drained before data members are being
  // deleted
  sedp_.shutdown();

  // release lock for reset of event handler, which may delete transport
  tport_->close();
  eh_.reset();
  {
    ACE_GUARD(ACE_Thread_Mutex, g, lock_);
    while (!eh_shutdown_) {
      shutdown_cond_.wait();
    }
  }
}

void
Spdp::data_received(const DataSubmessage& data, const ParameterList& plist)
{
  if (shutdown_flag_.value()) { return; }

  const ACE_Time_Value time = ACE_OS::gettimeofday();
  SPDPdiscoveredParticipantData pdata;
  if (ParameterListConverter::from_param_list(plist, pdata) < 0) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: Spdp::data_received - ")
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
    // Ignore, this is our domain participant or one that the user has
    // asked us to ignore.
    return;
  }

  // Find the participant - iterator valid only as long as we hold the lock
  DiscoveredParticipantIter iter = participants_.find(guid);

  // Must unlock when calling into part_bit() as it may call back into us
  ACE_Reverse_Lock<ACE_Thread_Mutex> rev_lock(lock_);

  if (iter == participants_.end()) {
    // copy guid prefix (octet[12]) into BIT key (long[3])
    std::memcpy(pdata.ddsParticipantData.key.value,
                pdata.participantProxy.guidPrefix,
                sizeof(pdata.ddsParticipantData.key.value));

    if (DCPS::DCPS_debug_level) {
      DCPS::GuidConverter local(guid_), remote(guid);
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Spdp::data_received - %C discovered %C lease %ds\n"),
        OPENDDS_STRING(local).c_str(), OPENDDS_STRING(remote).c_str(),
        pdata.leaseDuration.seconds));
    }

    // add a new participant
    participants_[guid] = DiscoveredParticipant(pdata, time);
    DDS::InstanceHandle_t bit_instance_handle = DDS::HANDLE_NIL;
#ifndef DDS_HAS_MINIMUM_BIT
    DCPS::ParticipantBuiltinTopicDataDataReaderImpl* bit = part_bit();
    if (bit) {
      ACE_GUARD(ACE_Reverse_Lock<ACE_Thread_Mutex>, rg, rev_lock);
      bit_instance_handle =
        bit->store_synthetic_data(pdata.ddsParticipantData,
                                  DDS::NEW_VIEW_STATE);
    }
#endif /* DDS_HAS_MINIMUM_BIT */

    // notify Sedp of association
    // Sedp may call has_discovered_participant.
    // This is what the participant must be added before this call to associate.
    sedp_.associate(pdata);

    // Since we've just seen a new participant, let's send out our
    // own announcement, so they don't have to wait.
    this->tport_->write_i();

    // Iterator is no longer valid
    iter = participants_.find(guid);
    if (iter != participants_.end()) {
      iter->second.bit_ih_ = bit_instance_handle;
    }

  } else if (data.inlineQos.length() && disposed(data.inlineQos)) {
    remove_discovered_participant(iter);

  } else {
    // update an existing participant
    pdata.ddsParticipantData.key = iter->second.pdata_.ddsParticipantData.key;
#ifndef OPENDDS_SAFETY_PROFILE
    using OpenDDS::DCPS::operator!=;
#endif
    if (iter->second.pdata_.ddsParticipantData.user_data !=
        pdata.ddsParticipantData.user_data) {
      iter->second.pdata_.ddsParticipantData.user_data =
        pdata.ddsParticipantData.user_data;
#ifndef DDS_HAS_MINIMUM_BIT
      DCPS::ParticipantBuiltinTopicDataDataReaderImpl* bit = part_bit();
      if (bit) {
        ACE_GUARD(ACE_Reverse_Lock<ACE_Thread_Mutex>, rg, rev_lock);
        bit->store_synthetic_data(pdata.ddsParticipantData,
                                  DDS::NOT_NEW_VIEW_STATE);
      }
#endif /* DDS_HAS_MINIMUM_BIT */
      // Perform search again, so iterator becomes valid
      iter = participants_.find(guid);
    }
    // Participant may have been removed while lock released
    if (iter != participants_.end()) {
      iter->second.pdata_ = pdata;
      iter->second.last_seen_ = time;
    }
  }
}

void
Spdp::remove_expired_participants()
{
  // Find and remove any expired discovered participant
  ACE_GUARD (ACE_Thread_Mutex, g, lock_);
  // Iterate through a copy of the repo Ids, rather than the map
  //   as it gets unlocked in remove_discovered_participant()
  DCPS::RepoIdSet participant_ids;
  get_discovered_participant_ids(participant_ids);
  for (DCPS::RepoIdSet::iterator participant_id = participant_ids.begin();
       participant_id != participant_ids.end();
       ++participant_id)
  {
    DiscoveredParticipantIter part = participants_.find(*participant_id);
    if (part != participants_.end()) {
      if (part->second.last_seen_ <
          ACE_OS::gettimeofday() -
          ACE_Time_Value(part->second.pdata_.leaseDuration.seconds)) {
        if (DCPS::DCPS_debug_level > 1) {
          DCPS::GuidConverter conv(part->first);
          ACE_DEBUG((LM_WARNING,
            ACE_TEXT("(%P|%t) Spdp::remove_expired_participants() - ")
            ACE_TEXT("participant %C exceeded lease duration, removing\n"),
            OPENDDS_STRING(conv).c_str()));
        }
        remove_discovered_participant(part);
      }
    }
  }
}

void
Spdp::init_bit(const DDS::Subscriber_var& bit_subscriber)
{
  bit_subscriber_ = bit_subscriber;
  tport_->open();
}

void
Spdp::fini_bit()
{
  bit_subscriber_ = 0;
  wait_for_acks_.reset();
  // request for SpdpTransport(actually Reactor) thread and Sedp::Task
  // to acknowledge
  tport_->acknowledge();
  sedp_.acknowledge();
  // wait for the 2 acknowledgements
  wait_for_acks_.wait_for_acks(2);
}

#ifndef DDS_HAS_MINIMUM_BIT
DCPS::ParticipantBuiltinTopicDataDataReaderImpl*
Spdp::part_bit()
{
  if (!bit_subscriber_.in())
    return 0;

  DDS::DataReader_var d =
    bit_subscriber_->lookup_datareader(DCPS::BUILT_IN_PARTICIPANT_TOPIC);
  return dynamic_cast<DCPS::ParticipantBuiltinTopicDataDataReaderImpl*>(d.in());
}
#endif /* DDS_HAS_MINIMUM_BIT */

ACE_Reactor*
Spdp::reactor() const
{
  return disco_->reactor();
}

WaitForAcks&
Spdp::wait_for_acks()
{
  return wait_for_acks_;
}

Spdp::SpdpTransport::SpdpTransport(Spdp* outer)
  : outer_(outer), lease_duration_(outer_->disco_->resend_period() * LEASE_MULT)
  , buff_(64 * 1024)
  , wbuff_(64 * 1024)
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

  // Ports are set by the formulas in RTPS v2.1 Table 9.8
  const u_short port_common = outer_->disco_->pb() +
                              (outer_->disco_->dg() * outer_->domain_),
    mc_port = port_common + outer_->disco_->d0();

  u_short participantId = (hdr_.guidPrefix[10] << 8) | hdr_.guidPrefix[11];

#ifdef OPENDDS_SAFETY_PROFILE
  const u_short startingParticipantId = participantId;
#endif

  while (!open_unicast_socket(port_common, participantId)) {
    ++participantId;
  }

#ifdef OPENDDS_SAFETY_PROFILE
  if (participantId > startingParticipantId && ACE_OS::getpid() == -1) {
    // Since pids are not available, use the fact that we had to increment
    // participantId to modify the GUID's pid bytes.  This avoids GUID conflicts
    // between processes on the same host which start at the same time
    // (resulting in the same seed value for the random number generator).
    hdr_.guidPrefix[8] = static_cast<CORBA::Octet>(participantId >> 8);
    hdr_.guidPrefix[9] = static_cast<CORBA::Octet>(participantId & 0xFF);
    outer_->guid_.guidPrefix[8] = hdr_.guidPrefix[8];
    outer_->guid_.guidPrefix[9] = hdr_.guidPrefix[9];
  }
#endif

  OPENDDS_STRING mc_addr = outer_->disco_->default_multicast_group();
  ACE_INET_Addr default_multicast;
  if (0 != default_multicast.set(mc_port, mc_addr.c_str())) {
    ACE_DEBUG((
          LM_ERROR,
          ACE_TEXT("(%P|%t) ERROR: Spdp::SpdpTransport::SpdpTransport() - ")
          ACE_TEXT("failed setting default_multicast address %C:%hd %p\n"),
          mc_addr.c_str(), mc_port, ACE_TEXT("ACE_INET_Addr::set")));
    throw std::runtime_error("failed to set default_multicast address");
  }

  const OPENDDS_STRING& net_if = outer_->disco_->multicast_interface();

  if (DCPS::DCPS_debug_level > 3) {
    ACE_DEBUG((LM_INFO,
               ACE_TEXT("(%P|%t) Spdp::SpdpTransport::SpdpTransport ")
               ACE_TEXT("joining group %C %C:%hd\n"),
               net_if.c_str (),
               mc_addr.c_str (),
               mc_port));
  }

#ifdef ACE_HAS_MAC_OSX
  multicast_socket_.opts(ACE_SOCK_Dgram_Mcast::OPT_BINDADDR_NO |
                         ACE_SOCK_Dgram_Mcast::DEFOPT_NULLIFACE);
#endif

  if (0 != multicast_socket_.join(default_multicast, 1,
                                  net_if.empty() ? 0 :
                                  ACE_TEXT_CHAR_TO_TCHAR(net_if.c_str()))) {
    ACE_DEBUG((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: Spdp::SpdpTransport::SpdpTransport() - ")
        ACE_TEXT("failed to join multicast group %C:%hd %p\n"),
        mc_addr.c_str(), mc_port, ACE_TEXT("ACE_SOCK_Dgram_Mcast::join")));
    throw std::runtime_error("failed to join multicast group");
  }

  send_addrs_.insert(default_multicast);

  typedef RtpsDiscovery::AddrVec::iterator iter;
  for (iter it = outer_->disco_->spdp_send_addrs().begin(),
       end = outer_->disco_->spdp_send_addrs().end(); it != end; ++it) {
    send_addrs_.insert(ACE_INET_Addr(it->c_str()));
  }
}

void
Spdp::SpdpTransport::open()
{
  ACE_Reactor* reactor = outer_->reactor();
  if (reactor->register_handler(unicast_socket_.get_handle(),
                                this, ACE_Event_Handler::READ_MASK) != 0) {
    throw std::runtime_error("failed to register unicast input handler");
  }

  if (reactor->register_handler(multicast_socket_.get_handle(),
                                this, ACE_Event_Handler::READ_MASK) != 0) {
    throw std::runtime_error("failed to register multicast input handler");
  }

  const ACE_Time_Value per = outer_->disco_->resend_period();
  if (-1 == reactor->schedule_timer(this, 0, ACE_Time_Value(0), per)) {
    throw std::runtime_error("failed to schedule timer with reactor");
  }
}

Spdp::SpdpTransport::~SpdpTransport()
{
  if (DCPS::DCPS_debug_level > 3) {
    ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) SpdpTransport::~SpdpTransport\n")));
  }
  try {
    dispose_unregister();
  }
  catch (const CORBA::Exception& ex) {
    if (DCPS::DCPS_debug_level > 0) {
      ACE_DEBUG((LM_WARNING,
        ACE_TEXT("(%P|%t) WARNING: Exception caught in ")
        ACE_TEXT("SpdpTransport::~SpdpTransport: %C\n"),
        ex._info().c_str()));
    }
  }
  {
    // Acquire lock for modification of condition variable
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
  data_.smHeader.flags = 1 /*FLAG_E*/ | 2 /*FLAG_Q*/ | 8 /*FLAG_K*/;
  data_.inlineQos.length(1);
  static const StatusInfo_t dispose_unregister = { {0, 0, 0, 3} };
  data_.inlineQos[0].status_info(dispose_unregister);

  ParameterList plist(1);
  plist.length(1);
  plist[0].guid(outer_->guid_);
  plist[0]._d(PID_PARTICIPANT_GUID);

  wbuff_.reset();
  DCPS::Serializer ser(&wbuff_, false, DCPS::Serializer::ALIGN_CDR);
  CORBA::UShort options = 0;
  if (!(ser << hdr_) || !(ser << data_) || !(ser << encap_LE) || !(ser << options)
      || !(ser << plist)) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: Spdp::SpdpTransport::dispose_unregister() - ")
      ACE_TEXT("failed to serialize headers for dispose/unregister\n")));
    return;
  }

  typedef OPENDDS_SET(ACE_INET_Addr)::const_iterator iter_t;
  for (iter_t iter = send_addrs_.begin(); iter != send_addrs_.end(); ++iter) {
    const ssize_t res =
      unicast_socket_.send(wbuff_.rd_ptr(), wbuff_.length(), *iter);
    if (res < 0) {
      ACE_TCHAR addr_buff[256] = {};
      iter->addr_to_string(addr_buff, 256, 0);
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: Spdp::SpdpTransport::dispose_unregister() - ")
        ACE_TEXT("destination %s failed %p\n"), addr_buff, ACE_TEXT("send")));
    }
  }
}

void
Spdp::SpdpTransport::close()
{
  if (DCPS::DCPS_debug_level > 3) {
    ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) SpdpTransport::close\n")));
  }
  ACE_Reactor* reactor = outer_->reactor();
  reactor->cancel_timer(this);
  const ACE_Reactor_Mask mask =
    ACE_Event_Handler::READ_MASK | ACE_Event_Handler::DONT_CALL;
  reactor->remove_handler(multicast_socket_.get_handle(), mask);
  reactor->remove_handler(unicast_socket_.get_handle(), mask);
}

void
Spdp::SpdpTransport::write()
{
  ACE_GUARD(ACE_Thread_Mutex, g, outer_->lock_);
  write_i();
}

void
Spdp::SpdpTransport::write_i()
{
  static const BuiltinEndpointSet_t availableBuiltinEndpoints =
    DISC_BUILTIN_ENDPOINT_PARTICIPANT_ANNOUNCER |
    DISC_BUILTIN_ENDPOINT_PARTICIPANT_DETECTOR |
    DISC_BUILTIN_ENDPOINT_PUBLICATION_ANNOUNCER |
    DISC_BUILTIN_ENDPOINT_PUBLICATION_DETECTOR |
    DISC_BUILTIN_ENDPOINT_SUBSCRIPTION_ANNOUNCER |
    DISC_BUILTIN_ENDPOINT_SUBSCRIPTION_DETECTOR |
    BUILTIN_ENDPOINT_PARTICIPANT_MESSAGE_DATA_WRITER |
    BUILTIN_ENDPOINT_PARTICIPANT_MESSAGE_DATA_READER
    ;
  // The RTPS spec has no constants for the builtinTopics{Writer,Reader}

  // This locator list should not be empty, but we won't actually be using it.
  // The OpenDDS publication/subscription data will have locators included.
  OpenDDS::DCPS::LocatorSeq nonEmptyList(1);
  nonEmptyList.length(1);
  nonEmptyList[0].kind = LOCATOR_KIND_UDPv4;
  nonEmptyList[0].port = 12345;
  std::memset(nonEmptyList[0].address, 0, 12);
  nonEmptyList[0].address[12] = 127;
  nonEmptyList[0].address[13] = 0;
  nonEmptyList[0].address[14] = 0;
  nonEmptyList[0].address[15] = 1;

  data_.writerSN.high = seq_.getHigh();
  data_.writerSN.low = seq_.getLow();
  ++seq_;

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
      nonEmptyList /*defaultMulticastLocatorList*/,
      nonEmptyList /*defaultUnicastLocatorList*/,
      {0 /*manualLivelinessCount*/}   //FUTURE: implement manual liveliness
    },
    { // Duration_t (leaseDuration)
      static_cast<CORBA::Long>(lease_duration_.sec()),
      0 // we are not supporting fractional seconds in the lease duration
    }
  };

  ParameterList plist;
  if (ParameterListConverter::to_param_list(pdata, plist) < 0) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: ")
      ACE_TEXT("Spdp::SpdpTransport::write() - ")
      ACE_TEXT("failed to convert from SPDPdiscoveredParticipantData ")
      ACE_TEXT("to ParameterList\n")));
    return;
  }

  wbuff_.reset();
  CORBA::UShort options = 0;
  DCPS::Serializer ser(&wbuff_, false, DCPS::Serializer::ALIGN_CDR);
  if (!(ser << hdr_) || !(ser << data_) || !(ser << encap_LE) || !(ser << options)
      || !(ser << plist)) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: Spdp::SpdpTransport::write() - ")
      ACE_TEXT("failed to serialize headers for SPDP\n")));
    return;
  }

  typedef OPENDDS_SET(ACE_INET_Addr)::const_iterator iter_t;
  for (iter_t iter = send_addrs_.begin(); iter != send_addrs_.end(); ++iter) {
    const ssize_t res =
      unicast_socket_.send(wbuff_.rd_ptr(), wbuff_.length(), *iter);
    if (res < 0) {
      ACE_TCHAR addr_buff[256] = {};
      iter->addr_to_string(addr_buff, 256, 0);
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: Spdp::SpdpTransport::write() - ")
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
    ACE_DEBUG((
          LM_ERROR,
          ACE_TEXT("(%P|%t) ERROR: Spdp::SpdpTransport::handle_input() - ")
          ACE_TEXT("error reading from %C socket %p\n")
          , (h == unicast_socket_.get_handle()) ? "unicast" : "multicast",
          ACE_TEXT("ACE_SOCK_Dgram::recv")));
    return -1;
  }

  // Handle some RTI protocol multicast to the same address
  if ((buff_.size() >= 4) && (!ACE_OS::memcmp(buff_.rd_ptr(), "RTPX", 4))) {
    return 0; // Ignore
  }

  DCPS::Serializer ser(&buff_, false, DCPS::Serializer::ALIGN_CDR);
  Header header;
  if (!(ser >> header)) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: Spdp::SpdpTransport::handle_input() - ")
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
        ACE_ERROR((
              LM_ERROR,
              ACE_TEXT("(%P|%t) ERROR: Spdp::SpdpTransport::handle_input() - ")
              ACE_TEXT("failed to deserialize DATA header for SPDP\n")));
        return 0;
      }
      submessageLength = data.smHeader.submessageLength;

      if (data.writerId != ENTITYID_SPDP_BUILTIN_PARTICIPANT_WRITER) {
        // Not our message: this could be the same multicast group used
        // for SEDP and other traffic.
        break;
      }

      ParameterList plist;
      if (data.smHeader.flags & (4 /*FLAG_D*/ | 8 /*FLAG_K*/)) {
        ser.swap_bytes(!ACE_CDR_BYTE_ORDER); // read "encap" itself in LE
        CORBA::UShort encap, options;
        if (!(ser >> encap) || (encap != encap_LE && encap != encap_BE)) {
          ACE_ERROR((LM_ERROR,
            ACE_TEXT("(%P|%t) ERROR: Spdp::SpdpTransport::handle_input() - ")
            ACE_TEXT("failed to deserialize encapsulation header for SPDP\n")));
          return 0;
        }
        ser >> options;
        // bit 8 in encap is on if it's PL_CDR_LE
        ser.swap_bytes(((encap & 0x100) >> 8) != ACE_CDR_BYTE_ORDER);
        if (!(ser >> plist)) {
          ACE_ERROR((LM_ERROR,
            ACE_TEXT("(%P|%t) ERROR: Spdp::SpdpTransport::handle_input() - ")
            ACE_TEXT("failed to deserialize data payload for SPDP\n")));
          return 0;
        }
      } else {
        plist.length(1);
        RepoId guid;
        std::memcpy(guid.guidPrefix, header.guidPrefix, sizeof(GuidPrefix_t));
        guid.entityId = ENTITYID_PARTICIPANT;
        plist[0].guid(guid);
        plist[0]._d(PID_PARTICIPANT_GUID);
      }

      outer_->data_received(data, plist);
      break;
    }
    default:
      SubmessageHeader smHeader;
      if (!(ser >> smHeader)) {
        ACE_ERROR((
              LM_ERROR,
              ACE_TEXT("(%P|%t) ERROR: Spdp::SpdpTransport::handle_input() - ")
              ACE_TEXT("failed to deserialize SubmessageHeader for SPDP\n")));
        return 0;
      }
      submessageLength = smHeader.submessageLength;
      break;
    }
    if (submessageLength && buff_.length()) {
      const size_t read = start - buff_.length();
      if (read < static_cast<size_t>(submessageLength + SMHDR_SZ)) {
        ser.skip(static_cast<CORBA::UShort>(submessageLength + SMHDR_SZ - read));
      }
    } else if (!submessageLength) {
      break; // submessageLength of 0 indicates the last submessage
    }
  }

  return 0;
}

int
Spdp::SpdpTransport::handle_exception(ACE_HANDLE )
{
  outer_->wait_for_acks().ack();
  return 0;
}

void
Spdp::SpdpTransport::acknowledge()
{
  ACE_Reactor* reactor = outer_->reactor();
  reactor->notify(this);
}

void
Spdp::signal_liveliness(DDS::LivelinessQosPolicyKind kind)
{
  sedp_.signal_liveliness(kind);
}

bool
Spdp::SpdpTransport::open_unicast_socket(u_short port_common,
                                         u_short participant_id)
{
  const u_short uni_port = port_common + outer_->disco_->d1() +
                           (outer_->disco_->pg() * participant_id);

  ACE_INET_Addr local_addr;
  OPENDDS_STRING spdpaddr = outer_->disco_->spdp_local_address().c_str();

  if (spdpaddr.empty()) {
    spdpaddr = "0.0.0.0";
  }

  if (0 != local_addr.set(uni_port, spdpaddr.c_str())) {
    ACE_DEBUG((
          LM_ERROR,
          ACE_TEXT("(%P|%t) Spdp::SpdpTransport::open_unicast_socket() - ")
          ACE_TEXT("failed setting unicast local_addr to port %d %p\n"),
          uni_port, ACE_TEXT("ACE_INET_Addr::set")));
    throw std::runtime_error("failed to set unicast local address");
  }

  if (!OpenDDS::DCPS::open_appropriate_socket_type(unicast_socket_, local_addr)) {
    if (DCPS::DCPS_debug_level > 3) {
      ACE_DEBUG((
            LM_WARNING,
            ACE_TEXT("(%P|%t) Spdp::SpdpTransport::open_unicast_socket() - ")
            ACE_TEXT("failed to open_appropriate_socket_type unicast socket on port %d %p.  ")
            ACE_TEXT("Trying next participantId...\n"),
            uni_port, ACE_TEXT("ACE_SOCK_Dgram::open")));
    }
    return false;

  } else if (DCPS::DCPS_debug_level > 3) {
    ACE_DEBUG((
          LM_INFO,
          ACE_TEXT("(%P|%t) Spdp::SpdpTransport::open_unicast_socket() - ")
          ACE_TEXT("opened unicast socket on port %d\n"),
          uni_port));
  }

  if (!OpenDDS::DCPS::set_socket_multicast_ttl(unicast_socket_, outer_->disco_->ttl())) {
    ACE_DEBUG((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: Spdp::SpdpTransport::open_unicast_socket() - ")
               ACE_TEXT("failed to set TTL value to %d ")
               ACE_TEXT("for port:%hd %p\n"),
               outer_->disco_->ttl(), uni_port, ACE_TEXT("OpenDDS::DCPS::set_socket_multicast_ttl:")));
    throw std::runtime_error("failed to set TTL");
  }
  return true;
}

bool
Spdp::get_default_locators(const RepoId& part_id, OpenDDS::DCPS::LocatorSeq& target,
                           bool& inlineQos)
{
  DiscoveredParticipantIter part_iter = participants_.find(part_id);
  if (part_iter == participants_.end()) {
    return false;
  } else {
    inlineQos = part_iter->second.pdata_.participantProxy.expectsInlineQos;
    OpenDDS::DCPS::LocatorSeq& mc_source =
          part_iter->second.pdata_.participantProxy.defaultMulticastLocatorList;
    OpenDDS::DCPS::LocatorSeq& uc_source =
          part_iter->second.pdata_.participantProxy.defaultUnicastLocatorList;
    CORBA::ULong mc_source_len = mc_source.length();
    CORBA::ULong uc_source_len = uc_source.length();
    CORBA::ULong target_len = target.length();
    target.length(mc_source_len + uc_source_len + target_len);
    // Copy multicast
    for (CORBA::ULong mci = 0; mci < mc_source.length(); ++mci) {
      target[target_len + mci] = mc_source[mci];
    }
    // Copy unicast
    for (CORBA::ULong uci = 0; uci < uc_source.length(); ++uci) {
      target[target_len + mc_source_len + uci] = uc_source[uci];
    }
  }
  return true;
}

bool
Spdp::associated() const
{
  return !participants_.empty();
}

bool
Spdp::has_discovered_participant(const DCPS::RepoId& guid)
{
  return participants_.find(guid) != participants_.end();
}


void
Spdp::get_discovered_participant_ids(DCPS::RepoIdSet& results) const
{
  DiscoveredParticipantMap::const_iterator idx;
  for (idx = participants_.begin(); idx != participants_.end(); ++idx)
  {
    results.insert(idx->first);
  }
}

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
