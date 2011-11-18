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

#include "dds/DdsDcpsGuidC.h"


namespace OpenDDS {
namespace RTPS {
using DCPS::RepoId;

namespace {
  void assign(DCPS::EntityKey_t& lhs, unsigned int rhs)
  {
    lhs[0] = static_cast<CORBA::Octet>(rhs);
    lhs[1] = static_cast<CORBA::Octet>(rhs >> 8);
    lhs[2] = static_cast<CORBA::Octet>(rhs >> 16);
  }
}


Spdp::Spdp(DDS::DomainId_t domain, const RepoId& guid,
           const DDS::DomainParticipantQos& qos)
  : domain_(domain), guid_(guid), qos_(qos), spdp_(this), topic_counter_(0)
{
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
Spdp::bit_subscriber(const DDS::Subscriber_var& bit_subscriber)
{
  bit_subscriber_ = bit_subscriber;
}

Spdp::SpdpTransport::SpdpTransport(Spdp* outer)
  : outer_(outer)
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
  static const CORBA::ULong encap = 0x00000300; // {options, PL_CDR_LE}

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
      0 /*manualLivelinessCount*/
    },
    { // Duration_t (leaseDuration)
      static_cast<CORBA::Long>(outer_->lease_duration_.sec()),
      0 // we are not supporting fractional seconds in the lease duration
    }
  };

  ParameterList plist;
  ParameterListConverter::to_param_list(pdata, plist);
  size_t size = 0, padding = 0;
  using DCPS::gen_find_size;
  gen_find_size(hdr_, size, padding);
  gen_find_size(data_, size, padding);
  size += DCPS::max_marshaled_size_ulong(); // encap
  gen_find_size(plist, size, padding);

  ACE_Message_Block send_buff(size + padding);
  DCPS::Serializer ser(&send_buff, false, DCPS::Serializer::ALIGN_CDR);
  if (!(ser << hdr_) || !(ser << data_) || !(ser << encap) || !(ser << plist)) {
    //TODO: error
  }

  typedef std::set<ACE_INET_Addr>::const_iterator iter_t;
  for (iter_t iter = send_addrs_.begin(); iter != send_addrs_.end(); ++iter) {
    ssize_t res =
      unicast_socket_.send(send_buff.rd_ptr(), send_buff.length(), *iter);
    //TODO: check
  }
}

int
Spdp::SpdpTransport::handle_timeout(const ACE_Time_Value&, const void*)
{
  write();
  return 0;
}

int
Spdp::SpdpTransport::handle_input(ACE_HANDLE h)
{
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
