#include "CompleteToMinimalTypeObjectTypeSupportImpl.h"

#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/XTypes/TypeObject.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/TopicDescriptionImpl.h>
#include <dds/DCPS/PublisherImpl.h>
#include <dds/DCPS/DomainParticipantImpl.h>
#include <dds/DCPS/Qos_Helper.h>
#include <dds/DCPS/transport/framework/TransportRegistry.h>

#include <gtest/gtest.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace XTypes {

bool compare_member_hash(const NameHash& lhs, const NameHash& rhs)
{
  return std::memcmp(lhs, rhs, sizeof(NameHash)) == 0;
}

template <typename T>
bool operator==(const Sequence<T>& lhs, const Sequence<T>& rhs)
{
  if (lhs.length() != lhs.length()) {
    return false;
  }

  for (ACE_CDR::ULong i = 0; i < lhs.length(); ++i) {
    if (!(lhs[i] == rhs[i])) {
      return false;
    }
  }
  return true;
}

bool operator==(const MinimalMemberDetail& lhs, const MinimalMemberDetail& rhs)
{
  return compare_member_hash(lhs.name_hash, rhs.name_hash);
}

bool operator==(const CommonStructMember& lhs, const CommonStructMember& rhs)
{
  return lhs.member_id == rhs.member_id &&
    lhs.member_flags == rhs.member_flags &&
    lhs.member_type_id == rhs.member_type_id;
}

bool operator==(const MinimalStructMember& lhs, const MinimalStructMember& rhs)
{
  return lhs.common == rhs.common &&
    compare_member_hash(lhs.detail.name_hash, rhs.detail.name_hash);
}

bool operator==(const MinimalStructHeader& lhs, const MinimalStructHeader& rhs)
{
  return lhs.base_type == rhs.base_type;
}

bool operator==(const MinimalStructType& lhs, const MinimalStructType& rhs)
{
  return lhs.struct_flags == rhs.struct_flags &&
    lhs.header == rhs.header &&
    lhs.member_seq == rhs.member_seq;
}

bool operator==(const CommonUnionMember& lhs, const CommonUnionMember& rhs)
{
  return lhs.member_id == rhs.member_id &&
    lhs.member_flags == rhs.member_flags &&
    lhs.type_id == rhs.type_id &&
    lhs.label_seq == rhs.label_seq;
}

bool operator==(const MinimalUnionMember& lhs, const MinimalUnionMember& rhs)
{
  return lhs.common == rhs.common && lhs.detail == rhs.detail;
}

bool operator==(const CommonDiscriminatorMember& lhs, const CommonDiscriminatorMember& rhs)
{
  return lhs.member_flags == rhs.member_flags && lhs.type_id == rhs.type_id;
}

bool operator==(const MinimalDiscriminatorMember& lhs, const MinimalDiscriminatorMember& rhs)
{
  return lhs.common == rhs.common;
}

bool operator==(const MinimalUnionType& lhs, const MinimalUnionType& rhs)
{
  return lhs.union_flags == rhs.union_flags &&
    lhs.discriminator == rhs.discriminator &&
    lhs.member_seq == rhs.member_seq;
}

bool operator==(const CommonAnnotationParameter& lhs, const CommonAnnotationParameter& rhs)
{
  return lhs.member_flags == rhs.member_flags &&
    rhs.member_type_id == rhs.member_type_id;
}

bool operator==(const AnnotationParameterValue& lhs, const AnnotationParameterValue& rhs)
{
  if (lhs.kind != rhs.kind) {
    return false;
  }

  switch (rhs.kind) {
  case TK_BOOLEAN:
    return lhs.boolean_value == rhs.boolean_value;
  case TK_BYTE:
    return lhs.byte_value == rhs.byte_value;
  case TK_INT16:
    return lhs.int16_value == rhs.int16_value;
  case TK_UINT16:
    return lhs.uint16_value == rhs.uint16_value;
  case TK_INT32:
    return lhs.int32_value == rhs.int32_value;
  case TK_UINT32:
    return lhs.uint32_value == rhs.uint32_value;
  case TK_INT64:
    return lhs.int64_value == rhs.int64_value;
  case TK_UINT64:
    return lhs.uint64_value == rhs.uint64_value;
  case TK_FLOAT32:
    return lhs.float32_value == rhs.float32_value;
  case TK_FLOAT64:
    return lhs.float64_value == rhs.float64_value;
  case TK_FLOAT128:
    return lhs.float128_value == rhs.float128_value;
  case TK_CHAR8:
    return lhs.char_value == rhs.char_value;
  case TK_CHAR16:
    return lhs.wchar_value == rhs.wchar_value;
  case TK_ENUM:
    return lhs.enumerated_value == rhs.enumerated_value;
  case TK_STRING8:
    return lhs.string8_value == rhs.string8_value;
  case TK_STRING16:
    return lhs.string16_value == rhs.string16_value;
  default:
    return true;
  }
}

bool operator==(const MinimalAnnotationParameter& lhs, const MinimalAnnotationParameter& rhs)
{
  return lhs.common == rhs.common &&
    compare_member_hash(lhs.name_hash, rhs.name_hash) &&
    lhs.default_value == rhs.default_value;
}

bool operator==(const MinimalAnnotationType& lhs, const MinimalAnnotationType& rhs)
{
  return lhs.member_seq == rhs.member_seq;
}

bool operator==(const CommonAliasBody& lhs, const CommonAliasBody& rhs)
{
  return lhs.related_flags == rhs.related_flags &&
    lhs.related_type == rhs.related_type;
}

bool operator==(const MinimalAliasBody& lhs, const MinimalAliasBody& rhs) {
  return lhs.common == rhs.common;
}

bool operator==(const MinimalAliasType& lhs, const MinimalAliasType& rhs)
{
  return lhs.alias_flags == rhs.alias_flags && lhs.body == rhs.body;
}

bool operator==(const CommonCollectionElement& lhs, const CommonCollectionElement& rhs)
{
  return lhs.element_flags == rhs.element_flags && lhs.type == rhs.type;
}

bool operator==(const MinimalCollectionElement& lhs, const MinimalCollectionElement& rhs)
{
  return lhs.common == rhs.common;
}

bool operator==(const CommonCollectionHeader& lhs, const CommonCollectionHeader& rhs)
{
  return lhs.bound == rhs.bound;
}

bool operator==(const MinimalCollectionHeader& lhs, const MinimalCollectionHeader& rhs)
{
  return lhs.common == rhs.common;
}

bool operator==(const MinimalSequenceType& lhs, const MinimalSequenceType& rhs)
{
  return lhs.collection_flag == rhs.collection_flag &&
    lhs.header == rhs.header &&
    lhs.element == rhs.element;
}

bool operator==(const CommonArrayHeader& lhs, const CommonArrayHeader& rhs)
{
  return lhs.bound_seq == rhs.bound_seq;
}

bool operator==(const MinimalArrayHeader& lhs, const MinimalArrayHeader& rhs)
{
  return lhs.common == rhs.common;
}

bool operator==(const MinimalArrayType& lhs, const MinimalArrayType& rhs)
{
  return lhs.header == rhs.header && lhs.element == rhs.element;
}

bool operator==(const MinimalMapType& lhs, const MinimalMapType& rhs)
{
  return lhs.collection_flag == rhs.collection_flag &&
    lhs.header == rhs.header &&
    lhs.key == rhs.key &&
    lhs. element == rhs.element;
}

bool operator==(const CommonEnumeratedLiteral& lhs, const CommonEnumeratedLiteral& rhs)
{
  return lhs.value == rhs.value && lhs.flags == rhs.flags;
}

bool operator==(const MinimalEnumeratedLiteral& lhs, const MinimalEnumeratedLiteral& rhs)
{
  return lhs.common == rhs.common && lhs.detail == rhs.detail;
}

bool operator==(const CommonEnumeratedHeader& lhs, const CommonEnumeratedHeader& rhs)
{
  return lhs.bit_bound == rhs.bit_bound;
}

bool operator==(const MinimalEnumeratedHeader& lhs, const MinimalEnumeratedHeader& rhs)
{
  return lhs.common == rhs.common;
}

bool operator==(const MinimalEnumeratedType& lhs, const MinimalEnumeratedType& rhs)
{
  return lhs.header == rhs.header && lhs.literal_seq == rhs.literal_seq;
}

bool operator==(const CommonBitflag& lhs, const CommonBitflag& rhs)
{
  return lhs.position == rhs.position && lhs.flags == rhs.flags;
}

bool operator==(const MinimalBitflag& lhs, const MinimalBitflag& rhs)
{
  return lhs.common == rhs.common && lhs.detail == rhs.detail;
}

bool operator==(const MinimalBitmaskType& lhs, const MinimalBitmaskType& rhs)
{
  return lhs.header == rhs.header && lhs.flag_seq == rhs.flag_seq;
}

bool operator==(const CommonBitfield& lhs, const CommonBitfield& rhs)
{
  return lhs.position == rhs.position &&
    lhs.flags == rhs.flags &&
    lhs.bitcount == rhs.bitcount &&
    lhs.holder_type == rhs.holder_type;
}

bool operator==(const MinimalBitfield& lhs, const MinimalBitfield& rhs)
{
  return lhs.common == rhs.common &&
    compare_member_hash(lhs.name_hash, rhs.name_hash);
}

bool operator==(const MinimalBitsetType& lhs, const MinimalBitsetType& rhs)
{
  return lhs.field_seq == rhs.field_seq;
}

bool operator==(const MinimalTypeObject& lhs, const MinimalTypeObject& rhs)
{
  if (lhs.kind != rhs.kind) {
    return false;
  }

  switch (lhs.kind) {
  case TK_ALIAS:
    return lhs.alias_type == rhs.alias_type;
  case TK_ANNOTATION:
    return lhs.annotation_type == rhs.annotation_type;
  case TK_STRUCTURE:
    return lhs.struct_type == rhs.struct_type;
  case TK_UNION:
    return lhs.union_type == rhs.union_type;
  case TK_BITSET:
    return lhs.bitset_type == rhs.bitset_type;
  case TK_SEQUENCE:
    return lhs.sequence_type == rhs.sequence_type;
  case TK_ARRAY:
    return lhs.array_type == rhs.array_type;
  case TK_MAP:
    return lhs.map_type == rhs.map_type;
  case TK_ENUM:
    return lhs.enumerated_type == rhs.enumerated_type;
  case TK_BITMASK:
    return lhs.bitmask_type == rhs.bitmask_type;
  }
  return false;
}

bool operator==(const TypeObject& lhs, const TypeObject& rhs)
{
  if (lhs.kind != rhs.kind) {
    return false;
  }

  if (lhs.kind == EK_MINIMAL) {
    return lhs.minimal == rhs.minimal;
  }
  return true;
}
} // namespace XTypes
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

using namespace OpenDDS;

XTypes::TypeLookupService tls;

template<typename T>
void test_conversion()
{
  const XTypes::TypeIdentifier& min_ti = DCPS::getMinimalTypeIdentifier<T>();
  const XTypes::TypeMap& min_map = DCPS::getMinimalTypeMap<T>();
  XTypes::TypeMap::const_iterator pos = min_map.find(min_ti);
  EXPECT_TRUE(pos != min_map.end());
  const XTypes::TypeObject& expected_min_to = pos->second;

  const XTypes::TypeIdentifier& com_ti = DCPS::getCompleteTypeIdentifier<T>();
  const XTypes::TypeMap& com_map = DCPS::getCompleteTypeMap<T>();
  pos = com_map.find(com_ti);
  EXPECT_TRUE(pos != com_map.end());
  const XTypes::TypeObject& com_to = pos->second;

  XTypes::TypeObject converted_min_to;
  EXPECT_TRUE(tls.complete_to_minimal_type_object(com_to, converted_min_to));
  EXPECT_EQ(expected_min_to, converted_min_to);
}

TEST(CompleteToMinimalTypeObject, MyStruct)
{
  test_conversion<DCPS::MyMod_MyStruct_xtag>();
}

TEST(CompleteToMinimalTypeObject, MyUnion)
{
  test_conversion<DCPS::MyMod_MyUnion_xtag>();
}

TEST(CompleteToMinimalTypeObject, SCC)
{
  // TODO(sonndinh): Verify conversion of the types in the SCC including
  // CircularStruct, CircularStruct2, sequence<CircularStruct>,
  // sequence<CircularStruct2>, and CircularStruct[3].
}

TEST(CompleteToMinimalTypeObject, LSeq)
{
  test_conversion<DCPS::MyMod_LSeq_xtag>();
}

TEST(CompleteToMinimalTypeObject, LArr)
{
  test_conversion<DCPS::MyMod_LArr_xtag>();
}

TEST(CompleteToMinimalTypeObject, MyEnum)
{
  test_conversion<DCPS::MyMod_MyEnum_xtag>();
}

int main(int argc, char* argv[])
{
  ::testing::InitGoogleTest(&argc, argv);
  XTypes::TypeIdentifierPairSeq tid_pairs;

  XTypes::TypeIdentifierPair my_struct_tids;
  my_struct_tids.type_identifier1 = DCPS::getCompleteTypeIdentifier<DCPS::MyMod_MyStruct_xtag>();
  my_struct_tids.type_identifier2 = DCPS::getMinimalTypeIdentifier<DCPS::MyMod_MyStruct_xtag>();
  tid_pairs.append(my_struct_tids);

  XTypes::TypeIdentifierPair circular_struct_tids;
  circular_struct_tids.type_identifier1 = DCPS::getCompleteTypeIdentifier<DCPS::MyMod_CircularStruct_xtag>();
  circular_struct_tids.type_identifier2 = DCPS::getMinimalTypeIdentifier<DCPS::MyMod_CircularStruct_xtag>();
  tid_pairs.append(circular_struct_tids);

  XTypes::TypeIdentifierPair circular_struct2_tids;
  circular_struct2_tids.type_identifier1 = DCPS::getCompleteTypeIdentifier<DCPS::MyMod_CircularStruct2_xtag>();
  circular_struct2_tids.type_identifier2 = DCPS::getMinimalTypeIdentifier<DCPS::MyMod_CircularStruct2_xtag>();
  tid_pairs.append(circular_struct2_tids);

  XTypes::TypeIdentifierPair my_enum_tids;
  my_enum_tids.type_identifier1 = DCPS::getCompleteTypeIdentifier<DCPS::MyMod_MyEnum_xtag>();
  my_enum_tids.type_identifier2 = DCPS::getMinimalTypeIdentifier<DCPS::MyMod_MyEnum_xtag>();
  tid_pairs.append(my_enum_tids);

  XTypes::TypeIdentifierPair my_union_tids;
  my_union_tids.type_identifier1 = DCPS::getCompleteTypeIdentifier<DCPS::MyMod_MyUnion_xtag>();
  my_union_tids.type_identifier2 = DCPS::getMinimalTypeIdentifier<DCPS::MyMod_MyUnion_xtag>();
  tid_pairs.append(my_union_tids);

  XTypes::TypeIdentifierPair lseq_tids;
  lseq_tids.type_identifier1 = DCPS::getCompleteTypeIdentifier<DCPS::MyMod_LSeq_xtag>();
  lseq_tids.type_identifier2 = DCPS::getMinimalTypeIdentifier<DCPS::MyMod_LSeq_xtag>();
  tid_pairs.append(lseq_tids);

  XTypes::TypeIdentifierPair larr_tids;
  larr_tids.type_identifier1 = DCPS::getCompleteTypeIdentifier<DCPS::MyMod_LArr_xtag>();
  larr_tids.type_identifier2 = DCPS::getMinimalTypeIdentifier<DCPS::MyMod_LArr_xtag>();
  tid_pairs.append(larr_tids);

  tls.update_type_identifier_map(tid_pairs);

  return RUN_ALL_TESTS();
}
