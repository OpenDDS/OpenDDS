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

  struct Action {
    PublishSubscribe_t ps_type;
    std::vector<std::string> topics;
    std::vector<std::string> partitions;

    bool topic_matches(const char* topic) const;
    bool partitions_match(const DDS::StringSeq& entity_partitions, AllowDeny_t allow_or_deny) const;
  };

  typedef std::list<Action> Actions;

  struct Rule {
    AllowDeny_t ad_type;
    std::set< ::DDS::Security::DomainId_t > domain_list;
    Actions actions;
  };

  typedef std::list<Rule> Rules;

  struct Grant {
    std::string name;
    SSL::SubjectName subject;
    Validity_t validity;
    AllowDeny_t default_permission;
    Rules rules;
  };

  typedef std::vector<Grant> Grants;

  struct AcPerms {
    Grants grants;
    DDS::Security::PermissionsToken perm_token;
    DDS::Security::PermissionsCredentialToken perm_cred_token;
  };

  Permissions();

  int load(const SSL::SignedDocument& doc);

  AcPerms& data()
  {
    return perm_data_;
  }

  bool find_grant(const SSL::SubjectName& name, Grant* found = 0) const;

private:

  AcPerms perm_data_;
};

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif
