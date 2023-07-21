#include "SubjectName.h"

#include <iostream>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace Security {
namespace SSL {

  SubjectName::SubjectName() {}

  SubjectName::SubjectName(const std::string& in, bool permissive)
  {
    parse(in, permissive);
  }

  SubjectName::SubjectName(const char* in, bool permissive)
  {
    parse(in, permissive);
  }

  int SubjectName::parse(const std::string& in, bool permissive)
  {
    return parse(in.data(), permissive);
  }

  int SubjectName::parse(const char* in, bool permissive)
  {
    if (!in) {
      return -1;
    }

    // For now, assume ASCII encoding and not UTF-8
    if (permissive) {
      return parse_permissive(in);
    } else {
      if (in[0] == '/') {
        return parse_dce(in);
      } else {
        return parse_ldap_v3(in);
      }
    }
  }

  int SubjectName::simple_avp_seq_parse(const char* in, const char* s_del,
                                        const char* a_del, const char* s_trim,
                                        const char* a_trim, bool push_back)
  {
    std::string input(in);
    size_t input_end = input.size() - 1;
    attr_vec_.clear();

    ACE_UNUSED_ARG(push_back);

    // This parser is meant only to cover basic cases, more advanced cases
    // will need something different Specifically, this won't correctly handle
    // all escaped characters or all UTF-8 strings

    // Use size_t variables to mark positions of token beginnings and ends

    // We'll use "st" to mark positions for sequence tokens
    size_t st_begin = 0;
    size_t st_end = input.find_first_of(s_del);

    // All attributes are stored in a single map.
    std::map<std::string, std::string> attributes;

    // Loop over all the sequence tokens
    while (st_begin != std::string::npos) {
      std::string st = input.substr(
        st_begin,
        (st_end == std::string::npos ? input_end + 1 : st_end) - st_begin);

      // Use once we've found a sequnce token, trim the beginning and end to
      // get a clean token
      size_t st_begin_clean = st.find_first_not_of(s_trim);
      size_t st_end_clean = st.find_last_not_of(s_trim);

      // If we've found a clean sequence token
      if (st_begin_clean != std::string::npos && st_end_clean != std::string::npos) {
        std::string st_clean = st.substr(st_begin_clean, st_end_clean - st_begin_clean + 1);

        // We'll use "nt" to mark positions for name tokens
        size_t nt_begin = 0;
        size_t nt_end = st_clean.find_first_of(a_del);

        // If we actually found a delimiter
        if (nt_end != std::string::npos) {
          --nt_end;

          std::string nt = st_clean.substr(nt_begin, nt_end - nt_begin + 1);

          size_t nt_begin_clean = nt.find_first_not_of(a_trim);
          size_t nt_end_clean = nt.find_last_not_of(a_trim);

          // If we found a clean name token
          if (nt_begin_clean != std::string::npos && nt_end_clean != std::string::npos) {
            std::string nt_clean = nt.substr(nt_begin_clean, nt_end_clean - nt_begin_clean + 1);

            // We'll use "vt" to mark positions for value tokens
            size_t vt_begin = nt_end + 2;  // Skip over the (single) delimiter
            size_t vt_end = st_clean.size() - 1;

            std::string vt = st_clean.substr(vt_begin, vt_end - vt_begin + 1);

            size_t vt_begin_clean = vt.find_first_not_of(a_trim);
            size_t vt_end_clean = vt.find_last_not_of(a_trim);

            // If we found a clean value token
            if (vt_begin_clean != std::string::npos && vt_end_clean != std::string::npos) {
              std::string vt_clean = vt.substr(vt_begin_clean, vt_end_clean - vt_begin_clean + 1);

              // Push our clean pair into the map
              attributes.insert(std::make_pair(nt_clean, vt_clean));
            }
          }
        }
      }

      // Prepare for next iteration of loop
      if (st_end == std::string::npos) {
        st_begin = std::string::npos;
      } else {
        st_begin = st_end + 1;
        st_end = input.find_first_of(s_del, st_begin);
      }
    }

    if (!attributes.empty()) {
      attr_vec_.push_back(attributes);
    }

    return attr_vec_.empty() ? 1 : 0;
  }

  int SubjectName::parse_permissive(const char* in)
  {
    return simple_avp_seq_parse(in, ",/", "=", " ", " ", false);
  }

  int SubjectName::parse_dce(const char* in)
  {
    return simple_avp_seq_parse(in, "/", "=", " ", " ", true);
  }

  int SubjectName::parse_ldap_v3(const char* in)
  {
    Parser parser(in);
    return parser.parse(attr_vec_) ? 0 : 1;
  }

  bool SubjectName::operator==(const SubjectName& rhs) const
  {
    return attr_vec_ == rhs.attr_vec_;
  }

  bool SubjectName::operator!=(const SubjectName& rhs) const
  {
    return !(*this == rhs);
  }

  SubjectName::const_iterator SubjectName::find(const std::string& key) const
  {
    for (const_iterator it = begin(); it != end(); ++it) {
      if (it->find(key) != it->end()) {
        return it;
      }
    }
    return end();
  }

  void Parser::reset(std::string in)
  {
    in_ = in;
    pos_ = 0;
  }

  bool Parser::parse(RDNVec& store)
  {
    return distinguished_name(store);
  }

  bool Parser::distinguished_name(RDNVec& store)
  {
    if (in_.empty()) {
      return true;
    }

    RelativeDistinguishedName rdn;
    if (!relative_distinguished_name(rdn)) {
      return false;
    }
    store.push_back(rdn);

    while (accept(',')) {
      rdn.clear();
      if (!relative_distinguished_name(rdn)) {
        return false;
      }
      store.push_back(rdn);
    }
    return true;
  }

  bool Parser::accept(char c)
  {
    if (pos_ == in_.size()) {
      return false;
    }
    if (in_[pos_] == c) {
      ++pos_;
      return true;
    }
    return false;
  }

  bool Parser::relative_distinguished_name(RelativeDistinguishedName& rdn)
  {
    if (!attribute_type_value(rdn)) {
      return false;
    }
    while (accept('+')) {
      if (!attribute_type_value(rdn)) {
        return false;
      }
    }
    return true;
  }

  bool Parser::attribute_type_value(RelativeDistinguishedName& rdn)
  {
    std::string at, av;
    if (!attribute_type(at) || !accept('=') || !attribute_value(av)) {
      return false;
    }
    rdn.insert(std::make_pair(at, av));
    return true;
  }

#include <iostream>
  bool Parser::attribute_type(std::string& at)
  {
    const std::string::size_type equal_pos = in_.find_first_of("=", pos_);
    if (equal_pos == std::string::npos) {
      return false;
    }
    // Diverse from RFC 4514 to allow extra leading and trailing spaces.
    at = in_.substr(pos_, equal_pos - pos_);
    const std::string::size_type start = at.find_first_not_of(" ");
    const std::string::size_type end = at.find_last_not_of(" ");
    if (start == std::string::npos || end == std::string::npos) {
      return false;
    }

    at = at.substr(start, end - start + 1);
    pos_ = equal_pos;
    return validate_attribute_type(at);
  }

  bool Parser::is_alpha(char c) const
  {
    return ('A' <= c && c <= 'Z') || ('a' <= c && c <= 'z');
  }

  bool Parser::validate_attribute_type(const std::string& at) const
  {
    // Attibute type must start with a lower-case alphabet and followed by
    // any number of alphabets (lower or upper-case) or digits or hyphens.
    if (at.empty() || !is_alpha(at[0])) {
      return false;
    }
    for (std::string::size_type i = 0; i < at.size(); ++i) {
      const char c = at[i];
      if (!is_alpha(c) && !('0' <= c && c <= '9') && c != '-') {
        return false;
      }
    }
    return true;
  }

  bool Parser::attribute_value(std::string& av)
  {
    // The first comma or plus character that is not escaped marks the end
    // of the current value. If not found, the value runs to the end of the input.
    std::string::size_type start_pos = pos_;
    bool got_value = false;
    while (start_pos < in_.size() && !got_value) {
      std::string::size_type delimiter_pos = in_.find_first_of(",+", start_pos);
      if (delimiter_pos == std::string::npos) {
        av = in_.substr(pos_, in_.size() - pos_);
        pos_ = in_.size();
        got_value = true;
      } else {
        std::string::size_type tmp_pos = delimiter_pos - 1;
        size_t esc_count = 0;
        while (tmp_pos > 0 && in_[tmp_pos--] == '\\') {
          ++esc_count;
        }
        if (esc_count % 2 == 0) {
          av = in_.substr(pos_, delimiter_pos - pos_);
          pos_ = delimiter_pos;
          got_value = true;
        } else {
          start_pos = delimiter_pos + 1;
        }
      }
    }

    if (!got_value) {
      av = in_.substr(pos_, in_.size() - pos_);
      pos_ = in_.size();
      got_value = true;
    }

    // Diverse from RFC 4514 to allow extra leading and trailing whitespaces.
    // Note that the value itself can contain leading and trailing whitespaces.
    // In that case, the leftmost whitespace and the rightmost whitespace are escaped.
    const std::string::size_type start = av.find_first_not_of(" ");
    std::string::size_type end = av.find_last_not_of(" ");
    if (end != std::string::npos && end < av.size() - 1 && av[end] == '\\') {
      end += 1;
    }
    av = av.substr(start, end - start + 1);

    unescape(av);
    return validate_attribute_value(av);
  }

  void Parser::unescape(std::string& av) const
  {
    // Unescape the escaped characters from the attribute value.
    replace_all(av, "\\\"", "\"");
    replace_all(av, "\\+", "+");
    replace_all(av, "\\,", ",");
    replace_all(av, "\\;", ";");
    replace_all(av, "\\<", "<");
    replace_all(av, "\\>", ">");
    replace_all(av, "\\ ", " ");
    replace_all(av, "\\#", "#");
    replace_all(av, "\\=", "=");
    replace_all(av, "\\\\", "\\");
  }

  void Parser::replace_all(std::string& str, const std::string& s, const std::string& t) const
  {
    // Replace all occurrences of substring s in string str with t.
    std::string::size_type n = 0;
    while ((n = str.find(s, n)) != std::string::npos) {
      str.replace(n, s.size(), t);
      n += t.size();
    }
  }

  bool Parser::validate_attribute_value(const std::string& /*av*/) const
  {
    // TODO: Check that there is no prohibited character.
    return true;
  }

}  // namespace SSL
}  // namespace Security
}  // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
