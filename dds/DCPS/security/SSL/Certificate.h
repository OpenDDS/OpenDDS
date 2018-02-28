/*
 * Distributed under the DDS License.
 * See: http://www.DDS.org/license.html
 */

#ifndef OPENDDS_SECURITY_SSL_CERTIFICATE_H
#define OPENDDS_SECURITY_SSL_CERTIFICATE_H

#include "dds/DCPS/security/DdsSecurity_Export.h"
#include <string>
#include <vector>
#include <iostream>
#include <openssl/x509.h>

namespace OpenDDS {
  namespace Security {
    namespace SSL {

      class DdsSecurity_Export Certificate
      {
      public:

        friend std::ostream& operator<<(std::ostream&, const Certificate&);

        Certificate(const std::string& uri, const std::string& password = "");

        Certificate();

        ~Certificate();

        Certificate& operator=(const Certificate& rhs);

        void load(const std::string& uri, const std::string& password = "");

        int validate(Certificate& ca, unsigned long int flags = 0u);

        int subject_name_to_DER(std::vector<unsigned char>& dst) const;

        int subject_name_to_str(std::string& dst) const;

        int subject_name_digest(std::vector<unsigned char>& dst) const;

      private:

        static X509* x509_from_pem(const std::string& path, const std::string& password = "");

        X509* x_;
      };

      std::ostream& operator<<(std::ostream&, const Certificate&);
    }
  }
}

#endif
