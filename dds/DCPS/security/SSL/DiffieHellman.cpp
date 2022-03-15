/*
 * Distributed under the OpenDDS License.
 * See: http://www.OpenDDS.org/license.html
 */

#include "DiffieHellman.h"
#include "Utils.h"
#include "Err.h"

#include <openssl/evp.h>
#include <openssl/dh.h>
#if OPENSSL_VERSION_NUMBER >= 0x30000000L
#include <openssl/core_names.h>
#include <openssl/param_build.h>
#endif
#include "../OpenSSL_legacy.h"  // Must come after all other OpenSSL includes

#include <ace/OS_NS_strings.h>

#include <cstring>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace Security {
namespace SSL {

namespace {
  // Assumes both buffers are null-terminated (and that s2_len includes final null, as with sizeof() for a const char[])
  int compare(const DDS::OctetSeq& s1, const char* s2, const size_t s2_len) {
    return s1.length() != s2_len ? -1 : std::memcmp(s1.get_buffer(), s2, s2_len);
  }
}

#ifndef OPENSSL_V_3_0
struct DH_Handle {
  DH* dh_;
  explicit DH_Handle(EVP_PKEY* key)
#if defined OPENSSL_V_1_0 || defined OPENSSL_V_3_0
    : dh_(EVP_PKEY_get1_DH(key))
#else
    : dh_(EVP_PKEY_get0_DH(key))
#endif
  {}
  operator DH*() { return dh_; }
  ~DH_Handle()
  {
#if defined OPENSSL_V_1_0 || defined OPENSSL_V_3_0
    DH_free(dh_);
#endif
  }
};
#endif

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
#ifndef OPENSSL_V_3_0
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
#else
    BIGNUM* pubkey = 0;
    if (EVP_PKEY_get_bn_param(k_, OSSL_PKEY_PARAM_PUB_KEY, &pubkey)) {
      dst.length(BN_num_bytes(pubkey));
      if (0 < BN_bn2bin(pubkey, dst.get_buffer())) {
        result = 0;
      } else {
        OPENDDS_SSL_LOG_ERR("BN_bn2bin failed");
      }
    } else {
      OPENDDS_SSL_LOG_ERR("EVP_PKEY_get_bn_param failed");
    }
    BN_free(pubkey);
#endif
  }
  return result;
}

class dh_shared_secret
{
public:
  explicit dh_shared_secret(EVP_PKEY* pkey)
    : keypair(pkey)
#ifdef OPENSSL_V_3_0
    , dh_ctx(0)
    , fd_ctx(0)
    , peer(0)
    , param_bld(0)
    , params(0)
    , glen(32)
    , grp(new char[glen])
#endif
    , pubkey(0)
  {
    if (!keypair) {
#ifndef OPENSSL_V_3_0
      OPENDDS_SSL_LOG_ERR("EVP_PKEY_get0_DH failed");
#endif
    }
  }

  ~dh_shared_secret()
  {
    BN_free(pubkey);
#ifdef OPENSSL_V_3_0
    EVP_PKEY_CTX_free(dh_ctx);
    EVP_PKEY_CTX_free(fd_ctx);
    EVP_PKEY_free(peer);
    OSSL_PARAM_BLD_free(param_bld);
    OSSL_PARAM_free(params);
    delete [] grp;
#endif
  }

  int operator()(const DDS::OctetSeq& pub_key, DDS::OctetSeq& dst)
  {
    if (!keypair) return 1;

    if (0 == (pubkey = BN_bin2bn(pub_key.get_buffer(), pub_key.length(), 0))) {
      OPENDDS_SSL_LOG_ERR("BN_bin2bn failed");
      return 1;
    }

#ifndef OPENSSL_V_3_0
    int len = DH_size(keypair);
    dst.length(len);
    len = DH_compute_key(dst.get_buffer(), pubkey, keypair);
    if (len < 0) {
      OPENDDS_SSL_LOG_ERR("DH_compute_key failed");
      dst.length(0u);
      return 1;
    }
#else
    if (!EVP_PKEY_get_utf8_string_param(keypair, "group", grp, glen, &glen)) {
      OPENDDS_SSL_LOG_ERR("Failed to find group name");
      return 1;
    }
    OSSL_PARAM_free(params);
    params = 0;

    if ((param_bld = OSSL_PARAM_BLD_new()) == 0) {
      OPENDDS_SSL_LOG_ERR("OSSL_PARAM_BLD_new failed");
      return 1;
    }

    if ((OSSL_PARAM_BLD_push_utf8_string(param_bld, "group", grp, 0) == 0)) {
      OPENDDS_SSL_LOG_ERR("Building prarms list failed");
      return 1;
    }

    if ((OSSL_PARAM_BLD_push_BN(param_bld, "pub", pubkey) == 0)) {
      OPENDDS_SSL_LOG_ERR("Building prarms list failed");
      return 1;
    }
    params = OSSL_PARAM_BLD_to_param(param_bld);

    if ((fd_ctx = EVP_PKEY_CTX_new(keypair, 0)) == 0) {
      OPENDDS_SSL_LOG_ERR("new ctx failed.");
      return 1;
    }

    EVP_PKEY_fromdata_init(fd_ctx);

    if (EVP_PKEY_fromdata(fd_ctx, &peer, EVP_PKEY_PUBLIC_KEY, params) != 1) {
      OPENDDS_SSL_LOG_ERR("EVP_PKEY_fromdata Failed");
      return 1;
    }

    if ((dh_ctx = EVP_PKEY_CTX_new(keypair,0)) == 0) {
      OPENDDS_SSL_LOG_ERR("new ctx from name BH failed.");
      return 1;
    }

    if (!EVP_PKEY_derive_init(dh_ctx)) {
      OPENDDS_SSL_LOG_ERR("EVP_PKEY_derive_init failed");
      return 1;
    }

    if (EVP_PKEY_derive_set_peer(dh_ctx, peer) <= 0) {
      OPENDDS_SSL_LOG_ERR("EVP_PKEY_derive_set peer failed");
      return 1;
    }

    size_t len = 0;
    if (EVP_PKEY_derive(dh_ctx, 0, &len) <= 0) {
      OPENDDS_SSL_LOG_ERR("DH compute_key error getting length");
      return 1;
    }
    dst.length(static_cast<ACE_CDR::ULong>(len));
    if (EVP_PKEY_derive(dh_ctx, dst.get_buffer(), &len) <= 0) {
      OPENDDS_SSL_LOG_ERR("EVP_PKEY_derive failed");
      dst.length(0u);
      return 1;
    }
#endif
    return 0;
  }

private:
#ifndef OPENSSL_V_3_0
  DH_Handle keypair;
#else
  EVP_PKEY* keypair;
  EVP_PKEY_CTX* dh_ctx;
  EVP_PKEY_CTX* fd_ctx;
  EVP_PKEY* peer;
  OSSL_PARAM_BLD* param_bld;
  OSSL_PARAM* params;
  size_t glen;
  char* grp;
#endif
  BIGNUM* pubkey;
};

int DH_2048_MODP_256_PRIME::compute_shared_secret(const DDS::OctetSeq& pub_key)
{
  dh_shared_secret secret(k_);
  return secret(pub_key, shared_secret_);
}

#ifndef OPENSSL_V_3_0
struct EC_Handle {
  EC_KEY* ec_;
  explicit EC_Handle(EVP_PKEY* key)
#if defined OPENSSL_V_1_0 || defined OPENSSL_V_3_0
    : ec_(EVP_PKEY_get1_EC_KEY(key))
#else
    : ec_(EVP_PKEY_get0_EC_KEY(key))
#endif
  {}
  operator EC_KEY*() { return ec_; }
  ~EC_Handle()
  {
#if defined OPENSSL_V_1_0 || defined OPENSSL_V_3_0
    EC_KEY_free(ec_);
#endif
  }
};
#endif

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
      OPENDDS_SSL_LOG_ERR("EVP_PKEY_CTX_set_ec_paramgen_curve_ni = failed");
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
#ifdef OPENSSL_V_3_0
    , params(0)
#endif
  {
  }

#ifdef OPENSSL_V_3_0
  ~ecdh_pubkey_as_octets()
  {
    OSSL_PARAM_free(params);
  }
#endif

  int operator()(DDS::OctetSeq& dst)
  {
    if (!keypair) return 1;

#ifndef OPENSSL_V_3_0
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
#else
    if (EVP_PKEY_todata(keypair, EVP_PKEY_KEYPAIR, &params) <= 0) {
      OPENDDS_SSL_LOG_ERR("pkey to data failed");
      return 1;
    } else {
      const char* gname = 0;
      const unsigned char* pubbuf = 0;
      size_t pubbuflen = 0;
      for (OSSL_PARAM* p = params; p != 0 && p->key != 0; ++p) {
        if (ACE_OS::strcasecmp(p->key, "group") == 0) {
          gname = static_cast<const char*>(p->data);
        } else if (ACE_OS::strcasecmp(p->key, "pub") == 0) {
          pubbuf = static_cast<const unsigned char*>(p->data);
          pubbuflen = p->data_size;
        }
      }

      const int nid = OBJ_txt2nid(gname);
      if (nid == 0) {
        OPENDDS_SSL_LOG_ERR("failed to find Nid");
        return 1;
      }
      const EC_GROUP* const ecg = EC_GROUP_new_by_curve_name(nid);
      const point_conversion_form_t cf = EC_GROUP_get_point_conversion_form(ecg);
      EC_POINT* const ec = EC_POINT_new(ecg);
      if (!EC_POINT_oct2point(ecg, ec, pubbuf, pubbuflen, 0)) {
        OPENDDS_SSL_LOG_ERR("failed to extract ec point from octet sequence");
        EC_POINT_free(ec);
        return 1;
      }
      const size_t eclen = EC_POINT_point2oct(ecg, ec, cf, 0, 0u, 0);
      dst.length(static_cast<ACE_CDR::ULong>(eclen));
      EC_POINT_point2oct(ecg, ec, cf, dst.get_buffer(), eclen, 0);
      EC_POINT_free(ec);
    }
#endif
    return 0;
  }

private:
  EVP_PKEY* keypair;
#ifdef OPENSSL_V_3_0
  OSSL_PARAM* params;
#endif
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
    : keypair(pkey)
#ifdef OPENSSL_V_3_0
    , ec_ctx(0)
    , fd_ctx(0)
    , peer(0)
    , param_bld(0)
    , params(0)
#endif
    , pubkey(0), group(0), bignum_ctx(0)
  {
    if (!keypair) {
      OPENDDS_SSL_LOG_ERR("EVP_PKEY_get0_EC_KEY failed");
    }
  }

  ~ecdh_shared_secret_from_octets()
  {
    EC_POINT_free(pubkey);
    BN_CTX_free(bignum_ctx);
#ifdef OPENSSL_V_3_0
    EVP_PKEY_CTX_free(ec_ctx);
    EVP_PKEY_CTX_free(fd_ctx);
    EVP_PKEY_free(peer);
    OSSL_PARAM_BLD_free(param_bld);
    OSSL_PARAM_free(params);
#endif
  }

  int operator()(const DDS::OctetSeq& src, DDS::OctetSeq& dst)
  {
    if (!keypair) return 1;

    if (0 == (bignum_ctx = BN_CTX_new())) {
      OPENDDS_SSL_LOG_ERR("BN_CTX_new failed");
      return 1;
    }
#ifndef OPENSSL_V_3_0
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
#else
    const char* grp = 0;
    if (EVP_PKEY_todata(keypair, EVP_PKEY_PUBLIC_KEY, &params) <= 0) {
      OPENDDS_SSL_LOG_ERR("pkey to data failed");
      return 1;
    } else {
      for (OSSL_PARAM* p = params; grp == 0 && p != 0 && p->key != 0; p++) {
        if (strcmp(p->key, "group") == 0) {
          grp = static_cast<const char*>(p->data);
        }
      }
      if (grp == 0) {
        OPENDDS_SSL_LOG_ERR("could not find group id");
        return 1;
      }
    }

    if ((param_bld = OSSL_PARAM_BLD_new()) == 0) {
      OPENDDS_SSL_LOG_ERR("OSSL_PARAM_BLD_new failed");
      return 1;
    }

    if ((OSSL_PARAM_BLD_push_utf8_string(param_bld, "group", grp, 0) == 0)) {
      OPENDDS_SSL_LOG_ERR("Building prarms list failed");
      return 1;
    }

    if ((OSSL_PARAM_BLD_push_octet_string(param_bld, "pub", src.get_buffer(),src.length()) == 0)) {
      OPENDDS_SSL_LOG_ERR("Building prarms list failed");
      return 1;
    }
    params = OSSL_PARAM_BLD_to_param(param_bld);

    if ((fd_ctx = EVP_PKEY_CTX_new(keypair,0)) == 0) {
      OPENDDS_SSL_LOG_ERR("new ctx from name ECBH failed.");
      return 1;
    }

    EVP_PKEY_fromdata_init(fd_ctx);

    if (EVP_PKEY_fromdata(fd_ctx, &peer, EVP_PKEY_PUBLIC_KEY, params) != 1) {
      OPENDDS_SSL_LOG_ERR("EVP_PKEY_fromdata Failed");
      return 1;
    }

    if ((ec_ctx = EVP_PKEY_CTX_new(keypair,0)) == 0) {
      OPENDDS_SSL_LOG_ERR("new ctx from name ECBH failed.");
      return 1;
    }

    if (!EVP_PKEY_derive_init(ec_ctx)) {
      OPENDDS_SSL_LOG_ERR("EVP_PKEY_derive_init failed");
      return 1;
    }

    if (EVP_PKEY_derive_set_peer(ec_ctx, peer) <= 0) {
      OPENDDS_SSL_LOG_ERR("EVP_PKEY_derive_set peer failed");
      return 1;
    }

    size_t len = 0;
    if (EVP_PKEY_derive(ec_ctx, 0, &len) <= 0) {
      OPENDDS_SSL_LOG_ERR("DH compute_key error getting length");
      return 1;
    }
    dst.length(static_cast<ACE_CDR::ULong>(len));
    if (EVP_PKEY_derive(ec_ctx, dst.get_buffer(), &len) <= 0) {
      OPENDDS_SSL_LOG_ERR("EVP_PKEY_derive failed");
      dst.length(0u);
      return 1;
    }
#endif
    return 0;
  }

private:
#ifndef OPENSSL_V_3_0
  EC_Handle keypair;
#else
  EVP_PKEY* keypair;
  EVP_PKEY_CTX* ec_ctx;
  EVP_PKEY_CTX* fd_ctx;
  EVP_PKEY* peer;
  OSSL_PARAM_BLD* param_bld;
  OSSL_PARAM* params;
#endif
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
  if (0 == compare(kagree_algo, DH_2048_MODP_256_PRIME_STR, sizeof (DH_2048_MODP_256_PRIME_STR))) {
    return new DiffieHellman(new DH_2048_MODP_256_PRIME);

  } else if (0 == compare(kagree_algo, ECDH_PRIME_256_V1_CEUM_STR, sizeof (ECDH_PRIME_256_V1_CEUM_STR))) {
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
