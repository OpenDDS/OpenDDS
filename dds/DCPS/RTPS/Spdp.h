/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_RTPS_SPDP_H
#define OPENDDS_RTPS_SPDP_H

#include "dds/DdsDcpsInfrastructureC.h"
#include "dds/DdsDcpsInfoUtilsC.h"
#include "dds/DdsDcpsInfoC.h"

#include "dds/DCPS/RcObject_T.h"
#include "dds/DCPS/GuidUtils.h"

#include <map>
#include <string>

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

namespace OpenDDS {
namespace RTPS {

/// Each instance of class Spdp represents the implementation of the RTPS
/// Simple Participant Discovery Protocol for a single local DomainParticipant.
class Spdp : public DCPS::RcObject<ACE_SYNCH_MUTEX> {
public:
  Spdp(DDS::DomainId_t domain, const DCPS::RepoId& guid,
       const DDS::DomainParticipantQos& qos);

  // Participant
  void ignore_domain_participant(const DCPS::RepoId& ignoreId);
  bool update_domain_participant_qos(const DDS::DomainParticipantQos& qos);

  // Topic
  DCPS::TopicStatus assert_topic(DCPS::RepoId_out topicId,
                                 const char* topicName,
                                 const char* dataTypeName,
                                 const DDS::TopicQos& qos);
  DCPS::TopicStatus remove_topic(const DCPS::RepoId& topicId,
                                 std::string& name);
  void ignore_topic(const DCPS::RepoId& ignoreId);
  bool update_topic_qos(const DCPS::RepoId& topicId, const DDS::TopicQos& qos,
                        std::string& name);
  struct TopicDetails {
    std::string data_type_;
    DDS::TopicQos qos_;
    DCPS::RepoId repo_id_;
  };

private:
  const DDS::DomainId_t domain_;
  const DCPS::RepoId guid_;
  DDS::DomainParticipantQos qos_;
  std::map<std::string, TopicDetails> topics_;
  std::map<DCPS::RepoId, std::string, DCPS::GUID_tKeyLessThan> topic_names_;
  unsigned int topic_counter_;
};

}
}

#endif // OPENDDS_RTPS_SPDP_H
