/*
 * Distributed under the DDS License.
 * See: http://www.DDS.org/license.html
 */

#include "Utils.h"
#include "dds/DCPS/GuidUtils.h"
#include <vector>
#include <utility>
#include <bitset>

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

/* Gets hash[i] given i > 0 with the entire byte-array right-shifted by 1 bit. */
#define HASH_RSHIFT_1BIT(HASH, IDX) ((((HASH)[(IDX)-1] << 7u) & 0x80) | (((HASH)[(IDX)] >> 1u) & 0x7F))

      int make_adjusted_guid(const OpenDDS::DCPS::GUID_t src, OpenDDS::DCPS::GUID_t dst, const Certificate& target)
      {
        int result = 1;

        dst = OpenDDS::DCPS::GUID_UNKNOWN;

        /* Grab hash for first 48 bytes of prefix */

        std::vector<unsigned char> hash;
        result = target.subject_name_digest(hash);

        if (result == 0 && hash.size() >= 6) {
            unsigned char bytes[] = reinterpret_cast<unsigned char[]>(dst);
            bytes[0] = 0x80 | ((hash[0] >> 7u) & 0x7F);
            bytes[1] = HASH_RSHIFT_1BIT(hash, 1);
            bytes[2] = HASH_RSHIFT_1BIT(hash, 2);
            bytes[3] = HASH_RSHIFT_1BIT(hash, 3);
            bytes[4] = HASH_RSHIFT_1BIT(hash, 4);
            bytes[5] = HASH_RSHIFT_1BIT(hash, 5);

            /* Now calculate hash from src guid for remaining 48 bytes */

        }

        return result;
      }

    }
  }
}
