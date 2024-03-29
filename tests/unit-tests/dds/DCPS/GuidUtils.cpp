#include <dds/DCPS/GuidUtils.h>

#include <gtest/gtest.h>

using namespace OpenDDS::DCPS;

/**
 * Test that assign(OctetArray16& dest, const GUID_t& src) will work since dest
 * is an array and src is an unpacked struct, so theoretically the compiler
 * could add padding to GUID_t, even though this is unlikely.
 */
TEST(dds_DCPS_GuidUtils, guid_t_vs_octet_array16_size_test)
{
  ASSERT_EQ(sizeof(DDS::OctetArray16), sizeof(GUID_t));
}

TEST(dds_DCPS_GuidUtils, guid_pair_cmp)
{
  GuidPair a(GUID_UNKNOWN, GUID_UNKNOWN);
  GuidPair b(GUID_UNKNOWN, GUID_UNKNOWN);
  ASSERT_EQ(a.cmp(b), 0); // (0, 0) == (0, 0)
  b.remote.guidPrefix[0] = 1;
  ASSERT_LT(a.cmp(b), 0); // (0, 0) < (0, 1)
  a.remote.guidPrefix[0] = 2;
  ASSERT_GT(a.cmp(b), 0); // (0, 2) > (0, 1)
  b.remote.guidPrefix[0] = 2;
  ASSERT_EQ(a.cmp(b), 0); // (0, 2) == (0, 2)
  b.local.guidPrefix[0] = 1;
  ASSERT_LT(a.cmp(b), 0); // (0, 2) < (1, 2)
  a.local.guidPrefix[0] = 2;
  ASSERT_GT(a.cmp(b), 0); // (2, 2) > (1, 2)
  b.local.guidPrefix[0] = 2;
  ASSERT_EQ(a.cmp(b), 0); // (2, 2) == (2, 2)
}
