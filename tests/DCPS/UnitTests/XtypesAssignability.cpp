#include "dds/DCPS/TypeAssignability.h"

#include "gtest/gtest.h"

using namespace OpenDDS::XTypes;

TEST(PrimitiveTypesTest, Assignable)
{
  TypeAssignability test;
  TypeIdentifier tia, tib;
  tia.kind(TK_BOOLEAN);
  tib.kind(tia.kind());
  EXPECT_TRUE(test.assignable(tia, tib));

  tia.kind(TK_BYTE);
  tib.kind(tia.kind());
  EXPECT_TRUE(test.assignable(tia, tib));

  tia.kind(TK_INT16);
  tib.kind(tia.kind());
  EXPECT_TRUE(test.assignable(tia, tib));

  tia.kind(TK_INT32);
  tib.kind(tia.kind());
  EXPECT_TRUE(test.assignable(tia, tib));

  tia.kind(TK_INT64);
  tib.kind(tia.kind());
  EXPECT_TRUE(test.assignable(tia, tib));

  tia.kind(TK_UINT16);
  tib.kind(tia.kind());
  EXPECT_TRUE(test.assignable(tia, tib));

  tia.kind(TK_UINT32);
  tib.kind(tia.kind());
  EXPECT_TRUE(test.assignable(tia, tib));

  tia.kind(TK_UINT64);
  tib.kind(tia.kind());
  EXPECT_TRUE(test.assignable(tia, tib));

  tia.kind(TK_FLOAT32);
  tib.kind(tia.kind());
  EXPECT_TRUE(test.assignable(tia, tib));

  tia.kind(TK_FLOAT64);
  tib.kind(tia.kind());
  EXPECT_TRUE(test.assignable(tia, tib));

  tia.kind(TK_FLOAT128);
  tib.kind(tia.kind());
  EXPECT_TRUE(test.assignable(tia, tib));

  tia.kind(TK_INT8);
  tib.kind(tia.kind());
  EXPECT_TRUE(test.assignable(tia, tib));

  tia.kind(TK_UINT8);
  tib.kind(tia.kind());
  EXPECT_TRUE(test.assignable(tia, tib));

  tia.kind(TK_CHAR8);
  tib.kind(tia.kind());
  EXPECT_TRUE(test.assignable(tia, tib));

  tia.kind(TK_CHAR16);
  tib.kind(tia.kind());
  EXPECT_TRUE(test.assignable(tia, tib));

  // Assignability from bitmask
  tia.kind(TK_UINT8);
  BitBound bound = 8;
  CommonEnumeratedHeader common_header(bound);
  MinimalBitmaskHeader header(common_header);
  MinimalBitmaskType bitmask;
  bitmask.header = header;
  MinimalTypeObject tob(bitmask);
  EXPECT_TRUE(test.assignable(tia, TypeObject(tob)));

  tia.kind(TK_UINT16);
  tob.bitmask_type.header.common.bit_bound = 16;
  EXPECT_TRUE(test.assignable(tia, TypeObject(tob)));

  tia.kind(TK_UINT32);
  tob.bitmask_type.header.common.bit_bound = 32;
  EXPECT_TRUE(test.assignable(tia, TypeObject(tob)));

  tia.kind(TK_UINT64);
  tob.bitmask_type.header.common.bit_bound = 64;
  EXPECT_TRUE(test.assignable(tia, TypeObject(tob)));
}

TEST(PrimitiveTypesTest, NotAssignable)
{
  TypeAssignability test;
  TypeIdentifier tia, tib;
  tia.kind(TK_BOOLEAN);
  tib.kind(TK_BYTE);
  EXPECT_FALSE(test.assignable(tia, tib));

  tia.kind(TK_BYTE);
  tib.kind(TK_FLOAT32);
  EXPECT_FALSE(test.assignable(tia, tib));

  tia.kind(TK_INT16);
  tib.kind(TK_INT64);
  EXPECT_FALSE(test.assignable(tia, tib));

  tia.kind(TK_INT32);
  tib.kind(TK_INT16);
  EXPECT_FALSE(test.assignable(tia, tib));

  tia.kind(TK_INT64);
  tib.kind(TK_CHAR8);
  EXPECT_FALSE(test.assignable(tia, tib));

  tia.kind(TK_UINT16);
  tib.kind(TK_FLOAT32);
  EXPECT_FALSE(test.assignable(tia, tib));

  tia.kind(TK_UINT32);
  tib.kind(TK_BYTE);
  EXPECT_FALSE(test.assignable(tia, tib));

  tia.kind(TK_UINT64);
  tib.kind(TK_FLOAT64);
  EXPECT_FALSE(test.assignable(tia, tib));

  tia.kind(TK_FLOAT32);
  tib.kind(TK_INT64);
  EXPECT_FALSE(test.assignable(tia, tib));

  tia.kind(TK_FLOAT64);
  tib.kind(TK_INT64);
  EXPECT_FALSE(test.assignable(tia, tib));

  tia.kind(TK_FLOAT128);
  tib.kind(TK_UINT64);
  EXPECT_FALSE(test.assignable(tia, tib));

  tia.kind(TK_INT8);
  tib.kind(TK_UINT16);
  EXPECT_FALSE(test.assignable(tia, tib));

  tia.kind(TK_UINT8);
  tib.kind(TK_CHAR8);
  EXPECT_FALSE(test.assignable(tia, tib));

  tia.kind(TK_CHAR8);
  tib.kind(TK_INT16);
  EXPECT_FALSE(test.assignable(tia, tib));

  tia.kind(TK_CHAR16);
  tib.kind(TK_INT32);
  EXPECT_FALSE(test.assignable(tia, tib));

  // Assignability from bitmask
  tia.kind(TK_UINT8);
  BitBound bound = 9;
  CommonEnumeratedHeader common_header(bound);
  MinimalBitmaskHeader header(common_header);
  MinimalBitmaskType bitmask;
  bitmask.header = header;
  MinimalTypeObject tob(bitmask);
  EXPECT_FALSE(test.assignable(tia, TypeObject(tob)));

  tia.kind(TK_UINT16);
  tob.bitmask_type.header.common.bit_bound = 17;
  EXPECT_FALSE(test.assignable(tia, TypeObject(tob)));

  tia.kind(TK_UINT32);
  tob.bitmask_type.header.common.bit_bound = 33;
  EXPECT_FALSE(test.assignable(tia, TypeObject(tob)));

  tia.kind(TK_UINT64);
  tob.bitmask_type.header.common.bit_bound = 31;
  EXPECT_FALSE(test.assignable(tia, TypeObject(tob)));
}

TEST(StringTypesTest, Assignable)
{
  TypeAssignability test;
  TypeIdentifier tia, tib;
  tia.kind(TI_STRING8_SMALL);
  tib.kind(TI_STRING8_SMALL);
  EXPECT_TRUE(test.assignable(tia, tib));
  tib.kind(TI_STRING8_LARGE);
  EXPECT_TRUE(test.assignable(tia, tib));

  tia.kind(TI_STRING8_LARGE);
  EXPECT_TRUE(test.assignable(tia, tib));
  tib.kind(TI_STRING8_SMALL);
  EXPECT_TRUE(test.assignable(tia, tib));

  tia.kind(TI_STRING16_SMALL);
  tib.kind(TI_STRING16_SMALL);
  EXPECT_TRUE(test.assignable(tia, tib));
  tib.kind(TI_STRING16_LARGE);
  EXPECT_TRUE(test.assignable(tia, tib));

  tia.kind(TI_STRING16_LARGE);
  EXPECT_TRUE(test.assignable(tia, tib));
  tib.kind(TI_STRING16_SMALL);
  EXPECT_TRUE(test.assignable(tia, tib));
}

void string_expect_false(const TypeAssignability& test, const TypeIdentifier& tia, TypeIdentifier& tib)
{
  tib.kind(TK_BOOLEAN);
  EXPECT_FALSE(test.assignable(tia, tib));
  tib.kind(TK_BYTE);
  EXPECT_FALSE(test.assignable(tia, tib));
  tib.kind(TK_INT16);
  EXPECT_FALSE(test.assignable(tia, tib));
  tib.kind(TK_INT32);
  EXPECT_FALSE(test.assignable(tia, tib));
  tib.kind(TK_INT64);
  EXPECT_FALSE(test.assignable(tia, tib));
  tib.kind(TK_UINT16);
  EXPECT_FALSE(test.assignable(tia, tib));
  tib.kind(TK_UINT32);
  EXPECT_FALSE(test.assignable(tia, tib));
  tib.kind(TK_UINT64);
  EXPECT_FALSE(test.assignable(tia, tib));
  tib.kind(TK_FLOAT32);
  EXPECT_FALSE(test.assignable(tia, tib));
  tib.kind(TK_FLOAT64);
  EXPECT_FALSE(test.assignable(tia, tib));
  tib.kind(TK_FLOAT128);
  EXPECT_FALSE(test.assignable(tia, tib));
  tib.kind(TK_INT8);
  EXPECT_FALSE(test.assignable(tia, tib));
  tib.kind(TK_UINT8);
  EXPECT_FALSE(test.assignable(tia, tib));
  tib.kind(TK_CHAR8);
  EXPECT_FALSE(test.assignable(tia, tib));
  tib.kind(TK_CHAR16);
  EXPECT_FALSE(test.assignable(tia, tib));

  if (TI_STRING8_SMALL == tia.kind() || TI_STRING8_LARGE == tia.kind()) {
    tib.kind(TI_STRING16_SMALL);
    EXPECT_FALSE(test.assignable(tia, tib));
    tib.kind(TI_STRING16_LARGE);
    EXPECT_FALSE(test.assignable(tia, tib));
  } else {
    tib.kind(TI_STRING8_SMALL);
    EXPECT_FALSE(test.assignable(tia, tib));
    tib.kind(TI_STRING8_LARGE);
    EXPECT_FALSE(test.assignable(tia, tib));
  }

  tib.kind(TI_PLAIN_SEQUENCE_SMALL);
  EXPECT_FALSE(test.assignable(tia, tib));
  tib.kind(TI_PLAIN_SEQUENCE_LARGE);
  EXPECT_FALSE(test.assignable(tia, tib));
  tib.kind(TI_PLAIN_ARRAY_SMALL);
  EXPECT_FALSE(test.assignable(tia, tib));
  tib.kind(TI_PLAIN_ARRAY_LARGE);
  EXPECT_FALSE(test.assignable(tia, tib));
  tib.kind(TI_PLAIN_MAP_SMALL);
  EXPECT_FALSE(test.assignable(tia, tib));
  tib.kind(TI_PLAIN_MAP_LARGE);
  EXPECT_FALSE(test.assignable(tia, tib));
  tib.kind(TI_STRONGLY_CONNECTED_COMPONENT);
  EXPECT_FALSE(test.assignable(tia, tib));
  tib.kind(EK_COMPLETE);
  EXPECT_FALSE(test.assignable(tia, tib));
  tib.kind(EK_MINIMAL);
  EXPECT_FALSE(test.assignable(tia, tib));
}

TEST(StringTypesTest, NotAssignable)
{
  TypeAssignability test;
  TypeIdentifier tia, tib;
  tia.kind(TI_STRING8_SMALL);
  string_expect_false(test, tia, tib);
  tia.kind(TI_STRING8_LARGE);
  string_expect_false(test, tia, tib);
  tia.kind(TI_STRING16_SMALL);
  string_expect_false(test, tia, tib);
  tia.kind(TI_STRING16_LARGE);
  string_expect_false(test, tia, tib);
}

class EnumTypeTest : public ::testing::Test {
protected:
  void SetUp()
  {
    enum_a_.enum_flags = IS_APPENDABLE;
    enum_b_.enum_flags = enum_a_.enum_flags;
    MinimalEnumeratedLiteral l1_a, l2_a;
    l1_a.common.value = 3;
    l1_a.common.flags = IS_DEFAULT;
    ACE_CDR::Octet tmp1[4] = {0x11, 0x22, 0x33, 0x44};
    std::memcpy(l1_a.detail.name_hash, tmp1, sizeof l1_a.detail.name_hash);
    enum_a_.literal_seq.append(l1_a);
    l2_a.common.value = 5;
    ACE_CDR::Octet tmp2[4] = {0x55, 0x66, 0x77, 0x88};
    std::memcpy(l2_a.detail.name_hash, tmp2, sizeof l2_a.detail.name_hash);
    enum_a_.literal_seq.append(l2_a);

    MinimalEnumeratedLiteral l1_b, l2_b, l3_b;
    l1_b.common.value = 3;
    ACE_CDR::Octet tmp3[4] = {0x11, 0x22, 0x33, 0x44};
    std::memcpy(l1_b.detail.name_hash, tmp3, sizeof l1_b.detail.name_hash);
    enum_b_.literal_seq.append(l1_b);
    l2_b.common.value = 5;
    l2_b.common.flags = IS_DEFAULT;
    ACE_CDR::Octet tmp4[4] = {0x55, 0x66, 0x77, 0x88};
    std::memcpy(l2_b.detail.name_hash, tmp4, sizeof l2_b.detail.name_hash);
    enum_b_.literal_seq.append(l2_b);
    l3_b.common.value = 7;
    ACE_CDR::Octet tmp5[4] = {0x99, 0xAA, 0xBB, 0xCC};
    std::memcpy(l3_b.detail.name_hash, tmp5, sizeof l3_b.detail.name_hash);
    enum_b_.literal_seq.append(l3_b);
  }

  MinimalEnumeratedType enum_a_;
  MinimalEnumeratedType enum_b_;
};

TEST_F(EnumTypeTest, Assignable)
{
  TypeAssignability test;
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(enum_a_)),
                              TypeObject(MinimalTypeObject(enum_b_))));

  // Literal sets are expected to be identical
  enum_a_.enum_flags = IS_FINAL;
  enum_b_.enum_flags = enum_a_.enum_flags;
  enum_b_.literal_seq.members.pop_back();
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(enum_a_)),
                              TypeObject(MinimalTypeObject(enum_b_))));
}

TEST_F(EnumTypeTest, NotAssignable)
{
  TypeAssignability test;
  // Do not have identical literal sets
  enum_a_.enum_flags = IS_FINAL;
  enum_b_.enum_flags = enum_a_.enum_flags;
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(enum_a_)),
                               TypeObject(MinimalTypeObject(enum_b_))));

  // Different extensibility flags
  enum_a_.enum_flags = IS_APPENDABLE;
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(enum_a_)),
                               TypeObject(MinimalTypeObject(enum_b_))));

  // Some literals with the same name have different values
  enum_b_.enum_flags = IS_APPENDABLE;
  enum_b_.literal_seq.members[1].common.value = 13;
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(enum_a_)),
                               TypeObject(MinimalTypeObject(enum_b_))));

  // Some literals with the same value have different names
  enum_b_.literal_seq.members[1].common.value = 5;
  ACE_CDR::Octet tmp[4] = {0x12, 0x34, 0x56, 0x78};
  std::memcpy(enum_b_.literal_seq.members[1].detail.name_hash, tmp, sizeof(NameHash));
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(enum_a_)),
                               TypeObject(MinimalTypeObject(enum_b_))));

  // Different types
  MinimalAnnotationType annotation_b;
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(enum_a_)),
                               TypeObject(MinimalTypeObject(annotation_b))));
  MinimalStructType struct_b;
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(enum_a_)),
                               TypeObject(MinimalTypeObject(struct_b))));
  MinimalUnionType union_b;
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(enum_a_)),
                               TypeObject(MinimalTypeObject(union_b))));
  MinimalBitsetType bitset_b;
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(enum_a_)),
                               TypeObject(MinimalTypeObject(bitset_b))));
  MinimalSequenceType sequence_b;
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(enum_a_)),
                               TypeObject(MinimalTypeObject(sequence_b))));
  MinimalArrayType array_b;
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(enum_a_)),
                               TypeObject(MinimalTypeObject(array_b))));
  MinimalMapType map_b;
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(enum_a_)),
                               TypeObject(MinimalTypeObject(map_b))));
  MinimalBitmaskType bitmask_b;
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(enum_a_)),
                               TypeObject(MinimalTypeObject(bitmask_b))));

  TypeIdentifier tib;
  tib.kind(TK_BOOLEAN);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(enum_a_)), tib));
  tib.kind(TK_BYTE);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(enum_a_)), tib));
  tib.kind(TK_INT16);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(enum_a_)), tib));
  tib.kind(TK_INT32);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(enum_a_)), tib));
  tib.kind(TK_INT64);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(enum_a_)), tib));
  tib.kind(TK_UINT16);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(enum_a_)), tib));
  tib.kind(TK_UINT32);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(enum_a_)), tib));
  tib.kind(TK_UINT64);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(enum_a_)), tib));
  tib.kind(TK_FLOAT32);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(enum_a_)), tib));
  tib.kind(TK_FLOAT64);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(enum_a_)), tib));
  tib.kind(TK_FLOAT128);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(enum_a_)), tib));
  tib.kind(TK_INT8);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(enum_a_)), tib));
  tib.kind(TK_UINT8);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(enum_a_)), tib));
  tib.kind(TK_CHAR8);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(enum_a_)), tib));
  tib.kind(TK_CHAR16);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(enum_a_)), tib));
  tib.kind(TI_STRING8_SMALL);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(enum_a_)), tib));
  tib.kind(TI_STRING16_SMALL);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(enum_a_)), tib));
  tib.kind(TI_STRING8_LARGE);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(enum_a_)), tib));
  tib.kind(TI_STRING16_LARGE);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(enum_a_)), tib));
  tib.kind(TI_PLAIN_SEQUENCE_SMALL);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(enum_a_)), tib));
  tib.kind(TI_PLAIN_SEQUENCE_LARGE);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(enum_a_)), tib));
  tib.kind(TI_PLAIN_ARRAY_SMALL);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(enum_a_)), tib));
  tib.kind(TI_PLAIN_ARRAY_LARGE);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(enum_a_)), tib));
  tib.kind(TI_PLAIN_MAP_SMALL);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(enum_a_)), tib));
  tib.kind(TI_PLAIN_MAP_LARGE);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(enum_a_)), tib));
  tib.kind(TI_STRONGLY_CONNECTED_COMPONENT);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(enum_a_)), tib));
  tib.kind(EK_COMPLETE);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(enum_a_)), tib));
}

TEST(BitmaskTypeTest, Assignable)
{
  TypeAssignability test;
  MinimalBitmaskType bitmask_a, bitmask_b;
  bitmask_a.header.common.bit_bound = 16;
  bitmask_b.header.common.bit_bound = bitmask_a.header.common.bit_bound;
  // Throw in some random flag
  MinimalBitflag flag_b;
  flag_b.common.position = 0;
  flag_b.common.flags = IS_DEFAULT | IS_MUST_UNDERSTAND;
  bitmask_b.flag_seq.append(flag_b);
  MinimalTypeObject tobj_a(bitmask_a), tobj_b(bitmask_b);
  EXPECT_TRUE(test.assignable(TypeObject(tobj_a), TypeObject(tobj_b)));

  TypeIdentifier tib;
  tib.kind(TK_UINT8);
  tobj_a.bitmask_type.header.common.bit_bound = 6;
  EXPECT_TRUE(test.assignable(TypeObject(tobj_a), tib));

  tib.kind(TK_UINT16);
  tobj_a.bitmask_type.header.common.bit_bound = 13;
  EXPECT_TRUE(test.assignable(TypeObject(tobj_a), tib));

  tib.kind(TK_UINT32);
  tobj_a.bitmask_type.header.common.bit_bound = 30;
  EXPECT_TRUE(test.assignable(TypeObject(tobj_a), tib));

  tib.kind(TK_UINT64);
  tobj_a.bitmask_type.header.common.bit_bound = 61;
  EXPECT_TRUE(test.assignable(TypeObject(tobj_a), tib));
}

TEST(BitmaskTypeTest, NotAssignable)
{
  TypeAssignability test;
  MinimalBitmaskType bitmask_a;
  bitmask_a.header.common.bit_bound = 32;
  MinimalTypeObject tobj_a(bitmask_a);

  MinimalAnnotationType annotation_b;
  EXPECT_FALSE(test.assignable(TypeObject(tobj_a),
                               TypeObject(MinimalTypeObject(annotation_b))));
  MinimalStructType struct_b;
  EXPECT_FALSE(test.assignable(TypeObject(tobj_a),
                               TypeObject(MinimalTypeObject(struct_b))));
  MinimalUnionType union_b;
  EXPECT_FALSE(test.assignable(TypeObject(tobj_a),
                               TypeObject(MinimalTypeObject(union_b))));
  MinimalBitsetType bitset_b;
  EXPECT_FALSE(test.assignable(TypeObject(tobj_a),
                               TypeObject(MinimalTypeObject(bitset_b))));
  MinimalSequenceType sequence_b;
  EXPECT_FALSE(test.assignable(TypeObject(tobj_a),
                               TypeObject(MinimalTypeObject(sequence_b))));
  MinimalArrayType array_b;
  EXPECT_FALSE(test.assignable(TypeObject(tobj_a),
                               TypeObject(MinimalTypeObject(array_b))));
  MinimalMapType map_b;
  EXPECT_FALSE(test.assignable(TypeObject(tobj_a),
                               TypeObject(MinimalTypeObject(map_b))));
  MinimalEnumeratedType enum_b;
  EXPECT_FALSE(test.assignable(TypeObject(tobj_a),
                               TypeObject(MinimalTypeObject(enum_b))));
  MinimalBitmaskType bitmask_b;
  // A different bit bound than bitmask_a
  bitmask_b.header.common.bit_bound = 16;
  EXPECT_FALSE(test.assignable(TypeObject(tobj_a),
                               TypeObject(MinimalTypeObject(bitmask_b))));

  TypeIdentifier tib;
  tib.kind(TK_BOOLEAN);
  EXPECT_FALSE(test.assignable(TypeObject(tobj_a), tib));
  tib.kind(TK_BYTE);
  EXPECT_FALSE(test.assignable(TypeObject(tobj_a), tib));
  tib.kind(TK_INT16);
  EXPECT_FALSE(test.assignable(TypeObject(tobj_a), tib));
  tib.kind(TK_INT32);
  EXPECT_FALSE(test.assignable(TypeObject(tobj_a), tib));
  tib.kind(TK_INT64);
  EXPECT_FALSE(test.assignable(TypeObject(tobj_a), tib));

  tib.kind(TK_UINT16);
  bitmask_a.header.common.bit_bound = 17;
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(bitmask_a)), tib));
  tib.kind(TK_UINT32);
  bitmask_a.header.common.bit_bound = 33;
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(bitmask_a)), tib));
  tib.kind(TK_UINT64);
  bitmask_a.header.common.bit_bound = 25;
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(bitmask_a)), tib));

  tib.kind(TK_FLOAT32);
  EXPECT_FALSE(test.assignable(TypeObject(tobj_a), tib));
  tib.kind(TK_FLOAT64);
  EXPECT_FALSE(test.assignable(TypeObject(tobj_a), tib));
  tib.kind(TK_FLOAT128);
  EXPECT_FALSE(test.assignable(TypeObject(tobj_a), tib));
  tib.kind(TK_INT8);
  EXPECT_FALSE(test.assignable(TypeObject(tobj_a), tib));

  tib.kind(TK_UINT8);
  bitmask_a.header.common.bit_bound = 9;
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(bitmask_a)), tib));

  tib.kind(TK_CHAR8);
  EXPECT_FALSE(test.assignable(TypeObject(tobj_a), tib));
  tib.kind(TK_CHAR16);
  EXPECT_FALSE(test.assignable(TypeObject(tobj_a), tib));
  tib.kind(TI_STRING8_SMALL);
  EXPECT_FALSE(test.assignable(TypeObject(tobj_a), tib));
  tib.kind(TI_STRING8_LARGE);
  EXPECT_FALSE(test.assignable(TypeObject(tobj_a), tib));
  tib.kind(TI_STRING16_SMALL);
  EXPECT_FALSE(test.assignable(TypeObject(tobj_a), tib));
  tib.kind(TI_STRING16_LARGE);
  EXPECT_FALSE(test.assignable(TypeObject(tobj_a), tib));
  tib.kind(TI_PLAIN_SEQUENCE_SMALL);
  EXPECT_FALSE(test.assignable(TypeObject(tobj_a), tib));
  tib.kind(TI_PLAIN_SEQUENCE_LARGE);
  EXPECT_FALSE(test.assignable(TypeObject(tobj_a), tib));
  tib.kind(TI_PLAIN_ARRAY_SMALL);
  EXPECT_FALSE(test.assignable(TypeObject(tobj_a), tib));
  tib.kind(TI_PLAIN_ARRAY_LARGE);
  EXPECT_FALSE(test.assignable(TypeObject(tobj_a), tib));
  tib.kind(TI_PLAIN_MAP_SMALL);
  EXPECT_FALSE(test.assignable(TypeObject(tobj_a), tib));
  tib.kind(TI_PLAIN_MAP_LARGE);
  EXPECT_FALSE(test.assignable(TypeObject(tobj_a), tib));
  tib.kind(TI_STRONGLY_CONNECTED_COMPONENT);
  EXPECT_FALSE(test.assignable(TypeObject(tobj_a), tib));
  tib.kind(EK_COMPLETE);
  EXPECT_FALSE(test.assignable(TypeObject(tobj_a), tib));
}

TEST(SequenceTypeTest, Assignable)
{
  TypeAssignability test;
  MinimalSequenceType seq_a, seq_b;
  seq_a.header.common.bound = 10;
  seq_b.header.common.bound = 20;

  seq_a.element.common.type = TypeIdentifier(TK_BOOLEAN);
  seq_b.element.common.type = TypeIdentifier(TK_BOOLEAN);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(seq_a)),
                              TypeObject(MinimalTypeObject(seq_b))));
  seq_a.element.common.type = TypeIdentifier(TK_BYTE);
  seq_b.element.common.type = TypeIdentifier(TK_BYTE);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(seq_a)),
                              TypeObject(MinimalTypeObject(seq_b))));
  seq_a.element.common.type = TypeIdentifier(TK_INT16);
  seq_b.element.common.type = TypeIdentifier(TK_INT16);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(seq_a)),
                              TypeObject(MinimalTypeObject(seq_b))));
  seq_a.element.common.type = TypeIdentifier(TK_INT32);
  seq_b.element.common.type = TypeIdentifier(TK_INT32);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(seq_a)),
                              TypeObject(MinimalTypeObject(seq_b))));
  seq_a.element.common.type = TypeIdentifier(TK_INT64);
  seq_b.element.common.type = TypeIdentifier(TK_INT64);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(seq_a)),
                              TypeObject(MinimalTypeObject(seq_b))));
  seq_a.element.common.type = TypeIdentifier(TK_UINT16);
  seq_b.element.common.type = TypeIdentifier(TK_UINT16);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(seq_a)),
                              TypeObject(MinimalTypeObject(seq_b))));
  seq_a.element.common.type = TypeIdentifier(TK_UINT32);
  seq_b.element.common.type = TypeIdentifier(TK_UINT32);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(seq_a)),
                              TypeObject(MinimalTypeObject(seq_b))));
  seq_a.element.common.type = TypeIdentifier(TK_UINT64);
  seq_b.element.common.type = TypeIdentifier(TK_UINT64);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(seq_a)),
                              TypeObject(MinimalTypeObject(seq_b))));
  seq_a.element.common.type = TypeIdentifier(TK_FLOAT32);
  seq_b.element.common.type = TypeIdentifier(TK_FLOAT32);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(seq_a)),
                              TypeObject(MinimalTypeObject(seq_b))));
  seq_a.element.common.type = TypeIdentifier(TK_FLOAT64);
  seq_b.element.common.type = TypeIdentifier(TK_FLOAT64);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(seq_a)),
                              TypeObject(MinimalTypeObject(seq_b))));
  seq_a.element.common.type = TypeIdentifier(TK_FLOAT128);
  seq_b.element.common.type = TypeIdentifier(TK_FLOAT128);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(seq_a)),
                              TypeObject(MinimalTypeObject(seq_b))));
  seq_a.element.common.type = TypeIdentifier(TK_INT8);
  seq_b.element.common.type = TypeIdentifier(TK_INT8);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(seq_a)),
                              TypeObject(MinimalTypeObject(seq_b))));
  seq_a.element.common.type = TypeIdentifier(TK_UINT8);
  seq_b.element.common.type = TypeIdentifier(TK_UINT8);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(seq_a)),
                              TypeObject(MinimalTypeObject(seq_b))));
  seq_a.element.common.type = TypeIdentifier(TK_CHAR8);
  seq_b.element.common.type = TypeIdentifier(TK_CHAR8);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(seq_a)),
                              TypeObject(MinimalTypeObject(seq_b))));
  seq_a.element.common.type = TypeIdentifier(TK_CHAR16);
  seq_b.element.common.type = TypeIdentifier(TK_CHAR16);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(seq_a)),
                              TypeObject(MinimalTypeObject(seq_b))));

  seq_a.element.common.type = TypeIdentifier::makeString(false, StringSTypeDefn(100));
  seq_b.element.common.type = TypeIdentifier::makeString(false, StringSTypeDefn(200));
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(seq_a)),
                              TypeObject(MinimalTypeObject(seq_b))));
  seq_a.element.common.type = TypeIdentifier::makeString(false, StringSTypeDefn(100));
  seq_b.element.common.type = TypeIdentifier::makeString(false, StringLTypeDefn(200));
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(seq_a)),
                              TypeObject(MinimalTypeObject(seq_b))));
  seq_a.element.common.type = TypeIdentifier::makeString(true, StringSTypeDefn(100));
  seq_b.element.common.type = TypeIdentifier::makeString(true, StringSTypeDefn(200));
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(seq_a)),
                              TypeObject(MinimalTypeObject(seq_b))));
  seq_a.element.common.type = TypeIdentifier::makeString(true, StringSTypeDefn(100));
  seq_b.element.common.type = TypeIdentifier::makeString(true, StringLTypeDefn(200));
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(seq_a)),
                              TypeObject(MinimalTypeObject(seq_b))));

  // Sequence of plain sequence of integer
  seq_a.element.common.type = TypeIdentifier::makePlainSequence(TypeIdentifier(TK_INT32),
                                                                static_cast<SBound>(50));
  seq_b.element.common.type = TypeIdentifier::makePlainSequence(TypeIdentifier(TK_INT32),
                                                                static_cast<LBound>(150));
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(seq_a)),
                              TypeObject(MinimalTypeObject(seq_b))));
  // Sequence of plain array of integer
  SBoundSeq bounds_a;
  bounds_a.append(10).append(20).append(30);
  seq_a.element.common.type = TypeIdentifier::makePlainArray(TypeIdentifier(TK_UINT16), bounds_a);
  LBoundSeq bounds_b;
  bounds_b.append(10).append(20).append(30);
  seq_b.element.common.type = TypeIdentifier::makePlainArray(TypeIdentifier(TK_UINT16), bounds_b);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(seq_a)),
                              TypeObject(MinimalTypeObject(seq_b))));

  // Different element types but T1 is-assignable-from T2 and T2 is delimited
  seq_a.element.common.type = TypeIdentifier(TK_UINT8);
  // Get a fake hash for the type object of a bitmask type
  EquivalenceHash hash;
  TypeLookup::get_equivalence_hash(hash);
  seq_b.element.common.type = TypeIdentifier::make(EK_MINIMAL, hash);
  MinimalBitmaskType bitmask_b;
  bitmask_b.header.common.bit_bound = 8;
  TypeLookup::insert_entry(seq_b.element.common.type, MinimalTypeObject(bitmask_b));
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(seq_a)),
                              TypeObject(MinimalTypeObject(seq_b))));

  seq_a.element.common.type = TypeIdentifier(TK_UINT16);
  TypeLookup::get_equivalence_hash(hash);
  seq_b.element.common.type = TypeIdentifier::make(EK_MINIMAL, hash);
  bitmask_b.header.common.bit_bound = 16;
  TypeLookup::insert_entry(seq_b.element.common.type, MinimalTypeObject(bitmask_b));
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(seq_a)),
                              TypeObject(MinimalTypeObject(seq_b))));

  seq_a.element.common.type = TypeIdentifier(TK_UINT32);
  TypeLookup::get_equivalence_hash(hash);
  seq_b.element.common.type = TypeIdentifier::make(EK_MINIMAL, hash);
  bitmask_b.header.common.bit_bound = 32;
  TypeLookup::insert_entry(seq_b.element.common.type, MinimalTypeObject(bitmask_b));
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(seq_a)),
                              TypeObject(MinimalTypeObject(seq_b))));

  seq_a.element.common.type = TypeIdentifier(TK_UINT64);
  TypeLookup::get_equivalence_hash(hash);
  seq_b.element.common.type = TypeIdentifier::make(EK_MINIMAL, hash);
  bitmask_b.header.common.bit_bound = 64;
  TypeLookup::insert_entry(seq_b.element.common.type, MinimalTypeObject(bitmask_b));
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(seq_a)),
                              TypeObject(MinimalTypeObject(seq_b))));
}

TEST(SequenceTypeTest, NotAssignable)
{
  TypeAssignability test;
  MinimalSequenceType seq_a, seq_b;
  seq_a.header.common.bound = 10;
  seq_b.header.common.bound = 20;

  seq_a.element.common.type = TypeIdentifier(TK_BOOLEAN);
  seq_b.element.common.type = TypeIdentifier(TK_INT32);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(seq_a)),
                               TypeObject(MinimalTypeObject(seq_b))));
  seq_a.element.common.type = TypeIdentifier(TK_BYTE);
  seq_b.element.common.type = TypeIdentifier(TK_BOOLEAN);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(seq_a)),
                               TypeObject(MinimalTypeObject(seq_b))));
  seq_a.element.common.type = TypeIdentifier(TK_INT16);
  seq_b.element.common.type = TypeIdentifier(TK_INT8);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(seq_a)),
                               TypeObject(MinimalTypeObject(seq_b))));
  seq_a.element.common.type = TypeIdentifier(TK_INT32);
  seq_b.element.common.type = TypeIdentifier(TK_INT64);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(seq_a)),
                               TypeObject(MinimalTypeObject(seq_b))));
  seq_a.element.common.type = TypeIdentifier(TK_INT64);
  seq_b.element.common.type = TypeIdentifier(TK_FLOAT32);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(seq_a)),
                               TypeObject(MinimalTypeObject(seq_b))));
  seq_a.element.common.type = TypeIdentifier(TK_UINT16);
  seq_b.element.common.type = TypeIdentifier(TK_BYTE);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(seq_a)),
                               TypeObject(MinimalTypeObject(seq_b))));
  seq_a.element.common.type = TypeIdentifier(TK_UINT32);
  seq_b.element.common.type = TypeIdentifier(TK_INT32);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(seq_a)),
                               TypeObject(MinimalTypeObject(seq_b))));
  seq_a.element.common.type = TypeIdentifier(TK_UINT64);
  seq_b.element.common.type = TypeIdentifier(TK_FLOAT128);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(seq_a)),
                               TypeObject(MinimalTypeObject(seq_b))));
  seq_a.element.common.type = TypeIdentifier(TK_FLOAT32);
  seq_b.element.common.type = TypeIdentifier(TK_INT32);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(seq_a)),
                               TypeObject(MinimalTypeObject(seq_b))));
  seq_a.element.common.type = TypeIdentifier(TK_FLOAT64);
  seq_b.element.common.type = TypeIdentifier(TK_UINT32);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(seq_a)),
                               TypeObject(MinimalTypeObject(seq_b))));
  seq_a.element.common.type = TypeIdentifier(TK_FLOAT128);
  seq_b.element.common.type = TypeIdentifier(TK_BOOLEAN);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(seq_a)),
                               TypeObject(MinimalTypeObject(seq_b))));
  seq_a.element.common.type = TypeIdentifier(TK_INT8);
  seq_b.element.common.type = TypeIdentifier(TK_INT64);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(seq_a)),
                               TypeObject(MinimalTypeObject(seq_b))));
  seq_a.element.common.type = TypeIdentifier(TK_UINT8);
  seq_b.element.common.type = TypeIdentifier(TK_FLOAT64);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(seq_a)),
                               TypeObject(MinimalTypeObject(seq_b))));
  seq_a.element.common.type = TypeIdentifier(TK_CHAR8);
  seq_b.element.common.type = TypeIdentifier(TK_INT8);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(seq_a)),
                               TypeObject(MinimalTypeObject(seq_b))));
  seq_a.element.common.type = TypeIdentifier(TK_CHAR16);
  seq_b.element.common.type = TypeIdentifier(TK_INT64);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(seq_a)),
                               TypeObject(MinimalTypeObject(seq_b))));

  seq_a.element.common.type = TypeIdentifier::makeString(false, StringSTypeDefn(50));
  seq_b.element.common.type = TypeIdentifier::makeString(true, StringLTypeDefn(100));
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(seq_a)),
                               TypeObject(MinimalTypeObject(seq_b))));
  seq_a.element.common.type = TypeIdentifier::makeString(true, StringLTypeDefn(100));
  seq_b.element.common.type = TypeIdentifier::makeString(false, StringSTypeDefn(50));
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(seq_a)),
                               TypeObject(MinimalTypeObject(seq_b))));

  // Sequence of plain sequence of integer
  seq_a.element.common.type = TypeIdentifier::makePlainSequence(TypeIdentifier(TK_UINT32),
                                                                static_cast<SBound>(50));
  seq_b.element.common.type = TypeIdentifier::makePlainSequence(TypeIdentifier(TK_INT64),
                                                                static_cast<LBound>(150));
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(seq_a)),
                               TypeObject(MinimalTypeObject(seq_b))));
  // Sequence of plain array of integer
  SBoundSeq bounds_a;
  bounds_a.append(10).append(20).append(30);
  seq_a.element.common.type = TypeIdentifier::makePlainArray(TypeIdentifier(TK_UINT16), bounds_a);
  LBoundSeq bounds_b;
  bounds_b.append(10).append(20).append(40);
  seq_b.element.common.type = TypeIdentifier::makePlainArray(TypeIdentifier(TK_UINT16), bounds_b);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(seq_a)),
                               TypeObject(MinimalTypeObject(seq_b))));

  seq_a.element.common.type = TypeIdentifier(TK_UINT8);
  // Get a fake hash for the type object of a bitmask type
  EquivalenceHash hash;
  TypeLookup::get_equivalence_hash(hash);
  seq_b.element.common.type = TypeIdentifier::make(EK_MINIMAL, hash);
  MinimalBitmaskType bitmask_b;
  bitmask_b.header.common.bit_bound = 9;
  TypeLookup::insert_entry(seq_b.element.common.type, MinimalTypeObject(bitmask_b));
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(seq_a)),
                               TypeObject(MinimalTypeObject(seq_b))));

  seq_a.element.common.type = TypeIdentifier(TK_UINT16);
  TypeLookup::get_equivalence_hash(hash);
  seq_b.element.common.type = TypeIdentifier::make(EK_MINIMAL, hash);
  bitmask_b.header.common.bit_bound = 17;
  TypeLookup::insert_entry(seq_b.element.common.type, MinimalTypeObject(bitmask_b));
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(seq_a)),
                               TypeObject(MinimalTypeObject(seq_b))));

  seq_a.element.common.type = TypeIdentifier(TK_UINT32);
  TypeLookup::get_equivalence_hash(hash);
  seq_b.element.common.type = TypeIdentifier::make(EK_MINIMAL, hash);
  bitmask_b.header.common.bit_bound = 34;
  TypeLookup::insert_entry(seq_b.element.common.type, MinimalTypeObject(bitmask_b));
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(seq_a)),
                               TypeObject(MinimalTypeObject(seq_b))));

  seq_a.element.common.type = TypeIdentifier(TK_UINT64);
  TypeLookup::get_equivalence_hash(hash);
  seq_b.element.common.type = TypeIdentifier::make(EK_MINIMAL, hash);
  bitmask_b.header.common.bit_bound = 13;
  TypeLookup::insert_entry(seq_b.element.common.type, MinimalTypeObject(bitmask_b));
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(seq_a)),
                               TypeObject(MinimalTypeObject(seq_b))));
}

TEST(ArrayTypeTest, Assignable)
{
  TypeAssignability test;
  MinimalArrayType arr_a, arr_b;
  arr_a.header.common.bound_seq.append(10).append(20).append(30);
  arr_b.header.common.bound_seq.append(10).append(20).append(30);

  arr_a.element.common.type = TypeIdentifier(TK_BOOLEAN);
  arr_b.element.common.type = TypeIdentifier(TK_BOOLEAN);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(arr_a)),
                              TypeObject(MinimalTypeObject(arr_b))));
  arr_a.element.common.type = TypeIdentifier(TK_BYTE);
  arr_b.element.common.type = TypeIdentifier(TK_BYTE);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(arr_a)),
                              TypeObject(MinimalTypeObject(arr_b))));
  arr_a.element.common.type = TypeIdentifier(TK_INT16);
  arr_b.element.common.type = TypeIdentifier(TK_INT16);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(arr_a)),
                              TypeObject(MinimalTypeObject(arr_b))));
  arr_a.element.common.type = TypeIdentifier(TK_INT32);
  arr_b.element.common.type = TypeIdentifier(TK_INT32);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(arr_a)),
                              TypeObject(MinimalTypeObject(arr_b))));
  arr_a.element.common.type = TypeIdentifier(TK_INT64);
  arr_b.element.common.type = TypeIdentifier(TK_INT64);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(arr_a)),
                              TypeObject(MinimalTypeObject(arr_b))));
  arr_a.element.common.type = TypeIdentifier(TK_UINT16);
  arr_b.element.common.type = TypeIdentifier(TK_UINT16);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(arr_a)),
                              TypeObject(MinimalTypeObject(arr_b))));
  arr_a.element.common.type = TypeIdentifier(TK_UINT32);
  arr_b.element.common.type = TypeIdentifier(TK_UINT32);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(arr_a)),
                              TypeObject(MinimalTypeObject(arr_b))));
  arr_a.element.common.type = TypeIdentifier(TK_UINT64);
  arr_b.element.common.type = TypeIdentifier(TK_UINT64);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(arr_a)),
                              TypeObject(MinimalTypeObject(arr_b))));
  arr_a.element.common.type = TypeIdentifier(TK_FLOAT32);
  arr_b.element.common.type = TypeIdentifier(TK_FLOAT32);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(arr_a)),
                              TypeObject(MinimalTypeObject(arr_b))));
  arr_a.element.common.type = TypeIdentifier(TK_FLOAT64);
  arr_b.element.common.type = TypeIdentifier(TK_FLOAT64);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(arr_a)),
                              TypeObject(MinimalTypeObject(arr_b))));
  arr_a.element.common.type = TypeIdentifier(TK_FLOAT128);
  arr_b.element.common.type = TypeIdentifier(TK_FLOAT128);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(arr_a)),
                              TypeObject(MinimalTypeObject(arr_b))));
  arr_a.element.common.type = TypeIdentifier(TK_INT8);
  arr_b.element.common.type = TypeIdentifier(TK_INT8);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(arr_a)),
                              TypeObject(MinimalTypeObject(arr_b))));
  arr_a.element.common.type = TypeIdentifier(TK_UINT8);
  arr_b.element.common.type = TypeIdentifier(TK_UINT8);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(arr_a)),
                              TypeObject(MinimalTypeObject(arr_b))));
  arr_a.element.common.type = TypeIdentifier(TK_CHAR8);
  arr_b.element.common.type = TypeIdentifier(TK_CHAR8);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(arr_a)),
                              TypeObject(MinimalTypeObject(arr_b))));
  arr_a.element.common.type = TypeIdentifier(TK_CHAR16);
  arr_b.element.common.type = TypeIdentifier(TK_CHAR16);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(arr_a)),
                              TypeObject(MinimalTypeObject(arr_b))));

  arr_a.element.common.type = TypeIdentifier::makeString(false, StringSTypeDefn(100));
  arr_b.element.common.type = TypeIdentifier::makeString(false, StringSTypeDefn(200));
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(arr_a)),
                              TypeObject(MinimalTypeObject(arr_b))));
  arr_a.element.common.type = TypeIdentifier::makeString(false, StringSTypeDefn(100));
  arr_b.element.common.type = TypeIdentifier::makeString(false, StringLTypeDefn(200));
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(arr_a)),
                              TypeObject(MinimalTypeObject(arr_b))));
  arr_a.element.common.type = TypeIdentifier::makeString(true, StringSTypeDefn(100));
  arr_b.element.common.type = TypeIdentifier::makeString(true, StringSTypeDefn(200));
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(arr_a)),
                              TypeObject(MinimalTypeObject(arr_b))));
  arr_a.element.common.type = TypeIdentifier::makeString(true, StringSTypeDefn(100));
  arr_b.element.common.type = TypeIdentifier::makeString(true, StringLTypeDefn(200));
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(arr_a)),
                              TypeObject(MinimalTypeObject(arr_b))));

  // Array of plain sequence of integer
  arr_a.element.common.type = TypeIdentifier::makePlainSequence(TypeIdentifier(TK_INT32),
                                                                static_cast<SBound>(50));
  arr_b.element.common.type = TypeIdentifier::makePlainSequence(TypeIdentifier(TK_INT32),
                                                                static_cast<LBound>(150));
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(arr_a)),
                              TypeObject(MinimalTypeObject(arr_b))));
  // Array of plain array of integer
  SBoundSeq bounds_a;
  bounds_a.append(10).append(20).append(30);
  arr_a.element.common.type = TypeIdentifier::makePlainArray(TypeIdentifier(TK_UINT16), bounds_a);
  LBoundSeq bounds_b;
  bounds_b.append(10).append(20).append(30);
  arr_b.element.common.type = TypeIdentifier::makePlainArray(TypeIdentifier(TK_UINT16), bounds_b);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(arr_a)),
                              TypeObject(MinimalTypeObject(arr_b))));

  // Different element types but T1 is-assignable-from T2 and T2 is delimited
  arr_a.element.common.type = TypeIdentifier(TK_UINT8);
  // Get a fake hash for the type object of a bitmask type
  EquivalenceHash hash;
  TypeLookup::get_equivalence_hash(hash);
  arr_b.element.common.type = TypeIdentifier::make(EK_MINIMAL, hash);
  MinimalBitmaskType bitmask_b;
  bitmask_b.header.common.bit_bound = 8;
  TypeLookup::insert_entry(arr_b.element.common.type, MinimalTypeObject(bitmask_b));
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(arr_a)),
                              TypeObject(MinimalTypeObject(arr_b))));

  arr_a.element.common.type = TypeIdentifier(TK_UINT16);
  TypeLookup::get_equivalence_hash(hash);
  arr_b.element.common.type = TypeIdentifier::make(EK_MINIMAL, hash);
  bitmask_b.header.common.bit_bound = 16;
  TypeLookup::insert_entry(arr_b.element.common.type, MinimalTypeObject(bitmask_b));
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(arr_a)),
                              TypeObject(MinimalTypeObject(arr_b))));

  arr_a.element.common.type = TypeIdentifier(TK_UINT32);
  TypeLookup::get_equivalence_hash(hash);
  arr_b.element.common.type = TypeIdentifier::make(EK_MINIMAL, hash);
  bitmask_b.header.common.bit_bound = 32;
  TypeLookup::insert_entry(arr_b.element.common.type, MinimalTypeObject(bitmask_b));
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(arr_a)),
                              TypeObject(MinimalTypeObject(arr_b))));

  arr_a.element.common.type = TypeIdentifier(TK_UINT64);
  TypeLookup::get_equivalence_hash(hash);
  arr_b.element.common.type = TypeIdentifier::make(EK_MINIMAL, hash);
  bitmask_b.header.common.bit_bound = 64;
  TypeLookup::insert_entry(arr_b.element.common.type, MinimalTypeObject(bitmask_b));
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(arr_a)),
                              TypeObject(MinimalTypeObject(arr_b))));
}

TEST(ArrayTypeTest, NotAssignable)
{
  TypeAssignability test;
  MinimalArrayType arr_a, arr_b;
  arr_a.header.common.bound_seq.append(10).append(20).append(30);
  arr_b.header.common.bound_seq.append(10).append(20).append(30);

  arr_a.element.common.type = TypeIdentifier(TK_BOOLEAN);
  arr_b.element.common.type = TypeIdentifier(TK_INT32);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(arr_a)),
                               TypeObject(MinimalTypeObject(arr_b))));
  arr_a.element.common.type = TypeIdentifier(TK_BYTE);
  arr_b.element.common.type = TypeIdentifier(TK_BOOLEAN);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(arr_a)),
                               TypeObject(MinimalTypeObject(arr_b))));
  arr_a.element.common.type = TypeIdentifier(TK_INT16);
  arr_b.element.common.type = TypeIdentifier(TK_INT8);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(arr_a)),
                               TypeObject(MinimalTypeObject(arr_b))));
  arr_a.element.common.type = TypeIdentifier(TK_INT32);
  arr_b.element.common.type = TypeIdentifier(TK_INT64);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(arr_a)),
                               TypeObject(MinimalTypeObject(arr_b))));
  arr_a.element.common.type = TypeIdentifier(TK_INT64);
  arr_b.element.common.type = TypeIdentifier(TK_FLOAT32);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(arr_a)),
                               TypeObject(MinimalTypeObject(arr_b))));
  arr_a.element.common.type = TypeIdentifier(TK_UINT16);
  arr_b.element.common.type = TypeIdentifier(TK_BYTE);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(arr_a)),
                               TypeObject(MinimalTypeObject(arr_b))));
  arr_a.element.common.type = TypeIdentifier(TK_UINT32);
  arr_b.element.common.type = TypeIdentifier(TK_INT32);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(arr_a)),
                               TypeObject(MinimalTypeObject(arr_b))));
  arr_a.element.common.type = TypeIdentifier(TK_UINT64);
  arr_b.element.common.type = TypeIdentifier(TK_FLOAT128);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(arr_a)),
                               TypeObject(MinimalTypeObject(arr_b))));
  arr_a.element.common.type = TypeIdentifier(TK_FLOAT32);
  arr_b.element.common.type = TypeIdentifier(TK_INT32);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(arr_a)),
                               TypeObject(MinimalTypeObject(arr_b))));
  arr_a.element.common.type = TypeIdentifier(TK_FLOAT64);
  arr_b.element.common.type = TypeIdentifier(TK_UINT32);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(arr_a)),
                               TypeObject(MinimalTypeObject(arr_b))));
  arr_a.element.common.type = TypeIdentifier(TK_FLOAT128);
  arr_b.element.common.type = TypeIdentifier(TK_BOOLEAN);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(arr_a)),
                               TypeObject(MinimalTypeObject(arr_b))));
  arr_a.element.common.type = TypeIdentifier(TK_INT8);
  arr_b.element.common.type = TypeIdentifier(TK_INT64);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(arr_a)),
                               TypeObject(MinimalTypeObject(arr_b))));
  arr_a.element.common.type = TypeIdentifier(TK_UINT8);
  arr_b.element.common.type = TypeIdentifier(TK_FLOAT64);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(arr_a)),
                               TypeObject(MinimalTypeObject(arr_b))));
  arr_a.element.common.type = TypeIdentifier(TK_CHAR8);
  arr_b.element.common.type = TypeIdentifier(TK_INT8);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(arr_a)),
                               TypeObject(MinimalTypeObject(arr_b))));
  arr_a.element.common.type = TypeIdentifier(TK_CHAR16);
  arr_b.element.common.type = TypeIdentifier(TK_INT64);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(arr_a)),
                               TypeObject(MinimalTypeObject(arr_b))));

  arr_a.element.common.type = TypeIdentifier::makeString(false, StringSTypeDefn(50));
  arr_b.element.common.type = TypeIdentifier::makeString(true, StringLTypeDefn(100));
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(arr_a)),
                               TypeObject(MinimalTypeObject(arr_b))));
  arr_a.element.common.type = TypeIdentifier::makeString(true, StringLTypeDefn(100));
  arr_b.element.common.type = TypeIdentifier::makeString(false, StringSTypeDefn(50));
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(arr_a)),
                               TypeObject(MinimalTypeObject(arr_b))));

  // Array of plain sequence of integer
  arr_a.element.common.type = TypeIdentifier::makePlainSequence(TypeIdentifier(TK_UINT32),
                                                                static_cast<SBound>(50));
  arr_b.element.common.type = TypeIdentifier::makePlainSequence(TypeIdentifier(TK_INT64),
                                                                static_cast<LBound>(150));
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(arr_a)),
                               TypeObject(MinimalTypeObject(arr_b))));
  // Array of plain array of integer
  SBoundSeq bounds_a;
  bounds_a.append(10).append(20).append(30);
  arr_a.element.common.type = TypeIdentifier::makePlainArray(TypeIdentifier(TK_UINT16), bounds_a);
  LBoundSeq bounds_b;
  bounds_b.append(10).append(20).append(40);
  arr_b.element.common.type = TypeIdentifier::makePlainArray(TypeIdentifier(TK_UINT16), bounds_b);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(arr_a)),
                               TypeObject(MinimalTypeObject(arr_b))));

  arr_a.element.common.type = TypeIdentifier(TK_UINT8);
  // Get a fake hash for the type object of a bitmask type
  EquivalenceHash hash;
  TypeLookup::get_equivalence_hash(hash);
  arr_b.element.common.type = TypeIdentifier::make(EK_MINIMAL, hash);
  MinimalBitmaskType bitmask_b;
  bitmask_b.header.common.bit_bound = 9;
  TypeLookup::insert_entry(arr_b.element.common.type, MinimalTypeObject(bitmask_b));
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(arr_a)),
                               TypeObject(MinimalTypeObject(arr_b))));

  arr_a.element.common.type = TypeIdentifier(TK_UINT16);
  TypeLookup::get_equivalence_hash(hash);
  arr_b.element.common.type = TypeIdentifier::make(EK_MINIMAL, hash);
  bitmask_b.header.common.bit_bound = 17;
  TypeLookup::insert_entry(arr_b.element.common.type, MinimalTypeObject(bitmask_b));
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(arr_a)),
                               TypeObject(MinimalTypeObject(arr_b))));

  arr_a.element.common.type = TypeIdentifier(TK_UINT32);
  TypeLookup::get_equivalence_hash(hash);
  arr_b.element.common.type = TypeIdentifier::make(EK_MINIMAL, hash);
  bitmask_b.header.common.bit_bound = 34;
  TypeLookup::insert_entry(arr_b.element.common.type, MinimalTypeObject(bitmask_b));
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(arr_a)),
                               TypeObject(MinimalTypeObject(arr_b))));

  arr_a.element.common.type = TypeIdentifier(TK_UINT64);
  TypeLookup::get_equivalence_hash(hash);
  arr_b.element.common.type = TypeIdentifier::make(EK_MINIMAL, hash);
  bitmask_b.header.common.bit_bound = 13;
  TypeLookup::insert_entry(arr_b.element.common.type, MinimalTypeObject(bitmask_b));
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(arr_a)),
                               TypeObject(MinimalTypeObject(arr_b))));
}

TEST(MapTypeTest, Assignable)
{
  TypeAssignability test;
  MinimalMapType map_a, map_b;
  map_a.header.common.bound = 50;
  map_b.header.common.bound = 100;

  // Key element can be of signed and unsigned integer types, and
  // of narrow and wide string types
  map_a.key.common.type = TypeIdentifier(TK_UINT32);
  map_b.key.common.type = TypeIdentifier(TK_UINT32);
  map_a.element.common.type = TypeIdentifier(TK_BOOLEAN);
  map_b.element.common.type = TypeIdentifier(TK_BOOLEAN);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(map_a)),
                              TypeObject(MinimalTypeObject(map_b))));
  map_a.key.common.type = TypeIdentifier(TK_UINT16);
  map_b.key.common.type = TypeIdentifier(TK_UINT16);
  map_a.element.common.type = TypeIdentifier(TK_BYTE);
  map_b.element.common.type = TypeIdentifier(TK_BYTE);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(map_a)),
                              TypeObject(MinimalTypeObject(map_b))));
  map_a.key.common.type = TypeIdentifier(TK_UINT32);
  map_b.key.common.type = TypeIdentifier(TK_UINT32);
  map_a.element.common.type = TypeIdentifier(TK_INT16);
  map_b.element.common.type = TypeIdentifier(TK_INT16);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(map_a)),
                              TypeObject(MinimalTypeObject(map_b))));
  map_a.key.common.type = TypeIdentifier(TK_UINT64);
  map_b.key.common.type = TypeIdentifier(TK_UINT64);
  map_a.element.common.type = TypeIdentifier(TK_INT32);
  map_b.element.common.type = TypeIdentifier(TK_INT32);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(map_a)),
                              TypeObject(MinimalTypeObject(map_b))));
  map_a.key.common.type = TypeIdentifier(TK_INT16);
  map_b.key.common.type = TypeIdentifier(TK_INT16);
  map_a.element.common.type = TypeIdentifier(TK_INT64);
  map_b.element.common.type = TypeIdentifier(TK_INT64);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(map_a)),
                              TypeObject(MinimalTypeObject(map_b))));
  map_a.key.common.type = TypeIdentifier(TK_UINT32);
  map_b.key.common.type = TypeIdentifier(TK_UINT32);
  map_a.element.common.type = TypeIdentifier(TK_UINT16);
  map_b.element.common.type = TypeIdentifier(TK_UINT16);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(map_a)),
                              TypeObject(MinimalTypeObject(map_b))));
  map_a.key.common.type = TypeIdentifier(TK_UINT16);
  map_b.key.common.type = TypeIdentifier(TK_UINT16);
  map_a.element.common.type = TypeIdentifier(TK_UINT32);
  map_b.element.common.type = TypeIdentifier(TK_UINT32);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(map_a)),
                              TypeObject(MinimalTypeObject(map_b))));
  map_a.key.common.type = TypeIdentifier(TK_INT32);
  map_b.key.common.type = TypeIdentifier(TK_INT32);
  map_a.element.common.type = TypeIdentifier(TK_UINT64);
  map_b.element.common.type = TypeIdentifier(TK_UINT64);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(map_a)),
                              TypeObject(MinimalTypeObject(map_b))));
  map_a.key.common.type = TypeIdentifier(TK_UINT16);
  map_b.key.common.type = TypeIdentifier(TK_UINT16);
  map_a.element.common.type = TypeIdentifier(TK_FLOAT32);
  map_b.element.common.type = TypeIdentifier(TK_FLOAT32);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(map_a)),
                              TypeObject(MinimalTypeObject(map_b))));
  map_a.key.common.type = TypeIdentifier(TK_INT64);
  map_b.key.common.type = TypeIdentifier(TK_INT64);
  map_a.element.common.type = TypeIdentifier(TK_FLOAT64);
  map_b.element.common.type = TypeIdentifier(TK_FLOAT64);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(map_a)),
                              TypeObject(MinimalTypeObject(map_b))));
  map_a.key.common.type = TypeIdentifier(TK_INT32);
  map_b.key.common.type = TypeIdentifier(TK_INT32);
  map_a.element.common.type = TypeIdentifier(TK_FLOAT128);
  map_b.element.common.type = TypeIdentifier(TK_FLOAT128);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(map_a)),
                              TypeObject(MinimalTypeObject(map_b))));
  map_a.key.common.type = TypeIdentifier(TK_UINT32);
  map_b.key.common.type = TypeIdentifier(TK_UINT32);
  map_a.element.common.type = TypeIdentifier(TK_INT8);
  map_b.element.common.type = TypeIdentifier(TK_INT8);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(map_a)),
                              TypeObject(MinimalTypeObject(map_b))));
  map_a.key.common.type = TypeIdentifier(TK_UINT64);
  map_b.key.common.type = TypeIdentifier(TK_UINT64);
  map_a.element.common.type = TypeIdentifier(TK_UINT8);
  map_b.element.common.type = TypeIdentifier(TK_UINT8);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(map_a)),
                              TypeObject(MinimalTypeObject(map_b))));
  map_a.key.common.type = TypeIdentifier(TK_INT8);
  map_b.key.common.type = TypeIdentifier(TK_INT8);
  map_a.element.common.type = TypeIdentifier(TK_CHAR8);
  map_b.element.common.type = TypeIdentifier(TK_CHAR8);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(map_a)),
                              TypeObject(MinimalTypeObject(map_b))));
  map_a.key.common.type = TypeIdentifier::makeString(false, StringSTypeDefn(50));
  map_b.key.common.type = TypeIdentifier::makeString(false, StringLTypeDefn(70));
  map_a.element.common.type = TypeIdentifier(TK_CHAR16);
  map_b.element.common.type = TypeIdentifier(TK_CHAR16);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(map_a)),
                              TypeObject(MinimalTypeObject(map_b))));

  map_a.key.common.type = TypeIdentifier::makeString(false, StringSTypeDefn(100));
  map_b.key.common.type = TypeIdentifier::makeString(false, StringLTypeDefn(200));
  map_a.element.common.type = TypeIdentifier::makeString(true, StringLTypeDefn(50));
  map_b.element.common.type = TypeIdentifier::makeString(true, StringSTypeDefn(70));
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(map_a)),
                              TypeObject(MinimalTypeObject(map_b))));
  map_a.key.common.type = TypeIdentifier::makeString(true, StringLTypeDefn(123));
  map_b.key.common.type = TypeIdentifier::makeString(true, StringLTypeDefn(321));
  map_a.element.common.type = TypeIdentifier::makeString(false, StringSTypeDefn(150));
  map_b.element.common.type = TypeIdentifier::makeString(false, StringLTypeDefn(80));
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(map_a)),
                              TypeObject(MinimalTypeObject(map_b))));

  // Map of plain sequences
  map_a.key.common.type = TypeIdentifier::makeString(true, StringSTypeDefn(45));
  map_b.key.common.type = TypeIdentifier::makeString(true, StringLTypeDefn(56));
  map_a.element.common.type = TypeIdentifier::makePlainSequence(TypeIdentifier(TK_UINT32),
                                                                static_cast<SBound>(50));
  map_b.element.common.type = TypeIdentifier::makePlainSequence(TypeIdentifier(TK_UINT32),
                                                                static_cast<LBound>(100));
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(map_a)),
                              TypeObject(MinimalTypeObject(map_b))));
  // Map of plain arrays
  map_a.key.common.type = TypeIdentifier::makeString(false, StringSTypeDefn(78));
  map_b.key.common.type = TypeIdentifier::makeString(false, StringLTypeDefn(67));
  SBoundSeq bounds_a;
  bounds_a.append(30).append(20).append(40);
  map_a.element.common.type = TypeIdentifier::makePlainArray(TypeIdentifier::makeString(false, StringSTypeDefn(50)), bounds_a);
  LBoundSeq bounds_b;
  bounds_b.append(30).append(20).append(40);
  map_b.element.common.type = TypeIdentifier::makePlainArray(TypeIdentifier::makeString(false, StringLTypeDefn(100)), bounds_b);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(map_a)),
                              TypeObject(MinimalTypeObject(map_b))));
  // Map of plain maps
  map_a.key.common.type = TypeIdentifier(TK_UINT32);
  map_b.key.common.type = TypeIdentifier(TK_UINT32);
  PlainMapSTypeDefn elem_a(PlainCollectionHeader(EquivalenceKind(EK_MINIMAL), CollectionElementFlag()),
                           static_cast<SBound>(50),
                           TypeIdentifier(TK_FLOAT64),
                           CollectionElementFlag(),
                           TypeIdentifier(TK_UINT16));
  map_a.element.common.type = TypeIdentifier::make(TI_PLAIN_MAP_SMALL, elem_a);
  PlainMapLTypeDefn elem_b(PlainCollectionHeader(EquivalenceKind(EK_MINIMAL), CollectionElementFlag()),
                           static_cast<LBound>(100),
                           TypeIdentifier(TK_FLOAT64),
                           CollectionElementFlag(),
                           TypeIdentifier(TK_UINT16));
  map_b.element.common.type = TypeIdentifier::make(TI_PLAIN_MAP_LARGE, elem_b);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(map_a)),
                              TypeObject(MinimalTypeObject(map_b))));

  map_a.key.common.type = TypeIdentifier(TK_INT32);
  map_b.key.common.type = TypeIdentifier(TK_INT32);
  map_a.element.common.type = TypeIdentifier(TK_UINT8);
  EquivalenceHash hash;
  TypeLookup::get_equivalence_hash(hash);
  map_b.element.common.type = TypeIdentifier::make(EK_MINIMAL, hash);
  MinimalBitmaskType bitmask_b;
  bitmask_b.header.common.bit_bound = 7;
  TypeLookup::insert_entry(map_b.element.common.type, MinimalTypeObject(bitmask_b));
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(map_a)),
                              TypeObject(MinimalTypeObject(map_b))));

  map_a.key.common.type = TypeIdentifier(TK_INT16);
  map_b.key.common.type = TypeIdentifier(TK_INT16);
  map_a.element.common.type = TypeIdentifier(TK_UINT16);
  TypeLookup::get_equivalence_hash(hash);
  map_b.element.common.type = TypeIdentifier::make(EK_MINIMAL, hash);
  bitmask_b.header.common.bit_bound = 15;
  TypeLookup::insert_entry(map_b.element.common.type, MinimalTypeObject(bitmask_b));
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(map_a)),
                              TypeObject(MinimalTypeObject(map_b))));

  map_a.key.common.type = TypeIdentifier(TK_INT64);
  map_b.key.common.type = TypeIdentifier(TK_INT64);
  map_a.element.common.type = TypeIdentifier(TK_UINT32);
  TypeLookup::get_equivalence_hash(hash);
  map_b.element.common.type = TypeIdentifier::make(EK_MINIMAL, hash);
  bitmask_b.header.common.bit_bound = 31;
  TypeLookup::insert_entry(map_b.element.common.type, MinimalTypeObject(bitmask_b));
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(map_a)),
                              TypeObject(MinimalTypeObject(map_b))));

  map_a.key.common.type = TypeIdentifier::makeString(false, StringSTypeDefn(60));
  map_b.key.common.type = TypeIdentifier::makeString(false, StringSTypeDefn(80));
  map_a.element.common.type = TypeIdentifier(TK_UINT64);
  TypeLookup::get_equivalence_hash(hash);
  map_b.element.common.type = TypeIdentifier::make(EK_MINIMAL, hash);
  bitmask_b.header.common.bit_bound = 63;
  TypeLookup::insert_entry(map_b.element.common.type, MinimalTypeObject(bitmask_b));
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(map_a)),
                              TypeObject(MinimalTypeObject(map_b))));
}

TEST(MapTypeTest, NotAssignable)
{
  TypeAssignability test;
  MinimalMapType map_a, map_b;
  map_a.header.common.bound = 50;
  map_b.header.common.bound = 100;

  map_a.key.common.type = TypeIdentifier(TK_UINT32);
  map_b.key.common.type = TypeIdentifier(TK_UINT32);
  map_a.element.common.type = TypeIdentifier(TK_BOOLEAN);
  map_b.element.common.type = TypeIdentifier(TK_INT16);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(map_a)),
                               TypeObject(MinimalTypeObject(map_b))));
  map_a.key.common.type = TypeIdentifier(TK_UINT32);
  map_b.key.common.type = TypeIdentifier(TK_UINT16);
  map_a.element.common.type = TypeIdentifier(TK_BYTE);
  map_b.element.common.type = TypeIdentifier(TK_BYTE);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(map_a)),
                               TypeObject(MinimalTypeObject(map_b))));
  map_a.key.common.type = TypeIdentifier(TK_INT32);
  map_b.key.common.type = TypeIdentifier(TK_UINT32);
  map_a.element.common.type = TypeIdentifier(TK_INT16);
  map_b.element.common.type = TypeIdentifier(TK_UINT16);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(map_a)),
                               TypeObject(MinimalTypeObject(map_b))));
  map_a.key.common.type = TypeIdentifier(TK_UINT64);
  map_b.key.common.type = TypeIdentifier(TK_UINT64);
  map_a.element.common.type = TypeIdentifier(TK_INT32);
  map_b.element.common.type = TypeIdentifier(TK_INT64);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(map_a)),
                               TypeObject(MinimalTypeObject(map_b))));
  map_a.key.common.type = TypeIdentifier(TK_INT16);
  map_b.key.common.type = TypeIdentifier(TK_INT16);
  map_a.element.common.type = TypeIdentifier(TK_INT64);
  map_b.element.common.type = TypeIdentifier(TK_FLOAT32);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(map_a)),
                               TypeObject(MinimalTypeObject(map_b))));
  map_a.key.common.type = TypeIdentifier(TK_UINT32);
  map_b.key.common.type = TypeIdentifier(TK_UINT32);
  map_a.element.common.type = TypeIdentifier(TK_UINT16);
  map_b.element.common.type = TypeIdentifier(TK_UINT32);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(map_a)),
                               TypeObject(MinimalTypeObject(map_b))));
  map_a.key.common.type = TypeIdentifier(TK_UINT16);
  map_b.key.common.type = TypeIdentifier(TK_UINT16);
  map_a.element.common.type = TypeIdentifier(TK_UINT32);
  map_b.element.common.type = TypeIdentifier(TK_BYTE);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(map_a)),
                               TypeObject(MinimalTypeObject(map_b))));
  map_a.key.common.type = TypeIdentifier(TK_INT32);
  map_b.key.common.type = TypeIdentifier(TK_INT32);
  map_a.element.common.type = TypeIdentifier(TK_UINT64);
  map_b.element.common.type = TypeIdentifier(TK_FLOAT64);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(map_a)),
                               TypeObject(MinimalTypeObject(map_b))));
  map_a.key.common.type = TypeIdentifier(TK_UINT16);
  map_b.key.common.type = TypeIdentifier(TK_UINT16);
  map_a.element.common.type = TypeIdentifier(TK_FLOAT32);
  map_b.element.common.type = TypeIdentifier(TK_INT32);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(map_a)),
                               TypeObject(MinimalTypeObject(map_b))));
  map_a.key.common.type = TypeIdentifier(TK_INT64);
  map_b.key.common.type = TypeIdentifier(TK_INT64);
  map_a.element.common.type = TypeIdentifier(TK_FLOAT64);
  map_b.element.common.type = TypeIdentifier(TK_INT8);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(map_a)),
                               TypeObject(MinimalTypeObject(map_b))));
  map_a.key.common.type = TypeIdentifier(TK_INT32);
  map_b.key.common.type = TypeIdentifier(TK_INT32);
  map_a.element.common.type = TypeIdentifier(TK_FLOAT128);
  map_b.element.common.type = TypeIdentifier(TK_FLOAT32);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(map_a)),
                               TypeObject(MinimalTypeObject(map_b))));
  map_a.key.common.type = TypeIdentifier(TK_UINT32);
  map_b.key.common.type = TypeIdentifier(TK_UINT32);
  map_a.element.common.type = TypeIdentifier(TK_INT8);
  map_b.element.common.type = TypeIdentifier(TK_INT32);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(map_a)),
                               TypeObject(MinimalTypeObject(map_b))));
  map_a.key.common.type = TypeIdentifier(TK_UINT64);
  map_b.key.common.type = TypeIdentifier(TK_UINT64);
  map_a.element.common.type = TypeIdentifier(TK_UINT8);
  map_b.element.common.type = TypeIdentifier(TK_INT64);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(map_a)),
                               TypeObject(MinimalTypeObject(map_b))));
  map_a.key.common.type = TypeIdentifier(TK_INT8);
  map_b.key.common.type = TypeIdentifier(TK_INT8);
  map_a.element.common.type = TypeIdentifier(TK_CHAR8);
  map_b.element.common.type = TypeIdentifier(TK_CHAR16);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(map_a)),
                               TypeObject(MinimalTypeObject(map_b))));
  map_a.key.common.type = TypeIdentifier::makeString(false, StringSTypeDefn(50));
  map_b.key.common.type = TypeIdentifier::makeString(false, StringLTypeDefn(70));
  map_a.element.common.type = TypeIdentifier(TK_CHAR16);
  map_b.element.common.type = TypeIdentifier::makeString(true, StringSTypeDefn(50));
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(map_a)),
                               TypeObject(MinimalTypeObject(map_b))));

  map_a.key.common.type = TypeIdentifier::makeString(false, StringSTypeDefn(100));
  map_b.key.common.type = TypeIdentifier::makeString(true, StringLTypeDefn(200));
  map_a.element.common.type = TypeIdentifier::makeString(true, StringLTypeDefn(50));
  map_b.element.common.type = TypeIdentifier::makeString(true, StringSTypeDefn(70));
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(map_a)),
                               TypeObject(MinimalTypeObject(map_b))));
  map_a.key.common.type = TypeIdentifier::makeString(true, StringLTypeDefn(123));
  map_b.key.common.type = TypeIdentifier::makeString(true, StringLTypeDefn(321));
  map_a.element.common.type = TypeIdentifier::makeString(true, StringSTypeDefn(150));
  map_b.element.common.type = TypeIdentifier::makeString(false, StringLTypeDefn(80));
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(map_a)),
                               TypeObject(MinimalTypeObject(map_b))));

  // Map of plain sequences
  map_a.key.common.type = TypeIdentifier::makeString(true, StringSTypeDefn(45));
  map_b.key.common.type = TypeIdentifier::makeString(false, StringLTypeDefn(56));
  map_a.element.common.type = TypeIdentifier::makePlainSequence(TypeIdentifier(TK_UINT32),
                                                                static_cast<SBound>(50));
  map_b.element.common.type = TypeIdentifier::makePlainSequence(TypeIdentifier(TK_INT64),
                                                                static_cast<LBound>(100));
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(map_a)),
                               TypeObject(MinimalTypeObject(map_b))));
  // Map of plain arrays
  map_a.key.common.type = TypeIdentifier::makeString(false, StringSTypeDefn(78));
  map_b.key.common.type = TypeIdentifier::makeString(false, StringLTypeDefn(67));
  SBoundSeq bounds_a;
  bounds_a.append(30).append(20).append(40);
  map_a.element.common.type = TypeIdentifier::makePlainArray(TypeIdentifier::makeString(true, StringSTypeDefn(50)), bounds_a);
  LBoundSeq bounds_b;
  bounds_b.append(30).append(20).append(40);
  map_b.element.common.type = TypeIdentifier::makePlainArray(TypeIdentifier::makeString(false, StringLTypeDefn(100)), bounds_b);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(map_a)),
                               TypeObject(MinimalTypeObject(map_b))));
  // Map of plain maps
  map_a.key.common.type = TypeIdentifier(TK_UINT32);
  map_b.key.common.type = TypeIdentifier(TK_UINT32);
  PlainMapSTypeDefn elem_a(PlainCollectionHeader(EquivalenceKind(EK_MINIMAL), CollectionElementFlag()),
                           static_cast<SBound>(50),
                           TypeIdentifier(TK_FLOAT64),
                           CollectionElementFlag(),
                           TypeIdentifier(TK_UINT16));
  map_a.element.common.type = TypeIdentifier::make(TI_PLAIN_MAP_SMALL, elem_a);
  PlainMapLTypeDefn elem_b(PlainCollectionHeader(EquivalenceKind(EK_MINIMAL), CollectionElementFlag()),
                           static_cast<LBound>(100),
                           TypeIdentifier(TK_FLOAT64),
                           CollectionElementFlag(),
                           TypeIdentifier(TK_UINT32));
  map_b.element.common.type = TypeIdentifier::make(TI_PLAIN_MAP_LARGE, elem_b);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(map_a)),
                               TypeObject(MinimalTypeObject(map_b))));

  map_a.key.common.type = TypeIdentifier(TK_INT32);
  map_b.key.common.type = TypeIdentifier(TK_INT32);
  map_a.element.common.type = TypeIdentifier(TK_UINT8);
  EquivalenceHash hash;
  TypeLookup::get_equivalence_hash(hash);
  map_b.element.common.type = TypeIdentifier::make(EK_MINIMAL, hash);
  MinimalBitmaskType bitmask_b;
  bitmask_b.header.common.bit_bound = 9;
  TypeLookup::insert_entry(map_b.element.common.type, MinimalTypeObject(bitmask_b));
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(map_a)),
                               TypeObject(MinimalTypeObject(map_b))));

  map_a.key.common.type = TypeIdentifier(TK_INT16);
  map_b.key.common.type = TypeIdentifier(TK_INT16);
  map_a.element.common.type = TypeIdentifier(TK_UINT16);
  TypeLookup::get_equivalence_hash(hash);
  map_b.element.common.type = TypeIdentifier::make(EK_MINIMAL, hash);
  bitmask_b.header.common.bit_bound = 17;
  TypeLookup::insert_entry(map_b.element.common.type, MinimalTypeObject(bitmask_b));
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(map_a)),
                               TypeObject(MinimalTypeObject(map_b))));

  map_a.key.common.type = TypeIdentifier(TK_INT64);
  map_b.key.common.type = TypeIdentifier(TK_INT64);
  map_a.element.common.type = TypeIdentifier(TK_UINT32);
  TypeLookup::get_equivalence_hash(hash);
  map_b.element.common.type = TypeIdentifier::make(EK_MINIMAL, hash);
  bitmask_b.header.common.bit_bound = 33;
  TypeLookup::insert_entry(map_b.element.common.type, MinimalTypeObject(bitmask_b));
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(map_a)),
                               TypeObject(MinimalTypeObject(map_b))));

  map_a.key.common.type = TypeIdentifier::makeString(false, StringSTypeDefn(60));
  map_b.key.common.type = TypeIdentifier::makeString(false, StringSTypeDefn(80));
  map_a.element.common.type = TypeIdentifier(TK_UINT64);
  TypeLookup::get_equivalence_hash(hash);
  map_b.element.common.type = TypeIdentifier::make(EK_MINIMAL, hash);
  bitmask_b.header.common.bit_bound = 15;
  TypeLookup::insert_entry(map_b.element.common.type, MinimalTypeObject(bitmask_b));
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(map_a)),
                               TypeObject(MinimalTypeObject(map_b))));
}

void expect_true_non_alias_to_alias()
{
  TypeAssignability test;
  MinimalAliasType ali_a;

  // Primitive types
  ali_a.body.common.related_type = TypeIdentifier(TK_BOOLEAN);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(ali_a)), TypeIdentifier(TK_BOOLEAN)));
  ali_a.body.common.related_type = TypeIdentifier(TK_BYTE);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(ali_a)), TypeIdentifier(TK_BYTE)));
  ali_a.body.common.related_type = TypeIdentifier(TK_INT16);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(ali_a)), TypeIdentifier(TK_INT16)));
  ali_a.body.common.related_type = TypeIdentifier(TK_INT32);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(ali_a)), TypeIdentifier(TK_INT32)));
  ali_a.body.common.related_type = TypeIdentifier(TK_INT64);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(ali_a)), TypeIdentifier(TK_INT64)));
  ali_a.body.common.related_type = TypeIdentifier(TK_UINT16);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(ali_a)), TypeIdentifier(TK_UINT16)));
  ali_a.body.common.related_type = TypeIdentifier(TK_UINT32);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(ali_a)), TypeIdentifier(TK_UINT32)));
  ali_a.body.common.related_type = TypeIdentifier(TK_UINT64);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(ali_a)), TypeIdentifier(TK_UINT64)));
  ali_a.body.common.related_type = TypeIdentifier(TK_FLOAT32);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(ali_a)), TypeIdentifier(TK_FLOAT32)));
  ali_a.body.common.related_type = TypeIdentifier(TK_FLOAT64);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(ali_a)), TypeIdentifier(TK_FLOAT64)));
  ali_a.body.common.related_type = TypeIdentifier(TK_FLOAT128);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(ali_a)), TypeIdentifier(TK_FLOAT128)));
  ali_a.body.common.related_type = TypeIdentifier(TK_INT8);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(ali_a)), TypeIdentifier(TK_INT8)));
  ali_a.body.common.related_type = TypeIdentifier(TK_UINT8);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(ali_a)), TypeIdentifier(TK_UINT8)));
  ali_a.body.common.related_type = TypeIdentifier(TK_CHAR8);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(ali_a)), TypeIdentifier(TK_CHAR8)));
  ali_a.body.common.related_type = TypeIdentifier(TK_CHAR16);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(ali_a)), TypeIdentifier(TK_CHAR16)));

  // String
  ali_a.body.common.related_type = TypeIdentifier::makeString(false, StringSTypeDefn(70));
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(ali_a)),
                              TypeIdentifier::makeString(false, StringLTypeDefn(120))));
  ali_a.body.common.related_type = TypeIdentifier::makeString(true, StringSTypeDefn(70));
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(ali_a)),
                              TypeIdentifier::makeString(true, StringLTypeDefn(120))));

  // Sequence
  ali_a.body.common.related_type = TypeIdentifier::makePlainSequence(TypeIdentifier(TK_UINT32),
                                                                     static_cast<SBound>(100));
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(ali_a)),
                              TypeIdentifier::makePlainSequence(TypeIdentifier(TK_UINT32),
                                                                 static_cast<LBound>(200))));
  MinimalSequenceType seq_b;
  seq_b.header.common.bound = 300;
  seq_b.element.common.type = TypeIdentifier(TK_UINT32);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(ali_a)), TypeObject(MinimalTypeObject(seq_b))));

  // Array
  SBoundSeq bounds_a;
  bounds_a.append(50).append(60).append(70);
  ali_a.body.common.related_type = TypeIdentifier::makePlainArray(TypeIdentifier(TK_FLOAT32), bounds_a);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(ali_a)),
                              TypeIdentifier::makePlainArray(TypeIdentifier(TK_FLOAT32), bounds_a)));
  MinimalArrayType arr_b;
  arr_b.header.common.bound_seq.append(50).append(60).append(70);
  arr_b.element.common.type = TypeIdentifier(TK_FLOAT32);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(ali_a)), TypeObject(MinimalTypeObject(arr_b))));

  // Map
  PlainMapLTypeDefn plain_map_a(PlainCollectionHeader(EK_MINIMAL, CollectionElementFlag()),
                                static_cast<LBound>(111),
                                TypeIdentifier(TK_INT64),
                                CollectionElementFlag(),
                                TypeIdentifier(TK_UINT32));
  ali_a.body.common.related_type = TypeIdentifier::make(TI_PLAIN_MAP_LARGE, plain_map_a);
  PlainMapSTypeDefn plain_map_b(PlainCollectionHeader(EK_MINIMAL, CollectionElementFlag()),
                                static_cast<SBound>(200),
                                TypeIdentifier(TK_INT64),
                                CollectionElementFlag(),
                                TypeIdentifier(TK_UINT32));
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(ali_a)),
                              TypeIdentifier::make(TI_PLAIN_MAP_SMALL, plain_map_b)));
  MinimalMapType map_b;
  map_b.header.common.bound = 500;
  map_b.key.common.type = TypeIdentifier(TK_UINT32);
  map_b.element.common.type = TypeIdentifier(TK_INT64);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(ali_a)), TypeObject(MinimalTypeObject(map_b))));

  // Enumeration
  MinimalEnumeratedLiteralSeq literal_seq;
  literal_seq.append(MinimalEnumeratedLiteral(CommonEnumeratedLiteral(1, EnumeratedLiteralFlag(IS_DEFAULT)),
                                              MinimalMemberDetail("LITERAL1")));
  literal_seq.append(MinimalEnumeratedLiteral(CommonEnumeratedLiteral(2, EnumeratedLiteralFlag()),
                                              MinimalMemberDetail("LITERAL2")));
  literal_seq.append(MinimalEnumeratedLiteral(CommonEnumeratedLiteral(3, EnumeratedLiteralFlag()),
                                              MinimalMemberDetail("LITERAL3")));
  literal_seq.append(MinimalEnumeratedLiteral(CommonEnumeratedLiteral(4, EnumeratedLiteralFlag()),
                                              MinimalMemberDetail("LITERAL4")));
  MinimalEnumeratedType enum_a(EnumTypeFlag(),
                               MinimalEnumeratedHeader(CommonEnumeratedHeader(static_cast<BitBound>(4))),
                               literal_seq);
  EquivalenceHash hash;
  TypeLookup::get_equivalence_hash(hash);
  ali_a.body.common.related_type = TypeIdentifier::make(EK_MINIMAL, hash);
  TypeLookup::insert_entry(ali_a.body.common.related_type, MinimalTypeObject(enum_a));
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(ali_a)), TypeObject(MinimalTypeObject(enum_a))));

  // Bitmask
  MinimalBitmaskType bitmask_a;
  bitmask_a.header.common.bit_bound = 4;
  MinimalBitflag flag;
  flag.common.position = 0;
  flag.detail = MinimalMemberDetail("BIT1");
  bitmask_a.flag_seq.append(flag);
  flag.common.position = 1;
  flag.detail = MinimalMemberDetail("BIT2");
  bitmask_a.flag_seq.append(flag);
  flag.common.position = 2;
  flag.detail = MinimalMemberDetail("BIT3");
  bitmask_a.flag_seq.append(flag);
  flag.common.position = 3;
  flag.detail = MinimalMemberDetail("BIT4");
  bitmask_a.flag_seq.append(flag);
  TypeLookup::get_equivalence_hash(hash);
  ali_a.body.common.related_type = TypeIdentifier::make(EK_MINIMAL, hash);
  TypeLookup::insert_entry(ali_a.body.common.related_type, MinimalTypeObject(bitmask_a));
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(ali_a)), TypeObject(MinimalTypeObject(bitmask_a))));
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(ali_a)), TypeIdentifier(TK_UINT8)));

  bitmask_a.header.common.bit_bound = 14;
  TypeLookup::get_equivalence_hash(hash);
  ali_a.body.common.related_type = TypeIdentifier::make(EK_MINIMAL, hash);
  TypeLookup::insert_entry(ali_a.body.common.related_type, MinimalTypeObject(bitmask_a));
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(ali_a)), TypeIdentifier(TK_UINT16)));

  bitmask_a.header.common.bit_bound = 31;
  TypeLookup::get_equivalence_hash(hash);
  ali_a.body.common.related_type = TypeIdentifier::make(EK_MINIMAL, hash);
  TypeLookup::insert_entry(ali_a.body.common.related_type, MinimalTypeObject(bitmask_a));
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(ali_a)), TypeIdentifier(TK_UINT32)));

  bitmask_a.header.common.bit_bound = 60;
  TypeLookup::get_equivalence_hash(hash);
  ali_a.body.common.related_type = TypeIdentifier::make(EK_MINIMAL, hash);
  TypeLookup::insert_entry(ali_a.body.common.related_type, MinimalTypeObject(bitmask_a));
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(ali_a)), TypeIdentifier(TK_UINT64)));
}

void expect_true_alias_to_non_alias()
{
  TypeAssignability test;
  MinimalAliasType ali_a;

  // Primitive types
  ali_a.body.common.related_type = TypeIdentifier(TK_BOOLEAN);
  EXPECT_TRUE(test.assignable(TypeIdentifier(TK_BOOLEAN), TypeObject(MinimalTypeObject(ali_a))));
  ali_a.body.common.related_type = TypeIdentifier(TK_BYTE);
  EXPECT_TRUE(test.assignable(TypeIdentifier(TK_BYTE), TypeObject(MinimalTypeObject(ali_a))));
  ali_a.body.common.related_type = TypeIdentifier(TK_INT16);
  EXPECT_TRUE(test.assignable(TypeIdentifier(TK_INT16), TypeObject(MinimalTypeObject(ali_a))));
  ali_a.body.common.related_type = TypeIdentifier(TK_INT32);
  EXPECT_TRUE(test.assignable(TypeIdentifier(TK_INT32), TypeObject(MinimalTypeObject(ali_a))));
  ali_a.body.common.related_type = TypeIdentifier(TK_INT64);
  EXPECT_TRUE(test.assignable(TypeIdentifier(TK_INT64), TypeObject(MinimalTypeObject(ali_a))));
  ali_a.body.common.related_type = TypeIdentifier(TK_UINT16);
  EXPECT_TRUE(test.assignable(TypeIdentifier(TK_UINT16), TypeObject(MinimalTypeObject(ali_a))));
  ali_a.body.common.related_type = TypeIdentifier(TK_UINT32);
  EXPECT_TRUE(test.assignable(TypeIdentifier(TK_UINT32), TypeObject(MinimalTypeObject(ali_a))));
  ali_a.body.common.related_type = TypeIdentifier(TK_UINT64);
  EXPECT_TRUE(test.assignable(TypeIdentifier(TK_UINT64), TypeObject(MinimalTypeObject(ali_a))));
  ali_a.body.common.related_type = TypeIdentifier(TK_FLOAT32);
  EXPECT_TRUE(test.assignable(TypeIdentifier(TK_FLOAT32), TypeObject(MinimalTypeObject(ali_a))));
  ali_a.body.common.related_type = TypeIdentifier(TK_FLOAT64);
  EXPECT_TRUE(test.assignable(TypeIdentifier(TK_FLOAT64), TypeObject(MinimalTypeObject(ali_a))));
  ali_a.body.common.related_type = TypeIdentifier(TK_FLOAT128);
  EXPECT_TRUE(test.assignable(TypeIdentifier(TK_FLOAT128), TypeObject(MinimalTypeObject(ali_a))));
  ali_a.body.common.related_type = TypeIdentifier(TK_INT8);
  EXPECT_TRUE(test.assignable(TypeIdentifier(TK_INT8), TypeObject(MinimalTypeObject(ali_a))));
  ali_a.body.common.related_type = TypeIdentifier(TK_UINT8);
  EXPECT_TRUE(test.assignable(TypeIdentifier(TK_UINT8), TypeObject(MinimalTypeObject(ali_a))));
  ali_a.body.common.related_type = TypeIdentifier(TK_CHAR8);
  EXPECT_TRUE(test.assignable(TypeIdentifier(TK_CHAR8), TypeObject(MinimalTypeObject(ali_a))));
  ali_a.body.common.related_type = TypeIdentifier(TK_CHAR16);
  EXPECT_TRUE(test.assignable(TypeIdentifier(TK_CHAR16), TypeObject(MinimalTypeObject(ali_a))));

  // String
  ali_a.body.common.related_type = TypeIdentifier::makeString(false, StringSTypeDefn(70));
  EXPECT_TRUE(test.assignable(TypeIdentifier::makeString(false, StringLTypeDefn(130)),
                              TypeObject(MinimalTypeObject(ali_a))));
  ali_a.body.common.related_type = TypeIdentifier::makeString(true, StringSTypeDefn(70));
  EXPECT_TRUE(test.assignable(TypeIdentifier::makeString(true, StringLTypeDefn(130)),
                              TypeObject(MinimalTypeObject(ali_a))));

  // Sequence
  ali_a.body.common.related_type = TypeIdentifier::makePlainSequence(TypeIdentifier(TK_UINT32),
                                                                     static_cast<SBound>(100));
  EXPECT_TRUE(test.assignable(TypeIdentifier::makePlainSequence(TypeIdentifier(TK_UINT32),
                                                                 static_cast<LBound>(200)),
                              TypeObject(MinimalTypeObject(ali_a))));
  MinimalSequenceType seq_b;
  seq_b.header.common.bound = 300;
  seq_b.element.common.type = TypeIdentifier(TK_UINT32);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(seq_b)), TypeObject(MinimalTypeObject(ali_a))));

  // Array
  SBoundSeq bounds_a;
  bounds_a.append(50).append(60).append(70);
  ali_a.body.common.related_type = TypeIdentifier::makePlainArray(TypeIdentifier(TK_FLOAT32), bounds_a);
  EXPECT_TRUE(test.assignable(TypeIdentifier::makePlainArray(TypeIdentifier(TK_FLOAT32), bounds_a),
                              TypeObject(MinimalTypeObject(ali_a))));
  MinimalArrayType arr_b;
  arr_b.header.common.bound_seq.append(50).append(60).append(70);
  arr_b.element.common.type = TypeIdentifier(TK_FLOAT32);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(arr_b)), TypeObject(MinimalTypeObject(ali_a))));

  // Map
  PlainMapLTypeDefn plain_map_a(PlainCollectionHeader(EK_MINIMAL, CollectionElementFlag()),
                                static_cast<LBound>(111),
                                TypeIdentifier(TK_INT64),
                                CollectionElementFlag(),
                                TypeIdentifier(TK_UINT32));
  ali_a.body.common.related_type = TypeIdentifier::make(TI_PLAIN_MAP_LARGE, plain_map_a);
  PlainMapSTypeDefn plain_map_b(PlainCollectionHeader(EK_MINIMAL, CollectionElementFlag()),
                                static_cast<SBound>(200),
                                TypeIdentifier(TK_INT64),
                                CollectionElementFlag(),
                                TypeIdentifier(TK_UINT32));
  EXPECT_TRUE(test.assignable(TypeIdentifier::make(TI_PLAIN_MAP_SMALL, plain_map_b),
                              TypeObject(MinimalTypeObject(ali_a))));
  MinimalMapType map_b;
  map_b.header.common.bound = 500;
  map_b.key.common.type = TypeIdentifier(TK_UINT32);
  map_b.element.common.type = TypeIdentifier(TK_INT64);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(map_b)), TypeObject(MinimalTypeObject(ali_a))));

  // Enumeration
  MinimalEnumeratedLiteralSeq literal_seq;
  literal_seq.append(MinimalEnumeratedLiteral(CommonEnumeratedLiteral(1, EnumeratedLiteralFlag(IS_DEFAULT)),
                                              MinimalMemberDetail("LITERAL1")));
  literal_seq.append(MinimalEnumeratedLiteral(CommonEnumeratedLiteral(2, EnumeratedLiteralFlag()),
                                              MinimalMemberDetail("LITERAL2")));
  literal_seq.append(MinimalEnumeratedLiteral(CommonEnumeratedLiteral(3, EnumeratedLiteralFlag()),
                                              MinimalMemberDetail("LITERAL3")));
  literal_seq.append(MinimalEnumeratedLiteral(CommonEnumeratedLiteral(4, EnumeratedLiteralFlag()),
                                              MinimalMemberDetail("LITERAL4")));
  MinimalEnumeratedType enum_a(EnumTypeFlag(),
                               MinimalEnumeratedHeader(CommonEnumeratedHeader(static_cast<BitBound>(4))),
                               literal_seq);
  EquivalenceHash hash;
  TypeLookup::get_equivalence_hash(hash);
  ali_a.body.common.related_type = TypeIdentifier::make(EK_MINIMAL, hash);
  TypeLookup::insert_entry(ali_a.body.common.related_type, MinimalTypeObject(enum_a));
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(enum_a)), TypeObject(MinimalTypeObject(ali_a))));

  // Bitmask
  MinimalBitmaskType bitmask_a;
  bitmask_a.header.common.bit_bound = 4;
  MinimalBitflag flag;
  flag.common.position = 0;
  flag.detail = MinimalMemberDetail("BIT1");
  bitmask_a.flag_seq.append(flag);
  flag.common.position = 1;
  flag.detail = MinimalMemberDetail("BIT2");
  bitmask_a.flag_seq.append(flag);
  flag.common.position = 2;
  flag.detail = MinimalMemberDetail("BIT3");
  bitmask_a.flag_seq.append(flag);
  flag.common.position = 3;
  flag.detail = MinimalMemberDetail("BIT4");
  bitmask_a.flag_seq.append(flag);
  TypeLookup::get_equivalence_hash(hash);
  ali_a.body.common.related_type = TypeIdentifier::make(EK_MINIMAL, hash);
  TypeLookup::insert_entry(ali_a.body.common.related_type, MinimalTypeObject(bitmask_a));
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(bitmask_a)), TypeObject(MinimalTypeObject(ali_a))));
  EXPECT_TRUE(test.assignable(TypeIdentifier(TK_UINT8), TypeObject(MinimalTypeObject(ali_a))));

  bitmask_a.header.common.bit_bound = 14;
  TypeLookup::get_equivalence_hash(hash);
  ali_a.body.common.related_type = TypeIdentifier::make(EK_MINIMAL, hash);
  TypeLookup::insert_entry(ali_a.body.common.related_type, MinimalTypeObject(bitmask_a));
  EXPECT_TRUE(test.assignable(TypeIdentifier(TK_UINT16), TypeObject(MinimalTypeObject(ali_a))));

  bitmask_a.header.common.bit_bound = 31;
  TypeLookup::get_equivalence_hash(hash);
  ali_a.body.common.related_type = TypeIdentifier::make(EK_MINIMAL, hash);
  TypeLookup::insert_entry(ali_a.body.common.related_type, MinimalTypeObject(bitmask_a));
  EXPECT_TRUE(test.assignable(TypeIdentifier(TK_UINT32), TypeObject(MinimalTypeObject(ali_a))));

  bitmask_a.header.common.bit_bound = 60;
  TypeLookup::get_equivalence_hash(hash);
  ali_a.body.common.related_type = TypeIdentifier::make(EK_MINIMAL, hash);
  TypeLookup::insert_entry(ali_a.body.common.related_type, MinimalTypeObject(bitmask_a));
  EXPECT_TRUE(test.assignable(TypeIdentifier(TK_UINT64), TypeObject(MinimalTypeObject(ali_a))));
}

void expect_true_alias_to_alias()
{
  TypeAssignability test;
  MinimalAliasType ali_a, ali_b;

  // Primitive types
  ali_a.body.common.related_type = TypeIdentifier(TK_BOOLEAN);
  ali_b.body.common.related_type = TypeIdentifier(TK_BOOLEAN);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(ali_a)), TypeObject(MinimalTypeObject(ali_b))));
  ali_a.body.common.related_type = TypeIdentifier(TK_BYTE);
  ali_b.body.common.related_type = TypeIdentifier(TK_BYTE);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(ali_a)), TypeObject(MinimalTypeObject(ali_b))));
  ali_a.body.common.related_type = TypeIdentifier(TK_INT16);
  ali_b.body.common.related_type = TypeIdentifier(TK_INT16);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(ali_a)), TypeObject(MinimalTypeObject(ali_b))));
  ali_a.body.common.related_type = TypeIdentifier(TK_INT32);
  ali_b.body.common.related_type = TypeIdentifier(TK_INT32);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(ali_a)), TypeObject(MinimalTypeObject(ali_b))));
  ali_a.body.common.related_type = TypeIdentifier(TK_INT64);
  ali_b.body.common.related_type = TypeIdentifier(TK_INT64);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(ali_a)), TypeObject(MinimalTypeObject(ali_b))));
  ali_a.body.common.related_type = TypeIdentifier(TK_UINT16);
  ali_b.body.common.related_type = TypeIdentifier(TK_UINT16);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(ali_a)), TypeObject(MinimalTypeObject(ali_b))));
  ali_a.body.common.related_type = TypeIdentifier(TK_UINT32);
  ali_b.body.common.related_type = TypeIdentifier(TK_UINT32);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(ali_a)), TypeObject(MinimalTypeObject(ali_b))));
  ali_a.body.common.related_type = TypeIdentifier(TK_UINT64);
  ali_b.body.common.related_type = TypeIdentifier(TK_UINT64);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(ali_a)), TypeObject(MinimalTypeObject(ali_b))));
  ali_a.body.common.related_type = TypeIdentifier(TK_FLOAT32);
  ali_b.body.common.related_type = TypeIdentifier(TK_FLOAT32);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(ali_a)), TypeObject(MinimalTypeObject(ali_b))));
  ali_a.body.common.related_type = TypeIdentifier(TK_FLOAT64);
  ali_b.body.common.related_type = TypeIdentifier(TK_FLOAT64);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(ali_a)), TypeObject(MinimalTypeObject(ali_b))));
  ali_a.body.common.related_type = TypeIdentifier(TK_FLOAT128);
  ali_b.body.common.related_type = TypeIdentifier(TK_FLOAT128);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(ali_a)), TypeObject(MinimalTypeObject(ali_b))));
  ali_a.body.common.related_type = TypeIdentifier(TK_INT8);
  ali_b.body.common.related_type = TypeIdentifier(TK_INT8);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(ali_a)), TypeObject(MinimalTypeObject(ali_b))));
  ali_a.body.common.related_type = TypeIdentifier(TK_UINT8);
  ali_b.body.common.related_type = TypeIdentifier(TK_UINT8);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(ali_a)), TypeObject(MinimalTypeObject(ali_b))));
  ali_a.body.common.related_type = TypeIdentifier(TK_CHAR8);
  ali_b.body.common.related_type = TypeIdentifier(TK_CHAR8);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(ali_a)), TypeObject(MinimalTypeObject(ali_b))));
  ali_a.body.common.related_type = TypeIdentifier(TK_CHAR16);
  ali_b.body.common.related_type = TypeIdentifier(TK_CHAR16);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(ali_a)), TypeObject(MinimalTypeObject(ali_b))));

  // String
  ali_a.body.common.related_type = TypeIdentifier::makeString(false, StringSTypeDefn(70));
  ali_b.body.common.related_type = TypeIdentifier::makeString(false, StringLTypeDefn(700));
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(ali_a)), TypeObject(MinimalTypeObject(ali_b))));
  ali_a.body.common.related_type = TypeIdentifier::makeString(true, StringLTypeDefn(100));
  ali_b.body.common.related_type = TypeIdentifier::makeString(true, StringSTypeDefn(200));
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(ali_a)), TypeObject(MinimalTypeObject(ali_b))));

  // Sequence
  ali_a.body.common.related_type = TypeIdentifier::makePlainSequence(TypeIdentifier(TK_UINT32),
                                                                     static_cast<SBound>(100));
  ali_b.body.common.related_type = TypeIdentifier::makePlainSequence(TypeIdentifier(TK_UINT32),
                                                                     static_cast<LBound>(200));
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(ali_a)), TypeObject(MinimalTypeObject(ali_b))));
  MinimalSequenceType seq_b;
  seq_b.header.common.bound = 300;
  seq_b.element.common.type = TypeIdentifier(TK_UINT32);
  EquivalenceHash hash;
  TypeLookup::get_equivalence_hash(hash);
  ali_b.body.common.related_type = TypeIdentifier::make(EK_MINIMAL, hash);
  TypeLookup::insert_entry(ali_b.body.common.related_type, MinimalTypeObject(seq_b));
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(ali_a)), TypeObject(MinimalTypeObject(ali_b))));

  // Array
  SBoundSeq bounds;
  bounds.append(50).append(60).append(70);
  ali_a.body.common.related_type = TypeIdentifier::makePlainArray(TypeIdentifier(TK_FLOAT32), bounds);
  ali_b.body.common.related_type = TypeIdentifier::makePlainArray(TypeIdentifier(TK_FLOAT32), bounds);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(ali_a)), TypeObject(MinimalTypeObject(ali_b))));
  MinimalArrayType arr_b;
  arr_b.header.common.bound_seq.append(50).append(60).append(70);
  arr_b.element.common.type = TypeIdentifier(TK_FLOAT32);
  TypeLookup::get_equivalence_hash(hash);
  ali_b.body.common.related_type = TypeIdentifier::make(EK_MINIMAL, hash);
  TypeLookup::insert_entry(ali_b.body.common.related_type, MinimalTypeObject(arr_b));
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(ali_a)), TypeObject(MinimalTypeObject(ali_b))));

  // Map
  PlainMapLTypeDefn plain_map_a(PlainCollectionHeader(EK_MINIMAL, CollectionElementFlag()),
                                static_cast<LBound>(111),
                                TypeIdentifier(TK_INT64),
                                CollectionElementFlag(),
                                TypeIdentifier(TK_UINT32));
  ali_a.body.common.related_type = TypeIdentifier::make(TI_PLAIN_MAP_LARGE, plain_map_a);
  PlainMapSTypeDefn plain_map_b(PlainCollectionHeader(EK_MINIMAL, CollectionElementFlag()),
                                static_cast<SBound>(200),
                                TypeIdentifier(TK_INT64),
                                CollectionElementFlag(),
                                TypeIdentifier(TK_UINT32));
  ali_b.body.common.related_type = TypeIdentifier::make(TI_PLAIN_MAP_SMALL, plain_map_b);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(ali_a)), TypeObject(MinimalTypeObject(ali_b))));
  MinimalMapType map_b;
  map_b.header.common.bound = 500;
  map_b.key.common.type = TypeIdentifier(TK_UINT32);
  map_b.element.common.type = TypeIdentifier(TK_INT64);
  TypeLookup::get_equivalence_hash(hash);
  ali_b.body.common.related_type = TypeIdentifier::make(EK_MINIMAL, hash);
  TypeLookup::insert_entry(ali_b.body.common.related_type, MinimalTypeObject(map_b));
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(ali_a)), TypeObject(MinimalTypeObject(ali_b))));

  // Enumeration
  MinimalEnumeratedLiteralSeq literal_seq;
  literal_seq.append(MinimalEnumeratedLiteral(CommonEnumeratedLiteral(1, EnumeratedLiteralFlag(IS_DEFAULT)),
                                              MinimalMemberDetail("LITERAL1")));
  literal_seq.append(MinimalEnumeratedLiteral(CommonEnumeratedLiteral(2, EnumeratedLiteralFlag()),
                                              MinimalMemberDetail("LITERAL2")));
  literal_seq.append(MinimalEnumeratedLiteral(CommonEnumeratedLiteral(3, EnumeratedLiteralFlag()),
                                              MinimalMemberDetail("LITERAL3")));
  literal_seq.append(MinimalEnumeratedLiteral(CommonEnumeratedLiteral(4, EnumeratedLiteralFlag()),
                                              MinimalMemberDetail("LITERAL4")));
  MinimalEnumeratedType enum_a(EnumTypeFlag(),
                               MinimalEnumeratedHeader(CommonEnumeratedHeader(static_cast<BitBound>(4))),
                               literal_seq);
  TypeLookup::get_equivalence_hash(hash);
  ali_a.body.common.related_type = TypeIdentifier::make(EK_MINIMAL, hash);
  TypeLookup::insert_entry(ali_a.body.common.related_type, MinimalTypeObject(enum_a));
  literal_seq.append(MinimalEnumeratedLiteral(CommonEnumeratedLiteral(5, EnumeratedLiteralFlag()),
                                              MinimalMemberDetail("LITERAL5")));
  MinimalEnumeratedType enum_b(EnumTypeFlag(),
                               MinimalEnumeratedHeader(CommonEnumeratedHeader(static_cast<BitBound>(5))),
                               literal_seq);
  TypeLookup::get_equivalence_hash(hash);
  ali_b.body.common.related_type = TypeIdentifier::make(EK_MINIMAL, hash);
  TypeLookup::insert_entry(ali_b.body.common.related_type, MinimalTypeObject(enum_b));
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(ali_a)), TypeObject(MinimalTypeObject(ali_b))));

  // Bitmask
  MinimalBitmaskType bitmask_a;
  bitmask_a.header.common.bit_bound = 4;
  MinimalBitflag flag;
  flag.common.position = 0;
  flag.detail = MinimalMemberDetail("BIT1");
  bitmask_a.flag_seq.append(flag);
  flag.common.position = 1;
  flag.detail = MinimalMemberDetail("BIT2");
  bitmask_a.flag_seq.append(flag);
  flag.common.position = 2;
  flag.detail = MinimalMemberDetail("BIT3");
  bitmask_a.flag_seq.append(flag);
  flag.common.position = 3;
  flag.detail = MinimalMemberDetail("BIT4");
  bitmask_a.flag_seq.append(flag);
  TypeLookup::get_equivalence_hash(hash);
  ali_a.body.common.related_type = TypeIdentifier::make(EK_MINIMAL, hash);
  TypeLookup::insert_entry(ali_a.body.common.related_type, MinimalTypeObject(bitmask_a));
  TypeLookup::get_equivalence_hash(hash);
  ali_b.body.common.related_type = TypeIdentifier::make(EK_MINIMAL, hash);
  TypeLookup::insert_entry(ali_b.body.common.related_type, MinimalTypeObject(bitmask_a));
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(ali_a)), TypeObject(MinimalTypeObject(ali_b))));
  ali_b.body.common.related_type = TypeIdentifier(TK_UINT8);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(ali_a)), TypeObject(MinimalTypeObject(ali_b))));

  bitmask_a.header.common.bit_bound = 14;
  TypeLookup::get_equivalence_hash(hash);
  ali_a.body.common.related_type = TypeIdentifier::make(EK_MINIMAL, hash);
  TypeLookup::insert_entry(ali_a.body.common.related_type, MinimalTypeObject(bitmask_a));
  ali_b.body.common.related_type = TypeIdentifier(TK_UINT16);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(ali_a)), TypeObject(MinimalTypeObject(ali_b))));

  bitmask_a.header.common.bit_bound = 31;
  TypeLookup::get_equivalence_hash(hash);
  ali_a.body.common.related_type = TypeIdentifier::make(EK_MINIMAL, hash);
  TypeLookup::insert_entry(ali_a.body.common.related_type, MinimalTypeObject(bitmask_a));
  ali_b.body.common.related_type = TypeIdentifier(TK_UINT32);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(ali_a)), TypeObject(MinimalTypeObject(ali_b))));

  bitmask_a.header.common.bit_bound = 60;
  TypeLookup::get_equivalence_hash(hash);
  ali_a.body.common.related_type = TypeIdentifier::make(EK_MINIMAL, hash);
  TypeLookup::insert_entry(ali_a.body.common.related_type, MinimalTypeObject(bitmask_a));
  ali_b.body.common.related_type = TypeIdentifier(TK_UINT64);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(ali_a)), TypeObject(MinimalTypeObject(ali_b))));
}

TEST(AliasTypeTest, Assignable)
{
  expect_true_non_alias_to_alias();
  expect_true_alias_to_non_alias();
  expect_true_alias_to_alias();
}

void expect_false_non_alias_to_alias()
{
  TypeAssignability test;
  MinimalAliasType ali_a;

  // Primitive types
  ali_a.body.common.related_type = TypeIdentifier(TK_BOOLEAN);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(ali_a)), TypeIdentifier(TK_INT16)));
  ali_a.body.common.related_type = TypeIdentifier(TK_BYTE);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(ali_a)), TypeIdentifier(TK_FLOAT32)));
  ali_a.body.common.related_type = TypeIdentifier(TK_INT16);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(ali_a)), TypeIdentifier(TK_UINT32)));
  ali_a.body.common.related_type = TypeIdentifier(TK_INT32);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(ali_a)), TypeIdentifier(TK_BYTE)));
  ali_a.body.common.related_type = TypeIdentifier(TK_INT64);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(ali_a)), TypeIdentifier(TK_BOOLEAN)));
  ali_a.body.common.related_type = TypeIdentifier(TK_UINT16);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(ali_a)), TypeIdentifier(TK_INT16)));
  ali_a.body.common.related_type = TypeIdentifier(TK_UINT32);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(ali_a)), TypeIdentifier(TK_UINT16)));
  ali_a.body.common.related_type = TypeIdentifier(TK_UINT64);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(ali_a)), TypeIdentifier(TK_BYTE)));
  ali_a.body.common.related_type = TypeIdentifier(TK_FLOAT32);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(ali_a)), TypeIdentifier(TK_INT32)));
  ali_a.body.common.related_type = TypeIdentifier(TK_FLOAT64);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(ali_a)), TypeIdentifier(TK_INT64)));
  ali_a.body.common.related_type = TypeIdentifier(TK_FLOAT128);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(ali_a)), TypeIdentifier(TK_UINT64)));
  ali_a.body.common.related_type = TypeIdentifier(TK_INT8);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(ali_a)), TypeIdentifier(TK_CHAR8)));
  ali_a.body.common.related_type = TypeIdentifier(TK_UINT8);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(ali_a)), TypeIdentifier(TK_INT8)));
  ali_a.body.common.related_type = TypeIdentifier(TK_CHAR8);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(ali_a)), TypeIdentifier(TK_INT16)));
  ali_a.body.common.related_type = TypeIdentifier(TK_CHAR16);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(ali_a)), TypeIdentifier(TK_FLOAT32)));

  ali_a.body.common.related_type = TypeIdentifier::makeString(false, StringSTypeDefn(70));
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(ali_a)),
                               TypeIdentifier::makeString(true, StringLTypeDefn(120))));
  ali_a.body.common.related_type = TypeIdentifier::makeString(true, StringSTypeDefn(70));
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(ali_a)),
                               TypeIdentifier::makeString(false, StringLTypeDefn(120))));

  // Sequence
  ali_a.body.common.related_type = TypeIdentifier::makePlainSequence(TypeIdentifier(TK_UINT32),
                                                                     static_cast<SBound>(100));
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(ali_a)),
                               TypeIdentifier::makePlainSequence(TypeIdentifier(TK_FLOAT32),
                                                                  static_cast<LBound>(200))));
  MinimalSequenceType seq_b;
  seq_b.header.common.bound = 300;
  seq_b.element.common.type = TypeIdentifier(TK_CHAR16);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(ali_a)), TypeObject(MinimalTypeObject(seq_b))));

  // Array
  SBoundSeq bounds_a;
  bounds_a.append(50).append(60).append(70);
  ali_a.body.common.related_type = TypeIdentifier::makePlainArray(TypeIdentifier(TK_FLOAT32), bounds_a);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(ali_a)),
                               TypeIdentifier::makePlainArray(TypeIdentifier(TK_INT32), bounds_a)));
  MinimalArrayType arr_b;
  arr_b.header.common.bound_seq.append(50).append(60).append(70);
  arr_b.element.common.type = TypeIdentifier(TK_UINT64);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(ali_a)), TypeObject(MinimalTypeObject(arr_b))));

  // Map
  PlainMapLTypeDefn plain_map_a(PlainCollectionHeader(EK_MINIMAL, CollectionElementFlag()),
                                static_cast<LBound>(111),
                                TypeIdentifier(TK_INT64),
                                CollectionElementFlag(),
                                TypeIdentifier(TK_UINT32));
  ali_a.body.common.related_type = TypeIdentifier::make(TI_PLAIN_MAP_LARGE, plain_map_a);
  PlainMapSTypeDefn plain_map_b(PlainCollectionHeader(EK_MINIMAL, CollectionElementFlag()),
                                static_cast<SBound>(200),
                                TypeIdentifier(TK_INT64),
                                CollectionElementFlag(),
                                TypeIdentifier(TK_INT32));
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(ali_a)),
                               TypeIdentifier::make(TI_PLAIN_MAP_SMALL, plain_map_b)));
  MinimalMapType map_b;
  map_b.header.common.bound = 500;
  map_b.key.common.type = TypeIdentifier(TK_UINT32);
  map_b.element.common.type = TypeIdentifier(TK_FLOAT64);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(ali_a)), TypeObject(MinimalTypeObject(map_b))));

  // Enumeration
  MinimalEnumeratedLiteralSeq literal_seq;
  literal_seq.append(MinimalEnumeratedLiteral(CommonEnumeratedLiteral(1, EnumeratedLiteralFlag(IS_DEFAULT)),
                                              MinimalMemberDetail("LITERAL1")));
  literal_seq.append(MinimalEnumeratedLiteral(CommonEnumeratedLiteral(2, EnumeratedLiteralFlag()),
                                              MinimalMemberDetail("LITERAL2")));
  literal_seq.append(MinimalEnumeratedLiteral(CommonEnumeratedLiteral(3, EnumeratedLiteralFlag()),
                                              MinimalMemberDetail("LITERAL3")));
  literal_seq.append(MinimalEnumeratedLiteral(CommonEnumeratedLiteral(4, EnumeratedLiteralFlag()),
                                              MinimalMemberDetail("LITERAL4")));
  MinimalEnumeratedType enum_a(EnumTypeFlag(),
                               MinimalEnumeratedHeader(CommonEnumeratedHeader(static_cast<BitBound>(4))),
                               literal_seq);
  EquivalenceHash hash;
  TypeLookup::get_equivalence_hash(hash);
  ali_a.body.common.related_type = TypeIdentifier::make(EK_MINIMAL, hash);
  TypeLookup::insert_entry(ali_a.body.common.related_type, MinimalTypeObject(enum_a));
  MinimalEnumeratedLiteralSeq literal_seq_b;
  literal_seq_b.append(MinimalEnumeratedLiteral(CommonEnumeratedLiteral(2, EnumeratedLiteralFlag(IS_DEFAULT)),
                                                MinimalMemberDetail("LITERAL3")));
  MinimalEnumeratedType enum_b(EnumTypeFlag(),
                               MinimalEnumeratedHeader(CommonEnumeratedHeader(static_cast<BitBound>(1))),
                               literal_seq_b);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(ali_a)), TypeObject(MinimalTypeObject(enum_b))));

  // Bitmask
  MinimalBitmaskType bitmask_a;
  bitmask_a.header.common.bit_bound = 4;
  MinimalBitflag flag;
  flag.common.position = 0;
  flag.detail = MinimalMemberDetail("BIT1");
  bitmask_a.flag_seq.append(flag);
  flag.common.position = 1;
  flag.detail = MinimalMemberDetail("BIT2");
  bitmask_a.flag_seq.append(flag);
  flag.common.position = 2;
  flag.detail = MinimalMemberDetail("BIT3");
  bitmask_a.flag_seq.append(flag);
  flag.common.position = 3;
  flag.detail = MinimalMemberDetail("BIT4");
  bitmask_a.flag_seq.append(flag);
  TypeLookup::get_equivalence_hash(hash);
  ali_a.body.common.related_type = TypeIdentifier::make(EK_MINIMAL, hash);
  TypeLookup::insert_entry(ali_a.body.common.related_type, MinimalTypeObject(bitmask_a));
  MinimalBitmaskType bitmask_b;
  bitmask_b.header.common.bit_bound = 10;
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(ali_a)), TypeObject(MinimalTypeObject(bitmask_b))));
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(ali_a)), TypeIdentifier(TK_UINT32)));

  bitmask_a.header.common.bit_bound = 14;
  TypeLookup::get_equivalence_hash(hash);
  ali_a.body.common.related_type = TypeIdentifier::make(EK_MINIMAL, hash);
  TypeLookup::insert_entry(ali_a.body.common.related_type, MinimalTypeObject(bitmask_a));
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(ali_a)), TypeIdentifier(TK_UINT8)));

  bitmask_a.header.common.bit_bound = 31;
  TypeLookup::get_equivalence_hash(hash);
  ali_a.body.common.related_type = TypeIdentifier::make(EK_MINIMAL, hash);
  TypeLookup::insert_entry(ali_a.body.common.related_type, MinimalTypeObject(bitmask_a));
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(ali_a)), TypeIdentifier(TK_UINT16)));

  bitmask_a.header.common.bit_bound = 60;
  TypeLookup::get_equivalence_hash(hash);
  ali_a.body.common.related_type = TypeIdentifier::make(EK_MINIMAL, hash);
  TypeLookup::insert_entry(ali_a.body.common.related_type, MinimalTypeObject(bitmask_a));
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(ali_a)), TypeIdentifier(TK_UINT32)));
}

void expect_false_alias_to_non_alias()
{
  TypeAssignability test;
  MinimalAliasType ali_a;

  // Primitive types
  ali_a.body.common.related_type = TypeIdentifier(TK_BOOLEAN);
  EXPECT_FALSE(test.assignable(TypeIdentifier(TK_FLOAT32), TypeObject(MinimalTypeObject(ali_a))));
  ali_a.body.common.related_type = TypeIdentifier(TK_BYTE);
  EXPECT_FALSE(test.assignable(TypeIdentifier(TK_BOOLEAN), TypeObject(MinimalTypeObject(ali_a))));
  ali_a.body.common.related_type = TypeIdentifier(TK_INT16);
  EXPECT_FALSE(test.assignable(TypeIdentifier(TK_CHAR8), TypeObject(MinimalTypeObject(ali_a))));
  ali_a.body.common.related_type = TypeIdentifier(TK_INT32);
  EXPECT_FALSE(test.assignable(TypeIdentifier(TK_BYTE), TypeObject(MinimalTypeObject(ali_a))));
  ali_a.body.common.related_type = TypeIdentifier(TK_INT64);
  EXPECT_FALSE(test.assignable(TypeIdentifier(TK_UINT16), TypeObject(MinimalTypeObject(ali_a))));
  ali_a.body.common.related_type = TypeIdentifier(TK_UINT16);
  EXPECT_FALSE(test.assignable(TypeIdentifier(TK_INT16), TypeObject(MinimalTypeObject(ali_a))));
  ali_a.body.common.related_type = TypeIdentifier(TK_UINT32);
  EXPECT_FALSE(test.assignable(TypeIdentifier(TK_CHAR8), TypeObject(MinimalTypeObject(ali_a))));
  ali_a.body.common.related_type = TypeIdentifier(TK_UINT64);
  EXPECT_FALSE(test.assignable(TypeIdentifier(TK_INT16), TypeObject(MinimalTypeObject(ali_a))));
  ali_a.body.common.related_type = TypeIdentifier(TK_FLOAT32);
  EXPECT_FALSE(test.assignable(TypeIdentifier(TK_INT32), TypeObject(MinimalTypeObject(ali_a))));
  ali_a.body.common.related_type = TypeIdentifier(TK_FLOAT64);
  EXPECT_FALSE(test.assignable(TypeIdentifier(TK_UINT64), TypeObject(MinimalTypeObject(ali_a))));
  ali_a.body.common.related_type = TypeIdentifier(TK_FLOAT128);
  EXPECT_FALSE(test.assignable(TypeIdentifier(TK_BYTE), TypeObject(MinimalTypeObject(ali_a))));
  ali_a.body.common.related_type = TypeIdentifier(TK_INT8);
  EXPECT_FALSE(test.assignable(TypeIdentifier(TK_CHAR8), TypeObject(MinimalTypeObject(ali_a))));
  ali_a.body.common.related_type = TypeIdentifier(TK_UINT8);
  EXPECT_FALSE(test.assignable(TypeIdentifier(TK_INT8), TypeObject(MinimalTypeObject(ali_a))));
  ali_a.body.common.related_type = TypeIdentifier(TK_CHAR8);
  EXPECT_FALSE(test.assignable(TypeIdentifier(TK_INT16), TypeObject(MinimalTypeObject(ali_a))));
  ali_a.body.common.related_type = TypeIdentifier(TK_CHAR16);
  EXPECT_FALSE(test.assignable(TypeIdentifier(TK_FLOAT128), TypeObject(MinimalTypeObject(ali_a))));

  // String
  ali_a.body.common.related_type = TypeIdentifier::makeString(false, StringSTypeDefn(70));
  EXPECT_FALSE(test.assignable(TypeIdentifier::makeString(true, StringLTypeDefn(130)),
                               TypeObject(MinimalTypeObject(ali_a))));
  ali_a.body.common.related_type = TypeIdentifier::makeString(true, StringSTypeDefn(70));
  EXPECT_FALSE(test.assignable(TypeIdentifier::makeString(false, StringLTypeDefn(130)),
                               TypeObject(MinimalTypeObject(ali_a))));

  // Sequence
  ali_a.body.common.related_type = TypeIdentifier::makePlainSequence(TypeIdentifier(TK_UINT32),
                                                                     static_cast<SBound>(100));
  EXPECT_FALSE(test.assignable(TypeIdentifier::makePlainSequence(TypeIdentifier(TK_FLOAT128),
                                                                  static_cast<LBound>(200)),
                               TypeObject(MinimalTypeObject(ali_a))));
  MinimalSequenceType seq_b;
  seq_b.header.common.bound = 300;
  seq_b.element.common.type = TypeIdentifier(TK_CHAR16);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(seq_b)), TypeObject(MinimalTypeObject(ali_a))));

  // Array
  SBoundSeq bounds_a;
  bounds_a.append(50).append(60).append(70);
  ali_a.body.common.related_type = TypeIdentifier::makePlainArray(TypeIdentifier(TK_FLOAT32), bounds_a);
  EXPECT_FALSE(test.assignable(TypeIdentifier::makePlainArray(TypeIdentifier(TK_BYTE), bounds_a),
                               TypeObject(MinimalTypeObject(ali_a))));
  MinimalArrayType arr_b;
  arr_b.header.common.bound_seq.append(50).append(60).append(70);
  arr_b.element.common.type = TypeIdentifier(TK_FLOAT64);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(arr_b)), TypeObject(MinimalTypeObject(ali_a))));

  // Map
  PlainMapLTypeDefn plain_map_a(PlainCollectionHeader(EK_MINIMAL, CollectionElementFlag()),
                                static_cast<LBound>(111),
                                TypeIdentifier(TK_INT64),
                                CollectionElementFlag(),
                                TypeIdentifier(TK_INT32));
  ali_a.body.common.related_type = TypeIdentifier::make(TI_PLAIN_MAP_LARGE, plain_map_a);
  PlainMapSTypeDefn plain_map_b(PlainCollectionHeader(EK_MINIMAL, CollectionElementFlag()),
                                static_cast<SBound>(200),
                                TypeIdentifier(TK_FLOAT64),
                                CollectionElementFlag(),
                                TypeIdentifier(TK_INT32));
  EXPECT_FALSE(test.assignable(TypeIdentifier::make(TI_PLAIN_MAP_SMALL, plain_map_b),
                               TypeObject(MinimalTypeObject(ali_a))));
  MinimalMapType map_b;
  map_b.header.common.bound = 500;
  map_b.key.common.type = TypeIdentifier(TK_UINT32);
  map_b.element.common.type = TypeIdentifier(TK_INT64);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(map_b)), TypeObject(MinimalTypeObject(ali_a))));

  // Enumeration
  MinimalEnumeratedLiteralSeq literal_seq;
  literal_seq.append(MinimalEnumeratedLiteral(CommonEnumeratedLiteral(1, EnumeratedLiteralFlag(IS_DEFAULT)),
                                              MinimalMemberDetail("LITERAL1")));
  literal_seq.append(MinimalEnumeratedLiteral(CommonEnumeratedLiteral(2, EnumeratedLiteralFlag()),
                                              MinimalMemberDetail("LITERAL2")));
  literal_seq.append(MinimalEnumeratedLiteral(CommonEnumeratedLiteral(3, EnumeratedLiteralFlag()),
                                              MinimalMemberDetail("LITERAL3")));
  literal_seq.append(MinimalEnumeratedLiteral(CommonEnumeratedLiteral(4, EnumeratedLiteralFlag()),
                                              MinimalMemberDetail("LITERAL4")));
  MinimalEnumeratedType enum_a(EnumTypeFlag(),
                               MinimalEnumeratedHeader(CommonEnumeratedHeader(static_cast<BitBound>(4))),
                               literal_seq);
  EquivalenceHash hash;
  TypeLookup::get_equivalence_hash(hash);
  ali_a.body.common.related_type = TypeIdentifier::make(EK_MINIMAL, hash);
  TypeLookup::insert_entry(ali_a.body.common.related_type, MinimalTypeObject(enum_a));
  MinimalEnumeratedLiteralSeq literal_seq_b;
  literal_seq_b.append(MinimalEnumeratedLiteral(CommonEnumeratedLiteral(6, EnumeratedLiteralFlag()),
                                                MinimalMemberDetail("LITERAL1")));
  MinimalEnumeratedType enum_b(EnumTypeFlag(),
                               MinimalEnumeratedHeader(CommonEnumeratedHeader(static_cast<BitBound>(1))),
                               literal_seq_b);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(enum_b)), TypeObject(MinimalTypeObject(ali_a))));

  // Bitmask
  MinimalBitmaskType bitmask_a;
  bitmask_a.header.common.bit_bound = 4;
  MinimalBitflag flag;
  flag.common.position = 0;
  flag.detail = MinimalMemberDetail("BIT1");
  bitmask_a.flag_seq.append(flag);
  flag.common.position = 1;
  flag.detail = MinimalMemberDetail("BIT2");
  bitmask_a.flag_seq.append(flag);
  flag.common.position = 2;
  flag.detail = MinimalMemberDetail("BIT3");
  bitmask_a.flag_seq.append(flag);
  flag.common.position = 3;
  flag.detail = MinimalMemberDetail("BIT4");
  bitmask_a.flag_seq.append(flag);
  TypeLookup::get_equivalence_hash(hash);
  ali_a.body.common.related_type = TypeIdentifier::make(EK_MINIMAL, hash);
  TypeLookup::insert_entry(ali_a.body.common.related_type, MinimalTypeObject(bitmask_a));
  MinimalBitmaskType bitmask_b;
  bitmask_b.header.common.bit_bound = 9;
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(bitmask_b)), TypeObject(MinimalTypeObject(ali_a))));
  EXPECT_FALSE(test.assignable(TypeIdentifier(TK_UINT16), TypeObject(MinimalTypeObject(ali_a))));

  bitmask_a.header.common.bit_bound = 14;
  TypeLookup::get_equivalence_hash(hash);
  ali_a.body.common.related_type = TypeIdentifier::make(EK_MINIMAL, hash);
  TypeLookup::insert_entry(ali_a.body.common.related_type, MinimalTypeObject(bitmask_a));
  EXPECT_FALSE(test.assignable(TypeIdentifier(TK_UINT32), TypeObject(MinimalTypeObject(ali_a))));

  bitmask_a.header.common.bit_bound = 31;
  TypeLookup::get_equivalence_hash(hash);
  ali_a.body.common.related_type = TypeIdentifier::make(EK_MINIMAL, hash);
  TypeLookup::insert_entry(ali_a.body.common.related_type, MinimalTypeObject(bitmask_a));
  EXPECT_FALSE(test.assignable(TypeIdentifier(TK_UINT64), TypeObject(MinimalTypeObject(ali_a))));

  bitmask_a.header.common.bit_bound = 60;
  TypeLookup::get_equivalence_hash(hash);
  ali_a.body.common.related_type = TypeIdentifier::make(EK_MINIMAL, hash);
  TypeLookup::insert_entry(ali_a.body.common.related_type, MinimalTypeObject(bitmask_a));
  EXPECT_FALSE(test.assignable(TypeIdentifier(TK_UINT16), TypeObject(MinimalTypeObject(ali_a))));
}

void expect_false_alias_to_alias()
{
  TypeAssignability test;
  MinimalAliasType ali_a, ali_b;

  // Primitive types
  ali_a.body.common.related_type = TypeIdentifier(TK_BOOLEAN);
  ali_b.body.common.related_type = TypeIdentifier(TK_INT8);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(ali_a)), TypeObject(MinimalTypeObject(ali_b))));
  ali_a.body.common.related_type = TypeIdentifier(TK_BYTE);
  ali_b.body.common.related_type = TypeIdentifier(TK_CHAR8);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(ali_a)), TypeObject(MinimalTypeObject(ali_b))));
  ali_a.body.common.related_type = TypeIdentifier(TK_INT16);
  ali_b.body.common.related_type = TypeIdentifier(TK_BYTE);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(ali_a)), TypeObject(MinimalTypeObject(ali_b))));
  ali_a.body.common.related_type = TypeIdentifier(TK_INT32);
  ali_b.body.common.related_type = TypeIdentifier(TK_FLOAT32);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(ali_a)), TypeObject(MinimalTypeObject(ali_b))));
  ali_a.body.common.related_type = TypeIdentifier(TK_INT64);
  ali_b.body.common.related_type = TypeIdentifier(TK_UINT16);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(ali_a)), TypeObject(MinimalTypeObject(ali_b))));
  ali_a.body.common.related_type = TypeIdentifier(TK_UINT16);
  ali_b.body.common.related_type = TypeIdentifier(TK_CHAR16);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(ali_a)), TypeObject(MinimalTypeObject(ali_b))));
  ali_a.body.common.related_type = TypeIdentifier(TK_UINT32);
  ali_b.body.common.related_type = TypeIdentifier(TK_INT32);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(ali_a)), TypeObject(MinimalTypeObject(ali_b))));
  ali_a.body.common.related_type = TypeIdentifier(TK_UINT64);
  ali_b.body.common.related_type = TypeIdentifier(TK_FLOAT32);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(ali_a)), TypeObject(MinimalTypeObject(ali_b))));
  ali_a.body.common.related_type = TypeIdentifier(TK_FLOAT32);
  ali_b.body.common.related_type = TypeIdentifier(TK_INT64);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(ali_a)), TypeObject(MinimalTypeObject(ali_b))));
  ali_a.body.common.related_type = TypeIdentifier(TK_FLOAT64);
  ali_b.body.common.related_type = TypeIdentifier(TK_UINT64);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(ali_a)), TypeObject(MinimalTypeObject(ali_b))));
  ali_a.body.common.related_type = TypeIdentifier(TK_FLOAT128);
  ali_b.body.common.related_type = TypeIdentifier(TK_FLOAT64);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(ali_a)), TypeObject(MinimalTypeObject(ali_b))));
  ali_a.body.common.related_type = TypeIdentifier(TK_INT8);
  ali_b.body.common.related_type = TypeIdentifier(TK_INT32);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(ali_a)), TypeObject(MinimalTypeObject(ali_b))));
  ali_a.body.common.related_type = TypeIdentifier(TK_UINT8);
  ali_b.body.common.related_type = TypeIdentifier(TK_BYTE);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(ali_a)), TypeObject(MinimalTypeObject(ali_b))));
  ali_a.body.common.related_type = TypeIdentifier(TK_CHAR8);
  ali_b.body.common.related_type = TypeIdentifier(TK_INT16);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(ali_a)), TypeObject(MinimalTypeObject(ali_b))));
  ali_a.body.common.related_type = TypeIdentifier(TK_CHAR16);
  ali_b.body.common.related_type = TypeIdentifier(TK_UINT32);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(ali_a)), TypeObject(MinimalTypeObject(ali_b))));

  // String
  ali_a.body.common.related_type = TypeIdentifier::makeString(true, StringSTypeDefn(70));
  ali_b.body.common.related_type = TypeIdentifier::makeString(false, StringLTypeDefn(700));
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(ali_a)), TypeObject(MinimalTypeObject(ali_b))));
  ali_a.body.common.related_type = TypeIdentifier::makeString(false, StringLTypeDefn(100));
  ali_b.body.common.related_type = TypeIdentifier::makeString(true, StringSTypeDefn(200));
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(ali_a)), TypeObject(MinimalTypeObject(ali_b))));

  // Sequence
  ali_a.body.common.related_type = TypeIdentifier::makePlainSequence(TypeIdentifier(TK_INT64),
                                                                     static_cast<SBound>(100));
  ali_b.body.common.related_type = TypeIdentifier::makePlainSequence(TypeIdentifier(TK_UINT32),
                                                                     static_cast<LBound>(200));
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(ali_a)), TypeObject(MinimalTypeObject(ali_b))));
  MinimalSequenceType seq_b;
  seq_b.header.common.bound = 300;
  seq_b.element.common.type = TypeIdentifier(TK_FLOAT64);
  EquivalenceHash hash;
  TypeLookup::get_equivalence_hash(hash);
  ali_b.body.common.related_type = TypeIdentifier::make(EK_MINIMAL, hash);
  TypeLookup::insert_entry(ali_b.body.common.related_type, MinimalTypeObject(seq_b));
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(ali_a)), TypeObject(MinimalTypeObject(ali_b))));

  // Array
  SBoundSeq bounds;
  bounds.append(50).append(60).append(70);
  ali_a.body.common.related_type = TypeIdentifier::makePlainArray(TypeIdentifier(TK_FLOAT32), bounds);
  ali_b.body.common.related_type = TypeIdentifier::makePlainArray(TypeIdentifier(TK_BYTE), bounds);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(ali_a)), TypeObject(MinimalTypeObject(ali_b))));
  MinimalArrayType arr_b;
  arr_b.header.common.bound_seq.append(50).append(60).append(70);
  arr_b.element.common.type = TypeIdentifier(TK_INT32);
  TypeLookup::get_equivalence_hash(hash);
  ali_b.body.common.related_type = TypeIdentifier::make(EK_MINIMAL, hash);
  TypeLookup::insert_entry(ali_b.body.common.related_type, MinimalTypeObject(arr_b));
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(ali_a)), TypeObject(MinimalTypeObject(ali_b))));

  // Map
  PlainMapLTypeDefn plain_map_a(PlainCollectionHeader(EK_MINIMAL, CollectionElementFlag()),
                                static_cast<LBound>(111),
                                TypeIdentifier(TK_INT64),
                                CollectionElementFlag(),
                                TypeIdentifier(TK_UINT32));
  ali_a.body.common.related_type = TypeIdentifier::make(TI_PLAIN_MAP_LARGE, plain_map_a);
  PlainMapSTypeDefn plain_map_b(PlainCollectionHeader(EK_MINIMAL, CollectionElementFlag()),
                                static_cast<SBound>(200),
                                TypeIdentifier(TK_INT64),
                                CollectionElementFlag(),
                                TypeIdentifier(TK_CHAR16));
  ali_b.body.common.related_type = TypeIdentifier::make(TI_PLAIN_MAP_SMALL, plain_map_b);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(ali_a)), TypeObject(MinimalTypeObject(ali_b))));
  MinimalMapType map_b;
  map_b.header.common.bound = 500;
  map_b.key.common.type = TypeIdentifier(TK_UINT32);
  map_b.element.common.type = TypeIdentifier(TK_UINT64);
  TypeLookup::get_equivalence_hash(hash);
  ali_b.body.common.related_type = TypeIdentifier::make(EK_MINIMAL, hash);
  TypeLookup::insert_entry(ali_b.body.common.related_type, MinimalTypeObject(map_b));
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(ali_a)), TypeObject(MinimalTypeObject(ali_b))));

  // Enumeration
  MinimalEnumeratedLiteralSeq literal_seq;
  literal_seq.append(MinimalEnumeratedLiteral(CommonEnumeratedLiteral(1, EnumeratedLiteralFlag(IS_DEFAULT)),
                                              MinimalMemberDetail("LITERAL1")));
  literal_seq.append(MinimalEnumeratedLiteral(CommonEnumeratedLiteral(2, EnumeratedLiteralFlag()),
                                              MinimalMemberDetail("LITERAL2")));
  literal_seq.append(MinimalEnumeratedLiteral(CommonEnumeratedLiteral(3, EnumeratedLiteralFlag()),
                                              MinimalMemberDetail("LITERAL3")));
  literal_seq.append(MinimalEnumeratedLiteral(CommonEnumeratedLiteral(4, EnumeratedLiteralFlag()),
                                              MinimalMemberDetail("LITERAL4")));
  MinimalEnumeratedType enum_a(EnumTypeFlag(),
                               MinimalEnumeratedHeader(CommonEnumeratedHeader(static_cast<BitBound>(4))),
                               literal_seq);
  TypeLookup::get_equivalence_hash(hash);
  ali_a.body.common.related_type = TypeIdentifier::make(EK_MINIMAL, hash);
  TypeLookup::insert_entry(ali_a.body.common.related_type, MinimalTypeObject(enum_a));
  MinimalEnumeratedLiteralSeq literal_seq_b;
  literal_seq_b.append(MinimalEnumeratedLiteral(CommonEnumeratedLiteral(5, EnumeratedLiteralFlag()),
                                                MinimalMemberDetail("LITERAL2")));
  MinimalEnumeratedType enum_b(EnumTypeFlag(),
                               MinimalEnumeratedHeader(CommonEnumeratedHeader(static_cast<BitBound>(1))),
                               literal_seq_b);
  TypeLookup::get_equivalence_hash(hash);
  ali_b.body.common.related_type = TypeIdentifier::make(EK_MINIMAL, hash);
  TypeLookup::insert_entry(ali_b.body.common.related_type, MinimalTypeObject(enum_b));
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(ali_a)), TypeObject(MinimalTypeObject(ali_b))));

  // Bitmask
  MinimalBitmaskType bitmask_a;
  bitmask_a.header.common.bit_bound = 4;
  MinimalBitflag flag;
  flag.common.position = 0;
  flag.detail = MinimalMemberDetail("BIT1");
  bitmask_a.flag_seq.append(flag);
  flag.common.position = 1;
  flag.detail = MinimalMemberDetail("BIT2");
  bitmask_a.flag_seq.append(flag);
  flag.common.position = 2;
  flag.detail = MinimalMemberDetail("BIT3");
  bitmask_a.flag_seq.append(flag);
  flag.common.position = 3;
  flag.detail = MinimalMemberDetail("BIT4");
  bitmask_a.flag_seq.append(flag);
  TypeLookup::get_equivalence_hash(hash);
  ali_a.body.common.related_type = TypeIdentifier::make(EK_MINIMAL, hash);
  TypeLookup::insert_entry(ali_a.body.common.related_type, MinimalTypeObject(bitmask_a));
  MinimalBitmaskType bitmask_b;
  bitmask_b.header.common.bit_bound = 11;
  TypeLookup::get_equivalence_hash(hash);
  ali_b.body.common.related_type = TypeIdentifier::make(EK_MINIMAL, hash);
  TypeLookup::insert_entry(ali_b.body.common.related_type, MinimalTypeObject(bitmask_b));
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(ali_a)), TypeObject(MinimalTypeObject(ali_b))));
  ali_b.body.common.related_type = TypeIdentifier(TK_UINT16);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(ali_a)), TypeObject(MinimalTypeObject(ali_b))));

  bitmask_a.header.common.bit_bound = 14;
  TypeLookup::get_equivalence_hash(hash);
  ali_a.body.common.related_type = TypeIdentifier::make(EK_MINIMAL, hash);
  TypeLookup::insert_entry(ali_a.body.common.related_type, MinimalTypeObject(bitmask_a));
  ali_b.body.common.related_type = TypeIdentifier(TK_UINT32);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(ali_a)), TypeObject(MinimalTypeObject(ali_b))));

  bitmask_a.header.common.bit_bound = 31;
  TypeLookup::get_equivalence_hash(hash);
  ali_a.body.common.related_type = TypeIdentifier::make(EK_MINIMAL, hash);
  TypeLookup::insert_entry(ali_a.body.common.related_type, MinimalTypeObject(bitmask_a));
  ali_b.body.common.related_type = TypeIdentifier(TK_UINT16);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(ali_a)), TypeObject(MinimalTypeObject(ali_b))));

  bitmask_a.header.common.bit_bound = 60;
  TypeLookup::get_equivalence_hash(hash);
  ali_a.body.common.related_type = TypeIdentifier::make(EK_MINIMAL, hash);
  TypeLookup::insert_entry(ali_a.body.common.related_type, MinimalTypeObject(bitmask_a));
  ali_b.body.common.related_type = TypeIdentifier(TK_UINT32);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(ali_a)), TypeObject(MinimalTypeObject(ali_b))));
}

TEST(AliasTypeTest, NotAssignable)
{
  expect_false_non_alias_to_alias();
  expect_false_alias_to_non_alias();
  expect_false_alias_to_alias();
}

TEST(StructTypeTest, Assignable)
{
  TypeAssignability test;
  MinimalStructType a, b;
  a.struct_flags = IS_APPENDABLE;
  b.struct_flags = a.struct_flags;

  // Primitive members
  MinimalStructMember ma1(CommonStructMember(1, IS_KEY, TypeIdentifier(TK_UINT8)),
                          MinimalMemberDetail("m1"));
  MinimalStructMember mb1(CommonStructMember(1, StructMemberFlag(), TypeIdentifier(TK_CHAR8)),
                          MinimalMemberDetail("m1"));
  a.member_seq.append(ma1);
  b.member_seq.append(mb1);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(a)), TypeObject(MinimalTypeObject(b))));

  // Struct key members
  MinimalStructType inner_a, inner_b;
  inner_a.struct_flags = IS_FINAL;
  inner_b.struct_flags = inner_a.struct_flags;
  inner_a.member_seq.append(MinimalStructMember(CommonStructMember(1, IS_KEY, TypeIdentifier(TK_FLOAT128)),
                                                MinimalMemberDetail("inner_m1")));
  inner_a.member_seq.append(MinimalStructMember(CommonStructMember(2, StructMemberFlag(),
                                                                   TypeIdentifier::makeString(false,
                                                                                              StringSTypeDefn(100))),
                                                MinimalMemberDetail("inner_m2")));
  inner_b.member_seq.append(MinimalStructMember(CommonStructMember(1, IS_KEY, TypeIdentifier(TK_INT16)),
                                                MinimalMemberDetail("inner_m1")));
  inner_b.member_seq.append(MinimalStructMember(CommonStructMember(2, StructMemberFlag(),
                                                                   TypeIdentifier::makeString(false,
                                                                                              StringLTypeDefn(50))),
                                                MinimalMemberDetail("inner_m2")));
  EquivalenceHash hash;
  TypeLookup::get_equivalence_hash(hash);
  MinimalStructMember ma2(CommonStructMember(2, StructMemberFlag(), TypeIdentifier::make(EK_MINIMAL, hash)),
                          MinimalMemberDetail("m2"));
  TypeLookup::insert_entry(ma2.common.member_type_id, MinimalTypeObject(inner_a));
  TypeLookup::get_equivalence_hash(hash);
  MinimalStructMember mb2(CommonStructMember(2, IS_KEY, TypeIdentifier::make(EK_MINIMAL, hash)),
                          MinimalMemberDetail("m2"));
  TypeLookup::insert_entry(mb2.common.member_type_id, MinimalTypeObject(inner_b));
  a.member_seq.append(ma2);
  b.member_seq.append(mb2);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(a)), TypeObject(MinimalTypeObject(b))));

  // Members for which both optional is false and must_understand is true in either
  // T1 or T2 appear in both T1 and T2
  a.member_seq.members[1].common.member_flags &= ~IS_OPTIONAL;
  a.member_seq.members[1].common.member_flags |= IS_MUST_UNDERSTAND;
  b.member_seq.members[0].common.member_flags &= ~IS_OPTIONAL;
  b.member_seq.members[0].common.member_flags |= IS_MUST_UNDERSTAND;
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(a)), TypeObject(MinimalTypeObject(b))));

  // String key members
  MinimalStructMember ma3(CommonStructMember(3, StructMemberFlag(),
                                             TypeIdentifier::makeString(true, StringSTypeDefn(120))),
                          MinimalMemberDetail("m3"));
  MinimalStructMember mb3(CommonStructMember(3, IS_KEY, TypeIdentifier::makeString(true, StringLTypeDefn(100))),
                          MinimalMemberDetail("m3"));
  a.member_seq.append(ma3);
  b.member_seq.append(mb3);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(a)), TypeObject(MinimalTypeObject(b))));

  // Enumerated key members
  MinimalEnumeratedLiteralSeq literal_seq_a, literal_seq_b;
  literal_seq_a.append(MinimalEnumeratedLiteral(CommonEnumeratedLiteral(1, IS_DEFAULT),
                                                MinimalMemberDetail("LITERAL1")));
  literal_seq_a.append(MinimalEnumeratedLiteral(CommonEnumeratedLiteral(2, EnumeratedLiteralFlag()),
                                                MinimalMemberDetail("LITERAL2")));
  literal_seq_a.append(MinimalEnumeratedLiteral(CommonEnumeratedLiteral(3, EnumeratedLiteralFlag()),
                                                MinimalMemberDetail("LITERAL3")));
  literal_seq_a.append(MinimalEnumeratedLiteral(CommonEnumeratedLiteral(4, EnumeratedLiteralFlag()),
                                                MinimalMemberDetail("LITERAL4")));
  literal_seq_b.append(MinimalEnumeratedLiteral(CommonEnumeratedLiteral(2, IS_DEFAULT),
                                                MinimalMemberDetail("LITERAL2")));
  literal_seq_b.append(MinimalEnumeratedLiteral(CommonEnumeratedLiteral(1, EnumeratedLiteralFlag()),
                                                MinimalMemberDetail("LITERAL1")));
  literal_seq_b.append(MinimalEnumeratedLiteral(CommonEnumeratedLiteral(3, EnumeratedLiteralFlag()),
                                                MinimalMemberDetail("LITERAL3")));
  MinimalEnumeratedType enum_a(IS_APPENDABLE, MinimalEnumeratedHeader(CommonEnumeratedHeader(3)), literal_seq_a);
  MinimalEnumeratedType enum_b(EnumTypeFlag(), MinimalEnumeratedHeader(CommonEnumeratedHeader(3)), literal_seq_b);
  TypeLookup::get_equivalence_hash(hash);
  MinimalStructMember ma4(CommonStructMember(4, StructMemberFlag(), TypeIdentifier::make(EK_MINIMAL, hash)),
                          MinimalMemberDetail("m4"));
  TypeLookup::insert_entry(ma4.common.member_type_id, MinimalTypeObject(enum_a));
  TypeLookup::get_equivalence_hash(hash);
  MinimalStructMember mb4(CommonStructMember(4, IS_KEY, TypeIdentifier::make(EK_MINIMAL, hash)),
                          MinimalMemberDetail("m4"));
  TypeLookup::insert_entry(mb4.common.member_type_id, MinimalTypeObject(enum_b));
  a.member_seq.append(ma4);
  b.member_seq.append(mb4);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(a)), TypeObject(MinimalTypeObject(b))));

  // Sequence key members
  MinimalSequenceType seq_a, seq_b;
  seq_a.header.common.bound = 100;
  seq_a.element.common.type = TypeIdentifier(TK_UINT64);
  seq_b.header.common.bound = 60;
  seq_b.element.common.type = TypeIdentifier(TK_INT64);
  TypeLookup::get_equivalence_hash(hash);
  MinimalStructMember ma5(CommonStructMember(5, StructMemberFlag(), TypeIdentifier::make(EK_MINIMAL, hash)),
                          MinimalMemberDetail("m5"));
  TypeLookup::insert_entry(ma5.common.member_type_id, MinimalTypeObject(seq_a));
  TypeLookup::get_equivalence_hash(hash);
  MinimalStructMember mb5(CommonStructMember(5, IS_KEY, TypeIdentifier::make(EK_MINIMAL, hash)),
                          MinimalMemberDetail("m5"));
  TypeLookup::insert_entry(mb5.common.member_type_id, MinimalTypeObject(seq_b));
  a.member_seq.append(ma5);
  b.member_seq.append(mb5);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(a)), TypeObject(MinimalTypeObject(b))));

  // Plain sequence key members
  MinimalSequenceType seq_a2;
  seq_a2.header.common.bound = 120;
  seq_a2.element.common.type = TypeIdentifier(TK_FLOAT64);
  TypeLookup::get_equivalence_hash(hash);
  MinimalStructMember ma6(CommonStructMember(6, StructMemberFlag(), TypeIdentifier::make(EK_MINIMAL, hash)),
                          MinimalMemberDetail("m6"));
  TypeLookup::insert_entry(ma6.common.member_type_id, MinimalTypeObject(seq_a));
  MinimalStructMember mb6(CommonStructMember(6, IS_KEY,
                                             TypeIdentifier::makePlainSequence(TypeIdentifier(TK_UINT32),
                                                                               static_cast<SBound>(70))),
                          MinimalMemberDetail("m6"));
  a.member_seq.append(ma6);
  b.member_seq.append(mb6);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(a)), TypeObject(MinimalTypeObject(b))));

  // Map key members
  MinimalMapType map_a, map_b;
  map_a.header.common.bound = 200;
  map_a.key.common.type = TypeIdentifier(TK_UINT32);
  map_a.element.common.type = TypeIdentifier(TK_FLOAT64);
  map_b.header.common.bound = 150;
  map_b.key.common.type = TypeIdentifier(TK_UINT64);
  map_b.element.common.type = TypeIdentifier(TK_FLOAT128);
  TypeLookup::get_equivalence_hash(hash);
  MinimalStructMember ma7(CommonStructMember(7, StructMemberFlag(), TypeIdentifier::make(EK_MINIMAL, hash)),
                          MinimalMemberDetail("m7"));
  TypeLookup::insert_entry(ma7.common.member_type_id, MinimalTypeObject(map_a));
  TypeLookup::get_equivalence_hash(hash);
  MinimalStructMember mb7(CommonStructMember(7, IS_KEY, TypeIdentifier::make(EK_MINIMAL, hash)),
                          MinimalMemberDetail("m7"));
  TypeLookup::insert_entry(mb7.common.member_type_id, MinimalTypeObject(map_b));
  a.member_seq.append(ma7);
  b.member_seq.append(mb7);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(a)), TypeObject(MinimalTypeObject(b))));

  // Plain map key members
  MinimalMapType map_a2;
  map_a2.header.common.bound = 200;
  map_a2.key.common.type = TypeIdentifier(TK_INT32);
  map_a2.element.common.type = TypeIdentifier(TK_FLOAT32);
  TypeLookup::get_equivalence_hash(hash);
  MinimalStructMember ma8(CommonStructMember(8, StructMemberFlag(), TypeIdentifier::make(EK_MINIMAL, hash)),
                          MinimalMemberDetail("m8"));
  TypeLookup::insert_entry(ma8.common.member_type_id, MinimalTypeObject(map_a2));
  PlainMapLTypeDefn plain_map(PlainCollectionHeader(EK_MINIMAL, CollectionElementFlag()), 160,
                              TypeIdentifier(TK_CHAR16), CollectionElementFlag(),
                              TypeIdentifier(TK_UINT32));
  MinimalStructMember mb8(CommonStructMember(8, IS_KEY, TypeIdentifier::make(TI_PLAIN_MAP_LARGE, plain_map)),
                          MinimalMemberDetail("m8"));
  a.member_seq.append(ma8);
  b.member_seq.append(mb8);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(a)), TypeObject(MinimalTypeObject(b))));

  // Union key members
  MinimalStructType a2, b2;
  MinimalUnionType uni_a, uni_b;
  uni_a.discriminator.common.type_id = TypeIdentifier(TK_CHAR8);
  uni_b.discriminator.common.type_id = TypeIdentifier(TK_CHAR8);
  uni_a.discriminator.common.member_flags = 0;
  uni_b.discriminator.common.member_flags = 0;
  uni_a.member_seq.append(MinimalUnionMember(CommonUnionMember(1, UnionMemberFlag(),
                                                               TypeIdentifier::makeString(false, StringLTypeDefn(120)),
                                                               UnionCaseLabelSeq().append(1).append(2).append(3)),
                                             MinimalMemberDetail("inner1")));
  uni_a.member_seq.append(MinimalUnionMember(CommonUnionMember(2, IS_DEFAULT,
                                                               TypeIdentifier::makeString(false, StringSTypeDefn(100)),
                                                               UnionCaseLabelSeq().append(4).append(5).append(6)),
                                             MinimalMemberDetail("inner2")));
  uni_b.discriminator.common.type_id = TypeIdentifier(TK_CHAR8);
  uni_b.member_seq.append(MinimalUnionMember(CommonUnionMember(1, IS_DEFAULT,
                                                               TypeIdentifier::makeString(false, StringSTypeDefn(130)),
                                                               UnionCaseLabelSeq().append(1).append(2)),
                                             MinimalMemberDetail("inner1")));
  uni_b.member_seq.append(MinimalUnionMember(CommonUnionMember(2, UnionMemberFlag(),
                                                               TypeIdentifier::makeString(false, StringLTypeDefn(150)),
                                                               UnionCaseLabelSeq().append(3).append(4)),
                                             MinimalMemberDetail("inner2")));
  TypeLookup::get_equivalence_hash(hash);
  MinimalStructMember ma9(CommonStructMember(9, StructMemberFlag(), TypeIdentifier::make(EK_MINIMAL, hash)),
                          MinimalMemberDetail("m9"));
  TypeLookup::insert_entry(ma9.common.member_type_id, MinimalTypeObject(uni_a));
  TypeLookup::get_equivalence_hash(hash);
  MinimalStructMember mb9(CommonStructMember(9, IS_KEY, TypeIdentifier::make(EK_MINIMAL, hash)),
                          MinimalMemberDetail("m9"));
  TypeLookup::insert_entry(mb9.common.member_type_id, MinimalTypeObject(uni_b));
  a2.member_seq.append(ma9);
  b2.member_seq.append(mb9);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(a2)), TypeObject(MinimalTypeObject(b2))));
}

void expect_false_different_extensibilities()
{
  TypeAssignability test;
  MinimalStructType a, b;

  // Different extensibility kinds
  a.struct_flags = IS_APPENDABLE;
  b.struct_flags = IS_FINAL;
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(a)), TypeObject(MinimalTypeObject(b))));
}

void expect_false_different_ids()
{
  TypeAssignability test;
  MinimalStructType a, b;

  // Some members with the same name but different ID
  a.struct_flags = IS_MUTABLE;
  b.struct_flags = a.struct_flags;
  MinimalStructMember ma1(CommonStructMember(1, IS_KEY, TypeIdentifier(TK_UINT8)),
                          MinimalMemberDetail("m1"));
  MinimalStructMember mb1(CommonStructMember(13, StructMemberFlag(), TypeIdentifier(TK_CHAR8)),
                          MinimalMemberDetail("m1"));
  a.member_seq.append(ma1);
  b.member_seq.append(mb1);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(a)), TypeObject(MinimalTypeObject(b))));
}

void expect_false_different_names()
{
  TypeAssignability test;
  MinimalStructType a, b;

  // Some members with the same name but different ID
  a.struct_flags = IS_FINAL;
  b.struct_flags = a.struct_flags;
  MinimalStructMember ma1(CommonStructMember(1, StructMemberFlag(), TypeIdentifier(TK_UINT8)),
                          MinimalMemberDetail("m1"));
  MinimalStructMember mb1(CommonStructMember(1, IS_KEY, TypeIdentifier(TK_CHAR8)),
                          MinimalMemberDetail("m10"));
  a.member_seq.append(ma1);
  b.member_seq.append(mb1);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(a)), TypeObject(MinimalTypeObject(b))));
}

void expect_false_no_matched_member()
{
  TypeAssignability test;
  MinimalStructType a, b;

  // There no members with the same ID
  a.struct_flags = IS_FINAL;
  b.struct_flags = a.struct_flags;
  MinimalStructMember ma1(CommonStructMember(1, StructMemberFlag(), TypeIdentifier(TK_UINT8)),
                          MinimalMemberDetail("m1"));
  MinimalStructMember ma2(CommonStructMember(2, StructMemberFlag(), TypeIdentifier(TK_FLOAT64)),
                          MinimalMemberDetail("m2"));
  MinimalStructMember mb1(CommonStructMember(10, IS_KEY, TypeIdentifier(TK_CHAR8)),
                          MinimalMemberDetail("m10"));
  MinimalStructMember mb2(CommonStructMember(20, StructMemberFlag(), TypeIdentifier(TK_INT64)),
                          MinimalMemberDetail("m20"));
  a.member_seq.append(ma1).append(ma2);
  b.member_seq.append(mb1).append(mb2);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(a)), TypeObject(MinimalTypeObject(b))));
}

void expect_false_key_erased()
{
  TypeAssignability test;
  MinimalStructType a, b;

  // KeyErased of members are not assignable
  MinimalStructType inner_a, inner_b;
  inner_a.struct_flags = IS_FINAL;
  inner_b.struct_flags = inner_a.struct_flags;
  inner_a.member_seq.append(MinimalStructMember(CommonStructMember(1, IS_KEY, TypeIdentifier(TK_FLOAT128)),
                                                MinimalMemberDetail("inner_m1")));
  inner_a.member_seq.append(MinimalStructMember(CommonStructMember(2, StructMemberFlag(),
                                                                   TypeIdentifier::makeString(false,
                                                                                              StringSTypeDefn(100))),
                                                MinimalMemberDetail("inner_m2")));
  inner_b.member_seq.append(MinimalStructMember(CommonStructMember(1, IS_KEY, TypeIdentifier(TK_INT16)),
                                                MinimalMemberDetail("inner_m1")));
  inner_b.member_seq.append(MinimalStructMember(CommonStructMember(3, StructMemberFlag(),
                                                                   TypeIdentifier::makeString(true,
                                                                                              StringLTypeDefn(50))),
                                                MinimalMemberDetail("inner_m2")));
  EquivalenceHash hash;
  TypeLookup::get_equivalence_hash(hash);
  MinimalStructMember ma1(CommonStructMember(1, StructMemberFlag(), TypeIdentifier::make(EK_MINIMAL, hash)),
                          MinimalMemberDetail("m1"));
  TypeLookup::insert_entry(ma1.common.member_type_id, MinimalTypeObject(inner_a));
  TypeLookup::get_equivalence_hash(hash);
  MinimalStructMember mb1(CommonStructMember(1, IS_KEY, TypeIdentifier::make(EK_MINIMAL, hash)),
                          MinimalMemberDetail("m1"));
  TypeLookup::insert_entry(mb1.common.member_type_id, MinimalTypeObject(inner_b));
  a.member_seq.append(ma1);
  b.member_seq.append(mb1);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(a)), TypeObject(MinimalTypeObject(b))));

  MinimalStructType a2, b2;
  MinimalUnionType uni_a, uni_b;
  uni_a.discriminator.common.member_flags = IS_KEY;
  uni_a.discriminator.common.type_id = TypeIdentifier(TK_CHAR8);
  uni_a.member_seq.append(MinimalUnionMember(CommonUnionMember(1, UnionMemberFlag(),
                                                               TypeIdentifier::makeString(false, StringLTypeDefn(120)),
                                                               UnionCaseLabelSeq().append(1).append(2).append(3)),
                                             MinimalMemberDetail("inner1")));
  uni_a.member_seq.append(MinimalUnionMember(CommonUnionMember(2, IS_DEFAULT,
                                                               TypeIdentifier::makeString(false, StringSTypeDefn(100)),
                                                               UnionCaseLabelSeq().append(4).append(5).append(6)),
                                             MinimalMemberDetail("inner2")));
  uni_b.discriminator.common.member_flags = IS_KEY;
  uni_b.discriminator.common.type_id = TypeIdentifier(TK_BYTE);
  uni_b.member_seq.append(MinimalUnionMember(CommonUnionMember(1, IS_DEFAULT,
                                                               TypeIdentifier::makeString(false, StringSTypeDefn(130)),
                                                               UnionCaseLabelSeq().append(1).append(2)),
                                             MinimalMemberDetail("inner1")));
  uni_b.member_seq.append(MinimalUnionMember(CommonUnionMember(2, UnionMemberFlag(),
                                                               TypeIdentifier::makeString(false, StringLTypeDefn(150)),
                                                               UnionCaseLabelSeq().append(3).append(4)),
                                             MinimalMemberDetail("inner2")));
  TypeLookup::get_equivalence_hash(hash);
  MinimalStructMember ma9(CommonStructMember(9, StructMemberFlag(), TypeIdentifier::make(EK_MINIMAL, hash)),
                          MinimalMemberDetail("m9"));
  TypeLookup::insert_entry(ma9.common.member_type_id, MinimalTypeObject(uni_a));
  TypeLookup::get_equivalence_hash(hash);
  MinimalStructMember mb9(CommonStructMember(9, IS_KEY, TypeIdentifier::make(EK_MINIMAL, hash)),
                          MinimalMemberDetail("m9"));
  TypeLookup::insert_entry(mb9.common.member_type_id, MinimalTypeObject(uni_b));
  a2.member_seq.append(ma9);
  b2.member_seq.append(mb9);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(a2)), TypeObject(MinimalTypeObject(b2))));
}

void expect_false_not_optional_must_understand()
{
  TypeAssignability test;
  MinimalStructType a, b;

  a.struct_flags = IS_MUTABLE;
  b.struct_flags = a.struct_flags;
  MinimalStructMember ma1(CommonStructMember(1, StructMemberFlag(), TypeIdentifier(TK_UINT8)),
                          MinimalMemberDetail("m1"));
  MinimalStructMember ma2(CommonStructMember(2, StructMemberFlag(), TypeIdentifier(TK_FLOAT32)),
                          MinimalMemberDetail("m2"));
  MinimalStructMember mb1(CommonStructMember(1, IS_KEY, TypeIdentifier(TK_BYTE)),
                          MinimalMemberDetail("m1"));
  MinimalStructMember mb2(CommonStructMember(3, IS_KEY, TypeIdentifier(TK_UINT32)),
                          MinimalMemberDetail("m3"));
  a.member_seq.append(ma1).append(ma2);
  b.member_seq.append(mb1).append(mb2);
  a.member_seq.members[1].common.member_flags &= ~IS_OPTIONAL;
  a.member_seq.members[1].common.member_flags |= IS_MUST_UNDERSTAND;
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(a)), TypeObject(MinimalTypeObject(b))));
}

void expect_false_keys_must_in_both()
{
  TypeAssignability test;
  MinimalStructType a, b;

  a.struct_flags = IS_MUTABLE;
  b.struct_flags = a.struct_flags;
  MinimalStructMember ma1(CommonStructMember(1, IS_KEY, TypeIdentifier(TK_UINT8)),
                          MinimalMemberDetail("m1"));
  MinimalStructMember ma2(CommonStructMember(2, StructMemberFlag(), TypeIdentifier(TK_FLOAT32)),
                          MinimalMemberDetail("m2"));
  MinimalStructMember mb1(CommonStructMember(1, StructMemberFlag(), TypeIdentifier(TK_BYTE)),
                          MinimalMemberDetail("m1"));
  MinimalStructMember mb2(CommonStructMember(3, IS_KEY, TypeIdentifier(TK_UINT32)),
                          MinimalMemberDetail("m3"));
  a.member_seq.append(ma1).append(ma2);
  b.member_seq.append(mb1).append(mb2);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(a)), TypeObject(MinimalTypeObject(b))));
}

void expect_false_string_keys()
{
  TypeAssignability test;
  MinimalStructType a, b;

  MinimalStructMember ma1(CommonStructMember(1, StructMemberFlag(),
                                             TypeIdentifier::makeString(true, StringSTypeDefn(120))),
                          MinimalMemberDetail("m1"));
  MinimalStructMember mb1(CommonStructMember(1, IS_KEY,
                                             TypeIdentifier::makeString(true, StringLTypeDefn(200))),
                          MinimalMemberDetail("m1"));
  a.member_seq.append(ma1);
  b.member_seq.append(mb1);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(a)), TypeObject(MinimalTypeObject(b))));
}

void expect_false_enum_keys()
{
  TypeAssignability test;
  MinimalStructType a, b;

  MinimalEnumeratedLiteralSeq literal_seq_a, literal_seq_b;
  literal_seq_a.append(MinimalEnumeratedLiteral(CommonEnumeratedLiteral(1, IS_DEFAULT),
                                                MinimalMemberDetail("LITERAL1")));
  literal_seq_a.append(MinimalEnumeratedLiteral(CommonEnumeratedLiteral(2, EnumeratedLiteralFlag()),
                                                MinimalMemberDetail("LITERAL2")));
  literal_seq_a.append(MinimalEnumeratedLiteral(CommonEnumeratedLiteral(4, EnumeratedLiteralFlag()),
                                                MinimalMemberDetail("LITERAL4")));
  literal_seq_b.append(MinimalEnumeratedLiteral(CommonEnumeratedLiteral(2, IS_DEFAULT),
                                                MinimalMemberDetail("LITERAL2")));
  literal_seq_b.append(MinimalEnumeratedLiteral(CommonEnumeratedLiteral(1, EnumeratedLiteralFlag()),
                                                MinimalMemberDetail("LITERAL1")));
  literal_seq_b.append(MinimalEnumeratedLiteral(CommonEnumeratedLiteral(3, EnumeratedLiteralFlag()),
                                                MinimalMemberDetail("LITERAL3")));
  MinimalEnumeratedType enum_a(IS_APPENDABLE, MinimalEnumeratedHeader(CommonEnumeratedHeader(3)), literal_seq_a);
  MinimalEnumeratedType enum_b(EnumTypeFlag(), MinimalEnumeratedHeader(CommonEnumeratedHeader(3)), literal_seq_b);
  EquivalenceHash hash;
  TypeLookup::get_equivalence_hash(hash);
  MinimalStructMember ma1(CommonStructMember(1, StructMemberFlag(), TypeIdentifier::make(EK_MINIMAL, hash)),
                          MinimalMemberDetail("m1"));
  TypeLookup::insert_entry(ma1.common.member_type_id, MinimalTypeObject(enum_a));
  TypeLookup::get_equivalence_hash(hash);
  MinimalStructMember mb1(CommonStructMember(1, IS_KEY, TypeIdentifier::make(EK_MINIMAL, hash)),
                          MinimalMemberDetail("m1"));
  TypeLookup::insert_entry(mb1.common.member_type_id, MinimalTypeObject(enum_b));
  a.member_seq.append(ma1);
  b.member_seq.append(mb1);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(a)), TypeObject(MinimalTypeObject(b))));
}

void expect_false_sequence_keys()
{
  TypeAssignability test;
  MinimalStructType a, b;

  // Sequence key members
  MinimalSequenceType seq_a, seq_b;
  seq_a.header.common.bound = 100;
  seq_a.element.common.type = TypeIdentifier(TK_UINT64);
  seq_b.header.common.bound = 600;
  seq_b.element.common.type = TypeIdentifier(TK_INT64);
  EquivalenceHash hash;
  TypeLookup::get_equivalence_hash(hash);
  MinimalStructMember ma5(CommonStructMember(5, StructMemberFlag(), TypeIdentifier::make(EK_MINIMAL, hash)),
                          MinimalMemberDetail("m5"));
  TypeLookup::insert_entry(ma5.common.member_type_id, MinimalTypeObject(seq_a));
  TypeLookup::get_equivalence_hash(hash);
  MinimalStructMember mb5(CommonStructMember(5, IS_KEY, TypeIdentifier::make(EK_MINIMAL, hash)),
                          MinimalMemberDetail("m5"));
  TypeLookup::insert_entry(mb5.common.member_type_id, MinimalTypeObject(seq_b));
  a.member_seq.append(ma5);
  b.member_seq.append(mb5);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(a)), TypeObject(MinimalTypeObject(b))));

  // Plain sequence key members
  MinimalStructType a2, b2;
  MinimalSequenceType seq_a2;
  seq_a2.header.common.bound = 120;
  seq_a2.element.common.type = TypeIdentifier(TK_FLOAT64);
  TypeLookup::get_equivalence_hash(hash);
  MinimalStructMember ma6(CommonStructMember(6, StructMemberFlag(), TypeIdentifier::make(EK_MINIMAL, hash)),
                          MinimalMemberDetail("m6"));
  TypeLookup::insert_entry(ma6.common.member_type_id, MinimalTypeObject(seq_a));
  MinimalStructMember mb6(CommonStructMember(6, IS_KEY,
                                             TypeIdentifier::makePlainSequence(TypeIdentifier(TK_UINT32),
                                                                               static_cast<LBound>(200))),
                          MinimalMemberDetail("m6"));
  a2.member_seq.append(ma6);
  b2.member_seq.append(mb6);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(a2)), TypeObject(MinimalTypeObject(b2))));
}

void expect_false_map_keys()
{
  TypeAssignability test;
  MinimalStructType a, b;

  // Map key members
  MinimalMapType map_a, map_b;
  map_a.header.common.bound = 200;
  map_a.key.common.type = TypeIdentifier(TK_UINT32);
  map_a.element.common.type = TypeIdentifier(TK_FLOAT64);
  map_b.header.common.bound = 500;
  map_b.key.common.type = TypeIdentifier(TK_UINT64);
  map_b.element.common.type = TypeIdentifier(TK_FLOAT128);
  EquivalenceHash hash;
  TypeLookup::get_equivalence_hash(hash);
  MinimalStructMember ma7(CommonStructMember(7, StructMemberFlag(), TypeIdentifier::make(EK_MINIMAL, hash)),
                          MinimalMemberDetail("m7"));
  TypeLookup::insert_entry(ma7.common.member_type_id, MinimalTypeObject(map_a));
  TypeLookup::get_equivalence_hash(hash);
  MinimalStructMember mb7(CommonStructMember(7, IS_KEY, TypeIdentifier::make(EK_MINIMAL, hash)),
                          MinimalMemberDetail("m7"));
  TypeLookup::insert_entry(mb7.common.member_type_id, MinimalTypeObject(map_b));
  a.member_seq.append(ma7);
  b.member_seq.append(mb7);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(a)), TypeObject(MinimalTypeObject(b))));

  // Plain map key members
  MinimalStructType a2, b2;
  MinimalMapType map_a2;
  map_a2.header.common.bound = 200;
  map_a2.key.common.type = TypeIdentifier(TK_INT32);
  map_a2.element.common.type = TypeIdentifier(TK_FLOAT32);
  TypeLookup::get_equivalence_hash(hash);
  MinimalStructMember ma8(CommonStructMember(8, StructMemberFlag(), TypeIdentifier::make(EK_MINIMAL, hash)),
                          MinimalMemberDetail("m8"));
  TypeLookup::insert_entry(ma8.common.member_type_id, MinimalTypeObject(map_a2));
  PlainMapLTypeDefn plain_map(PlainCollectionHeader(EK_MINIMAL, CollectionElementFlag()), 300,
                              TypeIdentifier(TK_CHAR16), CollectionElementFlag(),
                              TypeIdentifier(TK_UINT32));
  MinimalStructMember mb8(CommonStructMember(8, IS_KEY, TypeIdentifier::make(TI_PLAIN_MAP_LARGE, plain_map)),
                          MinimalMemberDetail("m8"));
  a2.member_seq.append(ma8);
  b2.member_seq.append(mb8);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(a2)), TypeObject(MinimalTypeObject(b2))));
}

void expect_false_key_holder()
{
  TypeAssignability test;
  MinimalStructType a, b;

  MinimalStructType inner_a, inner_b;
  inner_a.struct_flags = IS_FINAL;
  inner_b.struct_flags = inner_a.struct_flags;
  inner_a.member_seq.append(MinimalStructMember(CommonStructMember(1, IS_KEY, TypeIdentifier(TK_FLOAT128)),
                                                MinimalMemberDetail("inner_m1")));
  inner_a.member_seq.append(MinimalStructMember(CommonStructMember(2, StructMemberFlag(),
                                                                   TypeIdentifier::makeString(false,
                                                                                              StringSTypeDefn(100))),
                                                MinimalMemberDetail("inner_m2")));
  inner_b.member_seq.append(MinimalStructMember(CommonStructMember(10, IS_KEY, TypeIdentifier(TK_INT16)),
                                                MinimalMemberDetail("inner_m1")));
  inner_b.member_seq.append(MinimalStructMember(CommonStructMember(3, StructMemberFlag(),
                                                                   TypeIdentifier::makeString(true,
                                                                                              StringLTypeDefn(50))),
                                                MinimalMemberDetail("inner_m2")));
  EquivalenceHash hash;
  TypeLookup::get_equivalence_hash(hash);
  MinimalStructMember ma1(CommonStructMember(1, StructMemberFlag(), TypeIdentifier::make(EK_MINIMAL, hash)),
                          MinimalMemberDetail("m1"));
  TypeLookup::insert_entry(ma1.common.member_type_id, MinimalTypeObject(inner_a));
  TypeLookup::get_equivalence_hash(hash);
  MinimalStructMember mb1(CommonStructMember(1, IS_KEY, TypeIdentifier::make(EK_MINIMAL, hash)),
                          MinimalMemberDetail("m1"));
  TypeLookup::insert_entry(mb1.common.member_type_id, MinimalTypeObject(inner_b));
  a.member_seq.append(ma1);
  b.member_seq.append(mb1);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(a)), TypeObject(MinimalTypeObject(b))));

  MinimalStructType a2, b2;
  MinimalUnionType uni_a, uni_b;
  uni_a.discriminator.common.type_id = TypeIdentifier(TK_CHAR8);
  uni_a.discriminator.common.member_flags = 0;
  uni_a.member_seq.append(MinimalUnionMember(CommonUnionMember(1, UnionMemberFlag(),
                                                               TypeIdentifier::makeString(false, StringLTypeDefn(120)),
                                                               UnionCaseLabelSeq().append(1).append(2).append(3)),
                                             MinimalMemberDetail("inner1")));
  uni_a.member_seq.append(MinimalUnionMember(CommonUnionMember(2, IS_DEFAULT,
                                                               TypeIdentifier::makeString(false, StringSTypeDefn(100)),
                                                               UnionCaseLabelSeq().append(4).append(5).append(6)),
                                             MinimalMemberDetail("inner2")));
  uni_b.discriminator.common.type_id = TypeIdentifier(TK_CHAR8);
  uni_b.discriminator.common.member_flags = 0;
  uni_b.member_seq.append(MinimalUnionMember(CommonUnionMember(1, IS_DEFAULT,
                                                               TypeIdentifier::makeString(false, StringSTypeDefn(130)),
                                                               UnionCaseLabelSeq().append(10).append(20)),
                                             MinimalMemberDetail("inner1")));
  uni_b.member_seq.append(MinimalUnionMember(CommonUnionMember(2, UnionMemberFlag(),
                                                               TypeIdentifier::makeString(false, StringLTypeDefn(150)),
                                                               UnionCaseLabelSeq().append(30).append(40)),
                                             MinimalMemberDetail("inner2")));
  TypeLookup::get_equivalence_hash(hash);
  MinimalStructMember ma9(CommonStructMember(9, StructMemberFlag(), TypeIdentifier::make(EK_MINIMAL, hash)),
                          MinimalMemberDetail("m9"));
  TypeLookup::insert_entry(ma9.common.member_type_id, MinimalTypeObject(uni_a));
  TypeLookup::get_equivalence_hash(hash);
  MinimalStructMember mb9(CommonStructMember(9, IS_KEY, TypeIdentifier::make(EK_MINIMAL, hash)),
                          MinimalMemberDetail("m9"));
  TypeLookup::insert_entry(mb9.common.member_type_id, MinimalTypeObject(uni_b));
  a2.member_seq.append(ma9);
  b2.member_seq.append(mb9);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(a2)), TypeObject(MinimalTypeObject(b2))));
}

TEST(StructTypeTest, NotAssignable)
{
  expect_false_different_extensibilities();
  expect_false_different_ids();
  expect_false_different_names();
  expect_false_no_matched_member();
  expect_false_key_erased();
  expect_false_not_optional_must_understand();
  expect_false_keys_must_in_both();
  expect_false_string_keys();
  expect_false_enum_keys();
  expect_false_sequence_keys();
  expect_false_map_keys();
  expect_false_key_holder();
}

TEST(UnionTypeTest, Assignable)
{
  TypeAssignability test;
  MinimalUnionType a, b;

  // Extensibility
  a.union_flags = IS_FINAL;
  b.union_flags = a.union_flags;

  // Discriminator type must be strongly assignable
  a.discriminator.common.type_id = TypeIdentifier(TK_UINT16);
  b.discriminator.common.type_id = TypeIdentifier(TK_UINT16);

  // Either the discriminators of both are keys or neither are keys
  a.discriminator.common.member_flags = IS_KEY;
  b.discriminator.common.member_flags = IS_KEY;

  // Members that have the same ID also have the same name and vice versa
  MinimalUnionMember ma1(CommonUnionMember(1, UnionMemberFlag(), TypeIdentifier(TK_INT32),
                                           UnionCaseLabelSeq().append(10).append(20)),
                         MinimalMemberDetail("m1"));
  MinimalUnionMember ma2(CommonUnionMember(2, IS_DEFAULT, TypeIdentifier(TK_INT32),
                                           UnionCaseLabelSeq().append(30)),
                         MinimalMemberDetail("m2"));
  MinimalUnionMember mb1(CommonUnionMember(1, IS_DEFAULT, TypeIdentifier(TK_INT32),
                                           UnionCaseLabelSeq().append(20).append(30)),
                         MinimalMemberDetail("m1"));
  MinimalUnionMember mb2(CommonUnionMember(2, UnionMemberFlag(), TypeIdentifier(TK_INT32),
                                           UnionCaseLabelSeq().append(10)),
                         MinimalMemberDetail("m2"));
  a.member_seq.append(ma1).append(ma2);
  b.member_seq.append(mb1).append(mb2);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(a)), TypeObject(MinimalTypeObject(b))));

  // Non-default labels in T2 that select some member in T1
  MinimalUnionType a2, b2;
  a2.union_flags = IS_MUTABLE;
  b2.union_flags = a2.union_flags;
  a2.discriminator.common.type_id = TypeIdentifier(TK_BYTE);
  b2.discriminator.common.type_id = TypeIdentifier(TK_BYTE);
  a2.discriminator.common.member_flags = 0;
  b2.discriminator.common.member_flags = 0;
  MinimalUnionMember ma2_1(CommonUnionMember(1, UnionMemberFlag(),
                                             TypeIdentifier::makeString(true, StringSTypeDefn(60)),
                                             UnionCaseLabelSeq().append(10).append(20)),
                           MinimalMemberDetail("member1"));
  MinimalUnionMember mb2_1(CommonUnionMember(1, UnionMemberFlag(),
                                             TypeIdentifier::makeString(true, StringLTypeDefn(100)),
                                             UnionCaseLabelSeq().append(10)),
                           MinimalMemberDetail("member1"));
  a2.member_seq.append(ma2_1);
  b2.member_seq.append(mb2_1);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(a2)), TypeObject(MinimalTypeObject(b2))));

  // Non-default labels in T1 that select the default member in T2
  MinimalUnionType a3, b3;
  a3.union_flags = IS_APPENDABLE;
  b3.union_flags = IS_APPENDABLE;
  a3.discriminator.common.type_id = TypeIdentifier(TK_UINT32);
  b3.discriminator.common.type_id = TypeIdentifier(TK_UINT32);
  a3.discriminator.common.member_flags = IS_KEY;
  b3.discriminator.common.member_flags = IS_KEY;
  MinimalUnionMember ma3_1(CommonUnionMember(1, UnionMemberFlag(),
                                             TypeIdentifier::makeString(false, StringLTypeDefn(120)),
                                             UnionCaseLabelSeq().append(10).append(20)),
                           MinimalMemberDetail("member1"));
  MinimalUnionMember ma3_2(CommonUnionMember(2, UnionMemberFlag(), TypeIdentifier(TK_FLOAT32),
                                             UnionCaseLabelSeq().append(30)),
                           MinimalMemberDetail("member2"));
  MinimalUnionMember mb3_1(CommonUnionMember(1, IS_DEFAULT,
                                             TypeIdentifier::makeString(false, StringLTypeDefn(100)),
                                             UnionCaseLabelSeq().append(20)),
                           MinimalMemberDetail("member1"));
  MinimalUnionMember mb3_2(CommonUnionMember(4, UnionMemberFlag(), TypeIdentifier(TK_FLOAT32),
                                             UnionCaseLabelSeq().append(30)),
                           MinimalMemberDetail("member4"));
  a3.member_seq.append(ma3_1).append(ma3_2);
  b3.member_seq.append(mb3_1).append(mb3_2);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(a3)), TypeObject(MinimalTypeObject(b3))));

  // T1 and T2 both have default labels
  MinimalUnionType a4, b4;
  a4.union_flags = IS_APPENDABLE;
  b4.union_flags = IS_APPENDABLE;
  a4.discriminator.common.type_id = TypeIdentifier(TK_UINT16);
  b4.discriminator.common.type_id = TypeIdentifier(TK_UINT16);
  a4.discriminator.common.member_flags = 0;
  b4.discriminator.common.member_flags = 0;
  MinimalUnionMember ma4_1(CommonUnionMember(1, IS_DEFAULT,
                                             TypeIdentifier::makeString(true, StringLTypeDefn(220)),
                                             UnionCaseLabelSeq().append(10).append(20)),
                           MinimalMemberDetail("member1"));
  MinimalUnionMember ma4_2(CommonUnionMember(2, UnionMemberFlag(), TypeIdentifier(TK_FLOAT128),
                                             UnionCaseLabelSeq().append(40)),
                           MinimalMemberDetail("member2"));
  MinimalUnionMember mb4_1(CommonUnionMember(1, IS_DEFAULT,
                                             TypeIdentifier::makeString(true, StringLTypeDefn(120)),
                                             UnionCaseLabelSeq().append(20)),
                           MinimalMemberDetail("member1"));
  MinimalUnionMember mb4_2(CommonUnionMember(5, UnionMemberFlag(), TypeIdentifier(TK_FLOAT128),
                                             UnionCaseLabelSeq().append(40)),
                           MinimalMemberDetail("member5"));
  a4.member_seq.append(ma4_1).append(ma4_2);
  b4.member_seq.append(mb4_1).append(mb4_2);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(a4)), TypeObject(MinimalTypeObject(b4))));
}

TEST(UnionTypeTest, NotAssignable)
{
  TypeAssignability test;
  MinimalUnionType a, b;

  // Different extensibility kinds
  a.union_flags = IS_FINAL;
  b.union_flags = IS_APPENDABLE;
  a.discriminator.common.type_id = TypeIdentifier(TK_UINT16);
  b.discriminator.common.type_id = TypeIdentifier(TK_UINT16);
  a.discriminator.common.member_flags = IS_KEY;
  b.discriminator.common.member_flags = IS_KEY;

  MinimalUnionMember ma1(CommonUnionMember(1, UnionMemberFlag(), TypeIdentifier(TK_INT32),
                                           UnionCaseLabelSeq().append(10).append(20)),
                         MinimalMemberDetail("m1"));
  MinimalUnionMember ma2(CommonUnionMember(2, IS_DEFAULT, TypeIdentifier(TK_INT32),
                                           UnionCaseLabelSeq().append(30)),
                         MinimalMemberDetail("m2"));
  MinimalUnionMember mb1(CommonUnionMember(1, IS_DEFAULT, TypeIdentifier(TK_INT32),
                                           UnionCaseLabelSeq().append(20).append(30)),
                         MinimalMemberDetail("m1"));
  MinimalUnionMember mb2(CommonUnionMember(2, UnionMemberFlag(), TypeIdentifier(TK_INT32),
                                           UnionCaseLabelSeq().append(10)),
                         MinimalMemberDetail("m2"));
  a.member_seq.append(ma1).append(ma2);
  b.member_seq.append(mb1).append(mb2);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(a)), TypeObject(MinimalTypeObject(b))));

  // T1's discriminator type is not strongly assignable from T2's discriminator type
  MinimalUnionType a2, b2;
  a2.union_flags = IS_FINAL;
  b2.union_flags = IS_FINAL;
  a2.discriminator.common.type_id = TypeIdentifier(TK_UINT16);
  b2.discriminator.common.type_id = TypeIdentifier(TK_UINT64);
  a2.discriminator.common.member_flags = 0;
  b2.discriminator.common.member_flags = 0;

  MinimalUnionMember ma2_1(CommonUnionMember(1, UnionMemberFlag(), TypeIdentifier(TK_INT32),
                                             UnionCaseLabelSeq().append(10).append(20)),
                           MinimalMemberDetail("m1"));
  MinimalUnionMember ma2_2(CommonUnionMember(2, IS_DEFAULT, TypeIdentifier(TK_INT32),
                                             UnionCaseLabelSeq().append(30)),
                           MinimalMemberDetail("m2"));
  MinimalUnionMember mb2_1(CommonUnionMember(1, IS_DEFAULT, TypeIdentifier(TK_INT32),
                                             UnionCaseLabelSeq().append(20).append(30)),
                           MinimalMemberDetail("m1"));
  MinimalUnionMember mb2_2(CommonUnionMember(2, UnionMemberFlag(), TypeIdentifier(TK_INT32),
                                             UnionCaseLabelSeq().append(10)),
                           MinimalMemberDetail("m2"));
  a2.member_seq.append(ma2_1).append(ma2_2);
  b2.member_seq.append(mb2_1).append(mb2_2);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(a2)), TypeObject(MinimalTypeObject(b2))));

  // One discriminator is key, the other is not key
  MinimalUnionType a3, b3;
  a3.union_flags = IS_MUTABLE;
  b3.union_flags = IS_MUTABLE;
  a3.discriminator.common.type_id = TypeIdentifier(TK_UINT16);
  b3.discriminator.common.type_id = TypeIdentifier(TK_UINT16);
  a3.discriminator.common.member_flags = IS_KEY;
  b3.discriminator.common.member_flags = 0;

  MinimalUnionMember ma3_1(CommonUnionMember(1, UnionMemberFlag(), TypeIdentifier(TK_INT32),
                                             UnionCaseLabelSeq().append(10).append(20)),
                           MinimalMemberDetail("m1"));
  MinimalUnionMember ma3_2(CommonUnionMember(2, IS_DEFAULT, TypeIdentifier(TK_INT32),
                                             UnionCaseLabelSeq().append(30)),
                           MinimalMemberDetail("m2"));
  MinimalUnionMember mb3_1(CommonUnionMember(1, IS_DEFAULT, TypeIdentifier(TK_INT32),
                                             UnionCaseLabelSeq().append(20).append(30)),
                           MinimalMemberDetail("m1"));
  MinimalUnionMember mb3_2(CommonUnionMember(2, UnionMemberFlag(), TypeIdentifier(TK_INT32),
                                             UnionCaseLabelSeq().append(10)),
                           MinimalMemberDetail("m2"));
  a3.member_seq.append(ma3_1).append(ma3_2);
  b3.member_seq.append(mb3_1).append(mb3_2);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(a3)), TypeObject(MinimalTypeObject(b3))));

  // Some members with the same name have different IDs
  MinimalUnionType a4, b4;
  a4.union_flags = IS_MUTABLE;
  b4.union_flags = IS_MUTABLE;
  a4.discriminator.common.type_id = TypeIdentifier(TK_UINT16);
  b4.discriminator.common.type_id = TypeIdentifier(TK_UINT16);
  a4.discriminator.common.member_flags = 0;
  b4.discriminator.common.member_flags = 0;

  MinimalUnionMember ma4_1(CommonUnionMember(1, UnionMemberFlag(), TypeIdentifier(TK_INT32),
                                             UnionCaseLabelSeq().append(10).append(20)),
                           MinimalMemberDetail("member1"));
  MinimalUnionMember ma4_2(CommonUnionMember(2, IS_DEFAULT, TypeIdentifier(TK_INT32),
                                             UnionCaseLabelSeq().append(30)),
                           MinimalMemberDetail("member2"));
  MinimalUnionMember mb4_1(CommonUnionMember(10, IS_DEFAULT, TypeIdentifier(TK_INT32),
                                             UnionCaseLabelSeq().append(20).append(30)),
                           MinimalMemberDetail("member1"));
  MinimalUnionMember mb4_2(CommonUnionMember(2, UnionMemberFlag(), TypeIdentifier(TK_INT32),
                                             UnionCaseLabelSeq().append(10)),
                           MinimalMemberDetail("member2"));
  a4.member_seq.append(ma4_1).append(ma4_2);
  b4.member_seq.append(mb4_1).append(mb4_2);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(a4)), TypeObject(MinimalTypeObject(b4))));

  // Some members with the same ID have different names
  MinimalUnionType a5, b5;
  a5.union_flags = IS_MUTABLE;
  b5.union_flags = IS_MUTABLE;
  a5.discriminator.common.type_id = TypeIdentifier(TK_UINT16);
  b5.discriminator.common.type_id = TypeIdentifier(TK_UINT16);
  a5.discriminator.common.member_flags = IS_KEY;
  b5.discriminator.common.member_flags = IS_KEY;

  MinimalUnionMember ma5_1(CommonUnionMember(1, UnionMemberFlag(), TypeIdentifier(TK_INT32),
                                             UnionCaseLabelSeq().append(10).append(20)),
                           MinimalMemberDetail("member1"));
  MinimalUnionMember ma5_2(CommonUnionMember(2, IS_DEFAULT, TypeIdentifier(TK_INT32),
                                             UnionCaseLabelSeq().append(30)),
                           MinimalMemberDetail("member2"));
  MinimalUnionMember mb5_1(CommonUnionMember(1, IS_DEFAULT, TypeIdentifier(TK_INT32),
                                             UnionCaseLabelSeq().append(20).append(30)),
                           MinimalMemberDetail("member100"));
  MinimalUnionMember mb5_2(CommonUnionMember(2, UnionMemberFlag(), TypeIdentifier(TK_INT32),
                                             UnionCaseLabelSeq().append(10)),
                           MinimalMemberDetail("member2"));
  a5.member_seq.append(ma5_1).append(ma5_2);
  b5.member_seq.append(mb5_1).append(mb5_2);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(a5)), TypeObject(MinimalTypeObject(b5))));

  // Non-default labels in T2 that select some member in T1
  MinimalUnionType a6, b6;
  a6.union_flags = IS_MUTABLE;
  b6.union_flags = a6.union_flags;
  a6.discriminator.common.type_id = TypeIdentifier(TK_BYTE);
  b6.discriminator.common.type_id = TypeIdentifier(TK_BYTE);
  a6.discriminator.common.member_flags = IS_KEY;
  b6.discriminator.common.member_flags = IS_KEY;
  MinimalUnionMember ma6_1(CommonUnionMember(1, UnionMemberFlag(),
                                             TypeIdentifier::makeString(true, StringSTypeDefn(60)),
                                             UnionCaseLabelSeq().append(10).append(20)),
                           MinimalMemberDetail("member1"));
  MinimalUnionMember ma6_2(CommonUnionMember(2, UnionMemberFlag(), TypeIdentifier(TK_INT32),
                                             UnionCaseLabelSeq().append(30)),
                           MinimalMemberDetail("member2"));
  MinimalUnionMember mb6_1(CommonUnionMember(1, UnionMemberFlag(),
                                             TypeIdentifier::makeString(false, StringLTypeDefn(100)),
                                             UnionCaseLabelSeq().append(10)),
                           MinimalMemberDetail("member1"));
  MinimalUnionMember mb6_2(CommonUnionMember(2, UnionMemberFlag(), TypeIdentifier(TK_INT32),
                                             UnionCaseLabelSeq().append(30)),
                           MinimalMemberDetail("member2"));
  a6.member_seq.append(ma6_1).append(ma6_2);
  b6.member_seq.append(mb6_1).append(mb6_2);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(a6)), TypeObject(MinimalTypeObject(b6))));

  // Non-default labels in T1 that select the default member in T2
  MinimalUnionType a7, b7;
  a7.union_flags = IS_APPENDABLE;
  b7.union_flags = IS_APPENDABLE;
  a7.discriminator.common.type_id = TypeIdentifier(TK_UINT32);
  b7.discriminator.common.type_id = TypeIdentifier(TK_UINT32);
  a7.discriminator.common.member_flags = 0;
  b7.discriminator.common.member_flags = 0;
  MinimalUnionMember ma7_1(CommonUnionMember(1, UnionMemberFlag(),
                                             TypeIdentifier::makeString(false, StringLTypeDefn(120)),
                                             UnionCaseLabelSeq().append(10).append(20)),
                           MinimalMemberDetail("member1"));
  MinimalUnionMember ma7_2(CommonUnionMember(2, UnionMemberFlag(), TypeIdentifier(TK_FLOAT32),
                                             UnionCaseLabelSeq().append(30)),
                           MinimalMemberDetail("member2"));
  MinimalUnionMember mb7_1(CommonUnionMember(1, IS_DEFAULT,
                                             TypeIdentifier::makeString(true, StringLTypeDefn(100)),
                                             UnionCaseLabelSeq().append(20)),
                           MinimalMemberDetail("member1"));
  MinimalUnionMember mb7_2(CommonUnionMember(4, UnionMemberFlag(), TypeIdentifier(TK_FLOAT32),
                                             UnionCaseLabelSeq().append(30)),
                           MinimalMemberDetail("member4"));
  a7.member_seq.append(ma7_1).append(ma7_2);
  b7.member_seq.append(mb7_1).append(mb7_2);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(a7)), TypeObject(MinimalTypeObject(b7))));

  // T1 and T2 both have default labels
  MinimalUnionType a8, b8;
  a8.union_flags = IS_APPENDABLE;
  b8.union_flags = IS_APPENDABLE;
  a8.discriminator.common.type_id = TypeIdentifier(TK_UINT16);
  b8.discriminator.common.type_id = TypeIdentifier(TK_UINT16);
  a8.discriminator.common.member_flags = IS_KEY;
  b8.discriminator.common.member_flags = IS_KEY;
  MinimalUnionMember ma8_1(CommonUnionMember(1, IS_DEFAULT,
                                             TypeIdentifier::makeString(true, StringLTypeDefn(220)),
                                             UnionCaseLabelSeq().append(10).append(20)),
                           MinimalMemberDetail("member1"));
  MinimalUnionMember ma8_2(CommonUnionMember(2, UnionMemberFlag(), TypeIdentifier(TK_FLOAT128),
                                             UnionCaseLabelSeq().append(40)),
                           MinimalMemberDetail("member2"));
  MinimalUnionMember mb8_1(CommonUnionMember(1, IS_DEFAULT,
                                             TypeIdentifier::makeString(false, StringLTypeDefn(120)),
                                             UnionCaseLabelSeq().append(20)),
                           MinimalMemberDetail("member1"));
  MinimalUnionMember mb8_2(CommonUnionMember(5, UnionMemberFlag(), TypeIdentifier(TK_FLOAT128),
                                             UnionCaseLabelSeq().append(40)),
                           MinimalMemberDetail("member5"));
  a8.member_seq.append(ma8_1).append(ma8_2);
  b8.member_seq.append(mb8_1).append(mb8_2);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(a8)), TypeObject(MinimalTypeObject(b8))));

  // Extensibility is final
  MinimalUnionType a9, b9;
  a9.union_flags = IS_FINAL;
  b9.union_flags = IS_FINAL;
  a9.discriminator.common.type_id = TypeIdentifier(TK_UINT16);
  b9.discriminator.common.type_id = TypeIdentifier(TK_UINT16);
  a9.discriminator.common.member_flags = 0;
  b9.discriminator.common.member_flags = 0;
  MinimalUnionMember ma9_1(CommonUnionMember(1, IS_DEFAULT,
                                             TypeIdentifier::makeString(true, StringLTypeDefn(220)),
                                             UnionCaseLabelSeq().append(10).append(20)),
                           MinimalMemberDetail("member1"));
  MinimalUnionMember ma9_2(CommonUnionMember(2, UnionMemberFlag(), TypeIdentifier(TK_FLOAT128),
                                             UnionCaseLabelSeq().append(40)),
                           MinimalMemberDetail("member2"));
  MinimalUnionMember mb9_1(CommonUnionMember(1, IS_DEFAULT,
                                             TypeIdentifier::makeString(true, StringLTypeDefn(120)),
                                             UnionCaseLabelSeq().append(20)),
                           MinimalMemberDetail("member1"));
  MinimalUnionMember mb9_2(CommonUnionMember(5, UnionMemberFlag(), TypeIdentifier(TK_FLOAT128),
                                             UnionCaseLabelSeq().append(50)),
                           MinimalMemberDetail("member5"));
  a9.member_seq.append(ma9_1).append(ma9_2);
  b9.member_seq.append(mb9_1).append(mb9_2);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(a9)), TypeObject(MinimalTypeObject(b9))));

  // Extensibility is not final
  MinimalUnionType a10, b10;
  a10.union_flags = IS_MUTABLE;
  b10.union_flags = IS_MUTABLE;
  a10.discriminator.common.type_id = TypeIdentifier(TK_UINT16);
  b10.discriminator.common.type_id = TypeIdentifier(TK_UINT16);
  a10.discriminator.common.member_flags = 0;
  b10.discriminator.common.member_flags = 0;
  MinimalUnionMember ma10_1(CommonUnionMember(1, IS_DEFAULT,
                                              TypeIdentifier::makeString(true, StringLTypeDefn(220)),
                                              UnionCaseLabelSeq().append(10).append(20)),
                            MinimalMemberDetail("member1"));
  MinimalUnionMember mb10_1(CommonUnionMember(1, IS_DEFAULT,
                                              TypeIdentifier::makeString(true, StringLTypeDefn(120)),
                                              UnionCaseLabelSeq().append(20)),
                            MinimalMemberDetail("member1"));
  a10.member_seq.append(ma10_1);
  b10.member_seq.append(mb10_1);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(a10)), TypeObject(MinimalTypeObject(b10))));
}

int main(int argc, char* argv[])
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
