/*
 * Distributed under the DDS License.
 * See: http://www.DDS.org/license.html
 */

#ifndef OPENDDS_SECURITY_SSL_H
#define OPENDDS_SECURITY_SSL_H

#include <string>
#include <iostream>
#include <openssl/x509.h>

namespace OpenDDS {
  namespace Security {
    namespace SSL {

      class Certificate
      {
      public:
        Certificate(const std::string& uri, const std::string& password = "");
        ~Certificate();

        X509* get()
        {
          return x_;
        }

      private:

        enum URI_SCHEME
        {
          URI_UNKNOWN,
          URI_FILE,
          URI_DATA,
          URI_PKCS11,
        };

        static X509* x509_from_pem(const std::string& path, const std::string& password = "");
        static URI_SCHEME extract_uri_info(const std::string& uri, std::string& path);

        X509* x_;
      };

      std::ostream& operator<<(std::ostream& lhs, Certificate& rhs);
    }
  }
}

#endif
