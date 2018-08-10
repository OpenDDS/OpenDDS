/*
 * Distributed under the OpenDDS License.
 * See: http://www.OpenDDS.org/license.html
 */

#ifndef OPENDDS_ACCESS_PERMISSIONS_H
#define OPENDDS_ACCESS_PERMISSIONS_H

#include "dds/DCPS/security/SSL/SignedDocument.h"
#include "Governance.h"
#include <list>

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
    PublishSubscribe_t  ps_type;
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
    std::string subject;
    Validity_t validity;
    std::string default_permission;
    TopicRules PermissionTopicRules;
    Partitions PermissionPartitions;
  };

  typedef std::vector<PermissionGrantRule> PermissionGrantRules;

  struct AcPerms {
    DDS::Security::DomainId_t domain_id;
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

  const std::string& subject_name()
  {
    return subject_name_;
  }

private:

  bool extract_subject_name(const SSL::SignedDocument& doc);

  AcPerms perm_data_;
  std::string subject_name_;

};

}
}

#endif
