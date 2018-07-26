/*
 * Distributed under the OpenDDS License.
 * See: http://www.OpenDDS.org/license.html
 */

#include "EllipticCurve.h"
#include "Err.h"
#include "Utils.h"
#include <openssl/evp.h>
#include <openssl/ec.h>
#include <vector>
#include <cstring>

namespace OpenDDS {
namespace Security {
namespace SSL {

  ECAlgorithm::~ECAlgorithm() {
    EVP_PKEY_free(k_);
  }

  bool ECAlgorithm::cmp_shared_secret(const ECAlgorithm & other) const
  {
    if (shared_secret_.length() != other.get_shared_secret().length()) {
      return false;
    }

    return (0 == std::memcmp(shared_secret_.get_buffer(),
                             other.get_shared_secret().get_buffer(),
                             shared_secret_.length()));
  }

  int ECAlgorithm::hash_shared_secret() {
    DDS::OctetSeq tmp = shared_secret_;
    std::vector<const DDS::OctetSeq*> hash_data;

    hash_data.push_back(&tmp);
    return SSL::hash(hash_data, shared_secret_);
  }

  class ecprime_constructor
  {
  public:
    ecprime_constructor() :
      params(NULL), paramgen_ctx(NULL), keygen_ctx(NULL) {}

    ~ecprime_constructor() {
      EVP_PKEY_free(params);
      EVP_PKEY_CTX_free(paramgen_ctx);
      EVP_PKEY_CTX_free(keygen_ctx);
    }

    EVP_PKEY* operator()() {
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

      // Generate parameters
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
    EVP_PKEY * params;
    EVP_PKEY_CTX* paramgen_ctx;
    EVP_PKEY_CTX* keygen_ctx;
  };

  EC_PRIME_256_V1_CEUM::EC_PRIME_256_V1_CEUM() {
    init();
  }

  EC_PRIME_256_V1_CEUM::~EC_PRIME_256_V1_CEUM()
  {

  }

  int EC_PRIME_256_V1_CEUM::init() {
    if (k_) return 0;

    ecprime_constructor ecprime;
    k_ = ecprime();

    if (k_) {
      return 0;
    }
    else {
      return 1;
    }
  }

  class ecprime_pubkey_as_octets
  {
  public:
    ecprime_pubkey_as_octets(EVP_PKEY* pkey) :
        keypair(pkey), keypair_ecdh(NULL), pubkey(NULL)
    {

    }

    ~ecprime_pubkey_as_octets()
    {

    }

    int operator()(DDS::OctetSeq& dst) {
      if (!keypair) return 1;

      if (NULL == (keypair_ecdh = EVP_PKEY_get0_EC_KEY(keypair))) {
        OPENDDS_SSL_LOG_ERR("EVP_PKEY_get0_EC_KEY failed");
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
                                         NULL))) {
        OPENDDS_SSL_LOG_ERR("EC_POINT_point2oct failed");
        return 1;
      }

      dst.length(len);

      if (0 == (len = EC_POINT_point2oct(EC_KEY_get0_group(keypair_ecdh),
                                         pubkey,
                                         EC_KEY_get_conv_form(keypair_ecdh),
                                         dst.get_buffer(),
                                         len,
                                         NULL))) {
        OPENDDS_SSL_LOG_ERR("EC_POINT_point2oct failed");
        return 1;
      }

      return 0;
    }

    private:
      EVP_PKEY * keypair;
      EC_KEY* keypair_ecdh;
      const EC_POINT* pubkey;
  };

  int EC_PRIME_256_V1_CEUM::pub_key(DDS::OctetSeq & dst) {
    ecprime_pubkey_as_octets pubkey(k_);
    return pubkey(dst);
  }

  class ecprime_shared_secret_from_octets
  {
  public:
    ecprime_shared_secret_from_octets(EVP_PKEY* pkey) :
      keypair(NULL),
      pubkey(NULL),
      group(NULL),
      bignum_ctx(NULL) {
        if (NULL == (keypair = EVP_PKEY_get0_EC_KEY(pkey))) {
          OPENDDS_SSL_LOG_ERR("EVP_PKEY_get0_EC_KEY failed");
        }
    }

    ~ecprime_shared_secret_from_octets() {
      EC_POINT_free(pubkey);
      BN_CTX_free(bignum_ctx);
    }

    int operator()(const DDS::OctetSeq& src, DDS::OctetSeq& dst) {
      if (!keypair) return 1;

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
                                  bignum_ctx)) {
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
    EC_KEY * keypair;
    EC_POINT* pubkey;
    const EC_GROUP* group;
    BN_CTX* bignum_ctx;
  };

  int EC_PRIME_256_V1_CEUM::compute_shared_secret(const DDS::OctetSeq & pub_key) {
    ecprime_shared_secret_from_octets secret(k_);
    return secret(pub_key, shared_secret_);
  }

  EllipticCurve * EllipticCurve::factory(const DDS::OctetSeq & kagree_algo)
  {
    if (0 == std::memcmp(kagree_algo.get_buffer(), "ECDH+prime256v1-CEUM", kagree_algo.length())) {
      return new EllipticCurve(new EC_PRIME_256_V1_CEUM);
    }
    else {
      OPENDDS_SSL_LOG_ERR("EllipticCurve::factory: Error, unknown kagree_algo\n");
      return NULL;
    }
  }

}
}
}
