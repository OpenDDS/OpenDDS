/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <dds/DCPS/XTypes/TypeLookupService.h>
#include <dds/DCPS/debug.h>

#include <gtest/gtest.h>

using namespace OpenDDS::XTypes;

namespace {

TypeObject minimal_bitmask(BitmaskTypeFlag flags)
{
  MinimalBitmaskType bitmask;
  bitmask.bitmask_flags = flags;
  bitmask.header.common.bit_bound = 8;
  MinimalBitflag flag;
  flag.detail = MinimalMemberDetail("FLAG_0");
  bitmask.flag_seq.append(flag);
  return TypeObject(MinimalTypeObject(bitmask));
}

TypeObject complete_bitmask(BitmaskTypeFlag flags)
{
  CompleteBitmaskType bitmask;
  bitmask.bitmask_flags = flags;
  bitmask.header.common.bit_bound = 8;
  bitmask.header.detail.type_name = "TypeLookupServiceTest::Flags";
  CompleteBitflag flag;
  flag.detail.name = "FLAG_0";
  bitmask.flag_seq.append(flag);
  return TypeObject(CompleteTypeObject(bitmask));
}

bool added_to_cache(const TypeObject& to)
{
  TypeLookupService tls;
  TypeIdentifierTypeObjectPairSeq types;
  types.append(TypeIdentifierTypeObjectPair(makeTypeIdentifier(to), to));
  tls.add_type_objects_to_cache(types);
  return tls.type_object_in_cache(types[0].type_identifier);
}

} // namespace

TEST(dds_DCPS_XTypes_TypeLookupService, AddBitmaskTypeObjectFlags)
{
  OpenDDS::DCPS::LogRestore restore;
  OpenDDS::DCPS::log_level.set(OpenDDS::DCPS::LogLevel::None);

  // BitmaskTypeFlag is unused, so a bitmask without any flags is valid.
  EXPECT_TRUE(added_to_cache(minimal_bitmask(0)));
  EXPECT_TRUE(added_to_cache(complete_bitmask(0)));

  // Extensibility flags some producers set are also accepted.
  EXPECT_TRUE(added_to_cache(minimal_bitmask(IS_FINAL)));
  EXPECT_TRUE(added_to_cache(complete_bitmask(IS_FINAL)));
  EXPECT_TRUE(added_to_cache(minimal_bitmask(IS_APPENDABLE)));
  EXPECT_TRUE(added_to_cache(complete_bitmask(IS_APPENDABLE)));

  EXPECT_FALSE(added_to_cache(minimal_bitmask(IS_MUTABLE)));
  EXPECT_FALSE(added_to_cache(complete_bitmask(IS_MUTABLE)));
}
