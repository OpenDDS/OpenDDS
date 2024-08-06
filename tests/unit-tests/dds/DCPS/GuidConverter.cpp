/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <gtest/gtest.h>

#include "dds/DCPS/GuidConverter.h"

using namespace OpenDDS::DCPS;

GUID_t InitGUID(int testID)
{
  // initialize testing GUID to a couple known values.
  GUID_t guid;

  memset(&guid, 0, sizeof(guid));
  if (testID >= 0 && testID <= 1) {
    guid.guidPrefix[0] = 0x01;
    guid.guidPrefix[1] = 0x02;
    guid.guidPrefix[2] = (testID == 0) ? 0x03 : 0xE9;
    guid.guidPrefix[3] = 0x04;
    guid.guidPrefix[4] = (testID == 0) ? 0x05 : 0xF4;
    guid.guidPrefix[5] = 0x06;
    guid.guidPrefix[6] = (testID == 0) ? 0x07 : 0xFF;
    guid.guidPrefix[7] = 0x08;
    guid.guidPrefix[8] = 0x09;
    guid.guidPrefix[9] = 0x0A;
    guid.guidPrefix[10] = 0x0B;
    guid.guidPrefix[11] = 0x0C;
    guid.entityId.entityKind = 0;
    guid.entityId.entityKey[0] = 0xd0;
    guid.entityId.entityKey[1] = 0xe0;
    guid.entityId.entityKey[2] = 0xf0;
  }
  return guid;
}

TEST(dds_DCPS_GuidConverter, prefixes_to_ParticipantID)
{
  // Test GUID conversion
  GUID_t guid0 = InitGUID(0);
  GUID_t guid1 = InitGUID(1);

  EXPECT_TRUE(GuidConverter(guid0).uniqueParticipantId() == "0102030405060708090a0b0c");

  EXPECT_TRUE(GuidConverter(guid1).uniqueParticipantId() == "0102e904f406ff08090a0b0c");
}

TEST(dds_DCPS_GuidConverter, validate_Checksum)
{
  // Test checksum calculator
  GUID_t guid0 = InitGUID(0);
  GUID_t guid1 = InitGUID(1);

  unsigned int crc0 = GuidConverter(guid0).checksum();
  unsigned int crc1 = GuidConverter(guid1).checksum();

  EXPECT_EQ(crc0, 0xf09df109);
  EXPECT_EQ(crc1, 0xf1b7254f);
}

TEST(dds_DCPS_GuidConverter, validate_IDs_and_Key)
{
  // Test additional IDs and Key
  GUID_t guid = InitGUID(1);

  long vendorId = GuidConverter(guid).vendorId();
  long entityId = GuidConverter(guid).entityId();
  long entityKey = GuidConverter(guid).entityKey();

  EXPECT_EQ(vendorId, 0x00000102);
  EXPECT_EQ(entityId, 0xd0e0f000);
  EXPECT_EQ(entityKey, 0x00d0e0f0);
}

