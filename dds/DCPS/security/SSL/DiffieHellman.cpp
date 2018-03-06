/*
 * Distributed under the DDS License.
 * See: http://www.DDS.org/license.html
 */

#include "DiffieHellman.h"
#include "Utils.h"
#include "Err.h"
#include <openssl/dh.h>

namespace OpenDDS {
  namespace Security {
    namespace SSL {

      DiffieHellman::DiffieHellman() : k_(NULL)
      {
        load();
      }

      DiffieHellman::~DiffieHellman()
      {
        if (k_) EVP_PKEY_free(k_);
      }

      DiffieHellman& DiffieHellman::operator=(const DiffieHellman& rhs)
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

      void DiffieHellman::load()
      {
        /* Although params is an EVP_PKEY pointer, it is only used temporarily to generate
         * the parameters for key-generation. k_ gets set in the EVP_PKEY_keygen routine. It
         * is easy to confuse the two given their types. */

        EVP_PKEY* params = EVP_PKEY_new();
        if (params) {
          if (1 == EVP_PKEY_set1_DH(params, DH_get_2048_256())) {

            EVP_PKEY_CTX* keygen_ctx = EVP_PKEY_CTX_new(params, NULL);
            if (keygen_ctx) {

              int result = EVP_PKEY_keygen_init(keygen_ctx);
              if (1 == result) {

                if (1 != EVP_PKEY_keygen(keygen_ctx, &k_)) {
                  OPENDDS_SSL_LOG_ERR("EVP_PKEY_keygen failed");
                }

              } else {
                OPENDDS_SSL_LOG_ERR("EVP_PKEY_keygen_init failed");
              }

            } else {
              OPENDDS_SSL_LOG_ERR("EVP_PKEY_CTX_new allocation failed");
            }

            EVP_PKEY_CTX_free(keygen_ctx);

          } else {
            OPENDDS_SSL_LOG_ERR("failed to set EVP_PKEY to DH_get_2048_256()");
          }

          EVP_PKEY_free(params);

        } else {
          OPENDDS_SSL_LOG_ERR("failed to allocate new params EVP_PKEY");
        }
      }

      int DiffieHellman::pub_key(DDS::OctetSeq& dst)
      {
        int result = 1;

        if (k_) {
          DH* dh = EVP_PKEY_get1_DH(k_);
          if (dh) {

            const BIGNUM *pubkey, *privkey; /* Ignore the privkey but pass it in anyway since nothing documents what happens when a NULL gets passed in */
            pubkey = privkey = NULL;
            DH_get0_key(dh, &pubkey, &privkey);
            if (pubkey) {

              dst.length(BN_num_bytes(pubkey));
              if (0 < BN_bn2bin(pubkey, dst.get_buffer())) {
                  result = 0;

              } else {
                  OPENDDS_SSL_LOG_ERR("BN_bn2bin failed");
              }
            }
          }
        }
        return result;
      }

    }
  }
}
