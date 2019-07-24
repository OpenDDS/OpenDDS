/*
 * Distributed under the OpenDDS License.
 * See: http://www.OpenDDS.org/license.html
 */

#include "DiffieHellman.h"
#include "Utils.h"
#include "Err.h"

#include <openssl/dh.h>
#include "../OpenSSL_legacy.h"  // Must come after all other OpenSSL includes

#include <cstring>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace Security {
namespace SSL {

struct DH_Handle {
  DH* dh_;
  explicit DH_Handle(EVP_PKEY* key)
#ifdef OPENSSL_V_1_0
    : dh_(EVP_PKEY_get1_DH(key))
#else
    : dh_(EVP_PKEY_get0_DH(key))
#endif
  {}
  operator DH*() { return dh_; }
  ~DH_Handle()
  {
#ifdef OPENSSL_V_1_0
    DH_free(dh_);
#endif
  }
};

DHAlgorithm::~DHAlgorithm() { EVP_PKEY_free(k_); }

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

DH_2048_MODP_256_PRIME::DH_2048_MODP_256_PRIME() { init(); }

DH_2048_MODP_256_PRIME::~DH_2048_MODP_256_PRIME() {}

class dh_constructor
{
public:
  dh_constructor() : params(0), paramgen_ctx(0), keygen_ctx(0) {}

  ~dh_constructor()
  {
    EVP_PKEY_free(params);
    EVP_PKEY_CTX_free(paramgen_ctx);
    EVP_PKEY_CTX_free(keygen_ctx);
  }

  EVP_PKEY* get_key()
  {
    EVP_PKEY* result = 0;

#if OPENSSL_VERSION_NUMBER < 0x10002000L
    OPENDDS_SSL_LOG_ERR("RFC 5114 2.3 - 2048-bit MODP Group with 256-bit Prime Order Subgroup - not provided by this OpenSSL library");
#else

    if (0 == (params = EVP_PKEY_new())) {
      OPENDDS_SSL_LOG_ERR("EVP_PKEY_new failed");
      return 0;
    }

    if (0 == (paramgen_ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_DHX, 0))) {
      OPENDDS_SSL_LOG_ERR("EVP_PKEY_CTX_new_id");
      return 0;
    }

    if (1 != EVP_PKEY_paramgen_init(paramgen_ctx)) {
      OPENDDS_SSL_LOG_ERR("EVP_PKEY_paramgen_init failed");
      return 0;
    }

    if (1 != EVP_PKEY_CTX_set_dh_rfc5114(paramgen_ctx, 3)) {
      OPENDDS_SSL_LOG_ERR("EVP_PKEY_CTX_set_dh_rfc5114 failed");
      return 0;
    }

    if ((1 != EVP_PKEY_paramgen(paramgen_ctx, &params)) || params == 0) {
      OPENDDS_SSL_LOG_ERR("EVP_PKEY_paramgen failed");
      return 0;
    }

    if (0 == (keygen_ctx = EVP_PKEY_CTX_new(params, 0))) {
      OPENDDS_SSL_LOG_ERR("EVP_PKEY_CTX_new failed");
      return 0;
    }

    if (1 != EVP_PKEY_keygen_init(keygen_ctx)) {
      OPENDDS_SSL_LOG_ERR("EVP_PKEY_keygen_init failed");
      return 0;
    }

    if (1 != EVP_PKEY_keygen(keygen_ctx, &result)) {
      OPENDDS_SSL_LOG_ERR("EVP_PKEY_keygen failed");
      return 0;
    }

#endif
    return result;
  }

private:
  EVP_PKEY* params;
  EVP_PKEY_CTX* paramgen_ctx;
  EVP_PKEY_CTX* keygen_ctx;
};

int DH_2048_MODP_256_PRIME::init()
{
  if (k_) return 0;

  dh_constructor dh;
  k_ = dh.get_key();

  return k_ ? 0 : 1;
}

int DH_2048_MODP_256_PRIME::pub_key(DDS::OctetSeq& dst)
{
  int result = 1;

  if (k_) {
    DH_Handle dh(k_);
    if (dh) {
      const BIGNUM *pubkey = 0, *privkey = 0;
      DH_get0_key(dh, &pubkey, &privkey);
      if (pubkey) {
        dst.length(BN_num_bytes(pubkey));
        if (0 < BN_bn2bin(pubkey, dst.get_buffer())) {
          result = 0;
        } else {
          OPENDDS_SSL_LOG_ERR("BN_bn2bin failed");
        }
      } else {
        OPENDDS_SSL_LOG_ERR("DH_get0_key failed");
      }
    }
  }
  return result;
}

class dh_shared_secret
{
public:
  explicit dh_shared_secret(EVP_PKEY* pkey)
    : keypair(pkey), pubkey(0)
  {
    if (!keypair) {
      OPENDDS_SSL_LOG_ERR("EVP_PKEY_get0_DH failed");
    }
  }

  ~dh_shared_secret() { BN_free(pubkey); }

  int operator()(const DDS::OctetSeq& pub_key, DDS::OctetSeq& dst)
  {
    if (!keypair) return 1;

    if (0 == (pubkey = BN_bin2bn(pub_key.get_buffer(), pub_key.length(), 0))) {
      OPENDDS_SSL_LOG_ERR("BN_bin2bn failed");
      return 1;
    }

    int len = DH_size(keypair);
    dst.length(len);

    len = DH_compute_key(dst.get_buffer(), pubkey, keypair);
    if (len < 0) {
      OPENDDS_SSL_LOG_ERR("DH_compute_key failed");
      dst.length(0u);
      return 1;
    }

    dst.length(len);
    return 0;
  }

private:
  DH_Handle keypair;
  BIGNUM* pubkey;
};

int DH_2048_MODP_256_PRIME::compute_shared_secret(const DDS::OctetSeq& pub_key)
{
  dh_shared_secret secret(k_);
  return secret(pub_key, shared_secret_);
}

struct EC_Handle {
  EC_KEY* ec_;
  explicit EC_Handle(EVP_PKEY* key)
#ifdef OPENSSL_V_1_0
    : ec_(EVP_PKEY_get1_EC_KEY(key))
#else
    : ec_(EVP_PKEY_get0_EC_KEY(key))
#endif
  {}
  operator EC_KEY*() { return ec_; }
  ~EC_Handle()
  {
#ifdef OPENSSL_V_1_0
    EC_KEY_free(ec_);
#endif
  }
};

ECDH_PRIME_256_V1_CEUM::ECDH_PRIME_256_V1_CEUM() { init(); }

ECDH_PRIME_256_V1_CEUM::~ECDH_PRIME_256_V1_CEUM() {}

class ecdh_constructor
{
public:
  ecdh_constructor() : params(0), paramgen_ctx(0), keygen_ctx(0) {}

  ~ecdh_constructor()
  {
    EVP_PKEY_free(params);
    EVP_PKEY_CTX_free(paramgen_ctx);
    EVP_PKEY_CTX_free(keygen_ctx);
  }

  EVP_PKEY* get_key()
  {
    EVP_PKEY* result = 0;

    if (0 == (params = EVP_PKEY_new())) {
      OPENDDS_SSL_LOG_ERR("EVP_PKEY_new failed");
      return 0;
    }

    if (0 == (paramgen_ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_EC, 0))) {
      OPENDDS_SSL_LOG_ERR("EVP_PKEY_CTX_new_id");
      return 0;
    }

    if (1 != EVP_PKEY_paramgen_init(paramgen_ctx)) {
      OPENDDS_SSL_LOG_ERR("EVP_PKEY_paramgen_init failed");
      return 0;
    }

    if (1 != EVP_PKEY_CTX_set_ec_paramgen_curve_nid(paramgen_ctx, NID_X9_62_prime256v1)) {
      OPENDDS_SSL_LOG_ERR("EVP_PKEY_CTX_set_ec_paramgen_curve_nid failed");
      return 0;
    }

    if (1 != EVP_PKEY_paramgen(paramgen_ctx, &params) || params == 0) {
      OPENDDS_SSL_LOG_ERR("EVP_PKEY_paramgen failed");
      return 0;
    }

    if (0 == (keygen_ctx = EVP_PKEY_CTX_new(params, 0))) {
      OPENDDS_SSL_LOG_ERR("EVP_PKEY_CTX_new failed");
      return 0;
    }

    if (1 != EVP_PKEY_keygen_init(keygen_ctx)) {
      OPENDDS_SSL_LOG_ERR("EVP_PKEY_keygen_init failed");
      return 0;
    }

    if (1 != EVP_PKEY_keygen(keygen_ctx, &result)) {
      OPENDDS_SSL_LOG_ERR("EVP_PKEY_keygen failed");
      return 0;
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

  ecdh_constructor ecdh;
  k_ = ecdh.get_key();

  return k_ ? 0 : 1;
}

class ecdh_pubkey_as_octets
{
public:
  explicit ecdh_pubkey_as_octets(EVP_PKEY* pkey)
    : keypair(pkey)
  {
  }

  ~ecdh_pubkey_as_octets() {}

  int operator()(DDS::OctetSeq& dst)
  {
    if (!keypair) return 1;

    EC_Handle keypair_ecdh(keypair);
    if (!keypair_ecdh) {
      OPENDDS_SSL_LOG_ERR("EVP_PKEY_get0_EC_KEY failed");
      return 1;
    }

    const EC_POINT* pubkey = EC_KEY_get0_public_key(keypair_ecdh);
    if (!pubkey) {
      OPENDDS_SSL_LOG_ERR("EC_KEY_get0_public_key failed");
      return 1;
    }

    size_t len = EC_POINT_point2oct(EC_KEY_get0_group(keypair_ecdh), pubkey,
                                    EC_KEY_get_conv_form(keypair_ecdh), 0,
                                    0u, 0);
    if (!len) {
      OPENDDS_SSL_LOG_ERR("EC_POINT_point2oct failed");
      return 1;
    }

    dst.length(static_cast<unsigned int>(len));

    if (0 == EC_POINT_point2oct(EC_KEY_get0_group(keypair_ecdh), pubkey,
                                EC_KEY_get_conv_form(keypair_ecdh),
                                dst.get_buffer(), len, 0)) {
      OPENDDS_SSL_LOG_ERR("EC_POINT_point2oct failed");
      return 1;
    }

    return 0;
  }

private:
  EVP_PKEY* keypair;
};

int ECDH_PRIME_256_V1_CEUM::pub_key(DDS::OctetSeq& dst)
{
  ecdh_pubkey_as_octets pubkey(k_);
  return pubkey(dst);
}

class ecdh_shared_secret_from_octets
{
public:
  explicit ecdh_shared_secret_from_octets(EVP_PKEY* pkey)
    : keypair(pkey), pubkey(0), group(0), bignum_ctx(0)
  {
    if (!keypair) {
      OPENDDS_SSL_LOG_ERR("EVP_PKEY_get0_EC_KEY failed");
    }
  }

  ~ecdh_shared_secret_from_octets()
  {
    EC_POINT_free(pubkey);
    BN_CTX_free(bignum_ctx);
  }

  int operator()(const DDS::OctetSeq& src, DDS::OctetSeq& dst)
  {
    if (!keypair) return 1;

    if (0 == (bignum_ctx = BN_CTX_new())) {
      OPENDDS_SSL_LOG_ERR("BN_CTX_new failed");
      return 1;
    }

    if (0 == (group = EC_KEY_get0_group(keypair))) {
      OPENDDS_SSL_LOG_ERR("EC_KEY_get0_group failed");
      return 1;
    }

    pubkey = EC_POINT_new(group);
    if (1 != EC_POINT_oct2point(group, pubkey, src.get_buffer(),
                                src.length(), bignum_ctx)) {
      OPENDDS_SSL_LOG_ERR("EC_POINT_point2oct failed");
      return 1;
    }

    const int numbits = EC_GROUP_get_degree(group);
    dst.length((numbits + 7) / 8);

    const int len = ECDH_compute_key(dst.get_buffer(), dst.length(), pubkey,
                                     keypair, 0);

    if (0 == len) {
      OPENDDS_SSL_LOG_ERR("ECDH_compute_key failed");
      return 1;
    }

    return 0;
  }

private:
  EC_Handle keypair;
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
  if (0 == std::memcmp(kagree_algo.get_buffer(), "DH+MODP-2048-256",
                       kagree_algo.length())) {
    return new DiffieHellman(new DH_2048_MODP_256_PRIME);

  } else if (0 == std::memcmp(kagree_algo.get_buffer(), "ECDH+prime256v1-CEUM",
                              kagree_algo.length())) {
    return new DiffieHellman(new ECDH_PRIME_256_V1_CEUM);

  } else {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) SSL::DiffieHellman::factory: ERROR, unknown kagree_algo\n")));
    return 0;
  }
}

}  // namespace SSL
}  // namespace Security
}  // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
