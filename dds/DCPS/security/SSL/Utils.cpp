/*
 * Distributed under the OpenDDS License.
 * See: http://www.OpenDDS.org/license.html
 */

#include "Utils.h"

#include "Err.h"

#include <dds/DCPS/Serializer.h>
#include <dds/DCPS/GuidUtils.h>

#include <dds/DdsDcpsCoreTypeSupportImpl.h>

#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/err.h>
#include "../OpenSSL_legacy.h"  // Must come after all other OpenSSL includes

#include <vector>
#include <utility>
#include <cstdio>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace Security {
namespace SSL {

using DCPS::Serializer;
using DCPS::Encoding;
using DCPS::serialized_size;

namespace {
  Encoding get_common_encoding() {
    Encoding encoding(Encoding::KIND_XCDR1, DCPS::ENDIAN_BIG);
    return encoding;
  }
}

int make_adjusted_guid(const OpenDDS::DCPS::GUID_t& src,
                       OpenDDS::DCPS::GUID_t& dst,
                       const Certificate& target)
{
  dst = OpenDDS::DCPS::GUID_UNKNOWN;
  dst.entityId = src.entityId;

  std::vector<unsigned char> hash;
  int result = target.subject_name_digest(hash);

  if (result == 0 && hash.size() >= 6) {
    unsigned char* bytes = reinterpret_cast<unsigned char*>(&dst);

    for (size_t i = 0; i < 6; ++i) {  // First 6 bytes of guid prefix
      bytes[i] = offset_1bit(&hash[0], i);
    }

    bytes[0] |= 0x80;

    // Last 6 bytes of guid prefix = hash of src guid (candidate guid)

    unsigned char hash2[EVP_MAX_MD_SIZE] = {0};
    unsigned int len = 0u;

    EVP_MD_CTX* hash_ctx = EVP_MD_CTX_new();
    if (hash_ctx) {
      EVP_DigestInit_ex(hash_ctx, EVP_sha256(), 0);
      EVP_DigestUpdate(hash_ctx, &src, sizeof(OpenDDS::DCPS::GUID_t));
      EVP_DigestFinal_ex(hash_ctx, hash2, &len);
      if (len > 5) {
        std::memcpy(bytes + 6, hash2, 6);
      } else {
        result = 1;
      }

      EVP_MD_CTX_free(hash_ctx);
    }
  }

  return result;
}

template <size_t Bits>
int make_nonce(std::vector<unsigned char>& nonce)
{
  nonce.clear();

  unsigned char tmp[Bits / 8] = { 0 };

  if (RAND_bytes(tmp, sizeof tmp) == 1) {
    nonce.insert(nonce.begin(), tmp, tmp + sizeof tmp);

    return 0;

  } else {
    unsigned long err = ERR_get_error();
    char msg[256] = { 0 };
    ERR_error_string_n(err, msg, sizeof(msg));

    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) SSL::make_nonce: ERROR '%C' returned by RAND_bytes(...)\n"),
               msg));
  }

  return 1;
}

int make_nonce_256(std::vector<unsigned char>& nonce)
{
  return make_nonce<256>(nonce);
}

int make_nonce_256(DDS::OctetSeq& nonce)
{
  /* A bit slower but the impl. for vectors is already complete */
  std::vector<unsigned char> tmp;
  int err = make_nonce<256>(tmp);
  if (!err) {
    nonce.length(static_cast<unsigned int>(tmp.size()));
    for (size_t i = 0; i < tmp.size(); ++i) {
      nonce[static_cast<unsigned int>(i)] = tmp[i];
    }
  }
  return err;
}

unsigned char offset_1bit(const unsigned char array[], size_t i)
{
  return (array[i] >> 1) | (i == 0 ? 0 : ((array[i - 1] & 1) ? 0x80 : 0));
}

int hash(const std::vector<const DDS::OctetSeq*>& src, DDS::OctetSeq& dst)
{
  EVP_MD_CTX* hash_ctx = EVP_MD_CTX_new();
  if (!hash_ctx) {
    OPENDDS_SSL_LOG_ERR("EVP_MD_CTX_new failed");
    return 1;
  }

  EVP_DigestInit_ex(hash_ctx, EVP_sha256(), 0);

  unsigned char hash[EVP_MAX_MD_SIZE] = { 0 };
  unsigned int len = 0u;

  std::vector<const DDS::OctetSeq*>::const_iterator i, n = src.end();
  for (i = src.begin(); i != n; ++i) {
    EVP_DigestUpdate(hash_ctx, (*i)->get_buffer(), (*i)->length());
  }

  EVP_DigestFinal_ex(hash_ctx, hash, &len);

  dst.length(len);
  std::memcpy(dst.get_buffer(), hash, len);

  EVP_MD_CTX_free(hash_ctx);

  return 0;
}

class hash_serialized_impl
{
public:
  hash_serialized_impl()
    : hash_ctx(EVP_MD_CTX_new())
  {
    if (!hash_ctx) {
      OPENDDS_SSL_LOG_ERR("EVP_MD_CTX_new failed");
    }
  }

  ~hash_serialized_impl()
  {
    if (hash_ctx) {
      EVP_MD_CTX_free(hash_ctx);
    }
  }

  int operator()(const DDS::BinaryPropertySeq& src, DDS::OctetSeq& dst)
  {
    if (!hash_ctx) return 1;

    EVP_DigestInit_ex(hash_ctx, EVP_sha256(), 0);

    const Encoding encoding = get_common_encoding();
    size_t size = 0;
    serialized_size(encoding, size, src);
    ACE_Message_Block buffer(size);
    Serializer serializer(&buffer, encoding);
    if (serializer << src) {
      EVP_DigestUpdate(hash_ctx, buffer.rd_ptr(), buffer.length());

      dst.length(EVP_MAX_MD_SIZE);

      unsigned int newlen = 0u;
      EVP_DigestFinal_ex(hash_ctx, dst.get_buffer(), &newlen);

      dst.length(newlen);

    } else {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) SSL::hash_serialized_impl::operator(): ERROR, failed to "
                          "serialize binary-property-sequence\n")));

      return 1;
    }

    return 0;
  }

private:
  EVP_MD_CTX* hash_ctx;
};

int hash_serialized(const DDS::BinaryPropertySeq& src, DDS::OctetSeq& dst)
{
  hash_serialized_impl hash;
  return hash(src, dst);
}

int sign_serialized(const DDS::BinaryPropertySeq& src,
                    const PrivateKey& key, DDS::OctetSeq& dst)
{
  const Encoding encoding = get_common_encoding();
  size_t size = 0;
  serialized_size(encoding, size, src);

  DDS::OctetSeq tmp;
  tmp.length(static_cast<unsigned int>(size));
  ACE_Message_Block buffer(reinterpret_cast<const char*>(tmp.get_buffer()),
                           tmp.length());
  Serializer serializer(&buffer, encoding);
  if (!(serializer << src)) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) SSL::sign_serialized: ERROR, failed to serialize "
                        "binary-property-sequence\n")));

    return 1;
  }

  std::vector<const DDS::OctetSeq*> sign_these;
  sign_these.push_back(&tmp);

  return key.sign(sign_these, dst);
}

int verify_serialized(const DDS::BinaryPropertySeq& src,
                      const Certificate& key,
                      const DDS::OctetSeq& signed_data)
{
  const Encoding encoding = get_common_encoding();
  size_t size = 0;
  serialized_size(encoding, size, src);

  DDS::OctetSeq tmp;
  tmp.length(static_cast<unsigned int>(size));
  ACE_Message_Block buffer(reinterpret_cast<const char*>(tmp.get_buffer()),
                           tmp.length());
  Serializer serializer(&buffer, encoding);
  if (!(serializer << src)) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) SSL::verify_serialized: ERROR, failed to serialize binary-property-sequence\n")));

    return 1;
  }

  std::vector<const DDS::OctetSeq*> verify_these;
  verify_these.push_back(&tmp);

  return key.verify_signature(signed_data, verify_these);
}

}  // namespace SSL
}  // namespace Security
}  // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
