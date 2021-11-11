/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

/**
 * Macros and other helpers to allow OpenDDS Security library to work with
 * OpenSSL 1.0, as it was written to use OpenSSL 1.1.
 */

#ifndef OPENDDS_DCPS_SECURITY_OPENSSL_LEGACY_H
#define OPENDDS_DCPS_SECURITY_OPENSSL_LEGACY_H

#if OPENSSL_VERSION_NUMBER < 0x10100000L

#define OPENSSL_V_1_0

#define EVP_MD_CTX_new EVP_MD_CTX_create
#define EVP_MD_CTX_free EVP_MD_CTX_destroy
#define EVP_CTRL_AEAD_GET_TAG EVP_CTRL_CCM_GET_TAG

inline int RSA_bits(const RSA* r)
{
  return BN_num_bits(r->n);
}

inline void DH_get0_key(const DH* dh, const BIGNUM** pub_key, const BIGNUM** priv_key)
{
  if (pub_key) {
    *pub_key = dh->pub_key;
  }
  if (priv_key) {
    *priv_key = dh->priv_key;
  }
}

#elif OPENSSL_VERSION_NUMBER < 0x30000000L
#define OPENSSL_V_1_1
#else
#define OPENSSL_V_3_0
#endif // OPENSSL_VERSION_NUMBER < 0x10100000L

#endif
