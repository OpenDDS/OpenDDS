/*
 * Distributed under the OpenDDS License.
 * See: http://www.OpenDDS.org/license.html
 */

#ifndef OPENDDS_DCPS_SECURITY_SSL_SUBJECTNAME_H
#define OPENDDS_DCPS_SECURITY_SSL_SUBJECTNAME_H

#include <dds/Versioned_Namespace.h>
#include <dds/DCPS/security/OpenDDS_Security_Export.h>

#include <string>
#include <map>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace Security {
namespace SSL {

class OpenDDS_Security_Export SubjectName {
 private:
    typedef std::map<std::string, std::string> AttrMap;

public:
  SubjectName();
  explicit SubjectName(const char*, bool permissive = false);
  explicit SubjectName(const std::string&, bool permissive = false);

  /**
    * @return int 0 on success; 1 on failure.
    */
  int parse(const char*, bool permissive = false);

  /**
    * @return int 0 on success; 1 on failure.
    */
  int parse(const std::string&, bool permissive = false);

  bool operator==(const SubjectName&) const;
  bool operator!=(const SubjectName&) const;

  typedef AttrMap::const_iterator const_iterator;
  const_iterator begin() const { return map_.begin(); }
  const_iterator end() const { return map_.end(); }
  const_iterator find(const std::string& key) const { return map_.find(key); }

private:
  /**
    * @return int 0 on success; 1 on failure.
    */
  int parse_permissive(const char*);

  /**
    * @return int 0 on success; 1 on failure.
    */
  int parse_dce(const char*);

  /**
    * @return int 0 on success; 1 on failure.
    */
  int parse_ldap_v3(const char*);

  int simple_avp_seq_parse(const char* in, const char* s_del,
                           const char* a_del, const char* s_trim,
                           const char* a_trim, bool push_back);

  AttrMap map_;
};

}  // namespace SSL
}  // namespace Security
}  // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif
