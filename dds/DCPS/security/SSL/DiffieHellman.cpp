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

      int DHAlgorithm::hash_shared_secret()
      {
        DDS::OctetSeq tmp = shared_secret_;
        std::vector<const DDS::OctetSeq*> hash_data;
        hash_data.push_back(&tmp);
        return SSL::hash(hash_data, shared_secret_);
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
        DH_Constructor() : result(NULL), params(NULL), keygen_ctx(NULL), dh_2048_256(NULL) {}
        ~DH_Constructor()
        {
          if (params) EVP_PKEY_free(params);
          if (keygen_ctx) EVP_PKEY_CTX_free(keygen_ctx);
          if (dh_2048_256) DH_free(dh_2048_256);
        }

        EVP_PKEY* operator()()
        {
          if (! (params = EVP_PKEY_new())) {
            OPENDDS_SSL_LOG_ERR("EVP_PKEY_new failed");
            return NULL;
          }

          if (NULL == (dh_2048_256 = DH_get_2048_256())) {
            OPENDDS_SSL_LOG_ERR("DH_get_2048_256 failed");
            return NULL;
          }

          if (1 != EVP_PKEY_set1_DH(params, dh_2048_256)) {
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
        DH* dh_2048_256;
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

      class DH_SharedSecret
      {
      public:
	DH_SharedSecret(EVP_PKEY* pkey) :
	  keypair(NULL), pubkey(NULL)
	{
	  if (NULL == (keypair = EVP_PKEY_get1_DH(pkey))) {
            OPENDDS_SSL_LOG_ERR("EVP_PKEY_get1_DH failed");
	  }
	}
	~DH_SharedSecret()
	{
	  if (pubkey) BN_free(pubkey);
	}
	int operator()(const DDS::OctetSeq& pub_key, DDS::OctetSeq& dst)
	{
	  if (! keypair) return 1;

	  if (NULL == (pubkey = BN_bin2bn(pub_key.get_buffer(), pub_key.length(), NULL))) {
            OPENDDS_SSL_LOG_ERR("BN_bin2bn failed");
            return 1;
	  }

	  int len = DH_size(keypair);
	  dst.length(len);

	  len = DH_compute_key(dst.get_buffer(), pubkey, keypair);
	  if (len < 0 ) {
            OPENDDS_SSL_LOG_ERR("DH_compute_key failed");
            dst.length(0u);
            return 1;
	  }

	  dst.length(len);
	  return 0;
	}

      private:
	DH* keypair;
	BIGNUM* pubkey;
      };

      int DH_2048_MODP_256_PRIME::compute_shared_secret(const DDS::OctetSeq& pub_key)
      {
	DH_SharedSecret secret(k_);
	return secret(pub_key, shared_secret_);
      }

      ECDH_PRIME_256_V1_CEUM::ECDH_PRIME_256_V1_CEUM()
      {
        init();
      }

      ECDH_PRIME_256_V1_CEUM::~ECDH_PRIME_256_V1_CEUM()
      {

      }

      class ECDH_Constructor {
      public:
        ECDH_Constructor() :
          params(NULL), paramgen_ctx(NULL), keygen_ctx(NULL) {}

        ~ECDH_Constructor()
        {
          if (params) EVP_PKEY_free(params);
          if (paramgen_ctx) EVP_PKEY_CTX_free(paramgen_ctx);
          if (keygen_ctx) EVP_PKEY_CTX_free(keygen_ctx);
        }

        EVP_PKEY* operator()()
        {
          EVP_PKEY* result = NULL;

          if (NULL == (paramgen_ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_EC, NULL))) {
            OPENDDS_SSL_LOG_ERR("EVP_PKEY_CTX_new_id");
            return NULL;
          }

          if (NULL == (params = EVP_PKEY_new())) {
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

          if (1 != EVP_PKEY_paramgen(paramgen_ctx, &params)) {
            OPENDDS_SSL_LOG_ERR("EVP_PKEY_paramgen failed");
            return NULL;
          }

          if (NULL == (keygen_ctx = EVP_PKEY_CTX_new(params, NULL))) {
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

      class ecdh_pubkey_as_bignum
      {
      public:
        ecdh_pubkey_as_bignum(EVP_PKEY* pkey) :
          keypair(pkey), keypair_ecdh(NULL), pubkey(NULL), pubkey_bn(NULL), bignum_ctx(NULL)
        {

        }

        ~ecdh_pubkey_as_bignum()
        {
          if (bignum_ctx) BN_CTX_free(bignum_ctx);
          if (pubkey_bn) BN_free(pubkey_bn);
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

          if (NULL == (pubkey = EC_KEY_get0_public_key(keypair_ecdh))) {
            OPENDDS_SSL_LOG_ERR("EC_KEY_get0_public_key failed");
            return 1;
          }

          if (NULL == (pubkey_bn = EC_POINT_point2bn(EC_KEY_get0_group(keypair_ecdh),
                                                     pubkey,
                                                     EC_KEY_get_conv_form(keypair_ecdh),
                                                     NULL,
                                                     bignum_ctx)))
          {
            OPENDDS_SSL_LOG_ERR("EC_POINT_point2bn failed");
            return 1;
          }

          dst.length(BN_num_bytes(pubkey_bn));
          if (0 >= BN_bn2bin(pubkey_bn, dst.get_buffer())) {
            OPENDDS_SSL_LOG_ERR("BN_bn2bin failed");
            return 1;
          }

          return 0;
        }

      private:
        EVP_PKEY* keypair;
        EC_KEY* keypair_ecdh;
        const EC_POINT* pubkey;
        BIGNUM* pubkey_bn;
        BN_CTX* bignum_ctx;
      };

      class ecdh_pubkey_as_octets
      {
      public:
        ecdh_pubkey_as_octets(EVP_PKEY* pkey) :
          keypair(pkey), keypair_ecdh(NULL), pubkey(NULL)
        {

        }

        ~ecdh_pubkey_as_octets()
        {

        }

        int operator()(DDS::OctetSeq& dst)
        {
          if (! keypair) return 1;

          if (NULL == (keypair_ecdh = EVP_PKEY_get1_EC_KEY(keypair))) {
            OPENDDS_SSL_LOG_ERR("EVP_PKEY_get1_EC_KEY failed");
            return 1;
          }

          if (NULL == (pubkey = EC_KEY_get0_public_key(keypair_ecdh))) {
            OPENDDS_SSL_LOG_ERR("EC_KEY_get0_public_key failed");
            return 1;
          }

          size_t len = 0u;
          if (0 == (len = EC_POINT_point2oct(EC_KEY_get0_group(keypair_ecdh),
                                             pubkey,
                                             EC_KEY_get_conv_form(keypair_ecdh),
                                             NULL,
                                             0u,
                                             NULL)))
          {
            OPENDDS_SSL_LOG_ERR("EC_POINT_point2oct failed");
            return 1;
          }

          dst.length(len);

          if (0 == (len = EC_POINT_point2oct(EC_KEY_get0_group(keypair_ecdh),
                                             pubkey,
                                             EC_KEY_get_conv_form(keypair_ecdh),
                                             dst.get_buffer(),
                                             len,
                                             NULL)))
          {
            OPENDDS_SSL_LOG_ERR("EC_POINT_point2oct failed");
            return 1;
          }

          return 0;
        }

      private:
        EVP_PKEY* keypair;
        EC_KEY* keypair_ecdh;
        const EC_POINT* pubkey;
      };

      int ECDH_PRIME_256_V1_CEUM::pub_key(DDS::OctetSeq& dst)
      {
        ecdh_pubkey_as_octets pubkey(k_);
        return pubkey(dst);
      }

      class ecdh_shared_secret_from_bignum
      {
      public:
        ecdh_shared_secret_from_bignum(EVP_PKEY* pkey) :
          keypair(NULL), pubkey_bn(NULL), pubkey_point(NULL), bignum_ctx(NULL)
        {
	  if (NULL == (keypair = EVP_PKEY_get1_EC_KEY(pkey))) {
	    OPENDDS_SSL_LOG_ERR("EVP_PKEY_get1_EC_KEY failed");
	  }
        }

        ~ecdh_shared_secret_from_bignum()
        {
          if (bignum_ctx) BN_CTX_free(bignum_ctx);
          if (pubkey_bn) BN_free(pubkey_bn);
          if (pubkey_point) EC_POINT_free(pubkey_point);
        }

        int operator()(const DDS::OctetSeq& src, DDS::OctetSeq& dst)
        {
          if (! keypair) return 1;

          if (NULL == (bignum_ctx = BN_CTX_new())) {
            OPENDDS_SSL_LOG_ERR("BN_CTX_new failed");
            return 1;
          }

          if (NULL == (pubkey_bn = BN_bin2bn(src.get_buffer(), src.length(), NULL))) {
            OPENDDS_SSL_LOG_ERR("BN_bin2bn failed");
            return 1;
          }

          if (NULL == (pubkey_point = EC_POINT_bn2point(EC_KEY_get0_group(keypair),
                                                        pubkey_bn,
                                                        NULL,
                                                        bignum_ctx)))
          {
            OPENDDS_SSL_LOG_ERR("EC_POINT_bn2point failed");
            return 1;
          }

          int numbits = EC_GROUP_get_degree(EC_KEY_get0_group(keypair));
          dst.length((numbits + 7) / 8);

          size_t len = ECDH_compute_key(dst.get_buffer(),
                                        dst.length(),
                                        pubkey_point,
                                        keypair,
                                        NULL);

          if (0 == len) {
              OPENDDS_SSL_LOG_ERR("ECDH_compute_key failed");
              return 1;
          }

          return 0;
        }

      private:
        EC_KEY* keypair;
        BIGNUM* pubkey_bn;
        EC_POINT* pubkey_point;
        BN_CTX* bignum_ctx;
      };

      class ecdh_shared_secret_from_octets
      {
      public:
        ecdh_shared_secret_from_octets(EVP_PKEY* pkey) :
          keypair(NULL), pubkey(NULL), group(NULL), bignum_ctx(NULL)
        {
          if (NULL == (keypair = EVP_PKEY_get1_EC_KEY(pkey))) {
            OPENDDS_SSL_LOG_ERR("EVP_PKEY_get1_EC_KEY failed");
          }
        }

        ~ecdh_shared_secret_from_octets()
        {
          if (pubkey) EC_POINT_free(pubkey);
          if (bignum_ctx) BN_CTX_free(bignum_ctx);
        }

        int operator()(const DDS::OctetSeq& src, DDS::OctetSeq& dst)
        {
          if (! keypair) return 1;

          if (NULL == (bignum_ctx = BN_CTX_new())) {
            OPENDDS_SSL_LOG_ERR("BN_CTX_new failed");
            return 1;
          }

          if (NULL == (group = EC_KEY_get0_group(keypair))) {
            OPENDDS_SSL_LOG_ERR("EC_KEY_get0_group failed");
            return 1;
          }

          pubkey = EC_POINT_new(group);
          if (1 != EC_POINT_oct2point(group,
                                      pubkey,
                                      src.get_buffer(),
                                      src.length(),
                                      bignum_ctx))
          {
            OPENDDS_SSL_LOG_ERR("EC_POINT_point2oct failed");
            return 1;
          }

          int numbits = EC_GROUP_get_degree(group);
          dst.length((numbits + 7) / 8);

          int len = ECDH_compute_key(dst.get_buffer(),
                                 dst.length(),
                                 pubkey,
                                 keypair,
                                 NULL);

          if (0 == len) {
              OPENDDS_SSL_LOG_ERR("ECDH_compute_key failed");
              return 1;
          }

          return 0;
        }

      private:
        EC_KEY* keypair;
        EC_POINT* pubkey;
        const EC_GROUP* group;
        BN_CTX* bignum_ctx;
      };

      int ECDH_PRIME_256_V1_CEUM::compute_shared_secret(const DDS::OctetSeq& pub_key)
      {
        ecdh_shared_secret_from_octets secret(k_);
        return secret(pub_key, shared_secret_);
      }

      DiffieHellman* DiffieHellman::factory(const DDS::OctetSeq& kagree_algo)
      {
        if (0 == std::memcmp(kagree_algo.get_buffer(), "DH+MODP-2048-256", kagree_algo.length())) {
            return new DiffieHellman(new DH_2048_MODP_256_PRIME);

        } else if (0 == std::memcmp(kagree_algo.get_buffer(), "ECDH+prime256v1-CEUM", kagree_algo.length())) {
            return new DiffieHellman(new ECDH_PRIME_256_V1_CEUM);

        } else {
            fprintf(stderr, "DiffieHellman::factory: Error, unknown kagree_algo\n");
            return NULL;
        }
      }

    }
  }
}
