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

int main(int argc, char* argv[])
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
