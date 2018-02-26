/*
 * Distributed under the DDS License.
 * See: http://www.DDS.org/license.html
 */

#include "PrivateKey.h"

namespace OpenDDS {
  namespace Security {
    namespace SSL {

#if 0
      PrivateKey::PrivateKey(const std::string& uri, const std::string password)
      {

      }
#endif

      PrivateKey::PrivateKey() : k_(NULL)
      {

      }

      PrivateKey::~PrivateKey()
      {
        if (k_) EVP_PKEY_free(k_);
      }

      PrivateKey& PrivateKey::operator=(const PrivateKey& rhs)
      {
        if (this != &rhs) {
            if (rhs.k_) {
                k_ = rhs.k_;
                EVP_PKEY_up_ref(k_);

            } else {
                k_ = NULL;
            }
        }
        return *this;
      }

    }
  }
}
