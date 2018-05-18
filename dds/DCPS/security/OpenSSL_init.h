/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

/**
 * This is seperate from OpenSSL_legacy.h to use in the SSL Unit Test Main.cpp
 */

#ifndef OPENSSL_INIT_H
#define OPENSSL_INIT_H

#include <openssl/evp.h>

#if OPENSSL_VERSION_NUMBER < 0x10100000L

inline void openssl_init()
{
  OpenSSL_add_all_algorithms();
}

inline void openssl_cleanup()
{
  EVP_cleanup();
}

#else

inline void openssl_init()
{
}

inline void openssl_cleanup()
{
}

#endif // OPENSSL_VERSION_NUMBER < 0x10100000L

#endif
