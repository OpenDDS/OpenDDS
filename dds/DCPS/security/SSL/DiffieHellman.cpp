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

      class DH_Constructor
      {
      public:
        DH_Constructor() : result(NULL), params(NULL), keygen_ctx(NULL) {}
        ~DH_Constructor()
        {
          if (params) EVP_PKEY_free(params);
          if (keygen_ctx) EVP_PKEY_CTX_free(keygen_ctx);
        }

        EVP_PKEY* operator()()
        {
          if (! (params = EVP_PKEY_new())) {
            OPENDDS_SSL_LOG_ERR("EVP_PKEY_new failed");
            return NULL;
          }

          if (1 != EVP_PKEY_set1_DH(params, DH_get_2048_256())) {
            OPENDDS_SSL_LOG_ERR("EVP_PKEY_set1_DH failed");
            return NULL;
          }

          if (! (keygen_ctx = EVP_PKEY_CTX_new(params, NULL))) {
            OPENDDS_SSL_LOG_ERR("EVP_PKEY_CTX_new failed");
            return NULL;
          }

          if (1 != EVP_PKEY_keygen_init(keygen_ctx)) {
            OPENDDS_SSL_LOG_ERR("EVP_PKEY_keygen_init failed");
            return NULL;
          }

          if (1 != EVP_PKEY_keygen(keygen_ctx, &result)) {
            OPENDDS_SSL_LOG_ERR("EVP_PKEY_keygen failed");
            return NULL;
          }

          return result;
        }

      private:
        EVP_PKEY* result;
        EVP_PKEY* params;
        EVP_PKEY_CTX* keygen_ctx;
      };

      int DH_2048_MODP_256_PRIME::init()
      {
        if (k_) return 0;

        DH_Constructor dh;
        k_ = dh();

        if (k_) {
            return 0;

        } else {
            return 1;
        }
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

      class ECDH_Constructor {
      public:
        ECDH_Constructor() :
          result(NULL), params(NULL), paramgen_ctx(NULL), keygen_ctx(NULL) {}

        ~ECDH_Constructor()
        {
          if (params) EVP_PKEY_free(params);
          if (paramgen_ctx) EVP_PKEY_CTX_free(paramgen_ctx);
          if (keygen_ctx) EVP_PKEY_CTX_free(keygen_ctx);
        }

        EVP_PKEY* operator()()
        {
          if (! (paramgen_ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_EC, NULL))) {
            OPENDDS_SSL_LOG_ERR("EVP_PKEY_CTX_new_id");
            return NULL;
          }

          if (! (params = EVP_PKEY_new())) {
            OPENDDS_SSL_LOG_ERR("EVP_PKEY_new failed");
            return NULL;
          }

          if (1 != EVP_PKEY_paramgen_init(paramgen_ctx)) {
            OPENDDS_SSL_LOG_ERR("EVP_PKEY_paramgen_init failed");
            return NULL;
          }

          if (1 != EVP_PKEY_CTX_set_ec_paramgen_curve_nid(paramgen_ctx, NID_X9_62_prime256v1)) {
            OPENDDS_SSL_LOG_ERR("EVP_PKEY_CTX_set_ec_paramgen_curve_nid failed");
            return NULL;
          }

          if (! EVP_PKEY_paramgen(paramgen_ctx, &params)) {
            OPENDDS_SSL_LOG_ERR("EVP_PKEY_paramgen failed");
            return NULL;
          }

          if (! (keygen_ctx = EVP_PKEY_CTX_new(params, NULL))) {
            OPENDDS_SSL_LOG_ERR("EVP_PKEY_CTX_new failed");
            return NULL;
          }

          if (1 != EVP_PKEY_keygen_init(keygen_ctx)) {
            OPENDDS_SSL_LOG_ERR("EVP_PKEY_keygen_init failed");
            return NULL;
          }

          if (1 != EVP_PKEY_keygen(keygen_ctx, &result)) {
            OPENDDS_SSL_LOG_ERR("EVP_PKEY_keygen failed");
            return NULL;
          }

          return result;
        }

      private:
        EVP_PKEY* result;
        EVP_PKEY* params;
        EVP_PKEY_CTX* paramgen_ctx;
        EVP_PKEY_CTX* keygen_ctx;
      };

      int ECDH_PRIME_256_V1_CEUM::init()
      {
        if (k_) return 0;

        ECDH_Constructor ecdh;
        k_ = ecdh();

        if (k_) {
            return 0;

        } else {
            return 1;
        }
      }

      class ECDH_Pubkey
      {
      public:
        ECDH_Pubkey(EVP_PKEY* pkey) : keypair(pkey), keypair_ecdh(NULL), bignum_ctx(NULL), tmp(NULL)
        {

        }
        ~ECDH_Pubkey()
        {
          if (bignum_ctx) BN_CTX_free(bignum_ctx);
          if (tmp) OPENSSL_free(tmp);
        }

        int operator()(DDS::OctetSeq& dst)
        {
          if (! keypair) return 1;

          if (NULL == (keypair_ecdh = EVP_PKEY_get1_EC_KEY(keypair))) {
            OPENDDS_SSL_LOG_ERR("EVP_PKEY_get1_EC_KEY failed");
            return 1;
          }

          if (NULL == (bignum_ctx = BN_CTX_new())) {
            OPENDDS_SSL_LOG_ERR("BN_CTX_new failed");
            return 1;
          }

          size_t count = 0u;
          if (0u == (count = EC_KEY_key2buf(keypair_ecdh,
                                            EC_KEY_get_conv_form(keypair_ecdh),
                                            &tmp,
                                            bignum_ctx)))
          {
            OPENDDS_SSL_LOG_ERR("EC_KEY_key2buf failed");
            return 1;
          }

          dst.length(count);
          std::memcpy(tmp, dst.get_buffer(), count);

          return 0;
        }

      private:
        EVP_PKEY* keypair;
        EC_KEY* keypair_ecdh;
        BN_CTX* bignum_ctx;
        unsigned char* tmp;
      };

      int ECDH_PRIME_256_V1_CEUM::pub_key(DDS::OctetSeq& dst)
      {
        ECDH_Pubkey pubkey(k_);
        return pubkey(dst);

      }

      int ECDH_PRIME_256_V1_CEUM::gen_shared_secret(const DDS::OctetSeq& pub_key)
      {
        return 1;
      }

    }
  }
}
