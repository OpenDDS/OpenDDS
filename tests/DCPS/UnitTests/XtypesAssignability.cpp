#include "dds/DCPS/TypeAssignability.h"

#include "gtest/gtest.h"

using namespace OpenDDS::XTypes;

TEST(PrimitiveTypesTest, Assignable)
{
  TypeAssignability test;
  TypeIdentifier tia, tib;
  tia.kind = TK_BOOLEAN;
  tib.kind = tia.kind;
  EXPECT_TRUE(test.assignable(tia, tib));

  tia.kind = TK_BYTE;
  tib.kind = tia.kind;
  EXPECT_TRUE(test.assignable(tia, tib));

  tia.kind = TK_INT16;
  tib.kind = tia.kind;
  EXPECT_TRUE(test.assignable(tia, tib));

  tia.kind = TK_INT32;
  tib.kind = tia.kind;
  EXPECT_TRUE(test.assignable(tia, tib));

  tia.kind = TK_INT64;
  tib.kind = tia.kind;
  EXPECT_TRUE(test.assignable(tia, tib));

  tia.kind = TK_UINT16;
  tib.kind = tia.kind;
  EXPECT_TRUE(test.assignable(tia, tib));

  tia.kind = TK_UINT32;
  tib.kind = tia.kind;
  EXPECT_TRUE(test.assignable(tia, tib));

  tia.kind = TK_UINT64;
  tib.kind = tia.kind;
  EXPECT_TRUE(test.assignable(tia, tib));

  tia.kind = TK_FLOAT32;
  tib.kind = tia.kind;
  EXPECT_TRUE(test.assignable(tia, tib));

  tia.kind = TK_FLOAT64;
  tib.kind = tia.kind;
  EXPECT_TRUE(test.assignable(tia, tib));

  tia.kind = TK_FLOAT128;
  tib.kind = tia.kind;
  EXPECT_TRUE(test.assignable(tia, tib));

  tia.kind = TK_INT8;
  tib.kind = tia.kind;
  EXPECT_TRUE(test.assignable(tia, tib));

  tia.kind = TK_UINT8;
  tib.kind = tia.kind;
  EXPECT_TRUE(test.assignable(tia, tib));

  tia.kind = TK_CHAR8;
  tib.kind = tia.kind;
  EXPECT_TRUE(test.assignable(tia, tib));

  tia.kind = TK_CHAR16;
  tib.kind = tia.kind;
  EXPECT_TRUE(test.assignable(tia, tib));

  // Assignability from bitmask
  tia.kind = TK_UINT8;
  TypeIdentifierPtr tib_ptr(new TypeIdentifier, OpenDDS::DCPS::keep_count());
  tib_ptr->kind = EK_MINIMAL;
  BitBound bound = 8;
  CommonEnumeratedHeader common_header(bound);
  MinimalBitmaskHeader header(common_header);
  MinimalBitmaskType bitmask;
  bitmask.header = header;
  MinimalTypeObject tob(bitmask);
  EXPECT_TRUE(test.assignable_primitive(tia, tob));

  tia.kind = TK_UINT16;
  tob.bitmask_type.header.common.bit_bound = 16;
  EXPECT_TRUE(test.assignable_primitive(tia, tob));

  tia.kind = TK_UINT32;
  tob.bitmask_type.header.common.bit_bound = 32;
  EXPECT_TRUE(test.assignable_primitive(tia, tob));

  tia.kind = TK_UINT64;
  tob.bitmask_type.header.common.bit_bound = 64;
  EXPECT_TRUE(test.assignable_primitive(tia, tob));
}

TEST(PrimitiveTypesTest, NotAssignable)
{
  TypeAssignability test;
  TypeIdentifier tia, tib;
  tia.kind = TK_BOOLEAN;
  tib.kind = TK_BYTE;
  EXPECT_FALSE(test.assignable(tia, tib));

  tia.kind = TK_BYTE;
  tib.kind = TK_FLOAT32;
  EXPECT_FALSE(test.assignable(tia, tib));

  tia.kind = TK_INT16;
  tib.kind = TK_INT64;
  EXPECT_FALSE(test.assignable(tia, tib));

  tia.kind = TK_INT32;
  tib.kind = TK_INT16;
  EXPECT_FALSE(test.assignable(tia, tib));

  tia.kind = TK_INT64;
  tib.kind = TK_CHAR8;
  EXPECT_FALSE(test.assignable(tia, tib));

  tia.kind = TK_UINT16;
  tib.kind = TK_FLOAT32;
  EXPECT_FALSE(test.assignable(tia, tib));

  tia.kind = TK_UINT32;
  tib.kind = TK_BYTE;
  EXPECT_FALSE(test.assignable(tia, tib));

  tia.kind = TK_UINT64;
  tib.kind = TK_FLOAT64;
  EXPECT_FALSE(test.assignable(tia, tib));

  tia.kind = TK_FLOAT32;
  tib.kind = TK_INT64;
  EXPECT_FALSE(test.assignable(tia, tib));

  tia.kind = TK_FLOAT64;
  tib.kind = TK_INT64;
  EXPECT_FALSE(test.assignable(tia, tib));

  tia.kind = TK_FLOAT128;
  tib.kind = TK_UINT64;
  EXPECT_FALSE(test.assignable(tia, tib));

  tia.kind = TK_INT8;
  tib.kind = TK_UINT16;
  EXPECT_FALSE(test.assignable(tia, tib));

  tia.kind = TK_UINT8;
  tib.kind = TK_CHAR8;
  EXPECT_FALSE(test.assignable(tia, tib));

  tia.kind = TK_CHAR8;
  tib.kind = TK_INT16;
  EXPECT_FALSE(test.assignable(tia, tib));

  tia.kind = TK_CHAR16;
  tib.kind = TK_INT32;
  EXPECT_FALSE(test.assignable(tia, tib));

  // Assignability from bitmask
  tia.kind = TK_UINT8;
  TypeIdentifierPtr tib_ptr(new TypeIdentifier, OpenDDS::DCPS::keep_count());
  tib_ptr->kind = EK_MINIMAL;
  BitBound bound = 9;
  CommonEnumeratedHeader common_header(bound);
  MinimalBitmaskHeader header(common_header);
  MinimalBitmaskType bitmask;
  bitmask.header = header;
  MinimalTypeObject tob(bitmask);
  EXPECT_FALSE(test.assignable_primitive(tia, tob));

  tia.kind = TK_UINT16;
  tob.bitmask_type.header.common.bit_bound = 17;
  EXPECT_FALSE(test.assignable_primitive(tia, tob));

  tia.kind = TK_UINT32;
  tob.bitmask_type.header.common.bit_bound = 33;
  EXPECT_FALSE(test.assignable_primitive(tia, tob));

  tia.kind = TK_UINT64;
  tob.bitmask_type.header.common.bit_bound = 31;
  EXPECT_FALSE(test.assignable_primitive(tia, tob));
}

TEST(StringTypesTest, Assignable)
{
}

TEST(StringTypesTest, NotAssignable)
{
}

int main(int argc, char* argv[])
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
