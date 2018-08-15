#include "dds/DCPS/security/CommonUtilities.h"

#include <string>
#include <cstdio>
#include <vector>

namespace OpenDDS {
namespace Security {
namespace CommonUtilities {

URI::URI(const std::string& src)
  : scheme(URI_UNKNOWN), everything_else("") //authority(), path(""), query(""), fragment("")
{
  typedef std::vector<std::pair<std::string, Scheme> > uri_pattern_t;

  uri_pattern_t uri_patterns;
  uri_patterns.push_back(std::make_pair("file:", URI_FILE));
  uri_patterns.push_back(std::make_pair("data:", URI_DATA));
  uri_patterns.push_back(std::make_pair("pkcs11:", URI_PKCS11));

  for (uri_pattern_t::iterator i = uri_patterns.begin();
       i != uri_patterns.end(); ++i) {
    const std::string& pfx = i->first;
    size_t pfx_end = pfx.length();

    if (src.substr(0, pfx_end) == pfx) {
      everything_else = src.substr(pfx_end, std::string::npos);
      scheme = i->second;
      break;
    }
  }
}

int increment_handle(int& next)
{
  // handles are 32-bit signed values (int on all supported platforms)
  // the only special value is 0 for HANDLE_NIL, 'next' starts at 1
  // signed increment is not guaranteed to roll over so we implement our own
  static const int LAST_POSITIVE_HANDLE(0x7fffffff);
  static const int FIRST_NEGATIVE_HANDLE(-LAST_POSITIVE_HANDLE);
  if (next == 0) {
    ACE_ERROR((LM_ERROR, "(%P|%t) OpenDDS::Security::CommonUtilities::"
               "increment_handle ERROR - out of handles\n"));
    return 0;
  }
  const int h = next;
  if (next == LAST_POSITIVE_HANDLE) {
    next = FIRST_NEGATIVE_HANDLE;
  } else {
    ++next;
  }
  return h;
}

void set_security_error(DDS::Security::SecurityException& ex,
                        int code,
                        int minor_code,
                        const char* message)
{
  ex.code = code;
  ex.minor_code = minor_code;
  ex.message = message;
}

void set_security_error(DDS::Security::SecurityException& ex,
                        int code,
                        int minor_code,
                        const char* message,
                        const unsigned char (&a1)[4],
                        const unsigned char (&a2)[4])
{
  std::string full(message);
  const size_t i = full.size();
  full.resize(i + 25);
  std::sprintf(&full[i], " %.2x %.2x %.2x %.2x, %.2x %.2x %.2x %.2x",
               a1[0], a1[1], a1[2], a1[3], a2[0], a2[1], a2[2], a2[3]);
  set_security_error(ex, code, minor_code, full.c_str());
}

}
}
}
