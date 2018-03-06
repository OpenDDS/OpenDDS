/*
 * Distributed under the DDS License.
 * See: http://www.DDS.org/license.html
 */

#ifndef OPENDDS_SECURITY_SSL_DIFFIE_HELLMAN_H
#define OPENDDS_SECURITY_SSL_DIFFIE_HELLMAN_H

#include "dds/DCPS/security/DdsSecurity_Export.h"
#include "dds/DdsDcpsCoreC.h"
#include <openssl/evp.h>

namespace OpenDDS {
  namespace Security {
    namespace SSL {

      class DdsSecurity_Export DiffieHellman
      {
      public:

        DiffieHellman();

        ~DiffieHellman();

        DiffieHellman& operator=(const DiffieHellman& rhs);

        int pub_key(DDS::OctetSeq& dst);

      private:

        EVP_PKEY* k_;
      };

    }
  }
}

#endif
