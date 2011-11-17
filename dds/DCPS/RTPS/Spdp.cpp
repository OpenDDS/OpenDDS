/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "Spdp.h"

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
  : domain_(domain), guid_(guid), qos_(qos), topic_counter_(0)
{}

void
Spdp::ignore_domain_participant(const RepoId& ignoreId)
{
  //TODO
}

bool
Spdp::update_domain_participant_qos(const DDS::DomainParticipantQos& qos)
{
  qos_ = qos;
  //TODO: write to transport
  return true;
}

void
Spdp::bit_subscriber(const DDS::Subscriber_var& bit_subscriber)
{
  bit_subscriber_ = bit_subscriber;
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

}
}
