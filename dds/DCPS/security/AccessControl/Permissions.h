/*
 * Distributed under the OpenDDS License.
 * See: http://www.OpenDDS.org/license.html
 */

#ifndef OPENDDS_ACCESS_PERMISSIONS_H
#define OPENDDS_ACCESS_PERMISSIONS_H

#include "dds/DCPS/security/SSL/SignedDocument.h"
#include "dds/DCPS/security/SSL/SubjectName.h"
#include "Governance.h"

#include <list>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace Security {

class Permissions : public DCPS::RcObject {
public:

  typedef DCPS::RcHandle<Permissions> shared_ptr;

  enum AllowDeny_t
  {
    ALLOW,
    DENY
  };

  enum PublishSubscribe_t
  {
    PUBLISH,
    SUBSCRIBE
  };

  struct Validity_t {
    std::string not_before;
    std::string not_after;
  };

  struct PermissionTopicPsRule {
    PublishSubscribe_t ps_type;
    std::vector<std::string> topic_list;
  };

  struct PermissionPartitionPs {
    PublishSubscribe_t ps_type;
    std::vector<std::string> partition_list;
  };

  typedef std::list<PermissionTopicPsRule> TopicPsRules;

  struct PermissionTopicRule {
    AllowDeny_t ad_type;
    std::set< ::DDS::Security::DomainId_t > domain_list;
    TopicPsRules topic_ps_rules;
  };

  typedef std::list<PermissionPartitionPs> PartitionPsList;

  struct PermissionsPartition {
    AllowDeny_t ad_type;
    std::set< ::DDS::Security::DomainId_t > domain_list;
    PartitionPsList partition_ps;
  };

  typedef std::list<PermissionTopicRule> TopicRules;
  typedef std::list<PermissionsPartition> Partitions;

  struct PermissionGrantRule {
    std::string grant_name;
    SSL::SubjectName subject;
    Validity_t validity;
    std::string default_permission;
    TopicRules PermissionTopicRules;
    Partitions PermissionPartitions;
  };

  typedef std::vector<PermissionGrantRule> PermissionGrantRules;

  struct AcPerms {
    PermissionGrantRules perm_rules;
    DDS::Security::PermissionsToken perm_token;
    DDS::Security::PermissionsCredentialToken perm_cred_token;
  };

  Permissions();

  int load(const SSL::SignedDocument& doc);

  AcPerms& data()
  {
    return perm_data_;
  }

  bool contains_subject_name(const SSL::SubjectName& name) const;

private:

  AcPerms perm_data_;
};

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif
