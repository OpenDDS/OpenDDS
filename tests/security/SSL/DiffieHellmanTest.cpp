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

}
