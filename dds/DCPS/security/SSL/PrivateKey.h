/*
 * Distributed under the DDS License.
 * See: http://www.DDS.org/license.html
 */

#ifndef OPENDDS_SECURITY_SSL_PRIVATEKEY_H
#define OPENDDS_SECURITY_SSL_PRIVATEKEY_H

#include "dds/DCPS/security/DdsSecurity_Export.h"
#include "dds/DCPS/unique_ptr.h"
#include <string>
#include <openssl/evp.h>

namespace OpenDDS {
  namespace Security {
    namespace SSL {

      class DdsSecurity_Export PrivateKey
      {
      public:

        typedef DCPS::unique_ptr<PrivateKey> unique_ptr;

        PrivateKey(const std::string& uri, const std::string password = "");

        PrivateKey();

        ~PrivateKey();

        PrivateKey& operator=(const PrivateKey& rhs);

        void load(const std::string& uri, const std::string& password = "");

      private:

        static EVP_PKEY* EVP_PKEY_from_pem(const std::string& path, const std::string& password = "");

        EVP_PKEY* k_;
      };

    }
  }
}

#endif
