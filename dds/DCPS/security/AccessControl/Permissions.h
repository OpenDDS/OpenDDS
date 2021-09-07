/*
 * Distributed under the OpenDDS License.
 * See: http://www.OpenDDS.org/license.html
 */

#ifndef OPENDDS_DCPS_SECURITY_ACCESSCONTROL_PERMISSIONS_H
#define OPENDDS_DCPS_SECURITY_ACCESSCONTROL_PERMISSIONS_H

#include "DomainIdSet.h"

#include <dds/DCPS/security/SSL/SignedDocument.h>
#include <dds/DCPS/security/SSL/SubjectName.h>
#include <dds/DCPS/RcHandle_T.h>
#include <dds/DCPS/RcObject.h>

#include <dds/DdsDcpsCoreC.h>
#include <dds/DdsSecurityCoreC.h>
#include <dds/DdsSecurityParamsC.h>

#include <string>
#include <vector>
#include <ctime>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace Security {

struct Permissions : DCPS::RcObject {
  typedef DCPS::RcHandle<Permissions> shared_ptr;

  enum AllowDeny_t {
    ALLOW,
    DENY
  };

  enum PublishSubscribe_t {
    PUBLISH,
    SUBSCRIBE
  };

  struct Validity_t {
    time_t not_before;
    time_t not_after;
  };

  struct Action {
    PublishSubscribe_t ps_type;
    std::vector<std::string> topics;
    std::vector<std::string> partitions;

    bool topic_matches(const char* topic) const;
    bool partitions_match(const DDS::StringSeq& entity_partitions, AllowDeny_t allow_or_deny) const;
  };

  typedef std::vector<Action> Actions;

  struct Rule {
    AllowDeny_t ad_type;
    DomainIdSet domains;
    Actions actions;
  };

  typedef std::vector<Rule> Rules;

  struct Grant : DCPS::RcObject {
    std::string name;
    SSL::SubjectName subject;
    Validity_t validity;
    AllowDeny_t default_permission;
    Rules rules;
  };

  typedef DCPS::RcHandle<Grant> Grant_rch;

  typedef std::vector<Grant_rch> Grants;

  int load(const SSL::SignedDocument& doc);

  bool has_grant(const SSL::SubjectName& name) const;
  Grant_rch find_grant(const SSL::SubjectName& name) const;

  Grants grants_;
  DDS::Security::PermissionsToken perm_token_;
  DDS::Security::PermissionsCredentialToken perm_cred_token_;
};

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif
