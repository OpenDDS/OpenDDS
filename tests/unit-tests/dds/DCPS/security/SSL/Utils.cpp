/*
 * Distributed under the OpenDDS License.
 * See: http://www.OpenDDS.org/license.html
 */

#ifdef OPENDDS_SECURITY

#include "gtest/gtest.h"
#include "dds/DCPS/security/OpenSSL_init.h"
#include "dds/DCPS/security/SSL/Utils.h"

TEST(dds_DCPS_security_SSL_Utils, MakeNonce256_Success)
{
  std::vector<unsigned char> nonce;
  int err = OpenDDS::Security::SSL::make_nonce_256(nonce);
  ASSERT_EQ(0, err);
}

#endif
