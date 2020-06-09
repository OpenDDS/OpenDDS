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
  EXPECT_TRUE(test.assignable(tia, TypeObject(tob)));

  tia.kind = TK_UINT16;
  tob.bitmask_type.header.common.bit_bound = 16;
  EXPECT_TRUE(test.assignable(tia, TypeObject(tob)));

  tia.kind = TK_UINT32;
  tob.bitmask_type.header.common.bit_bound = 32;
  EXPECT_TRUE(test.assignable(tia, TypeObject(tob)));

  tia.kind = TK_UINT64;
  tob.bitmask_type.header.common.bit_bound = 64;
  EXPECT_TRUE(test.assignable(tia, TypeObject(tob)));
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
  EXPECT_FALSE(test.assignable(tia, TypeObject(tob)));

  tia.kind = TK_UINT16;
  tob.bitmask_type.header.common.bit_bound = 17;
  EXPECT_FALSE(test.assignable(tia, TypeObject(tob)));

  tia.kind = TK_UINT32;
  tob.bitmask_type.header.common.bit_bound = 33;
  EXPECT_FALSE(test.assignable(tia, TypeObject(tob)));

  tia.kind = TK_UINT64;
  tob.bitmask_type.header.common.bit_bound = 31;
  EXPECT_FALSE(test.assignable(tia, TypeObject(tob)));
}

TEST(StringTypesTest, Assignable)
{
  TypeAssignability test;
  TypeIdentifier tia, tib;
  tia.kind = TI_STRING8_SMALL;
  tib.kind = TI_STRING8_SMALL;
  EXPECT_TRUE(test.assignable(tia, tib));
  tib.kind = TI_STRING8_LARGE;
  EXPECT_TRUE(test.assignable(tia, tib));

  tia.kind = TI_STRING8_LARGE;
  EXPECT_TRUE(test.assignable(tia, tib));
  tib.kind = TI_STRING8_SMALL;
  EXPECT_TRUE(test.assignable(tia, tib));

  tia.kind = TI_STRING16_SMALL;
  tib.kind = TI_STRING16_SMALL;
  EXPECT_TRUE(test.assignable(tia, tib));
  tib.kind = TI_STRING16_LARGE;
  EXPECT_TRUE(test.assignable(tia, tib));

  tia.kind = TI_STRING16_LARGE;
  EXPECT_TRUE(test.assignable(tia, tib));
  tib.kind = TI_STRING16_SMALL;
  EXPECT_TRUE(test.assignable(tia, tib));
}

void string_expect_false(const TypeAssignability& test, const TypeIdentifier& tia, TypeIdentifier& tib)
{
  tib.kind = TK_BOOLEAN;
  EXPECT_FALSE(test.assignable(tia, tib));
  tib.kind = TK_BYTE;
  EXPECT_FALSE(test.assignable(tia, tib));
  tib.kind = TK_INT16;
  EXPECT_FALSE(test.assignable(tia, tib));
  tib.kind = TK_INT32;
  EXPECT_FALSE(test.assignable(tia, tib));
  tib.kind = TK_INT64;
  EXPECT_FALSE(test.assignable(tia, tib));
  tib.kind = TK_UINT16;
  EXPECT_FALSE(test.assignable(tia, tib));
  tib.kind = TK_UINT32;
  EXPECT_FALSE(test.assignable(tia, tib));
  tib.kind = TK_UINT64;
  EXPECT_FALSE(test.assignable(tia, tib));
  tib.kind = TK_FLOAT32;
  EXPECT_FALSE(test.assignable(tia, tib));
  tib.kind = TK_FLOAT64;
  EXPECT_FALSE(test.assignable(tia, tib));
  tib.kind = TK_FLOAT128;
  EXPECT_FALSE(test.assignable(tia, tib));
  tib.kind = TK_INT8;
  EXPECT_FALSE(test.assignable(tia, tib));
  tib.kind = TK_UINT8;
  EXPECT_FALSE(test.assignable(tia, tib));
  tib.kind = TK_CHAR8;
  EXPECT_FALSE(test.assignable(tia, tib));
  tib.kind = TK_CHAR16;
  EXPECT_FALSE(test.assignable(tia, tib));

  if (TI_STRING8_SMALL == tia.kind || TI_STRING8_LARGE == tia.kind) {
    tib.kind = TI_STRING16_SMALL;
    EXPECT_FALSE(test.assignable(tia, tib));
    tib.kind = TI_STRING16_LARGE;
    EXPECT_FALSE(test.assignable(tia, tib));
  } else {
    tib.kind = TI_STRING8_SMALL;
    EXPECT_FALSE(test.assignable(tia, tib));
    tib.kind = TI_STRING8_LARGE;
    EXPECT_FALSE(test.assignable(tia, tib));
  }

  tib.kind = TI_PLAIN_SEQUENCE_SMALL;
  EXPECT_FALSE(test.assignable(tia, tib));
  tib.kind = TI_PLAIN_SEQUENCE_LARGE;
  EXPECT_FALSE(test.assignable(tia, tib));
  tib.kind = TI_PLAIN_ARRAY_SMALL;
  EXPECT_FALSE(test.assignable(tia, tib));
  tib.kind = TI_PLAIN_ARRAY_LARGE;
  EXPECT_FALSE(test.assignable(tia, tib));
  tib.kind = TI_PLAIN_MAP_SMALL;
  EXPECT_FALSE(test.assignable(tia, tib));
  tib.kind = TI_PLAIN_MAP_LARGE;
  EXPECT_FALSE(test.assignable(tia, tib));
  tib.kind = TI_STRONGLY_CONNECTED_COMPONENT;
  EXPECT_FALSE(test.assignable(tia, tib));
  tib.kind = EK_COMPLETE;
  EXPECT_FALSE(test.assignable(tia, tib));
  tib.kind = EK_MINIMAL;
  EXPECT_FALSE(test.assignable(tia, tib));
}

TEST(StringTypesTest, NotAssignable)
{
  TypeAssignability test;
  TypeIdentifier tia, tib;
  tia.kind = TI_STRING8_SMALL;
  string_expect_false(test, tia, tib);
  tia.kind = TI_STRING8_LARGE;
  string_expect_false(test, tia, tib);
  tia.kind = TI_STRING16_SMALL;
  string_expect_false(test, tia, tib);
  tia.kind = TI_STRING16_LARGE;
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
  tib.kind = TK_BOOLEAN;
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(enum_a_)), tib));
  tib.kind = TK_BYTE;
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(enum_a_)), tib));
  tib.kind = TK_INT16;
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(enum_a_)), tib));
  tib.kind = TK_INT32;
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(enum_a_)), tib));
  tib.kind = TK_INT64;
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(enum_a_)), tib));
  tib.kind = TK_UINT16;
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(enum_a_)), tib));
  tib.kind = TK_UINT32;
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(enum_a_)), tib));
  tib.kind = TK_UINT64;
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(enum_a_)), tib));
  tib.kind = TK_FLOAT32;
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(enum_a_)), tib));
  tib.kind = TK_FLOAT64;
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(enum_a_)), tib));
  tib.kind = TK_FLOAT128;
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(enum_a_)), tib));
  tib.kind = TK_INT8;
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(enum_a_)), tib));
  tib.kind = TK_UINT8;
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(enum_a_)), tib));
  tib.kind = TK_CHAR8;
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(enum_a_)), tib));
  tib.kind = TK_CHAR16;
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(enum_a_)), tib));
  tib.kind = TI_STRING8_SMALL;
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(enum_a_)), tib));
  tib.kind = TI_STRING16_SMALL;
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(enum_a_)), tib));
  tib.kind = TI_STRING8_LARGE;
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(enum_a_)), tib));
  tib.kind = TI_STRING16_LARGE;
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(enum_a_)), tib));
  tib.kind = TI_PLAIN_SEQUENCE_SMALL;
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(enum_a_)), tib));
  tib.kind = TI_PLAIN_SEQUENCE_LARGE;
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(enum_a_)), tib));
  tib.kind = TI_PLAIN_ARRAY_SMALL;
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(enum_a_)), tib));
  tib.kind = TI_PLAIN_ARRAY_LARGE;
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(enum_a_)), tib));
  tib.kind = TI_PLAIN_MAP_SMALL;
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(enum_a_)), tib));
  tib.kind = TI_PLAIN_MAP_LARGE;
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(enum_a_)), tib));
  tib.kind = TI_STRONGLY_CONNECTED_COMPONENT;
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(enum_a_)), tib));
  tib.kind = EK_COMPLETE;
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
  tib.kind = TK_UINT8;
  tobj_a.bitmask_type.header.common.bit_bound = 6;
  EXPECT_TRUE(test.assignable(TypeObject(tobj_a), tib));

  tib.kind = TK_UINT16;
  tobj_a.bitmask_type.header.common.bit_bound = 13;
  EXPECT_TRUE(test.assignable(TypeObject(tobj_a), tib));

  tib.kind = TK_UINT32;
  tobj_a.bitmask_type.header.common.bit_bound = 30;
  EXPECT_TRUE(test.assignable(TypeObject(tobj_a), tib));

  tib.kind = TK_UINT64;
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
  tib.kind = TK_BOOLEAN;
  EXPECT_FALSE(test.assignable(TypeObject(tobj_a), tib));
  tib.kind = TK_BYTE;
  EXPECT_FALSE(test.assignable(TypeObject(tobj_a), tib));
  tib.kind = TK_INT16;
  EXPECT_FALSE(test.assignable(TypeObject(tobj_a), tib));
  tib.kind = TK_INT32;
  EXPECT_FALSE(test.assignable(TypeObject(tobj_a), tib));
  tib.kind = TK_INT64;
  EXPECT_FALSE(test.assignable(TypeObject(tobj_a), tib));

  tib.kind = TK_UINT16;
  bitmask_a.header.common.bit_bound = 17;
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(bitmask_a)), tib));
  tib.kind = TK_UINT32;
  bitmask_a.header.common.bit_bound = 33;
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(bitmask_a)), tib));
  tib.kind = TK_UINT64;
  bitmask_a.header.common.bit_bound = 25;
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(bitmask_a)), tib));

  tib.kind = TK_FLOAT32;
  EXPECT_FALSE(test.assignable(TypeObject(tobj_a), tib));
  tib.kind = TK_FLOAT64;
  EXPECT_FALSE(test.assignable(TypeObject(tobj_a), tib));
  tib.kind = TK_FLOAT128;
  EXPECT_FALSE(test.assignable(TypeObject(tobj_a), tib));
  tib.kind = TK_INT8;
  EXPECT_FALSE(test.assignable(TypeObject(tobj_a), tib));

  tib.kind = TK_UINT8;
  bitmask_a.header.common.bit_bound = 9;
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(bitmask_a)), tib));

  tib.kind = TK_CHAR8;
  EXPECT_FALSE(test.assignable(TypeObject(tobj_a), tib));
  tib.kind = TK_CHAR16;
  EXPECT_FALSE(test.assignable(TypeObject(tobj_a), tib));
  tib.kind = TI_STRING8_SMALL;
  EXPECT_FALSE(test.assignable(TypeObject(tobj_a), tib));
  tib.kind = TI_STRING8_LARGE;
  EXPECT_FALSE(test.assignable(TypeObject(tobj_a), tib));
  tib.kind = TI_STRING16_SMALL;
  EXPECT_FALSE(test.assignable(TypeObject(tobj_a), tib));
  tib.kind = TI_STRING16_LARGE;
  EXPECT_FALSE(test.assignable(TypeObject(tobj_a), tib));
  tib.kind = TI_PLAIN_SEQUENCE_SMALL;
  EXPECT_FALSE(test.assignable(TypeObject(tobj_a), tib));
  tib.kind = TI_PLAIN_SEQUENCE_LARGE;
  EXPECT_FALSE(test.assignable(TypeObject(tobj_a), tib));
  tib.kind = TI_PLAIN_ARRAY_SMALL;
  EXPECT_FALSE(test.assignable(TypeObject(tobj_a), tib));
  tib.kind = TI_PLAIN_ARRAY_LARGE;
  EXPECT_FALSE(test.assignable(TypeObject(tobj_a), tib));
  tib.kind = TI_PLAIN_MAP_SMALL;
  EXPECT_FALSE(test.assignable(TypeObject(tobj_a), tib));
  tib.kind = TI_PLAIN_MAP_LARGE;
  EXPECT_FALSE(test.assignable(TypeObject(tobj_a), tib));
  tib.kind = TI_STRONGLY_CONNECTED_COMPONENT;
  EXPECT_FALSE(test.assignable(TypeObject(tobj_a), tib));
  tib.kind = EK_COMPLETE;
  EXPECT_FALSE(test.assignable(TypeObject(tobj_a), tib));
}

TEST(SequenceTypeTest, Assignable)
{
  TypeAssignability test;
  MinimalSequenceType seq_a, seq_b;
  seq_a.header.common.bound = 10;
  seq_b.header.common.bound = 20;

  seq_a.element.common.type = TypeIdentifier::make(TK_BOOLEAN);
  seq_b.element.common.type = TypeIdentifier::make(TK_BOOLEAN);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(seq_a)),
                              TypeObject(MinimalTypeObject(seq_b))));
  seq_a.element.common.type = TypeIdentifier::make(TK_BYTE);
  seq_b.element.common.type = TypeIdentifier::make(TK_BYTE);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(seq_a)),
                              TypeObject(MinimalTypeObject(seq_b))));
  seq_a.element.common.type = TypeIdentifier::make(TK_INT16);
  seq_b.element.common.type = TypeIdentifier::make(TK_INT16);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(seq_a)),
                              TypeObject(MinimalTypeObject(seq_b))));
  seq_a.element.common.type = TypeIdentifier::make(TK_INT32);
  seq_b.element.common.type = TypeIdentifier::make(TK_INT32);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(seq_a)),
                              TypeObject(MinimalTypeObject(seq_b))));
  seq_a.element.common.type = TypeIdentifier::make(TK_INT64);
  seq_b.element.common.type = TypeIdentifier::make(TK_INT64);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(seq_a)),
                              TypeObject(MinimalTypeObject(seq_b))));
  seq_a.element.common.type = TypeIdentifier::make(TK_UINT16);
  seq_b.element.common.type = TypeIdentifier::make(TK_UINT16);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(seq_a)),
                              TypeObject(MinimalTypeObject(seq_b))));
  seq_a.element.common.type = TypeIdentifier::make(TK_UINT32);
  seq_b.element.common.type = TypeIdentifier::make(TK_UINT32);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(seq_a)),
                              TypeObject(MinimalTypeObject(seq_b))));
  seq_a.element.common.type = TypeIdentifier::make(TK_UINT64);
  seq_b.element.common.type = TypeIdentifier::make(TK_UINT64);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(seq_a)),
                              TypeObject(MinimalTypeObject(seq_b))));
  seq_a.element.common.type = TypeIdentifier::make(TK_FLOAT32);
  seq_b.element.common.type = TypeIdentifier::make(TK_FLOAT32);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(seq_a)),
                              TypeObject(MinimalTypeObject(seq_b))));
  seq_a.element.common.type = TypeIdentifier::make(TK_FLOAT64);
  seq_b.element.common.type = TypeIdentifier::make(TK_FLOAT64);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(seq_a)),
                              TypeObject(MinimalTypeObject(seq_b))));
  seq_a.element.common.type = TypeIdentifier::make(TK_FLOAT128);
  seq_b.element.common.type = TypeIdentifier::make(TK_FLOAT128);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(seq_a)),
                              TypeObject(MinimalTypeObject(seq_b))));
  seq_a.element.common.type = TypeIdentifier::make(TK_INT8);
  seq_b.element.common.type = TypeIdentifier::make(TK_INT8);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(seq_a)),
                              TypeObject(MinimalTypeObject(seq_b))));
  seq_a.element.common.type = TypeIdentifier::make(TK_UINT8);
  seq_b.element.common.type = TypeIdentifier::make(TK_UINT8);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(seq_a)),
                              TypeObject(MinimalTypeObject(seq_b))));
  seq_a.element.common.type = TypeIdentifier::make(TK_CHAR8);
  seq_b.element.common.type = TypeIdentifier::make(TK_CHAR8);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(seq_a)),
                              TypeObject(MinimalTypeObject(seq_b))));
  seq_a.element.common.type = TypeIdentifier::make(TK_CHAR16);
  seq_b.element.common.type = TypeIdentifier::make(TK_CHAR16);
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
  seq_a.element.common.type = TypeIdentifier::makePlainSequence(TypeIdentifier::make(TK_INT32),
                                                                static_cast<SBound>(50));
  seq_b.element.common.type = TypeIdentifier::makePlainSequence(TypeIdentifier::make(TK_INT32),
                                                                static_cast<LBound>(150));
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(seq_a)),
                              TypeObject(MinimalTypeObject(seq_b))));
  // Sequence of plain array of integer
  SBoundSeq bounds_a;
  bounds_a.append(10).append(20).append(30);
  seq_a.element.common.type = TypeIdentifier::makePlainArray(TypeIdentifier::make(TK_UINT16), bounds_a);
  LBoundSeq bounds_b;
  bounds_b.append(10).append(20).append(30);
  seq_b.element.common.type = TypeIdentifier::makePlainArray(TypeIdentifier::make(TK_UINT16), bounds_b);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(seq_a)),
                              TypeObject(MinimalTypeObject(seq_b))));

  // Different element types but T1 is-assignable-from T2 and T2 is delimited
  seq_a.element.common.type = TypeIdentifier::make(TK_UINT8);
  // Get a fake hash for the type object of a bitmask type
  EquivalenceHash hash;
  TypeLookup::get_equivalence_hash(hash);
  seq_b.element.common.type = TypeIdentifier::make(EK_MINIMAL, hash);
  MinimalBitmaskType bitmask_b;
  bitmask_b.header.common.bit_bound = 8;
  TypeLookup::insert_entry(*seq_b.element.common.type, MinimalTypeObject(bitmask_b));
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(seq_a)),
                              TypeObject(MinimalTypeObject(seq_b))));

  seq_a.element.common.type = TypeIdentifier::make(TK_UINT16);
  TypeLookup::get_equivalence_hash(hash);
  seq_b.element.common.type = TypeIdentifier::make(EK_MINIMAL, hash);
  bitmask_b.header.common.bit_bound = 16;
  TypeLookup::insert_entry(*seq_b.element.common.type, MinimalTypeObject(bitmask_b));
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(seq_a)),
                              TypeObject(MinimalTypeObject(seq_b))));

  seq_a.element.common.type = TypeIdentifier::make(TK_UINT32);
  TypeLookup::get_equivalence_hash(hash);
  seq_b.element.common.type = TypeIdentifier::make(EK_MINIMAL, hash);
  bitmask_b.header.common.bit_bound = 32;
  TypeLookup::insert_entry(*seq_b.element.common.type, MinimalTypeObject(bitmask_b));
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(seq_a)),
                              TypeObject(MinimalTypeObject(seq_b))));

  seq_a.element.common.type = TypeIdentifier::make(TK_UINT64);
  TypeLookup::get_equivalence_hash(hash);
  seq_b.element.common.type = TypeIdentifier::make(EK_MINIMAL, hash);
  bitmask_b.header.common.bit_bound = 64;
  TypeLookup::insert_entry(*seq_b.element.common.type, MinimalTypeObject(bitmask_b));
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(seq_a)),
                              TypeObject(MinimalTypeObject(seq_b))));
}

TEST(SequenceTypeTest, NotAssignable)
{
  TypeAssignability test;
  MinimalSequenceType seq_a, seq_b;
  seq_a.header.common.bound = 10;
  seq_b.header.common.bound = 20;

  seq_a.element.common.type = TypeIdentifier::make(TK_BOOLEAN);
  seq_b.element.common.type = TypeIdentifier::make(TK_INT32);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(seq_a)),
                               TypeObject(MinimalTypeObject(seq_b))));
  seq_a.element.common.type = TypeIdentifier::make(TK_BYTE);
  seq_b.element.common.type = TypeIdentifier::make(TK_BOOLEAN);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(seq_a)),
                               TypeObject(MinimalTypeObject(seq_b))));
  seq_a.element.common.type = TypeIdentifier::make(TK_INT16);
  seq_b.element.common.type = TypeIdentifier::make(TK_INT8);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(seq_a)),
                               TypeObject(MinimalTypeObject(seq_b))));
  seq_a.element.common.type = TypeIdentifier::make(TK_INT32);
  seq_b.element.common.type = TypeIdentifier::make(TK_INT64);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(seq_a)),
                               TypeObject(MinimalTypeObject(seq_b))));
  seq_a.element.common.type = TypeIdentifier::make(TK_INT64);
  seq_b.element.common.type = TypeIdentifier::make(TK_FLOAT32);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(seq_a)),
                               TypeObject(MinimalTypeObject(seq_b))));
  seq_a.element.common.type = TypeIdentifier::make(TK_UINT16);
  seq_b.element.common.type = TypeIdentifier::make(TK_BYTE);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(seq_a)),
                               TypeObject(MinimalTypeObject(seq_b))));
  seq_a.element.common.type = TypeIdentifier::make(TK_UINT32);
  seq_b.element.common.type = TypeIdentifier::make(TK_INT32);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(seq_a)),
                               TypeObject(MinimalTypeObject(seq_b))));
  seq_a.element.common.type = TypeIdentifier::make(TK_UINT64);
  seq_b.element.common.type = TypeIdentifier::make(TK_FLOAT128);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(seq_a)),
                               TypeObject(MinimalTypeObject(seq_b))));
  seq_a.element.common.type = TypeIdentifier::make(TK_FLOAT32);
  seq_b.element.common.type = TypeIdentifier::make(TK_INT32);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(seq_a)),
                               TypeObject(MinimalTypeObject(seq_b))));
  seq_a.element.common.type = TypeIdentifier::make(TK_FLOAT64);
  seq_b.element.common.type = TypeIdentifier::make(TK_UINT32);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(seq_a)),
                               TypeObject(MinimalTypeObject(seq_b))));
  seq_a.element.common.type = TypeIdentifier::make(TK_FLOAT128);
  seq_b.element.common.type = TypeIdentifier::make(TK_BOOLEAN);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(seq_a)),
                               TypeObject(MinimalTypeObject(seq_b))));
  seq_a.element.common.type = TypeIdentifier::make(TK_INT8);
  seq_b.element.common.type = TypeIdentifier::make(TK_INT64);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(seq_a)),
                               TypeObject(MinimalTypeObject(seq_b))));
  seq_a.element.common.type = TypeIdentifier::make(TK_UINT8);
  seq_b.element.common.type = TypeIdentifier::make(TK_FLOAT64);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(seq_a)),
                               TypeObject(MinimalTypeObject(seq_b))));
  seq_a.element.common.type = TypeIdentifier::make(TK_CHAR8);
  seq_b.element.common.type = TypeIdentifier::make(TK_INT8);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(seq_a)),
                               TypeObject(MinimalTypeObject(seq_b))));
  seq_a.element.common.type = TypeIdentifier::make(TK_CHAR16);
  seq_b.element.common.type = TypeIdentifier::make(TK_INT64);
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
  seq_a.element.common.type = TypeIdentifier::makePlainSequence(TypeIdentifier::make(TK_UINT32),
                                                                static_cast<SBound>(50));
  seq_b.element.common.type = TypeIdentifier::makePlainSequence(TypeIdentifier::make(TK_INT64),
                                                                static_cast<LBound>(150));
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(seq_a)),
                               TypeObject(MinimalTypeObject(seq_b))));
  // Sequence of plain array of integer
  SBoundSeq bounds_a;
  bounds_a.append(10).append(20).append(30);
  seq_a.element.common.type = TypeIdentifier::makePlainArray(TypeIdentifier::make(TK_UINT16), bounds_a);
  LBoundSeq bounds_b;
  bounds_b.append(10).append(20).append(40);
  seq_b.element.common.type = TypeIdentifier::makePlainArray(TypeIdentifier::make(TK_UINT16), bounds_b);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(seq_a)),
                               TypeObject(MinimalTypeObject(seq_b))));

  seq_a.element.common.type = TypeIdentifier::make(TK_UINT8);
  // Get a fake hash for the type object of a bitmask type
  EquivalenceHash hash;
  TypeLookup::get_equivalence_hash(hash);
  seq_b.element.common.type = TypeIdentifier::make(EK_MINIMAL, hash);
  MinimalBitmaskType bitmask_b;
  bitmask_b.header.common.bit_bound = 9;
  TypeLookup::insert_entry(*seq_b.element.common.type, MinimalTypeObject(bitmask_b));
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(seq_a)),
                               TypeObject(MinimalTypeObject(seq_b))));

  seq_a.element.common.type = TypeIdentifier::make(TK_UINT16);
  TypeLookup::get_equivalence_hash(hash);
  seq_b.element.common.type = TypeIdentifier::make(EK_MINIMAL, hash);
  bitmask_b.header.common.bit_bound = 17;
  TypeLookup::insert_entry(*seq_b.element.common.type, MinimalTypeObject(bitmask_b));
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(seq_a)),
                               TypeObject(MinimalTypeObject(seq_b))));

  seq_a.element.common.type = TypeIdentifier::make(TK_UINT32);
  TypeLookup::get_equivalence_hash(hash);
  seq_b.element.common.type = TypeIdentifier::make(EK_MINIMAL, hash);
  bitmask_b.header.common.bit_bound = 34;
  TypeLookup::insert_entry(*seq_b.element.common.type, MinimalTypeObject(bitmask_b));
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(seq_a)),
                               TypeObject(MinimalTypeObject(seq_b))));

  seq_a.element.common.type = TypeIdentifier::make(TK_UINT64);
  TypeLookup::get_equivalence_hash(hash);
  seq_b.element.common.type = TypeIdentifier::make(EK_MINIMAL, hash);
  bitmask_b.header.common.bit_bound = 13;
  TypeLookup::insert_entry(*seq_b.element.common.type, MinimalTypeObject(bitmask_b));
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(seq_a)),
                               TypeObject(MinimalTypeObject(seq_b))));
}

TEST(ArrayTypeTest, Assignable)
{
  TypeAssignability test;
  MinimalArrayType arr_a, arr_b;
  arr_a.header.common.bound_seq.append(10).append(20).append(30);
  arr_b.header.common.bound_seq.append(10).append(20).append(30);

  arr_a.element.common.type = TypeIdentifier::make(TK_BOOLEAN);
  arr_b.element.common.type = TypeIdentifier::make(TK_BOOLEAN);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(arr_a)),
                              TypeObject(MinimalTypeObject(arr_b))));
  arr_a.element.common.type = TypeIdentifier::make(TK_BYTE);
  arr_b.element.common.type = TypeIdentifier::make(TK_BYTE);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(arr_a)),
                              TypeObject(MinimalTypeObject(arr_b))));
  arr_a.element.common.type = TypeIdentifier::make(TK_INT16);
  arr_b.element.common.type = TypeIdentifier::make(TK_INT16);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(arr_a)),
                              TypeObject(MinimalTypeObject(arr_b))));
  arr_a.element.common.type = TypeIdentifier::make(TK_INT32);
  arr_b.element.common.type = TypeIdentifier::make(TK_INT32);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(arr_a)),
                              TypeObject(MinimalTypeObject(arr_b))));
  arr_a.element.common.type = TypeIdentifier::make(TK_INT64);
  arr_b.element.common.type = TypeIdentifier::make(TK_INT64);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(arr_a)),
                              TypeObject(MinimalTypeObject(arr_b))));
  arr_a.element.common.type = TypeIdentifier::make(TK_UINT16);
  arr_b.element.common.type = TypeIdentifier::make(TK_UINT16);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(arr_a)),
                              TypeObject(MinimalTypeObject(arr_b))));
  arr_a.element.common.type = TypeIdentifier::make(TK_UINT32);
  arr_b.element.common.type = TypeIdentifier::make(TK_UINT32);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(arr_a)),
                              TypeObject(MinimalTypeObject(arr_b))));
  arr_a.element.common.type = TypeIdentifier::make(TK_UINT64);
  arr_b.element.common.type = TypeIdentifier::make(TK_UINT64);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(arr_a)),
                              TypeObject(MinimalTypeObject(arr_b))));
  arr_a.element.common.type = TypeIdentifier::make(TK_FLOAT32);
  arr_b.element.common.type = TypeIdentifier::make(TK_FLOAT32);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(arr_a)),
                              TypeObject(MinimalTypeObject(arr_b))));
  arr_a.element.common.type = TypeIdentifier::make(TK_FLOAT64);
  arr_b.element.common.type = TypeIdentifier::make(TK_FLOAT64);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(arr_a)),
                              TypeObject(MinimalTypeObject(arr_b))));
  arr_a.element.common.type = TypeIdentifier::make(TK_FLOAT128);
  arr_b.element.common.type = TypeIdentifier::make(TK_FLOAT128);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(arr_a)),
                              TypeObject(MinimalTypeObject(arr_b))));
  arr_a.element.common.type = TypeIdentifier::make(TK_INT8);
  arr_b.element.common.type = TypeIdentifier::make(TK_INT8);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(arr_a)),
                              TypeObject(MinimalTypeObject(arr_b))));
  arr_a.element.common.type = TypeIdentifier::make(TK_UINT8);
  arr_b.element.common.type = TypeIdentifier::make(TK_UINT8);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(arr_a)),
                              TypeObject(MinimalTypeObject(arr_b))));
  arr_a.element.common.type = TypeIdentifier::make(TK_CHAR8);
  arr_b.element.common.type = TypeIdentifier::make(TK_CHAR8);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(arr_a)),
                              TypeObject(MinimalTypeObject(arr_b))));
  arr_a.element.common.type = TypeIdentifier::make(TK_CHAR16);
  arr_b.element.common.type = TypeIdentifier::make(TK_CHAR16);
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
  arr_a.element.common.type = TypeIdentifier::makePlainSequence(TypeIdentifier::make(TK_INT32),
                                                                static_cast<SBound>(50));
  arr_b.element.common.type = TypeIdentifier::makePlainSequence(TypeIdentifier::make(TK_INT32),
                                                                static_cast<LBound>(150));
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(arr_a)),
                              TypeObject(MinimalTypeObject(arr_b))));
  // Array of plain array of integer
  SBoundSeq bounds_a;
  bounds_a.append(10).append(20).append(30);
  arr_a.element.common.type = TypeIdentifier::makePlainArray(TypeIdentifier::make(TK_UINT16), bounds_a);
  LBoundSeq bounds_b;
  bounds_b.append(10).append(20).append(30);
  arr_b.element.common.type = TypeIdentifier::makePlainArray(TypeIdentifier::make(TK_UINT16), bounds_b);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(arr_a)),
                              TypeObject(MinimalTypeObject(arr_b))));

  // Different element types but T1 is-assignable-from T2 and T2 is delimited
  arr_a.element.common.type = TypeIdentifier::make(TK_UINT8);
  // Get a fake hash for the type object of a bitmask type
  EquivalenceHash hash;
  TypeLookup::get_equivalence_hash(hash);
  arr_b.element.common.type = TypeIdentifier::make(EK_MINIMAL, hash);
  MinimalBitmaskType bitmask_b;
  bitmask_b.header.common.bit_bound = 8;
  TypeLookup::insert_entry(*arr_b.element.common.type, MinimalTypeObject(bitmask_b));
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(arr_a)),
                              TypeObject(MinimalTypeObject(arr_b))));

  arr_a.element.common.type = TypeIdentifier::make(TK_UINT16);
  TypeLookup::get_equivalence_hash(hash);
  arr_b.element.common.type = TypeIdentifier::make(EK_MINIMAL, hash);
  bitmask_b.header.common.bit_bound = 16;
  TypeLookup::insert_entry(*arr_b.element.common.type, MinimalTypeObject(bitmask_b));
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(arr_a)),
                              TypeObject(MinimalTypeObject(arr_b))));

  arr_a.element.common.type = TypeIdentifier::make(TK_UINT32);
  TypeLookup::get_equivalence_hash(hash);
  arr_b.element.common.type = TypeIdentifier::make(EK_MINIMAL, hash);
  bitmask_b.header.common.bit_bound = 32;
  TypeLookup::insert_entry(*arr_b.element.common.type, MinimalTypeObject(bitmask_b));
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(arr_a)),
                              TypeObject(MinimalTypeObject(arr_b))));

  arr_a.element.common.type = TypeIdentifier::make(TK_UINT64);
  TypeLookup::get_equivalence_hash(hash);
  arr_b.element.common.type = TypeIdentifier::make(EK_MINIMAL, hash);
  bitmask_b.header.common.bit_bound = 64;
  TypeLookup::insert_entry(*arr_b.element.common.type, MinimalTypeObject(bitmask_b));
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(arr_a)),
                              TypeObject(MinimalTypeObject(arr_b))));
}

TEST(ArrayTypeTest, NotAssignable)
{
  TypeAssignability test;
  MinimalArrayType arr_a, arr_b;
  arr_a.header.common.bound_seq.append(10).append(20).append(30);
  arr_b.header.common.bound_seq.append(10).append(20).append(30);

  arr_a.element.common.type = TypeIdentifier::make(TK_BOOLEAN);
  arr_b.element.common.type = TypeIdentifier::make(TK_INT32);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(arr_a)),
                               TypeObject(MinimalTypeObject(arr_b))));
  arr_a.element.common.type = TypeIdentifier::make(TK_BYTE);
  arr_b.element.common.type = TypeIdentifier::make(TK_BOOLEAN);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(arr_a)),
                               TypeObject(MinimalTypeObject(arr_b))));
  arr_a.element.common.type = TypeIdentifier::make(TK_INT16);
  arr_b.element.common.type = TypeIdentifier::make(TK_INT8);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(arr_a)),
                               TypeObject(MinimalTypeObject(arr_b))));
  arr_a.element.common.type = TypeIdentifier::make(TK_INT32);
  arr_b.element.common.type = TypeIdentifier::make(TK_INT64);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(arr_a)),
                               TypeObject(MinimalTypeObject(arr_b))));
  arr_a.element.common.type = TypeIdentifier::make(TK_INT64);
  arr_b.element.common.type = TypeIdentifier::make(TK_FLOAT32);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(arr_a)),
                               TypeObject(MinimalTypeObject(arr_b))));
  arr_a.element.common.type = TypeIdentifier::make(TK_UINT16);
  arr_b.element.common.type = TypeIdentifier::make(TK_BYTE);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(arr_a)),
                               TypeObject(MinimalTypeObject(arr_b))));
  arr_a.element.common.type = TypeIdentifier::make(TK_UINT32);
  arr_b.element.common.type = TypeIdentifier::make(TK_INT32);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(arr_a)),
                               TypeObject(MinimalTypeObject(arr_b))));
  arr_a.element.common.type = TypeIdentifier::make(TK_UINT64);
  arr_b.element.common.type = TypeIdentifier::make(TK_FLOAT128);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(arr_a)),
                               TypeObject(MinimalTypeObject(arr_b))));
  arr_a.element.common.type = TypeIdentifier::make(TK_FLOAT32);
  arr_b.element.common.type = TypeIdentifier::make(TK_INT32);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(arr_a)),
                               TypeObject(MinimalTypeObject(arr_b))));
  arr_a.element.common.type = TypeIdentifier::make(TK_FLOAT64);
  arr_b.element.common.type = TypeIdentifier::make(TK_UINT32);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(arr_a)),
                               TypeObject(MinimalTypeObject(arr_b))));
  arr_a.element.common.type = TypeIdentifier::make(TK_FLOAT128);
  arr_b.element.common.type = TypeIdentifier::make(TK_BOOLEAN);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(arr_a)),
                               TypeObject(MinimalTypeObject(arr_b))));
  arr_a.element.common.type = TypeIdentifier::make(TK_INT8);
  arr_b.element.common.type = TypeIdentifier::make(TK_INT64);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(arr_a)),
                               TypeObject(MinimalTypeObject(arr_b))));
  arr_a.element.common.type = TypeIdentifier::make(TK_UINT8);
  arr_b.element.common.type = TypeIdentifier::make(TK_FLOAT64);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(arr_a)),
                               TypeObject(MinimalTypeObject(arr_b))));
  arr_a.element.common.type = TypeIdentifier::make(TK_CHAR8);
  arr_b.element.common.type = TypeIdentifier::make(TK_INT8);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(arr_a)),
                               TypeObject(MinimalTypeObject(arr_b))));
  arr_a.element.common.type = TypeIdentifier::make(TK_CHAR16);
  arr_b.element.common.type = TypeIdentifier::make(TK_INT64);
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
  arr_a.element.common.type = TypeIdentifier::makePlainSequence(TypeIdentifier::make(TK_UINT32),
                                                                static_cast<SBound>(50));
  arr_b.element.common.type = TypeIdentifier::makePlainSequence(TypeIdentifier::make(TK_INT64),
                                                                static_cast<LBound>(150));
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(arr_a)),
                               TypeObject(MinimalTypeObject(arr_b))));
  // Array of plain array of integer
  SBoundSeq bounds_a;
  bounds_a.append(10).append(20).append(30);
  arr_a.element.common.type = TypeIdentifier::makePlainArray(TypeIdentifier::make(TK_UINT16), bounds_a);
  LBoundSeq bounds_b;
  bounds_b.append(10).append(20).append(40);
  arr_b.element.common.type = TypeIdentifier::makePlainArray(TypeIdentifier::make(TK_UINT16), bounds_b);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(arr_a)),
                               TypeObject(MinimalTypeObject(arr_b))));

  arr_a.element.common.type = TypeIdentifier::make(TK_UINT8);
  // Get a fake hash for the type object of a bitmask type
  EquivalenceHash hash;
  TypeLookup::get_equivalence_hash(hash);
  arr_b.element.common.type = TypeIdentifier::make(EK_MINIMAL, hash);
  MinimalBitmaskType bitmask_b;
  bitmask_b.header.common.bit_bound = 9;
  TypeLookup::insert_entry(*arr_b.element.common.type, MinimalTypeObject(bitmask_b));
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(arr_a)),
                               TypeObject(MinimalTypeObject(arr_b))));

  arr_a.element.common.type = TypeIdentifier::make(TK_UINT16);
  TypeLookup::get_equivalence_hash(hash);
  arr_b.element.common.type = TypeIdentifier::make(EK_MINIMAL, hash);
  bitmask_b.header.common.bit_bound = 17;
  TypeLookup::insert_entry(*arr_b.element.common.type, MinimalTypeObject(bitmask_b));
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(arr_a)),
                               TypeObject(MinimalTypeObject(arr_b))));

  arr_a.element.common.type = TypeIdentifier::make(TK_UINT32);
  TypeLookup::get_equivalence_hash(hash);
  arr_b.element.common.type = TypeIdentifier::make(EK_MINIMAL, hash);
  bitmask_b.header.common.bit_bound = 34;
  TypeLookup::insert_entry(*arr_b.element.common.type, MinimalTypeObject(bitmask_b));
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(arr_a)),
                               TypeObject(MinimalTypeObject(arr_b))));

  arr_a.element.common.type = TypeIdentifier::make(TK_UINT64);
  TypeLookup::get_equivalence_hash(hash);
  arr_b.element.common.type = TypeIdentifier::make(EK_MINIMAL, hash);
  bitmask_b.header.common.bit_bound = 13;
  TypeLookup::insert_entry(*arr_b.element.common.type, MinimalTypeObject(bitmask_b));
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(arr_a)),
                               TypeObject(MinimalTypeObject(arr_b))));
}

TEST(MapTypeTest, Assignable)
{
  TypeAssignability test;
  MinimalMapType map_a, map_b;
  map_a.header.common.bound = 50;
  map_b.header.common.bound = 100;

  map_a.key.common.type = TypeIdentifier::make(TK_UINT32);
  map_b.key.common.type = TypeIdentifier::make(TK_UINT32);
  map_a.element.common.type = TypeIdentifier::make(TK_BOOLEAN);
  map_b.element.common.type = TypeIdentifier::make(TK_BOOLEAN);
  EXPECT_TRUE(test.assignable(TypeObject(MinimalTypeObject(map_a)),
                              TypeObject(MinimalTypeObject(map_b))));
}

TEST(MapTypeTest, NotAssignable)
{
  TypeAssignability test;
  MinimalMapType map_a, map_b;
  map_a.header.common.bound = 50;
  map_b.header.common.bound = 100;

  map_a.key.common.type = TypeIdentifier::make(TK_UINT32);
  map_b.key.common.type = TypeIdentifier::make(TK_UINT16);
  map_a.element.common.type = TypeIdentifier::make(TK_BOOLEAN);
  map_b.element.common.type = TypeIdentifier::make(TK_BOOLEAN);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(map_a)),
                               TypeObject(MinimalTypeObject(map_b))));

  map_a.key.common.type = TypeIdentifier::make(TK_UINT32);
  map_b.key.common.type = TypeIdentifier::make(TK_UINT32);
  map_a.element.common.type = TypeIdentifier::make(TK_BYTE);
  map_b.element.common.type = TypeIdentifier::make(TK_BOOLEAN);
  EXPECT_FALSE(test.assignable(TypeObject(MinimalTypeObject(map_a)),
                               TypeObject(MinimalTypeObject(map_b))));
}

int main(int argc, char* argv[])
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
