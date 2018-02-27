/*
 * Distributed under the DDS License.
 * See: http://www.DDS.org/license.html
 */

#include "Utils.h"
#include "dds/DCPS/GuidUtils.h"
#include <vector>
#include <utility>

namespace OpenDDS {
  namespace Security {
    namespace SSL {

      URI_SCHEME extract_uri_info(const std::string& uri, std::string& path)
      {
        typedef std::vector<std::pair<std::string, URI_SCHEME> > uri_pattern_t;

        URI_SCHEME result = URI_UNKNOWN;
        path = "";

        uri_pattern_t uri_patterns;
        uri_patterns.push_back(std::make_pair("file:", URI_FILE));
        uri_patterns.push_back(std::make_pair("data:", URI_DATA));
        uri_patterns.push_back(std::make_pair("pkcs11:", URI_PKCS11));

        for(uri_pattern_t::iterator i = uri_patterns.begin(); i != uri_patterns.end(); ++i) {
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

      int make_adjusted_guid(const OpenDDS::DCPS::GUID_t src, OpenDDS::DCPS::GUID_t dst, const Certificate& target)
      {
        int result = 1;

        dst = OpenDDS::DCPS::GUID_UNKNOWN;

        std::vector<unsigned char> asn1_der_digest;
        target.subject_name_digest(asn1_der_digest);

        dst.guidPrefix[0] = 1;

        return result;
      }

    }
  }
}
