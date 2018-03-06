/*
 * Distributed under the DDS License.
 * See: http://www.DDS.org/license.html
 */

#ifndef OPENDDS_SECURITY_SSL_SignedDocument_H
#define OPENDDS_SECURITY_SSL_SignedDocument_H

#include "dds/DCPS/security/DdsSecurity_Export.h"
#include "dds/DCPS/unique_ptr.h"
#include <string>
#include <openssl/pkcs7.h>

namespace OpenDDS {
  namespace Security {
    namespace SSL {

      class DdsSecurity_Export SignedDocument
      {
      public:

        typedef DCPS::unique_ptr<SignedDocument> unique_ptr;

        SignedDocument(const std::string& uri, const std::string password = "");

        SignedDocument();

        ~SignedDocument();

        SignedDocument& operator=(const SignedDocument& rhs);

        void load(const std::string& uri, const std::string& password = "");

      private:

        static PKCS7* PKCS7_from_pem(const std::string& path, const std::string& password = "");

        PKCS7* doc_;
      };

    }
  }
}

#endif
