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

      DHAlgorithm::~DHAlgorithm()
      {
        if (k_) EVP_PKEY_free(k_);
      }

      DHAlgorithm& DHAlgorithm::operator= (const DHAlgorithm& rhs)
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

      bool DHAlgorithm::cmp_shared_secret(const DHAlgorithm& other) const
      {
        if (shared_secret_.length() != other.get_shared_secret().length()) {
            return false;
        }
        return (0 == std::memcmp(shared_secret_.get_buffer(),
                                 other.get_shared_secret().get_buffer(),
                                 shared_secret_.length()));
      }

      DH_2048_MODP_256_PRIME::DH_2048_MODP_256_PRIME()
      {
        init();
      }

      DH_2048_MODP_256_PRIME::~DH_2048_MODP_256_PRIME()
      {

      }

      int DH_2048_MODP_256_PRIME::init()
      {
        int result = 1;

        if (! k_) {

          /* Although params is an EVP_PKEY pointer, it is only used temporarily to generate
           * the parameters for key-generation. k_ gets set in the EVP_PKEY_keygen routine. It
           * is easy to confuse the two given their types. */

          EVP_PKEY* params = EVP_PKEY_new();
          if (params) {
            if (1 == EVP_PKEY_set1_DH(params, DH_get_2048_256())) {

              EVP_PKEY_CTX* keygen_ctx = EVP_PKEY_CTX_new(params, NULL);
              if (keygen_ctx) {

                int success = EVP_PKEY_keygen_init(keygen_ctx);
                if (1 == success) {

                  if (1 == EVP_PKEY_keygen(keygen_ctx, &k_)) {
                    result = 0;

                  } else {
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

        return result;
      }

      int DH_2048_MODP_256_PRIME::pub_key(DDS::OctetSeq& dst)
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

      int DH_2048_MODP_256_PRIME::gen_shared_secret(const DDS::OctetSeq& pub_key)
      {
        if (! k_) return 1;
        BIGNUM* pubkey = BN_bin2bn(pub_key.get_buffer(), pub_key.length(), NULL);
        if (! pubkey) {
            OPENDDS_SSL_LOG_ERR("BN_bin2bn failed");
            return 1;
        }

        DH* dh_keypair = EVP_PKEY_get1_DH(k_);
        if (! dh_keypair) {
            OPENDDS_SSL_LOG_ERR("EVP_PKEY_get1_DH failed");
            return 1;
        }

        shared_secret_.length(DH_size(dh_keypair));
        int newlen = DH_compute_key(shared_secret_.get_buffer(), pubkey, dh_keypair);
        if (newlen < 0 ) {
            OPENDDS_SSL_LOG_ERR("DH_compute_key failed");
            shared_secret_.length(0u);
            return 1;

        } else {
            shared_secret_.length(newlen);
        }

        return 0;
      }

      ECDH_PRIME_256_V1_CEUM::ECDH_PRIME_256_V1_CEUM()
      {

      }

      ECDH_PRIME_256_V1_CEUM::~ECDH_PRIME_256_V1_CEUM()
      {

      }

      int ECDH_PRIME_256_V1_CEUM::init()
      {
        if (! k_) return 1;

        EVP_PKEY* params = NULL;
        EVP_PKEY_CTX *paramgen_ctx, *keygen_ctx;
        paramgen_ctx = keygen_ctx = NULL;

        if (! (paramgen_ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_EC, NULL))) {
          OPENDDS_SSL_LOG_ERR("EVP_PKEY_CTX_new_id");
          goto error;
        }

        if (! (params = EVP_PKEY_new())) {
          OPENDDS_SSL_LOG_ERR("EVP_PKEY_new failed");
          goto error;
        }

        if (1 != EVP_PKEY_paramgen_init(paramgen_ctx)) {
          OPENDDS_SSL_LOG_ERR("EVP_PKEY_paramgen_init failed");
          goto error;
        }

        if (1 != EVP_PKEY_CTX_set_ec_paramgen_curve_nid(paramgen_ctx, NID_X9_62_prime256v1)) {
          OPENDDS_SSL_LOG_ERR("EVP_PKEY_CTX_set_ec_paramgen_curve_nid failed");
          goto error;
        }

        if (! EVP_PKEY_paramgen(paramgen_ctx, &params)) {
          OPENDDS_SSL_LOG_ERR("EVP_PKEY_paramgen failed");
          goto error;
        }

        if (! (keygen_ctx = EVP_PKEY_CTX_new(params, NULL))) {
          OPENDDS_SSL_LOG_ERR("EVP_PKEY_CTX_new failed");
          goto error;
        }

        if (1 != EVP_PKEY_keygen_init(keygen_ctx)) {
          OPENDDS_SSL_LOG_ERR("EVP_PKEY_keygen_init failed");
          goto error;
        }

        if (1 != EVP_PKEY_keygen(keygen_ctx, &k_)) {
          OPENDDS_SSL_LOG_ERR("EVP_PKEY_keygen failed");
          goto error;
        }

        if (paramgen_ctx) EVP_PKEY_CTX_free(paramgen_ctx);
        if (keygen_ctx) EVP_PKEY_CTX_free(keygen_ctx);
        if (params) EVP_PKEY_free(params);
        return 0;

        error:
          if (paramgen_ctx) EVP_PKEY_CTX_free(paramgen_ctx);
          if (keygen_ctx) EVP_PKEY_CTX_free(keygen_ctx);
          if (params) EVP_PKEY_free(params);
          return 1;
      }

      int ECDH_PRIME_256_V1_CEUM::pub_key(DDS::OctetSeq& dst)
      {
        return 1;
      }

      int ECDH_PRIME_256_V1_CEUM::gen_shared_secret(const DDS::OctetSeq& pub_key)
      {
        return 1;
      }

    }
  }
}
