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
  ASSERT_EQ(sizeof(OctetArray16), sizeof(GUID_t));
}
