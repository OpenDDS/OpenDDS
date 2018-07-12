#include "dds/DCPS/security/CommonUtilities.h"

#include <string>
#include <cstdio>
#include <vector>

namespace OpenDDS {
namespace Security {
namespace CommonUtilities {

URI_SCHEME extract_uri_info(const std::string& uri, std::string& path)
{
  typedef std::vector<std::pair<std::string, URI_SCHEME> > uri_pattern_t;

  URI_SCHEME result = URI_UNKNOWN;
  path = "";

  uri_pattern_t uri_patterns;
  uri_patterns.push_back(std::make_pair("file:", URI_FILE));
  uri_patterns.push_back(std::make_pair("data:", URI_DATA));
  uri_patterns.push_back(std::make_pair("pkcs11:", URI_PKCS11));

  for (uri_pattern_t::iterator i = uri_patterns.begin();
       i != uri_patterns.end(); ++i) {
    const std::string& pfx = i->first;
    size_t pfx_end = pfx.length();

    if (uri.substr(0, pfx_end) == pfx) {
      path = uri.substr(pfx_end, std::string::npos);
      result = i->second;
      break;
    }
  }
  return result;
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
