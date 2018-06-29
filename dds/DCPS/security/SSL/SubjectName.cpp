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
    if (in == NULL) {
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
    map_.clear();

    ACE_UNUSED_ARG(push_back);

    // This parser is meant only to cover basic cases, more advanced cases
    // will need something different Specifically, this won't correctly handle
    // all escaped characters or all UTF-8 strings

    // Use size_t variables to mark positions of token beginnings and ends

    // We'll use "st" to mark positions for sequence tokens
    size_t st_begin = 0;
    size_t st_end = input.find_first_of(s_del);

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
      if (st_begin_clean != std::string::npos &&
          st_end_clean != std::string::npos) {
        std::string st_clean =
          st.substr(st_begin_clean, st_end_clean - st_begin_clean + 1);

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
          if (nt_begin_clean != std::string::npos &&
              nt_end_clean != std::string::npos) {
            std::string nt_clean =
              nt.substr(nt_begin_clean, nt_end_clean - nt_begin_clean + 1);

            // We'll use "vt" to mark positions for value tokens
            size_t vt_begin = nt_end + 2;  // Skip over the (single) delimiter
            size_t vt_end = st_clean.size() - 1;

            std::string vt = st_clean.substr(vt_begin, vt_end - vt_begin + 1);

            size_t vt_begin_clean = vt.find_first_not_of(a_trim);
            size_t vt_end_clean = vt.find_last_not_of(a_trim);

            // If we found a clean value token
            if (vt_begin_clean != std::string::npos &&
                vt_end_clean != std::string::npos) {
              std::string vt_clean =
                vt.substr(vt_begin_clean, vt_end_clean - vt_begin_clean + 1);

              // Push our clean pair into the map
              map_[nt_clean] = vt_clean;
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

    return map_.empty() ? 1 : 0;
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
    return simple_avp_seq_parse(in, ",", "=", " ", " ", false);
  }

  bool SubjectName::operator==(const SubjectName& rhs) const
  {
    bool result = (map_.size() == rhs.map_.size());
    for (AttrMap::const_iterator i1 = map_.begin(), i2 = rhs.map_.begin();
         result == true && i1 != map_.end() && i2 != rhs.map_.end();
         ++i1, ++i2) {
      if (i1->first.compare(i2->first) != 0 ||
          i1->second.compare(i2->second) != 0) {
        result = false;
      }
    }
    return result;
  }

  bool SubjectName::operator!=(const SubjectName& rhs) const
  {
    return !(*this == rhs);
  }

}  // namespace SSL
}  // namespace Security
}  // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
