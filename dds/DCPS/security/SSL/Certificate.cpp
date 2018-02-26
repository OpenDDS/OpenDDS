/*
 * Distributed under the DDS License.
 * See: http://www.DDS.org/license.html
 */

#include <vector>
#include <utility>
#include <cstring>
#include <cerrno>
#include <openssl/pem.h>
#include <openssl/x509v3.h>
#include "Certificate.h"

namespace OpenDDS {
  namespace Security {
    namespace SSL {

      Certificate::Certificate(const std::string& uri, const std::string& password) : x_(NULL)
      {
        std::string p;
        URI_SCHEME s = extract_uri_info(uri, p);

        switch(s) {
          case URI_FILE:
            x_ = x509_from_pem(p, password);
            break;

          case URI_DATA:
          case URI_PKCS11:
          case URI_UNKNOWN:
          default:
            /* TODO use ACE logging */
            fprintf(stderr, "Certificate::Certificate: Unsupported URI scheme in cert path '%s'\n", uri.c_str());
            break;
        }
      }

      Certificate::~Certificate()
      {
        if (x_) X509_free(x_);
      }

      Certificate::URI_SCHEME Certificate::extract_uri_info(const std::string& uri, std::string& path)
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


      X509* Certificate::x509_from_pem(const std::string& path, const std::string& password)
      {
        X509* result = NULL;

        FILE* fp = fopen(path.c_str(), "r");
        if (fp) {
          if (password != "") {
              result = PEM_read_X509_AUX(fp, NULL, NULL, (void*)password.c_str());

          } else {
              result = PEM_read_X509_AUX(fp, NULL, NULL, NULL);
          }

          fclose(fp);

        } else {
          /* TODO use ACE logging */
          fprintf(stderr, "Certificate::x509_from_pem: Error '%s' reading file '%s'\n", strerror(errno), path.c_str());
        }

        return result;
      }

      std::ostream& operator<<(std::ostream& lhs, Certificate& rhs)
      {
        X509* x = rhs.get();

        if (x) {
            lhs << "Certificate: { is_ca? '" << (X509_check_ca(x) ? "yes": "no") << "'; }";

        } else {
            lhs << "NULL";
        }
        return lhs;
      }
    }
  }
}

