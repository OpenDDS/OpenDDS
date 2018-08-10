/*
 * Distributed under the OpenDDS License.
 * See: http://www.OpenDDS.org/license.html
 */

#ifndef OPENDDS_ACCESS_GOVERNANCE_H
#define OPENDDS_ACCESS_GOVERNANCE_H

#include "dds/DCPS/security/SSL/SignedDocument.h"
#include "dds/DdsSecurityCoreC.h"
#include "dds/DCPS/RcObject.h"
#include <string>
#include <vector>
#include <set>

namespace OpenDDS {
namespace Security {

class Governance : public DCPS::RcObject {
public:

  typedef DCPS::RcHandle<Governance> shared_ptr;

  struct TopicAccessRule {
    std::string topic_expression;
    DDS::Security::TopicSecurityAttributes topic_attrs;
    std::string metadata_protection_kind;
    std::string data_protection_kind;
  };

  typedef std::vector<TopicAccessRule> TopicAccessRules;

  struct DomainRule {
    std::set<DDS::Security::DomainId_t> domain_list;
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

#endif
