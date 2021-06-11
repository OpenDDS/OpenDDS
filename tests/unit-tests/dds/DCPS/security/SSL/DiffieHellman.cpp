/*
 * Distributed under the OpenDDS License.
 * See: http://www.OpenDDS.org/license.html
 */

#ifdef OPENDDS_SECURITY

#include "gtest/gtest.h"
#include "dds/DCPS/security/OpenSSL_init.h"
#include "dds/DCPS/security/SSL/DiffieHellman.h"
#include <iostream>

using namespace OpenDDS::Security::SSL;
using OpenDDS::DCPS::move;


class DiffieHellmanTest : public ::testing::Test
{
public:
  DiffieHellmanTest() :
    dh1(new DH_2048_MODP_256_PRIME),
    dh2(new DH_2048_MODP_256_PRIME),
    dh3(new ECDH_PRIME_256_V1_CEUM),
    dh4(new ECDH_PRIME_256_V1_CEUM)
  {

  }

  ~DiffieHellmanTest()
  {

  }

  DiffieHellman dh1;
  DiffieHellman dh2;
  DiffieHellman dh3;
  DiffieHellman dh4;
};

TEST_F(DiffieHellmanTest, DH_SharedSecret_GenerationAndComparison)
{
  DDS::OctetSeq dh1_pubkey;
  dh1.pub_key(dh1_pubkey);

  DDS::OctetSeq dh2_pubkey;
  dh2.pub_key(dh2_pubkey);

  ASSERT_EQ(0, dh1.gen_shared_secret(dh2_pubkey));
  ASSERT_EQ(0, dh2.gen_shared_secret(dh1_pubkey));

  bool was_successful = dh1.cmp_shared_secret(dh2);
  ASSERT_TRUE(was_successful);
}

TEST_F(DiffieHellmanTest, EC_SharedSecret_GenerationAndComparison)
{
  DDS::OctetSeq dh3_pubkey;
  dh3.pub_key(dh3_pubkey);

  DDS::OctetSeq dh4_pubkey;
  dh4.pub_key(dh4_pubkey);

  ASSERT_EQ(0, dh3.gen_shared_secret(dh4_pubkey));
  ASSERT_EQ(0, dh4.gen_shared_secret(dh3_pubkey));

  bool was_successful = dh3.cmp_shared_secret(dh4);
  ASSERT_TRUE(was_successful);
}

#endif
