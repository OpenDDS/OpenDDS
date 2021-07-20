#include "UtilityImpl.h"

#include <openssl/rand.h>
#include <openssl/hmac.h>
#include <openssl/err.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace Security {

UtilityImpl::~UtilityImpl() {}

void UtilityImpl::generate_random_bytes(void* ptr, size_t size)
{
  int rc = RAND_bytes(static_cast<unsigned char*>(ptr),
                      static_cast<int>(size));

  if (rc != 1) {
    unsigned long err = ERR_get_error();
    char msg[256] = { 0 };
    ERR_error_string_n(err, msg, sizeof(msg));
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) UtilityImpl::generate_random_bytes: ERROR '%C' returned by RAND_bytes(...)\n"),
               msg));
  }
}

void UtilityImpl::hmac(void* out, void const* in, size_t size, const std::string& password) const
{
  unsigned char* digest = HMAC(EVP_sha1(), password.c_str(),
                               static_cast<int>(password.size()),
                               static_cast<const unsigned char*>(in),
                               static_cast<int>(size), NULL, NULL);
  memcpy(out, digest, 20);
}

} // Security
} // OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
