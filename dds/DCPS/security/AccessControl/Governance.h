/*
 * Distributed under the OpenDDS License.
 * See: http://www.OpenDDS.org/license.html
 */

#ifndef OPENDDS_DCPS_SECURITY_ACCESSCONTROL_GOVERNANCE_H
#define OPENDDS_DCPS_SECURITY_ACCESSCONTROL_GOVERNANCE_H

#include "DomainIdSet.h"

#include <dds/DCPS/security/SSL/SignedDocument.h>
#include <dds/DCPS/RcObject.h>

#include <dds/DdsSecurityCoreC.h>

#include <string>
#include <vector>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace Security {

class Governance : public DCPS::RcObject {
public:
  typedef DCPS::RcHandle<Governance> shared_ptr;

  struct TopicAccessRule {
    TopicAccessRule();
    std::string topic_expression;
    DDS::Security::TopicSecurityAttributes topic_attrs;
    std::string metadata_protection_kind;
    std::string data_protection_kind;
  };

  typedef std::vector<TopicAccessRule> TopicAccessRules;

  struct DomainRule {
    DomainIdSet domains;
    DDS::Security::ParticipantSecurityAttributes domain_attrs;
    TopicAccessRules topic_rules;
  };

  typedef std::vector<DomainRule> GovernanceAccessRules;

  Governance();

  int load(const SSL::SignedDocument& doc);

  GovernanceAccessRules& access_rules()
  {
    return access_rules_;
  }

private:
  GovernanceAccessRules access_rules_;
};

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif
