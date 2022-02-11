/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <gtest/gtest.h>

#include "dds/DCPS/NetworkAddress.h"

using namespace OpenDDS::DCPS;

TEST(dds_DCPS_NetworkAddress, Default)
{
  const NetworkAddress sa;
  EXPECT_EQ(sa.get_port_number(), 0);
  EXPECT_EQ(sa, NetworkAddress());
}

TEST(dds_DCPS_NetworkAddress, DefaultInput)
{
  const ACE_INET_Addr ia;
  const NetworkAddress sa(ia);
  EXPECT_EQ(sa, NetworkAddress());
  EXPECT_EQ(sa.to_addr(), ia);
  EXPECT_EQ(sa.to_addr(), ACE_INET_Addr());
}

TEST(dds_DCPS_NetworkAddress, IpFourPortAddr)
{
  const ACE_INET_Addr ia(1234, "127.0.10.13");
  const NetworkAddress sa(ia);
  EXPECT_EQ(sa.get_port_number(), 1234);
  EXPECT_EQ(sa.to_addr(), ia);
}

TEST(dds_DCPS_NetworkAddress, IpFourAddrString)
{
  const ACE_INET_Addr ia("127.0.10.13:4321");
  const NetworkAddress sa(ia);
  EXPECT_EQ(sa.get_port_number(), 4321);
  EXPECT_EQ(sa.to_addr(), ia);
}

TEST(dds_DCPS_NetworkAddress, IpSixPortAddr)
{
  const ACE_INET_Addr ia(1234, "::FD01");
  const NetworkAddress sa(ia);
  EXPECT_EQ(sa.get_port_number(), 1234);
  EXPECT_EQ(sa.to_addr(), ia);
}

TEST(dds_DCPS_NetworkAddress, IpSixAddrString)
{
  const ACE_INET_Addr ia("::FE03:4321");
  const NetworkAddress sa(ia);
  EXPECT_EQ(sa.get_port_number(), 4321);
  EXPECT_EQ(sa.to_addr(), ia);
}

