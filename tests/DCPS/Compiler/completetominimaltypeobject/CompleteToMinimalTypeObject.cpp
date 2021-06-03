#include "CompleteToMinimalTypeObjectTypeSupportImpl.h"

#include "dds/DCPS/Service_Participant.h"
#include "dds/DCPS/XTypes/TypeObject.h"
#include "dds/DCPS/Marked_Default_Qos.h"
#include "dds/DCPS/TopicDescriptionImpl.h"
#include "dds/DCPS/PublisherImpl.h"
#include "dds/DCPS/DomainParticipantImpl.h"
#include "dds/DCPS/Qos_Helper.h"
#include "dds/DCPS/transport/framework/TransportRegistry.h"

#include <gtest/gtest.h>
using namespace OpenDDS::XTypes;

namespace OpenDDS {
namespace XTypes {

bool compare_member_hash(const NameHash& lhs, const NameHash& rhs)
{
  return lhs[0] == rhs[0] &&
    lhs[1] == rhs[1] &&
    lhs[2] == rhs[2] &&
    lhs[3] == rhs[3];
}

template <typename T>
bool operator==(const Sequence<T>& lhs, const Sequence<T>& rhs)
{
  if (lhs.length() != lhs.length()) {
    return false;
  }
  for (unsigned long i = 0; i < lhs.length(); ++i) {
    if (!(lhs[i] == rhs[i])) {
      return false;
    }
  }
  return true;
}

bool operator== (const MinimalMemberDetail& lhs, const MinimalMemberDetail& rhs)
{
  return compare_member_hash(lhs.name_hash, rhs.name_hash);
}

bool operator== (const CommonStructMember& lhs, const CommonStructMember& rhs)
{
  return
    lhs.member_id == rhs.member_id &&
    lhs.member_flags == rhs.member_flags &&
    lhs.member_type_id == rhs.member_type_id;
}

bool operator== (const MinimalStructMember& lhs, const MinimalStructMember& rhs)
{
  return
    lhs.common == rhs.common &&
    compare_member_hash(lhs.detail.name_hash, rhs.detail.name_hash);
}

bool operator== (const MinimalStructHeader& lhs, const MinimalStructHeader& rhs)
{
  return
    lhs.base_type == rhs.base_type;
    //lhs.detail == rhs.detail //unused
}

bool operator== (const MinimalStructType& lhs, const MinimalStructType& rhs)
{
  return
    lhs.struct_flags == rhs.struct_flags &&
    lhs.header == rhs.header &&
    lhs.member_seq == rhs.member_seq;
}

bool operator== (const CommonUnionMember& lhs, const CommonUnionMember& rhs)
{
  return
    lhs.member_id == rhs.member_id &&
    lhs.member_flags == rhs.member_flags &&
    lhs.type_id == rhs.type_id &&
    lhs.label_seq == rhs.label_seq;
}

bool operator== (const MinimalUnionMember& lhs, const MinimalUnionMember& rhs)
{
  return
    lhs.common == rhs.common &&
    lhs.detail == rhs.detail;
}

bool operator== (const CommonDiscriminatorMember& lhs, const CommonDiscriminatorMember& rhs)
{
  return
    lhs.member_flags == rhs.member_flags &&
    lhs.type_id == rhs.type_id;
}

bool operator== (const MinimalDiscriminatorMember& lhs, const MinimalDiscriminatorMember& rhs)
{
  return lhs.common == rhs.common;
}

bool operator== (const MinimalUnionType& lhs, const MinimalUnionType& rhs)
{
  return
    lhs.union_flags == rhs.union_flags &&
    //lhs.header == rhs.header &&//unused
    lhs.discriminator == rhs.discriminator &&
    lhs.member_seq == rhs.member_seq;
}

bool operator== (const CommonAnnotationParameter& lhs, const CommonAnnotationParameter& rhs)
{
  return
    lhs.member_flags == rhs.member_flags &&
    rhs.member_type_id == rhs.member_type_id;
}

bool operator== (const AnnotationParameterValue& lhs, const AnnotationParameterValue& rhs)
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
    //return lhs.extended_value == rhs.extended_value; // unused
    return true;
  }
  return true;
}

bool operator== (const MinimalAnnotationParameter& lhs, const MinimalAnnotationParameter& rhs)
{
  return
    lhs.common == rhs.common &&
    compare_member_hash(lhs.name_hash, rhs.name_hash) &&
    lhs.default_value == rhs.default_value;
}

bool operator== (const MinimalAnnotationType& lhs, const MinimalAnnotationType& rhs)
{
  return
    //lhs.annotation_flag == rhs.annotation_flag && //unused
    //lhs.header == rhs.header && //unused
    lhs.member_seq == rhs.member_seq;
}


bool operator== (const CommonAliasBody& lhs, const CommonAliasBody& rhs)
{
  return
    lhs.related_flags == rhs.related_flags &&
    lhs.related_type == rhs.related_type;
}

bool operator== (const MinimalAliasBody& lhs, const MinimalAliasBody& rhs) {
  return lhs.common == rhs.common;
}

bool operator== (const MinimalAliasType& lhs, const MinimalAliasType& rhs)
{
  return
    lhs.alias_flags == rhs.alias_flags &&
    lhs.body == rhs.body;
}

bool operator== (const CommonCollectionElement& lhs, const CommonCollectionElement& rhs)
{
  return
    lhs.element_flags == rhs.element_flags &&
    lhs.type == rhs.type;
}

bool operator== (const MinimalCollectionElement& lhs, const MinimalCollectionElement& rhs)
{
  return lhs.common == rhs.common;
}

bool operator== (const CommonCollectionHeader& lhs, const CommonCollectionHeader& rhs)
{
  return lhs.bound == rhs.bound;
}

bool operator== (const MinimalCollectionHeader& lhs, const MinimalCollectionHeader& rhs)
{
  return lhs.common == rhs.common;
}

bool operator== (const MinimalSequenceType& lhs, const MinimalSequenceType& rhs)
{
  return
    lhs.collection_flag == rhs.collection_flag &&
    lhs.header == rhs.header &&
    lhs.element == rhs.element;
}

bool operator== (const CommonArrayHeader& lhs, const CommonArrayHeader& rhs)
{
  if (lhs.bound_seq.length() != lhs.bound_seq.length()) {
    return false;
  }
  for (unsigned long i = 0; i < lhs.bound_seq.length(); ++i) {
    if (lhs.bound_seq[i] != rhs.bound_seq[i]) {
      return false;
    }
  }
  return true;
}

bool operator== (const MinimalArrayHeader& lhs, const MinimalArrayHeader& rhs)
{
  return lhs.common == rhs.common;
}

bool operator== (const MinimalArrayType& lhs, const MinimalArrayType& rhs)
{
  return
    //lhs.collection_flag == rhs.collection_flag &&
    lhs.header == rhs.header  &&
    lhs.element == rhs.element;
}

bool operator== (const MinimalMapType& lhs, const MinimalMapType& rhs)
{
  return
    lhs.collection_flag == rhs.collection_flag &&
    lhs.header == rhs.header &&
    lhs.key == rhs.key &&
    lhs. element == rhs.element;
}

bool operator== (const CommonEnumeratedLiteral& lhs, const CommonEnumeratedLiteral& rhs)
{
  return
    lhs.value == rhs.value &&
    lhs.flags == rhs.flags;
}

bool operator== (const MinimalEnumeratedLiteral& lhs, const MinimalEnumeratedLiteral& rhs)
{
  return
    lhs.common == rhs.common &&
    lhs.detail == rhs.detail;
}

bool operator== (const CommonEnumeratedHeader& lhs, const CommonEnumeratedHeader& rhs)
{
  return lhs.bit_bound == rhs.bit_bound;
}

bool operator== (const MinimalEnumeratedHeader& lhs, const MinimalEnumeratedHeader& rhs)
{
  return lhs.common == rhs.common;
}

bool operator== (const MinimalEnumeratedType& lhs, const MinimalEnumeratedType& rhs)
{
  return
    // lhs.enum_flags == rhs.enum_flags // currently unusued
    lhs.header == rhs.header &&
    lhs.literal_seq == rhs.literal_seq;
}

bool operator== (const CommonBitflag& lhs, const CommonBitflag& rhs)
{
  return
    lhs.position == rhs.position &&
    lhs.flags == rhs.flags;
}

bool operator== (const MinimalBitflag& lhs, const MinimalBitflag& rhs)
{
  return
    lhs.common == rhs.common &&
    lhs.detail == rhs.detail;
}

bool operator== (const MinimalBitmaskType& lhs, const MinimalBitmaskType& rhs)
{
  return
    //lhs.bitmask_flags == rhs.bitmask_flags //unused
    lhs.header == rhs.header &&
    lhs.flag_seq == rhs.flag_seq;
}

bool operator== (const CommonBitfield& lhs, const CommonBitfield& rhs)
{
  return
    lhs.position == rhs.position &&
    lhs.flags == rhs.flags &&
    lhs.bitcount == rhs.bitcount &&
    lhs.holder_type == rhs.holder_type;
}

bool operator== (const MinimalBitfield& lhs, const MinimalBitfield& rhs)
{
  return
    lhs.common == rhs.common &&
    compare_member_hash(lhs.name_hash, rhs.name_hash);
}

bool operator== (const MinimalBitsetType& lhs, const MinimalBitsetType& rhs)
{
  return
    //lhs.bitset_flags == rhs.bitset_flags // unused
    //lhs.header == rhs.header //unused
    lhs.field_seq == rhs.field_seq;
}

bool operator== (const MinimalTypeObject& lhs, const MinimalTypeObject& rhs)
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

bool operator== (const TypeObject& lhs, const TypeObject& rhs)
{
  if (lhs.kind != rhs.kind) {
    return false;
  }
  if (!(lhs.minimal == rhs.minimal)) {
    return false;
  }
  return true;
    //lhs.complete == rhs.complete;
}
}
}

::DDS::DomainParticipant_var dp;
::DDS::DomainParticipantFactory_var dpf;
OpenDDS::XTypes::TypeLookupService_rch tls;

TEST(CompleteToMinimalTypeObject, Struct)
{
  using namespace OpenDDS::XTypes;
  my_mod::unTypeSupportImpl stru_tsi;
  stru_tsi.register_type(dp.in (), "stru");
  ::DDS::Topic_var my_topic =
    dp->create_topic ("stru",
                    "stru",
                    TOPIC_QOS_DEFAULT,
                    ::DDS::TopicListener::_nil(),
                    ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  ::DDS::Publisher_var pub =
    dp->create_publisher(PUBLISHER_QOS_DEFAULT,
                          ::DDS::PublisherListener::_nil(),
                          ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  ::DDS::DataWriter_var dw =
    pub->create_datawriter(my_topic.in (),
                            DATAWRITER_QOS_DEFAULT,
                            ::DDS::DataWriterListener::_nil(),
                            ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  TypeMap ctm = stru_tsi.getCompleteTypeMap();
  TypeObject cto = ctm[TypeIdentifier(EK_COMPLETE, EquivalenceHashWrapper(168, 72, 172, 8, 184, 103, 37, 100, 24, 24, 29, 129, 165, 47))];
  TypeMap mtm = stru_tsi.getMinimalTypeMap();
  TypeObject c_to_m_mto;
  if (!tls->complete_to_minimal_typeobject(cto, c_to_m_mto)) {
    ASSERT_EQ(1,0);
  }
  TypeObject mto2 = mtm[TypeIdentifier(EK_MINIMAL, EquivalenceHashWrapper(189, 72, 116, 15, 57, 244, 144, 185, 189, 193, 70, 196, 210, 42))];
  ASSERT_EQ(mto2, c_to_m_mto);
}

TEST(CompleteToMinimalTypeObject, Union)
{
  using namespace OpenDDS::XTypes;
  my_mod::unTypeSupportImpl uts;
  uts.register_type(dp.in (), "un");
  ::DDS::Topic_var my_topic =
    dp->create_topic ("un",
                    "un",
                    TOPIC_QOS_DEFAULT,
                    ::DDS::TopicListener::_nil(),
                    ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  ::DDS::Publisher_var pub =
    dp->create_publisher(PUBLISHER_QOS_DEFAULT,
                          ::DDS::PublisherListener::_nil(),
                          ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  ::DDS::DataWriter_var dw =
    pub->create_datawriter(my_topic.in (),
                            DATAWRITER_QOS_DEFAULT,
                            ::DDS::DataWriterListener::_nil(),
                            ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  TypeMap ctm = uts.getCompleteTypeMap();
  TypeObject cto = ctm[TypeIdentifier(EK_COMPLETE, EquivalenceHashWrapper(70, 184, 188, 58, 218, 154, 35, 224, 117, 149, 213, 242, 180, 239))];
  TypeObject c_to_m_mto;
  if (!tls->complete_to_minimal_typeobject(cto, c_to_m_mto)) {
    ASSERT_EQ(1,0);
  }
  TypeMap mtm = uts.getMinimalTypeMap();
  TypeObject mto2 = mtm[TypeIdentifier(EK_MINIMAL, EquivalenceHashWrapper(94, 42, 84, 196, 20, 69, 255, 70, 95, 183, 166, 22, 4, 142))];
  ASSERT_EQ(mto2, c_to_m_mto);
}

TEST(CompleteToMinimalTypeObject, Sequence)
{
  using namespace OpenDDS::XTypes;
  my_mod::unTypeSupportImpl stru_tsi;
  stru_tsi.register_type(dp.in (), "circular_struct_seq2");
  ::DDS::Topic_var my_topic =
    dp->create_topic ("circular_struct_seq2",
                    "circular_struct_seq2",
                    TOPIC_QOS_DEFAULT,
                    ::DDS::TopicListener::_nil(),
                    ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  ::DDS::Publisher_var pub =
    dp->create_publisher(PUBLISHER_QOS_DEFAULT,
                          ::DDS::PublisherListener::_nil(),
                          ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  ::DDS::DataWriter_var dw =
    pub->create_datawriter(my_topic.in (),
                            DATAWRITER_QOS_DEFAULT,
                            ::DDS::DataWriterListener::_nil(),
                            ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  TypeMap ctm = stru_tsi.getCompleteTypeMap();
  TypeObject cto = ctm[TypeIdentifier(TI_STRONGLY_CONNECTED_COMPONENT, StronglyConnectedComponentId(TypeObjectHashId(EK_COMPLETE, EquivalenceHashWrapper(42, 203, 214, 182, 80, 84, 4, 33, 118, 109, 127, 170, 57, 95)), 5, 2))];
  TypeMap mtm = stru_tsi.getMinimalTypeMap();
  TypeObject c_to_m_mto;
  if (!tls->complete_to_minimal_typeobject(cto, c_to_m_mto)) {
    ASSERT_EQ(1,0);
  }
  TypeObject mto2 = mtm[TypeIdentifier(TI_STRONGLY_CONNECTED_COMPONENT, StronglyConnectedComponentId(TypeObjectHashId(EK_MINIMAL, EquivalenceHashWrapper(23, 96, 109, 112, 215, 248, 161, 152, 79, 50, 37, 116, 6, 230)), 5, 2))];
  ASSERT_EQ(mto2, c_to_m_mto);
}

TEST(CompleteToMinimalTypeObject, Array)
{
  using namespace OpenDDS::XTypes;
  my_mod::unTypeSupportImpl un_tsi;
  un_tsi.register_type(dp.in (), "circular_struct_arr");
  ::DDS::Topic_var my_topic =
    dp->create_topic ("circular_struct_arr",
                    "circular_struct_arr",
                    TOPIC_QOS_DEFAULT,
                    ::DDS::TopicListener::_nil(),
                    ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  ::DDS::Publisher_var pub =
    dp->create_publisher(PUBLISHER_QOS_DEFAULT,
                          ::DDS::PublisherListener::_nil(),
                          ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  ::DDS::DataWriter_var dw =
    pub->create_datawriter(my_topic.in (),
                            DATAWRITER_QOS_DEFAULT,
                            ::DDS::DataWriterListener::_nil(),
                            ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  TypeMap ctm = un_tsi.getCompleteTypeMap();
  TypeObject cto = ctm[TypeIdentifier(TI_STRONGLY_CONNECTED_COMPONENT, StronglyConnectedComponentId(TypeObjectHashId(EK_COMPLETE, EquivalenceHashWrapper(42, 203, 214, 182, 80, 84, 4, 33, 118, 109, 127, 170, 57, 95)), 5, 1))];
  TypeMap mtm = un_tsi.getMinimalTypeMap();
  TypeObject c_to_m_mto;
  if (!tls->complete_to_minimal_typeobject(cto, c_to_m_mto)) {
    ASSERT_EQ(1,0);
  }
  TypeObject mto2 = mtm[TypeIdentifier(TI_STRONGLY_CONNECTED_COMPONENT, StronglyConnectedComponentId(TypeObjectHashId(EK_MINIMAL, EquivalenceHashWrapper(23, 96, 109, 112, 215, 248, 161, 152, 79, 50, 37, 116, 6, 230)), 5, 1))];
  ASSERT_EQ(mto2, c_to_m_mto);
}

TEST(CompleteToMinimalTypeObject, AliasSequence)
{
  using namespace OpenDDS::XTypes;
  my_mod::unTypeSupportImpl un_tsi;
  un_tsi.register_type(dp.in (), "lseq");
  ::DDS::Topic_var my_topic =
    dp->create_topic ("lseq",
                    "lseq",
                    TOPIC_QOS_DEFAULT,
                    ::DDS::TopicListener::_nil(),
                    ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  ::DDS::Publisher_var pub =
    dp->create_publisher(PUBLISHER_QOS_DEFAULT,
                          ::DDS::PublisherListener::_nil(),
                          ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  ::DDS::DataWriter_var dw =
    pub->create_datawriter(my_topic.in (),
                            DATAWRITER_QOS_DEFAULT,
                            ::DDS::DataWriterListener::_nil(),
                            ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  TypeMap ctm = un_tsi.getCompleteTypeMap();
  TypeObject cto = ctm[TypeIdentifier(EK_COMPLETE, EquivalenceHashWrapper(191, 174, 166, 236, 84, 2, 202, 95, 146, 223, 113, 33, 94, 89))];
  TypeMap mtm = un_tsi.getMinimalTypeMap();
  TypeObject c_to_m_mto;
  if (!tls->complete_to_minimal_typeobject(cto, c_to_m_mto)) {
    ASSERT_EQ(1,0);
  }
  TypeObject mto2 = mtm[TypeIdentifier(EK_MINIMAL, EquivalenceHashWrapper(64, 132, 84, 128, 187, 1, 123, 173, 187, 231, 55, 190, 22, 206))];
  ASSERT_EQ(mto2, c_to_m_mto);
}

TEST(CompleteToMinimalTypeObject, AliasArray)
{
  using namespace OpenDDS::XTypes;
  my_mod::unTypeSupportImpl un_tsi;
  un_tsi.register_type(dp.in (), "larr");
  ::DDS::Topic_var my_topic =
    dp->create_topic ("larr",
                    "larr",
                    TOPIC_QOS_DEFAULT,
                    ::DDS::TopicListener::_nil(),
                    ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  ::DDS::Publisher_var pub =
    dp->create_publisher(PUBLISHER_QOS_DEFAULT,
                          ::DDS::PublisherListener::_nil(),
                          ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  ::DDS::DataWriter_var dw =
    pub->create_datawriter(my_topic.in (),
                            DATAWRITER_QOS_DEFAULT,
                            ::DDS::DataWriterListener::_nil(),
                            ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  TypeMap ctm = un_tsi.getCompleteTypeMap();
  TypeObject cto = ctm[TypeIdentifier(EK_COMPLETE, EquivalenceHashWrapper(80, 3, 53, 153, 247, 134, 18, 76, 42, 156, 63, 245, 137, 180))];
  TypeMap mtm = un_tsi.getMinimalTypeMap();
  TypeObject c_to_m_mto;
  if (!tls->complete_to_minimal_typeobject(cto, c_to_m_mto)) {
    ASSERT_EQ(1,0);
  }
  TypeObject mto2 = mtm[TypeIdentifier(EK_MINIMAL, EquivalenceHashWrapper(115, 7, 58, 161, 22, 145, 40, 98, 164, 53, 75, 57, 105, 170))];
  ASSERT_EQ(mto2, c_to_m_mto);
}

TEST(CompleteToMinimalTypeObject, Enumerated)
{
  using namespace OpenDDS::XTypes;
  my_mod::unTypeSupportImpl un_tsi;
  un_tsi.register_type(dp.in (), "EnumType");
  ::DDS::Topic_var my_topic =
    dp->create_topic ("EnumType",
                    "EnumType",
                    TOPIC_QOS_DEFAULT,
                    ::DDS::TopicListener::_nil(),
                    ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  ::DDS::Publisher_var pub =
    dp->create_publisher(PUBLISHER_QOS_DEFAULT,
                          ::DDS::PublisherListener::_nil(),
                          ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  ::DDS::DataWriter_var dw =
    pub->create_datawriter(my_topic.in (),
                            DATAWRITER_QOS_DEFAULT,
                            ::DDS::DataWriterListener::_nil(),
                            ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  TypeMap ctm = un_tsi.getCompleteTypeMap();
  TypeObject cto = ctm[TypeIdentifier(EK_COMPLETE, EquivalenceHashWrapper(208, 102, 40, 140, 43, 232, 192, 255, 146, 120, 236, 77, 165, 0))];
  TypeMap mtm = un_tsi.getMinimalTypeMap();
  TypeObject c_to_m_mto;
  if (!tls->complete_to_minimal_typeobject(cto, c_to_m_mto)) {
    ASSERT_EQ(1,0);
  }
  TypeObject mto2 = mtm[TypeIdentifier(EK_MINIMAL, EquivalenceHashWrapper(197, 93, 228, 89, 233, 88, 151, 207, 208, 98, 147, 32, 200, 147))];
  ASSERT_EQ(mto2, c_to_m_mto);
}

int main(int argc, char* argv[])
{
  ::testing::InitGoogleTest(&argc, argv);
  dpf = TheParticipantFactoryWithArgs(argc, argv);
  dp = dpf->create_participant(184,
                               PARTICIPANT_QOS_DEFAULT,
                               0,
                               0);
  OpenDDS::DCPS::DomainParticipantImpl* dp_impl =
    dynamic_cast<OpenDDS::DCPS::DomainParticipantImpl*>(dp.in());
  tls = dp_impl->get_type_lookup_service();

  return RUN_ALL_TESTS();
}
