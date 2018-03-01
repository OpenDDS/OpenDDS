/*
 * Distributed under the DDS License.
 * See: http://www.DDS.org/license.html
 */

#include "gtest/gtest.h"
#include "dds/DCPS/security/SSL/Utils.h"

TEST(UtilsTest, MakeNonce256_Success)
{
  std::vector<unsigned char> nonce;
  int err = OpenDDS::Security::SSL::make_nonce_256(nonce);
  ASSERT_EQ(0, err);
}
