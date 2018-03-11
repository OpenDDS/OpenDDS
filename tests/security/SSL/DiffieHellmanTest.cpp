/*
 * Distributed under the DDS License.
 * See: http://www.DDS.org/license.html
 */

#include "gtest/gtest.h"
#include "dds/DCPS/security/SSL/DiffieHellman.h"
#include <iostream>

using namespace OpenDDS::Security::SSL;


class DiffieHellmanTest : public ::testing::Test
{
public:
  DiffieHellmanTest() :
    dh1(),
    dh2()
  {

  }

  ~DiffieHellmanTest()
  {

  }

  DiffieHellman dh1;
  DiffieHellman dh2;
};

TEST_F(DiffieHellmanTest, PubKey_Generation)
{
  DDS::OctetSeq pubserial;
  dh1.pub_key(pubserial);
  ASSERT_EQ(256u, pubserial.length());
}

TEST_F(DiffieHellmanTest, SharedSecret_GenerationAndComparison)
{
  DDS::OctetSeq dh1_pubkey;
  dh1.pub_key(dh1_pubkey);

  DDS::OctetSeq dh2_pubkey;
  dh2.pub_key(dh2_pubkey);

  dh1.gen_shared_secret(dh2_pubkey);
  dh2.gen_shared_secret(dh1_pubkey);

  bool was_successful = dh1.cmp_shared_secret(dh2);

  ASSERT_TRUE(was_successful);
}
