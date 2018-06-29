/*
* Distributed under the DDS License.
* See: http://www.DDS.org/license.html
*/

#include "gtest/gtest.h"
#include "dds/DCPS/security/SSL/EllipticCurve.h"
#include <iostream>

using namespace OpenDDS::Security::SSL;
using OpenDDS::DCPS::move;

class EllipticCurveTest : public ::testing::Test
{
public:
  EllipticCurveTest() :
      ec1(new EC_PRIME_256_V1_CEUM),
      ec2(new EC_PRIME_256_V1_CEUM)
  {

  }

  ~EllipticCurveTest()
  {

  }

  EllipticCurve ec1;
  EllipticCurve ec2;
};

TEST_F(EllipticCurveTest, EC_Prime_PubKey_Generation) {
  DDS::OctetSeq pubserial;

  int pk_ret_value = ec1.pub_key(pubserial);
  ASSERT_EQ(0, pk_ret_value);
  ASSERT_EQ(65u, pubserial.length());
}

TEST_F(EllipticCurveTest, EC_Prime_SharedSecret_GenerationAndComparison) {
  int pk_ret_value;

  DDS::OctetSeq ec1_pubkey;
  pk_ret_value = ec1.pub_key(ec1_pubkey);
  ASSERT_EQ(0, pk_ret_value);

  DDS::OctetSeq ec2_pubkey;
  pk_ret_value = ec2.pub_key(ec2_pubkey);
  ASSERT_EQ(0, pk_ret_value);

  ASSERT_EQ(0, ec1.gen_shared_secret(ec2_pubkey));
  ASSERT_EQ(0, ec2.gen_shared_secret(ec1_pubkey));

  bool was_successful = ec1.cmp_shared_secret(ec2);
  ASSERT_TRUE(was_successful);
}
