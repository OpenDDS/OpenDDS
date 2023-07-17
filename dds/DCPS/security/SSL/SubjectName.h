/*
 * Distributed under the OpenDDS License.
 * See: http://www.OpenDDS.org/license.html
 */

#ifndef OPENDDS_DCPS_SECURITY_SSL_SUBJECTNAME_H
#define OPENDDS_DCPS_SECURITY_SSL_SUBJECTNAME_H

#include <dds/Versioned_Namespace.h>
#include <dds/DCPS/security/OpenDDS_Security_Export.h>

#include <string>
#include <vector>
#include <algorithm>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace Security {
namespace SSL {

// A parser for subject names represented by the string format described in RFC 4514.
class Parser {
public:
  typedef std::pair<std::string, std::string> AttributeValueAssertion;
  typedef std::vector<AttributeValueAssertion> AVAVec;

  Parser(std::string in) : in_(in), pos_(0) {}

  // Parse and store the result to the provided vector.
  bool parse(AVAVec& store);
  void reset(std::string in);

private:
  bool is_alpha(char c) const;
  bool accept(char c);
  bool distinguished_name(AVAVec&);
  bool relative_distinguished_name(AVAVec&);
  bool attribute_type_value(AVAVec&);
  bool attribute_type(std::string& at);
  bool validate_attribute_type(const std::string& at) const;

  bool attribute_value(std::string& av);
  void unescape(std::string& av) const;
  void replace_all(std::string& str, const std::string& s, const std::string& t) const;
  bool validate_attribute_value(const std::string& av) const;

  std::string in_;

  // Current position in the input string.
  std::string::size_type pos_;
};

class OpenDDS_Security_Export SubjectName {
private:
  typedef std::vector<std::pair<std::string, std::string> > AttrVec;

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

  typedef AttrVec::const_iterator const_iterator;
  const_iterator begin() const { return attr_vec_.begin(); }
  const_iterator end() const { return attr_vec_.end(); }
  const_iterator find(const std::string& key) const;

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

  AttrVec attr_vec_;
};

}  // namespace SSL
}  // namespace Security
}  // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif
