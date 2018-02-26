/*
 * Distributed under the DDS License.
 * See: http://www.DDS.org/license.html
 */

#ifndef OPENDDS_SECURITY_SSL_PRIVATEKEY_H
#define OPENDDS_SECURITY_SSL_PRIVATEKEY_H

#include <string>
#include <openssl/evp.h>

namespace OpenDDS {
  namespace Security {
    namespace SSL {

      class PrivateKey
      {
      public:
        //PrivateKey(const std::string& uri, const std::string password = "");

        PrivateKey();

        ~PrivateKey();

        PrivateKey& operator=(const PrivateKey& rhs);

      private:
        EVP_PKEY* k_;

      };

    }
  }
}

#endif
