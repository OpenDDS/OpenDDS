/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "TypeObject.h"

#include "dds/DCPS/Message_Block_Ptr.h"
#include "dds/DCPS/Hash.h"


OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace XTypes {

using DCPS::Encoding;
using DCPS::serialized_size;

const Encoding& get_typeobject_encoding()
{
  static const Encoding encoding(Encoding::KIND_XCDR2, DCPS::ENDIAN_LITTLE);
  return encoding;
}

MinimalMemberDetail::MinimalMemberDetail(const OPENDDS_STRING& name)
{
  unsigned char result[16];
  DCPS::MD5Hash(result, name.c_str(), name.size());

  std::memcpy(name_hash, result, sizeof name_hash);
}

TypeIdentifier::TypeIdentifier(ACE_CDR::Octet kind)
  : kind_(kind)
  , active_(0)
{
  activate();
}

void TypeIdentifier::activate(const TypeIdentifier* other)
{
#define OPENDDS_BRANCH_ACTIVATE(T, N) \
  active_ = new(N ## _) T;            \
  if (other) N() = other->N();        \
  break

  switch (kind_) {
  case TI_STRING8_SMALL:
  case TI_STRING16_SMALL:
    OPENDDS_BRANCH_ACTIVATE(StringSTypeDefn, string_sdefn);
  case TI_STRING8_LARGE:
  case TI_STRING16_LARGE:
    OPENDDS_BRANCH_ACTIVATE(StringLTypeDefn, string_ldefn);
  case TI_PLAIN_SEQUENCE_SMALL:
    OPENDDS_BRANCH_ACTIVATE(PlainSequenceSElemDefn, seq_sdefn);
  case TI_PLAIN_SEQUENCE_LARGE:
    OPENDDS_BRANCH_ACTIVATE(PlainSequenceLElemDefn, seq_ldefn);
  case TI_PLAIN_ARRAY_SMALL:
    OPENDDS_BRANCH_ACTIVATE(PlainArraySElemDefn, array_sdefn);
  case TI_PLAIN_ARRAY_LARGE:
    OPENDDS_BRANCH_ACTIVATE(PlainArrayLElemDefn, array_ldefn);
  case TI_PLAIN_MAP_SMALL:
    OPENDDS_BRANCH_ACTIVATE(PlainMapSTypeDefn, map_sdefn);
  case TI_PLAIN_MAP_LARGE:
    OPENDDS_BRANCH_ACTIVATE(PlainMapLTypeDefn, map_ldefn);
  case TI_STRONGLY_CONNECTED_COMPONENT:
    OPENDDS_BRANCH_ACTIVATE(StronglyConnectedComponentId, sc_component_id);
  case EK_COMPLETE:
  case EK_MINIMAL:
    active_ = equivalence_hash_;
    if (other) {
      std::memcpy(equivalence_hash(), other->equivalence_hash(), sizeof(EquivalenceHash));
    }
    break;
  case TK_NONE:
  case TK_BOOLEAN:
  case TK_BYTE:
  case TK_INT16:
  case TK_INT32:
  case TK_INT64:
  case TK_UINT16:
  case TK_UINT32:
  case TK_UINT64:
  case TK_FLOAT32:
  case TK_FLOAT64:
  case TK_FLOAT128:
  case TK_INT8:
  case TK_UINT8:
  case TK_CHAR8:
  case TK_CHAR16:
    break; // no-op, no member selected
  default:
    OPENDDS_BRANCH_ACTIVATE(ExtendedTypeDefn, extended_defn);
  }
}

TypeIdentifier::TypeIdentifier(const TypeIdentifier& other)
  : kind_(other.kind_)
  , active_(0)
{
  activate(&other);
}

void TypeIdentifier::reset()
{
  if (!active_) {
    return;
  }
#define OPENDDS_BRANCH_RESET(T) static_cast<T*>(active_)->~T(); break
  switch (kind_) {
  case TI_STRING8_SMALL:
  case TI_STRING16_SMALL:
    OPENDDS_BRANCH_RESET(StringSTypeDefn);
  case TI_STRING8_LARGE:
  case TI_STRING16_LARGE:
    OPENDDS_BRANCH_RESET(StringLTypeDefn);
  case TI_PLAIN_SEQUENCE_SMALL:
    OPENDDS_BRANCH_RESET(PlainSequenceSElemDefn);
  case TI_PLAIN_SEQUENCE_LARGE:
    OPENDDS_BRANCH_RESET(PlainSequenceLElemDefn);
  case TI_PLAIN_ARRAY_SMALL:
    OPENDDS_BRANCH_RESET(PlainArraySElemDefn);
  case TI_PLAIN_ARRAY_LARGE:
    OPENDDS_BRANCH_RESET(PlainArrayLElemDefn);
  case TI_PLAIN_MAP_SMALL:
    OPENDDS_BRANCH_RESET(PlainMapSTypeDefn);
  case TI_PLAIN_MAP_LARGE:
    OPENDDS_BRANCH_RESET(PlainMapLTypeDefn);
  case TI_STRONGLY_CONNECTED_COMPONENT:
    OPENDDS_BRANCH_RESET(StronglyConnectedComponentId);
  case EK_COMPLETE:
  case EK_MINIMAL:
    break; // no-op, data is just an array of octets
  case TK_NONE:
  case TK_BOOLEAN:
  case TK_BYTE:
  case TK_INT16:
  case TK_INT32:
  case TK_INT64:
  case TK_UINT16:
  case TK_UINT32:
  case TK_UINT64:
  case TK_FLOAT32:
  case TK_FLOAT64:
  case TK_FLOAT128:
  case TK_INT8:
  case TK_UINT8:
  case TK_CHAR8:
  case TK_CHAR16:
    break; // no-op, no member selected
  default:
    OPENDDS_BRANCH_RESET(ExtendedTypeDefn);
  }
}

TypeIdentifier& TypeIdentifier::operator=(const TypeIdentifier& other)
{
  if (&other == this) {
    return *this;
  }
  reset();
  kind_ = other.kind_;
  activate(&other);
  return *this;
}

TypeIdentifier::TypeIdentifier(ACE_CDR::Octet k, const StringSTypeDefn& sdefn)
  : kind_(k)
{
  activate();
  string_sdefn() = sdefn;
}

TypeIdentifier::TypeIdentifier(ACE_CDR::Octet k, const StringLTypeDefn& ldefn)
  : kind_(k)
{
  activate();
  string_ldefn() = ldefn;
}

TypeIdentifier::TypeIdentifier(ACE_CDR::Octet k, const PlainSequenceSElemDefn& sdefn)
  : kind_(k)
{
  activate();
  seq_sdefn() = sdefn;
}

TypeIdentifier::TypeIdentifier(ACE_CDR::Octet k, const PlainSequenceLElemDefn& ldefn)
  : kind_(k)
{
  activate();
  seq_ldefn() = ldefn;
}

TypeIdentifier::TypeIdentifier(ACE_CDR::Octet k, const PlainArraySElemDefn& sdefn)
  : kind_(k)
{
  activate();
  array_sdefn() = sdefn;
}

TypeIdentifier::TypeIdentifier(ACE_CDR::Octet k, const PlainArrayLElemDefn& ldefn)
  : kind_(k)
{
  activate();
  array_ldefn() = ldefn;
}

TypeIdentifier::TypeIdentifier(ACE_CDR::Octet k, const EquivalenceHashWrapper& eh)
  : kind_(k)
{
  activate();
  std::memcpy(equivalence_hash(), eh.eh_, sizeof eh.eh_);
}

TypeIdentifier::TypeIdentifier(ACE_CDR::Octet k, const StronglyConnectedComponentId& id)
  : kind_(k)
{
  activate();
  sc_component_id() = id;
}

TypeIdentifier makeTypeIdentifier(const TypeObject& type_object, const DCPS::Encoding* encoding_option)
{
  OPENDDS_ASSERT(type_object.kind == EK_MINIMAL || type_object.kind == EK_COMPLETE);

  const Encoding& encoding = encoding_option ? *encoding_option : get_typeobject_encoding();
  size_t size = serialized_size(encoding, type_object);
  ACE_Message_Block buff(size);
  DCPS::Serializer ser(&buff, encoding);
  ser << type_object;

  unsigned char result[16];
  DCPS::MD5Hash(result, buff.rd_ptr(), buff.length());

  // First 14 bytes of MD5 of the serialized TypeObject using XCDR
  // version 2 with Little Endian encoding
  TypeIdentifier ti(type_object.kind);
  std::memcpy(ti.equivalence_hash(), result, sizeof(EquivalenceHash));

  return ti;
}

ACE_CDR::ULong hash_member_name_to_id(const OPENDDS_STRING& name)
{
  ACE_CDR::ULong name_hash;

  unsigned char result[16];
  DCPS::MD5Hash(result, name.c_str(), name.size());

  std::memcpy(&name_hash, result, sizeof name_hash);
  return name_hash & 0x0FFFFFFF;
}

void hash_member_name(NameHash& name_hash, const OPENDDS_STRING& name)
{
  unsigned char result[16];
  DCPS::MD5Hash(result, name.c_str(), name.size());

  std::memcpy(&name_hash, result, sizeof name_hash);
}

bool is_fully_descriptive(const TypeIdentifier& ti)
{
  switch (ti.kind()) {
  case TK_BOOLEAN:
  case TK_BYTE:
  case TK_INT16:
  case TK_INT32:
  case TK_INT64:
  case TK_UINT16:
  case TK_UINT32:
  case TK_UINT64:
  case TK_FLOAT32:
  case TK_FLOAT64:
  case TK_FLOAT128:
  case TK_INT8:
  case TK_UINT8:
  case TK_CHAR8:
  case TK_CHAR16:
  case TI_STRING8_SMALL:
  case TI_STRING8_LARGE:
  case TI_STRING16_SMALL:
  case TI_STRING16_LARGE:
    return true;
  case TI_PLAIN_SEQUENCE_SMALL:
    return ti.seq_sdefn().header.equiv_kind == EK_BOTH;
  case TI_PLAIN_SEQUENCE_LARGE:
    return ti.seq_ldefn().header.equiv_kind == EK_BOTH;
  case TI_PLAIN_ARRAY_SMALL:
    return ti.array_sdefn().header.equiv_kind == EK_BOTH;
  case TI_PLAIN_ARRAY_LARGE:
    return ti.array_ldefn().header.equiv_kind == EK_BOTH;
  case TI_PLAIN_MAP_SMALL:
    return ti.map_sdefn().header.equiv_kind == EK_BOTH;
  case TI_PLAIN_MAP_LARGE:
    return ti.map_ldefn().header.equiv_kind == EK_BOTH;
  }
  return false;
}

bool is_plain_collection(const TypeIdentifier& ti)
{
  switch (ti.kind()) {
  case TI_PLAIN_SEQUENCE_SMALL:
  case TI_PLAIN_SEQUENCE_LARGE:
  case TI_PLAIN_ARRAY_SMALL:
  case TI_PLAIN_ARRAY_LARGE:
  case TI_PLAIN_MAP_SMALL:
  case TI_PLAIN_MAP_LARGE:
    return true;
  }
  return false;
}

bool has_type_object(const TypeIdentifier& ti)
{
  return !is_fully_descriptive(ti) && !is_plain_collection(ti) &&
    ti.kind() != TK_NONE;
}

namespace {
template <typename T>
void compute_dependencies(const TypeMap& type_map,
                          const Sequence<T>& type,
                          OPENDDS_SET(TypeIdentifier)& dependencies);

void compute_dependencies(const TypeMap& type_map,
                          const CommonAliasBody& type,
                          OPENDDS_SET(TypeIdentifier)& dependencies)
{
  compute_dependencies(type_map, type.related_type, dependencies);
}

void compute_dependencies(const TypeMap& type_map,
                          const MinimalAliasBody& type,
                          OPENDDS_SET(TypeIdentifier)& dependencies)
{
  compute_dependencies(type_map, type.common, dependencies);
}

void compute_dependencies(const TypeMap& type_map,
                          const MinimalAliasType& type,
                          OPENDDS_SET(TypeIdentifier)& dependencies)
{
  compute_dependencies(type_map, type.body, dependencies);
}

void compute_dependencies(const TypeMap& type_map,
                          const AppliedAnnotation& ann,
                          OPENDDS_SET(TypeIdentifier)& dependencies)
{
  compute_dependencies(type_map, ann.annotation_typeid, dependencies);
}

void compute_dependencies(const TypeMap& type_map,
                          const Optional<AppliedAnnotationSeq>& ann_seq,
                          OPENDDS_SET(TypeIdentifier)& dependencies)
{
  if (ann_seq.present) {
    compute_dependencies(type_map, ann_seq.value, dependencies);
  }
}

void compute_dependencies(const TypeMap& type_map,
                          const CompleteTypeDetail& type_detail,
                          OPENDDS_SET(TypeIdentifier)& dependencies)
{
  compute_dependencies(type_map, type_detail.ann_custom, dependencies);
}

void compute_dependencies(const TypeMap& type_map,
                          const CompleteAliasHeader& header,
                          OPENDDS_SET(TypeIdentifier)& dependencies)
{
  compute_dependencies(type_map, header.detail, dependencies);
}

void compute_dependencies(const TypeMap& type_map,
                          const CompleteAliasBody& body,
                          OPENDDS_SET(TypeIdentifier)& dependencies)
{
  compute_dependencies(type_map, body.common, dependencies);
  compute_dependencies(type_map, body.ann_custom, dependencies);
}

void compute_dependencies(const TypeMap& type_map,
                          const CompleteAliasType& type,
                          OPENDDS_SET(TypeIdentifier)& dependencies)
{
  compute_dependencies(type_map, type.header, dependencies);
  compute_dependencies(type_map, type.body, dependencies);
}

void compute_dependencies(const TypeMap& type_map,
                          const CommonAnnotationParameter& type,
                          OPENDDS_SET(TypeIdentifier)& dependencies)
{
  compute_dependencies(type_map, type.member_type_id, dependencies);
}

void compute_dependencies(const TypeMap& type_map,
                          const MinimalAnnotationParameter& type,
                          OPENDDS_SET(TypeIdentifier)& dependencies)
{
  compute_dependencies(type_map, type.common, dependencies);
}

void compute_dependencies(const TypeMap& type_map,
                          const MinimalAnnotationType& type,
                          OPENDDS_SET(TypeIdentifier)& dependencies)
{
  compute_dependencies(type_map, type.member_seq, dependencies);
}

void compute_dependencies(const TypeMap& type_map,
                          const CompleteAnnotationParameter& type,
                          OPENDDS_SET(TypeIdentifier)& dependencies)
{
  compute_dependencies(type_map, type.common, dependencies);
}

void compute_dependencies(const TypeMap& type_map,
                          const CompleteAnnotationType& type,
                          OPENDDS_SET(TypeIdentifier)& dependencies)
{
  compute_dependencies(type_map, type.member_seq, dependencies);
}

void compute_dependencies(const TypeMap& type_map,
                          const MinimalStructHeader& type,
                          OPENDDS_SET(TypeIdentifier)& dependencies)
{
  compute_dependencies(type_map, type.base_type, dependencies);
}

void compute_dependencies(const TypeMap& type_map,
                          const CommonStructMember& type,
                          OPENDDS_SET(TypeIdentifier)& dependencies)
{
  compute_dependencies(type_map, type.member_type_id, dependencies);
}

void compute_dependencies(const TypeMap& type_map,
                          const MinimalStructMember& type,
                          OPENDDS_SET(TypeIdentifier)& dependencies)
{
  compute_dependencies(type_map, type.common, dependencies);
}

void compute_dependencies(const TypeMap& type_map,
                          const MinimalStructType& type,
                          OPENDDS_SET(TypeIdentifier)& dependencies)
{
  compute_dependencies(type_map, type.header, dependencies);
  compute_dependencies(type_map, type.member_seq, dependencies);
}

void compute_dependencies(const TypeMap& type_map,
                          const CompleteStructHeader& header,
                          OPENDDS_SET(TypeIdentifier)& dependencies)
{
  compute_dependencies(type_map, header.base_type, dependencies);
  compute_dependencies(type_map, header.detail, dependencies);
}

void compute_dependencies(const TypeMap& type_map,
                          const CompleteMemberDetail& detail,
                          OPENDDS_SET(TypeIdentifier)& dependencies)
{
  compute_dependencies(type_map, detail.ann_custom, dependencies);
}

void compute_dependencies(const TypeMap& type_map,
                          const CompleteStructMember& member,
                          OPENDDS_SET(TypeIdentifier)& dependencies)
{
  compute_dependencies(type_map, member.common, dependencies);
  compute_dependencies(type_map, member.detail, dependencies);
}

void compute_dependencies(const TypeMap& type_map,
                          const CompleteStructType& type,
                          OPENDDS_SET(TypeIdentifier)& dependencies)
{
  compute_dependencies(type_map, type.header, dependencies);
  compute_dependencies(type_map, type.member_seq, dependencies);
}

void compute_dependencies(const TypeMap& type_map,
                          const CommonDiscriminatorMember& type,
                          OPENDDS_SET(TypeIdentifier)& dependencies)
{
  compute_dependencies(type_map, type.type_id, dependencies);
}

void compute_dependencies(const TypeMap& type_map,
                          const MinimalDiscriminatorMember& type,
                          OPENDDS_SET(TypeIdentifier)& dependencies)
{
  compute_dependencies(type_map, type.common, dependencies);
}

void compute_dependencies(const TypeMap& type_map,
                          const CommonUnionMember& type,
                          OPENDDS_SET(TypeIdentifier)& dependencies)
{
  compute_dependencies(type_map, type.type_id, dependencies);
}

void compute_dependencies(const TypeMap& type_map,
                          const MinimalUnionMember& type,
                          OPENDDS_SET(TypeIdentifier)& dependencies)
{
  compute_dependencies(type_map, type.common, dependencies);
}

void compute_dependencies(const TypeMap& type_map,
                          const MinimalUnionType& type,
                          OPENDDS_SET(TypeIdentifier)& dependencies)
{
  compute_dependencies(type_map, type.discriminator, dependencies);
  compute_dependencies(type_map, type.member_seq, dependencies);
}

void compute_dependencies(const TypeMap& type_map,
                          const CompleteUnionHeader& header,
                          OPENDDS_SET(TypeIdentifier)& dependencies)
{
  compute_dependencies(type_map, header.detail, dependencies);
}

void compute_dependencies(const TypeMap& type_map,
                          const CompleteDiscriminatorMember& type,
                          OPENDDS_SET(TypeIdentifier)& dependencies)
{
  compute_dependencies(type_map, type.common, dependencies);
  compute_dependencies(type_map, type.ann_custom, dependencies);
}

void compute_dependencies(const TypeMap& type_map,
                          const CompleteUnionMember& type,
                          OPENDDS_SET(TypeIdentifier)& dependencies)
{
  compute_dependencies(type_map, type.common, dependencies);
  compute_dependencies(type_map, type.detail, dependencies);
}

void compute_dependencies(const TypeMap& type_map,
                          const CompleteUnionType& type,
                          OPENDDS_SET(TypeIdentifier)& dependencies)
{
  compute_dependencies(type_map, type.header, dependencies);
  compute_dependencies(type_map, type.discriminator, dependencies);
  compute_dependencies(type_map, type.member_seq, dependencies);
}

void compute_dependencies(const TypeMap&,
                          const MinimalBitsetType&,
                          OPENDDS_SET(TypeIdentifier)&)
{
  // Doesn't have any dependencies.
}

void compute_dependencies(const TypeMap& type_map,
                          const CompleteBitsetHeader& header,
                          OPENDDS_SET(TypeIdentifier)& dependencies)
{
  compute_dependencies(type_map, header.detail, dependencies);
}

void compute_dependencies(const TypeMap& type_map,
                          const CompleteBitfield& field,
                          OPENDDS_SET(TypeIdentifier)& dependencies)
{
  compute_dependencies(type_map, field.detail, dependencies);
}

void compute_dependencies(const TypeMap& type_map,
                          const CompleteBitsetType& type,
                          OPENDDS_SET(TypeIdentifier)& dependencies)
{
  compute_dependencies(type_map, type.header, dependencies);
  compute_dependencies(type_map, type.field_seq, dependencies);
}

void compute_dependencies(const TypeMap& type_map,
                          const CommonCollectionElement& type,
                          OPENDDS_SET(TypeIdentifier)& dependencies)
{
  compute_dependencies(type_map, type.type, dependencies);
}

void compute_dependencies(const TypeMap& type_map,
                          const MinimalCollectionElement& type,
                          OPENDDS_SET(TypeIdentifier)& dependencies)
{
  compute_dependencies(type_map, type.common, dependencies);
}

void compute_dependencies(const TypeMap& type_map,
                          const MinimalSequenceType& type,
                          OPENDDS_SET(TypeIdentifier)& dependencies)
{
  compute_dependencies(type_map, type.element, dependencies);
}

void compute_dependencies(const TypeMap& type_map,
                          const CompleteCollectionHeader& header,
                          OPENDDS_SET(TypeIdentifier)& dependencies)
{
  if (header.detail.present) {
    compute_dependencies(type_map, header.detail.value, dependencies);
  }
}

void compute_dependencies(const TypeMap& type_map,
                          const CompleteElementDetail& detail,
                          OPENDDS_SET(TypeIdentifier)& dependencies)
{
  compute_dependencies(type_map, detail.ann_custom, dependencies);
}

void compute_dependencies(const TypeMap& type_map,
                          const CompleteCollectionElement& element,
                          OPENDDS_SET(TypeIdentifier)& dependencies)
{
  compute_dependencies(type_map, element.common, dependencies);
  compute_dependencies(type_map, element.detail, dependencies);
}

void compute_dependencies(const TypeMap& type_map,
                          const CompleteSequenceType& type,
                          OPENDDS_SET(TypeIdentifier)& dependencies)
{
  compute_dependencies(type_map, type.header, dependencies);
  compute_dependencies(type_map, type.element, dependencies);
}

void compute_dependencies(const TypeMap& type_map,
                          const MinimalArrayType& type,
                          OPENDDS_SET(TypeIdentifier)& dependencies)
{
  compute_dependencies(type_map, type.element, dependencies);
}

void compute_dependencies(const TypeMap& type_map,
                          const CompleteArrayHeader& header,
                          OPENDDS_SET(TypeIdentifier)& dependencies)
{
  compute_dependencies(type_map, header.detail, dependencies);
}

void compute_dependencies(const TypeMap& type_map,
                          const CompleteArrayType& type,
                          OPENDDS_SET(TypeIdentifier)& dependencies)
{
  compute_dependencies(type_map, type.header, dependencies);
  compute_dependencies(type_map, type.element, dependencies);
}

void compute_dependencies(const TypeMap& type_map,
                          const MinimalMapType& type,
                          OPENDDS_SET(TypeIdentifier)& dependencies)
{
  compute_dependencies(type_map, type.key, dependencies);
  compute_dependencies(type_map, type.element, dependencies);
}

void compute_dependencies(const TypeMap& type_map,
                          const CompleteMapType& type,
                          OPENDDS_SET(TypeIdentifier)& dependencies)
{
  compute_dependencies(type_map, type.header, dependencies);
  compute_dependencies(type_map, type.key, dependencies);
  compute_dependencies(type_map, type.element, dependencies);
}

void compute_dependencies(const TypeMap&,
                          const MinimalEnumeratedType&,
                          OPENDDS_SET(TypeIdentifier)&)
{
  // Doesn't have any dependencies.
}

void compute_dependencies(const TypeMap& type_map,
                          const CompleteEnumeratedHeader& header,
                          OPENDDS_SET(TypeIdentifier)& dependencies)
{
  compute_dependencies(type_map, header.detail, dependencies);
}

void compute_dependencies(const TypeMap& type_map,
                          const CompleteEnumeratedLiteral& literal,
                          OPENDDS_SET(TypeIdentifier)& dependencies)
{
  compute_dependencies(type_map, literal.detail, dependencies);
}

void compute_dependencies(const TypeMap& type_map,
                          const CompleteEnumeratedType& type,
                          OPENDDS_SET(TypeIdentifier)& dependencies)
{
  compute_dependencies(type_map, type.header, dependencies);
  compute_dependencies(type_map, type.literal_seq, dependencies);
}

void compute_dependencies(const TypeMap&,
                          const MinimalBitmaskType&,
                          OPENDDS_SET(TypeIdentifier)&)
{
  // Doesn't have any dependencies.
}

void compute_dependencies(const TypeMap& type_map,
                          const CompleteBitflag& bitflag,
                          OPENDDS_SET(TypeIdentifier)& dependencies)
{
  compute_dependencies(type_map, bitflag.detail, dependencies);
}

void compute_dependencies(const TypeMap& type_map,
                          const CompleteBitmaskType& type,
                          OPENDDS_SET(TypeIdentifier)& dependencies)
{
  compute_dependencies(type_map, type.header, dependencies);
  compute_dependencies(type_map, type.flag_seq, dependencies);
}

void compute_dependencies(const TypeMap& type_map,
                          const MinimalTypeObject& type_object,
                          OPENDDS_SET(TypeIdentifier)& dependencies)
{
  switch (type_object.kind) {
  case TK_ALIAS:
    compute_dependencies(type_map, type_object.alias_type, dependencies);
    break;
  case TK_ANNOTATION:
    compute_dependencies(type_map, type_object.annotation_type, dependencies);
    break;
  case TK_STRUCTURE:
    compute_dependencies(type_map, type_object.struct_type, dependencies);
    break;
  case TK_UNION:
    compute_dependencies(type_map, type_object.union_type, dependencies);
    break;
  case TK_BITSET:
    compute_dependencies(type_map, type_object.bitset_type, dependencies);
    break;
  case TK_SEQUENCE:
    compute_dependencies(type_map, type_object.sequence_type, dependencies);
    break;
  case TK_ARRAY:
    compute_dependencies(type_map, type_object.array_type, dependencies);
    break;
  case TK_MAP:
    compute_dependencies(type_map, type_object.map_type, dependencies);
    break;
  case TK_ENUM:
    compute_dependencies(type_map, type_object.enumerated_type, dependencies);
    break;
  case TK_BITMASK:
    compute_dependencies(type_map, type_object.bitmask_type, dependencies);
    break;
  }
}

void compute_dependencies(const TypeMap& type_map,
                          const CompleteTypeObject& type_object,
                          OPENDDS_SET(TypeIdentifier)& dependencies)
{
  switch (type_object.kind) {
  case TK_ALIAS:
    compute_dependencies(type_map, type_object.alias_type, dependencies);
    break;
  case TK_ANNOTATION:
    compute_dependencies(type_map, type_object.annotation_type, dependencies);
    break;
  case TK_STRUCTURE:
    compute_dependencies(type_map, type_object.struct_type, dependencies);
    break;
  case TK_UNION:
    compute_dependencies(type_map, type_object.union_type, dependencies);
    break;
  case TK_BITSET:
    compute_dependencies(type_map, type_object.bitset_type, dependencies);
    break;
  case TK_SEQUENCE:
    compute_dependencies(type_map, type_object.sequence_type, dependencies);
    break;
  case TK_ARRAY:
    compute_dependencies(type_map, type_object.array_type, dependencies);
    break;
  case TK_MAP:
    compute_dependencies(type_map, type_object.map_type, dependencies);
    break;
  case TK_ENUM:
    compute_dependencies(type_map, type_object.enumerated_type, dependencies);
    break;
  case TK_BITMASK:
    compute_dependencies(type_map, type_object.bitmask_type, dependencies);
    break;
  }
}

void compute_dependencies(const TypeMap& type_map,
                          const TypeObject& type_object,
                          OPENDDS_SET(TypeIdentifier)& dependencies)
{
  switch (type_object.kind) {
  case EK_COMPLETE:
    compute_dependencies(type_map, type_object.complete, dependencies);
    break;
  case EK_MINIMAL:
    compute_dependencies(type_map, type_object.minimal, dependencies);
    break;
  }
}

void compute_dependencies(const TypeMap&,
                          const StringSTypeDefn&,
                          OPENDDS_SET(TypeIdentifier)&)
{
  // Do nothing.
}

void compute_dependencies(const TypeMap&,
                          const StringLTypeDefn&,
                          OPENDDS_SET(TypeIdentifier)&)
{
  // Do nothing.
}

void compute_dependencies(const TypeMap&,
                          const PlainCollectionHeader&,
                          OPENDDS_SET(TypeIdentifier)&)
{
  // Do nothing.
}

void compute_dependencies(const TypeMap& type_map,
                          const PlainSequenceSElemDefn& type,
                          OPENDDS_SET(TypeIdentifier)& dependencies)
{
  compute_dependencies(type_map, type.header, dependencies);
  compute_dependencies(type_map, *type.element_identifier, dependencies);
}

void compute_dependencies(const TypeMap& type_map,
                          const PlainSequenceLElemDefn& type,
                          OPENDDS_SET(TypeIdentifier)& dependencies)
{
  compute_dependencies(type_map, type.header, dependencies);
  compute_dependencies(type_map, *type.element_identifier, dependencies);
}

void compute_dependencies(const TypeMap& type_map,
                          const PlainArraySElemDefn& type,
                          OPENDDS_SET(TypeIdentifier)& dependencies)
{
  compute_dependencies(type_map, type.header, dependencies);
  compute_dependencies(type_map, *type.element_identifier, dependencies);
}

void compute_dependencies(const TypeMap& type_map,
                          const PlainArrayLElemDefn& type,
                          OPENDDS_SET(TypeIdentifier)& dependencies)
{
  compute_dependencies(type_map, type.header, dependencies);
  compute_dependencies(type_map, *type.element_identifier, dependencies);
}

void compute_dependencies(const TypeMap& type_map,
                          const PlainMapSTypeDefn& type,
                          OPENDDS_SET(TypeIdentifier)& dependencies)
{
  compute_dependencies(type_map, type.header, dependencies);
  compute_dependencies(type_map, *type.element_identifier, dependencies);
  compute_dependencies(type_map, *type.key_identifier, dependencies);
}

void compute_dependencies(const TypeMap& type_map,
                          const PlainMapLTypeDefn& type,
                          OPENDDS_SET(TypeIdentifier)& dependencies)
{
  compute_dependencies(type_map, type.header, dependencies);
  compute_dependencies(type_map, *type.element_identifier, dependencies);
  compute_dependencies(type_map, *type.key_identifier, dependencies);
}

void compute_dependencies(const TypeMap&,
                          const StronglyConnectedComponentId&,
                          OPENDDS_SET(TypeIdentifier)&)
{
  // Do nothing.
}

template <typename T>
void compute_dependencies(const TypeMap& type_map,
                          const Sequence<T>& type,
                          OPENDDS_SET(TypeIdentifier)& dependencies)
{
  for (typename Sequence<T>::const_iterator pos = type.begin(), limit = type.end(); pos != limit; ++pos) {
    compute_dependencies(type_map, *pos, dependencies);
  }
}

}

void compute_dependencies(const TypeMap& type_map,
                          const TypeIdentifier& type_identifier,
                          OPENDDS_SET(TypeIdentifier)& dependencies)
{
  if (dependencies.count(type_identifier) != 0) {
    return;
  }

  dependencies.insert(type_identifier);

  switch (type_identifier.kind()) {
  case TI_STRING8_SMALL:
  case TI_STRING16_SMALL:
    compute_dependencies(type_map, type_identifier.string_sdefn(), dependencies);
    break;
  case TI_STRING8_LARGE:
  case TI_STRING16_LARGE:
    compute_dependencies(type_map, type_identifier.string_ldefn(), dependencies);
    break;
  case TI_PLAIN_SEQUENCE_SMALL:
    compute_dependencies(type_map, type_identifier.seq_sdefn(), dependencies);
    break;
  case TI_PLAIN_SEQUENCE_LARGE:
    compute_dependencies(type_map, type_identifier.seq_ldefn(), dependencies);
    break;
  case TI_PLAIN_ARRAY_SMALL:
    compute_dependencies(type_map, type_identifier.array_sdefn(), dependencies);
    break;
  case TI_PLAIN_ARRAY_LARGE:
    compute_dependencies(type_map, type_identifier.array_ldefn(), dependencies);
    break;
  case TI_PLAIN_MAP_SMALL:
    compute_dependencies(type_map, type_identifier.map_sdefn(), dependencies);
    break;
  case TI_PLAIN_MAP_LARGE:
    compute_dependencies(type_map, type_identifier.map_ldefn(), dependencies);
    break;
  case TI_STRONGLY_CONNECTED_COMPONENT:
    compute_dependencies(type_map, type_identifier.sc_component_id(), dependencies);
    break;
  case EK_COMPLETE:
  case EK_MINIMAL:
    {
      TypeMap::const_iterator pos = type_map.find(type_identifier);
      if (pos != type_map.end()) {
        compute_dependencies(type_map, pos->second, dependencies);
      }
      break;
    }
  }
}

bool write_empty_xcdr2_nonfinal(DCPS::Serializer& strm)
{
  size_t size = 0;
  serialized_size_delimiter(strm.encoding(), size);
  return strm.write_delimiter(size);
}

bool read_empty_xcdr2_nonfinal(DCPS::Serializer& strm)
{
  size_t total_size = 0;
  if (!strm.read_delimiter(total_size)) {
    return false;
  }
  return strm.skip(total_size);
}

} // namespace XTypes

namespace DCPS {

// Serialization support for TypeObject and its components

void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::TypeIdentifier& uni)
{
  primitive_serialized_size_octet(encoding, size);

  switch (uni.kind()) {
  case XTypes::TI_STRING8_SMALL:
  case XTypes::TI_STRING16_SMALL:
    serialized_size(encoding, size, uni.string_sdefn());
    break;
  case XTypes::TI_STRING8_LARGE:
  case XTypes::TI_STRING16_LARGE:
    serialized_size(encoding, size, uni.string_ldefn());
    break;
  case XTypes::TI_PLAIN_SEQUENCE_SMALL:
    serialized_size(encoding, size, uni.seq_sdefn());
    break;
  case XTypes::TI_PLAIN_SEQUENCE_LARGE:
    serialized_size(encoding, size, uni.seq_ldefn());
    break;
  case XTypes::TI_PLAIN_ARRAY_SMALL:
    serialized_size(encoding, size, uni.array_sdefn());
    break;
  case XTypes::TI_PLAIN_ARRAY_LARGE:
    serialized_size(encoding, size, uni.array_ldefn());
    break;
  case XTypes::TI_PLAIN_MAP_SMALL:
    serialized_size(encoding, size, uni.map_sdefn());
    break;
  case XTypes::TI_PLAIN_MAP_LARGE:
    serialized_size(encoding, size, uni.map_ldefn());
    break;
  case XTypes::TI_STRONGLY_CONNECTED_COMPONENT:
    serialized_size(encoding, size, uni.sc_component_id());
    break;
  case XTypes::EK_COMPLETE:
  case XTypes::EK_MINIMAL:
    {
      XTypes::EquivalenceHash_forany uni_equivalence_hash(const_cast<XTypes::EquivalenceHash_slice*>(uni.equivalence_hash()));
      serialized_size(encoding, size, uni_equivalence_hash);
      break;
    }
  case XTypes::TK_NONE:
  case XTypes::TK_BOOLEAN:
  case XTypes::TK_BYTE:
  case XTypes::TK_INT16:
  case XTypes::TK_INT32:
  case XTypes::TK_INT64:
  case XTypes::TK_UINT16:
  case XTypes::TK_UINT32:
  case XTypes::TK_UINT64:
  case XTypes::TK_FLOAT32:
  case XTypes::TK_FLOAT64:
  case XTypes::TK_FLOAT128:
  case XTypes::TK_INT8:
  case XTypes::TK_UINT8:
  case XTypes::TK_CHAR8:
  case XTypes::TK_CHAR16:
    break; // no-op, no member selected
  default:
    serialized_size(encoding, size, uni.extended_defn());
  }
}

bool operator<<(Serializer& strm, const XTypes::TypeIdentifier& uni)
{
  if (!(strm << ACE_OutputCDR::from_octet(uni.kind()))) {
    return false;
  }

  switch (uni.kind()) {
  case XTypes::TI_STRING8_SMALL:
  case XTypes::TI_STRING16_SMALL:
    return (strm << uni.string_sdefn());
  case XTypes::TI_STRING8_LARGE:
  case XTypes::TI_STRING16_LARGE:
    return (strm << uni.string_ldefn());
  case XTypes::TI_PLAIN_SEQUENCE_SMALL:
    return (strm << uni.seq_sdefn());
  case XTypes::TI_PLAIN_SEQUENCE_LARGE:
    return (strm << uni.seq_ldefn());
  case XTypes::TI_PLAIN_ARRAY_SMALL:
    return (strm << uni.array_sdefn());
  case XTypes::TI_PLAIN_ARRAY_LARGE:
    return (strm << uni.array_ldefn());
  case XTypes::TI_PLAIN_MAP_SMALL:
    return (strm << uni.map_sdefn());
  case XTypes::TI_PLAIN_MAP_LARGE:
    return (strm << uni.map_ldefn());
  case XTypes::TI_STRONGLY_CONNECTED_COMPONENT:
    return (strm << uni.sc_component_id());
  case XTypes::EK_COMPLETE:
  case XTypes::EK_MINIMAL:
    {
      XTypes::EquivalenceHash_forany uni_equivalence_hash(const_cast<XTypes::EquivalenceHash_slice*>(uni.equivalence_hash()));
      return (strm << uni_equivalence_hash);
    }
  case XTypes::TK_NONE:
  case XTypes::TK_BOOLEAN:
  case XTypes::TK_BYTE:
  case XTypes::TK_INT16:
  case XTypes::TK_INT32:
  case XTypes::TK_INT64:
  case XTypes::TK_UINT16:
  case XTypes::TK_UINT32:
  case XTypes::TK_UINT64:
  case XTypes::TK_FLOAT32:
  case XTypes::TK_FLOAT64:
  case XTypes::TK_FLOAT128:
  case XTypes::TK_INT8:
  case XTypes::TK_UINT8:
  case XTypes::TK_CHAR8:
  case XTypes::TK_CHAR16:
    return true; // no-op, no member selected
  default:
    return (strm << uni.extended_defn());
  }
}

bool operator>>(Serializer& strm, XTypes::TypeIdentifier& uni)
{
  ACE_CDR::Octet k;
  if (!(strm >> ACE_InputCDR::to_octet(k))) {
    return false;
  }
  uni = XTypes::TypeIdentifier(k);

  switch (k) {
  case XTypes::TI_STRING8_SMALL:
  case XTypes::TI_STRING16_SMALL:
    return (strm >> uni.string_sdefn());
  case XTypes::TI_STRING8_LARGE:
  case XTypes::TI_STRING16_LARGE:
    return (strm >> uni.string_ldefn());
  case XTypes::TI_PLAIN_SEQUENCE_SMALL:
    return (strm >> uni.seq_sdefn());
  case XTypes::TI_PLAIN_SEQUENCE_LARGE:
    return (strm >> uni.seq_ldefn());
  case XTypes::TI_PLAIN_ARRAY_SMALL:
    return (strm >> uni.array_sdefn());
  case XTypes::TI_PLAIN_ARRAY_LARGE:
    return (strm >> uni.array_ldefn());
  case XTypes::TI_PLAIN_MAP_SMALL:
    return (strm >> uni.map_sdefn());
  case XTypes::TI_PLAIN_MAP_LARGE:
    return (strm >> uni.map_ldefn());
  case XTypes::TI_STRONGLY_CONNECTED_COMPONENT:
    return (strm >> uni.sc_component_id());
  case XTypes::EK_COMPLETE:
  case XTypes::EK_MINIMAL:
    {
      XTypes::EquivalenceHash_forany uni_equivalence_hash(uni.equivalence_hash());
      return (strm >> uni_equivalence_hash);
    }
  case XTypes::TK_NONE:
  case XTypes::TK_BOOLEAN:
  case XTypes::TK_BYTE:
  case XTypes::TK_INT16:
  case XTypes::TK_INT32:
  case XTypes::TK_INT64:
  case XTypes::TK_UINT16:
  case XTypes::TK_UINT32:
  case XTypes::TK_UINT64:
  case XTypes::TK_FLOAT32:
  case XTypes::TK_FLOAT64:
  case XTypes::TK_FLOAT128:
  case XTypes::TK_INT8:
  case XTypes::TK_UINT8:
  case XTypes::TK_CHAR8:
  case XTypes::TK_CHAR16:
    return true; // no-op, no member selected
  default:
    return (strm >> uni.extended_defn());
  }
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::LBoundSeq& seq)
{
  DCPS::primitive_serialized_size_ulong(encoding, size);
  if (seq.length() == 0) {
    return;
  }
  primitive_serialized_size(encoding, size, ACE_CDR::ULong(), seq.length());
}

bool operator<<(Serializer& strm, const XTypes::LBoundSeq& seq)
{
  const ACE_CDR::ULong length = seq.length();
  if (!(strm << length)) {
    return false;
  }
  if (length == 0) {
    return true;
  }
  return strm.write_ulong_array(seq.get_buffer(), length);
}

bool operator>>(Serializer& strm, XTypes::LBoundSeq& seq)
{
  ACE_CDR::ULong length;
  if (!(strm >> length)) {
    return false;
  }
  seq.length(length);
  if (length == 0) {
    return true;
  }
  return strm.read_ulong_array(seq.get_buffer(), length);
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::SBoundSeq& seq)
{
  DCPS::primitive_serialized_size_ulong(encoding, size);
  if (seq.length() == 0) {
    return;
  }
  primitive_serialized_size_octet(encoding, size, seq.length());
}

bool operator<<(Serializer& strm, const XTypes::SBoundSeq& seq)
{
  const ACE_CDR::ULong length = seq.length();
  if (!(strm << length)) {
    return false;
  }
  if (length == 0) {
    return true;
  }
  return strm.write_octet_array(seq.get_buffer(), length);
}

bool operator>>(Serializer& strm, XTypes::SBoundSeq& seq)
{
  ACE_CDR::ULong length;
  if (!(strm >> length)) {
    return false;
  }
  seq.length(length);
  if (length == 0) {
    return true;
  }
  return strm.read_octet_array(seq.get_buffer(), length);
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::UnionCaseLabelSeq& seq)
{
  DCPS::primitive_serialized_size_ulong(encoding, size);
  if (seq.length() == 0) {
    return;
  }
  primitive_serialized_size(encoding, size, ACE_CDR::Long(), seq.length());
}

bool operator<<(Serializer& strm, const XTypes::UnionCaseLabelSeq& seq)
{
  const ACE_CDR::ULong length = seq.length();
  if (!(strm << length)) {
    return false;
  }
  if (length == 0) {
    return true;
  }
  return strm.write_long_array(seq.get_buffer(), length);
}

bool operator>>(Serializer& strm, XTypes::UnionCaseLabelSeq& seq)
{
  ACE_CDR::ULong length;
  if (!(strm >> length)) {
    return false;
  }
  seq.length(length);
  if (length == 0) {
    return true;
  }
  return strm.read_long_array(seq.get_buffer(), length);
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::ExtendedAnnotationParameterValue&)
{
  serialized_size_delimiter(encoding, size);
}

bool operator<<(Serializer& strm, const XTypes::ExtendedAnnotationParameterValue&)
{
  return XTypes::write_empty_xcdr2_nonfinal(strm);
}

bool operator>>(Serializer& strm, XTypes::ExtendedAnnotationParameterValue&)
{
  return XTypes::read_empty_xcdr2_nonfinal(strm);
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::NameHash_forany&)
{
  primitive_serialized_size_octet(encoding, size, 4);
}

bool operator<<(Serializer& strm, const XTypes::NameHash_forany& arr)
{
  return strm.write_octet_array(arr.in(), 4);
}

bool operator>>(Serializer& strm, XTypes::NameHash_forany& arr)
{
  return strm.read_octet_array(arr.out(), 4);
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::EquivalenceHash_forany&)
{
  primitive_serialized_size_octet(encoding, size, 14);
}

bool operator<<(Serializer& strm, const XTypes::EquivalenceHash_forany& arr)
{
  return strm.write_octet_array(arr.in(), 14);
}

bool operator>>(Serializer& strm, XTypes::EquivalenceHash_forany& arr)
{
  return strm.read_octet_array(arr.out(), 14);
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::CompleteTypeDetail& stru)
{
  serialized_size(encoding, size, stru.ann_builtin);
  serialized_size(encoding, size, stru.ann_custom);
  DCPS::primitive_serialized_size_ulong(encoding, size);
  size += stru.type_name.size() + 1;
}

bool operator<<(Serializer& strm, const XTypes::CompleteTypeDetail& stru)
{
  return (strm << stru.ann_builtin)
    && (strm << stru.ann_custom)
    && (strm << Serializer::FromBoundedString<char>(stru.type_name, 256));
}

bool operator>>(Serializer& strm, XTypes::CompleteTypeDetail& stru)
{
  return (strm >> stru.ann_builtin)
    && (strm >> stru.ann_custom)
    && (strm >> Serializer::ToBoundedString<char>(stru.type_name, 256));
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::CompleteStructHeader& stru)
{
  serialized_size_delimiter(encoding, size);
  serialized_size(encoding, size, stru.base_type);
  serialized_size(encoding, size, stru.detail);
}

bool operator<<(Serializer& strm, const XTypes::CompleteStructHeader& stru)
{
  size_t total_size = 0;
  serialized_size(strm.encoding(), total_size, stru);
  if (!strm.write_delimiter(total_size)) {
    return false;
  }

  return (strm << stru.base_type)
    && (strm << stru.detail);
}

bool operator>>(Serializer& strm, XTypes::CompleteStructHeader& stru)
{
  size_t total_size = 0;
  if (!strm.read_delimiter(total_size)) {
    return false;
  }

  const size_t start_pos = strm.rpos();

  const bool ret = (strm >> stru.base_type)
    && (strm >> stru.detail);

  if (ret && strm.rpos() - start_pos < total_size) {
    strm.skip(total_size - strm.rpos() + start_pos);
  }
  return ret;
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::MinimalStructHeader& stru)
{
  serialized_size_delimiter(encoding, size);
  serialized_size(encoding, size, stru.base_type);
  serialized_size(encoding, size, stru.detail);
}

bool operator<<(Serializer& strm, const XTypes::MinimalStructHeader& stru)
{
  size_t total_size = 0;
  serialized_size(strm.encoding(), total_size, stru);
  if (!strm.write_delimiter(total_size)) {
    return false;
  }

  return (strm << stru.base_type)
    && (strm << stru.detail);
}

bool operator>>(Serializer& strm, XTypes::MinimalStructHeader& stru)
{
  size_t total_size = 0;
  if (!strm.read_delimiter(total_size)) {
    return false;
  }

  const size_t start_pos = strm.rpos();

  const bool ret = (strm >> stru.base_type)
    && (strm >> stru.detail);

  if (ret && strm.rpos() - start_pos < total_size) {
    strm.skip(total_size - strm.rpos() + start_pos);
  }
  return ret;
}

void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::CompleteStructType& stru)
{
  primitive_serialized_size(encoding, size, stru.struct_flags);
  serialized_size(encoding, size, stru.header);
  serialized_size(encoding, size, stru.member_seq);
}

bool operator<<(Serializer& strm, const XTypes::CompleteStructType& stru)
{
  return (strm << stru.struct_flags)
    && (strm << stru.header)
    && (strm << stru.member_seq);
}

bool operator>>(Serializer& strm, XTypes::CompleteStructType& stru)
{
  return (strm >> stru.struct_flags)
    && (strm >> stru.header)
    && (strm >> stru.member_seq);
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::MinimalStructType& stru)
{
  primitive_serialized_size(encoding, size, stru.struct_flags);
  serialized_size(encoding, size, stru.header);
  serialized_size(encoding, size, stru.member_seq);
}

bool operator<<(Serializer& strm, const XTypes::MinimalStructType& stru)
{
  return (strm << stru.struct_flags)
    && (strm << stru.header)
    && (strm << stru.member_seq);
}

bool operator>>(Serializer& strm, XTypes::MinimalStructType& stru)
{
  return (strm >> stru.struct_flags)
    && (strm >> stru.header)
    && (strm >> stru.member_seq);
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::CompleteUnionType& stru)
{
  primitive_serialized_size(encoding, size, stru.union_flags);
  serialized_size(encoding, size, stru.header);
  serialized_size(encoding, size, stru.discriminator);
  serialized_size(encoding, size, stru.member_seq);
}

bool operator<<(Serializer& strm, const XTypes::CompleteUnionType& stru)
{
  return (strm << stru.union_flags)
    && (strm << stru.header)
    && (strm << stru.discriminator)
    && (strm << stru.member_seq);
}

bool operator>>(Serializer& strm, XTypes::CompleteUnionType& stru)
{
  return (strm >> stru.union_flags)
    && (strm >> stru.header)
    && (strm >> stru.discriminator)
    && (strm >> stru.member_seq);
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::MinimalUnionType& stru)
{
  primitive_serialized_size(encoding, size, stru.union_flags);
  serialized_size(encoding, size, stru.header);
  serialized_size(encoding, size, stru.discriminator);
  serialized_size(encoding, size, stru.member_seq);
}

bool operator<<(Serializer& strm, const XTypes::MinimalUnionType& stru)
{
  return (strm << stru.union_flags)
    && (strm << stru.header)
    && (strm << stru.discriminator)
    && (strm << stru.member_seq);
}

bool operator>>(Serializer& strm, XTypes::MinimalUnionType& stru)
{
  return (strm >> stru.union_flags)
    && (strm >> stru.header)
    && (strm >> stru.discriminator)
    && (strm >> stru.member_seq);
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::CompleteAnnotationType& stru)
{
  primitive_serialized_size(encoding, size, stru.annotation_flag);
  serialized_size(encoding, size, stru.header);
  serialized_size(encoding, size, stru.member_seq);
}

bool operator<<(Serializer& strm, const XTypes::CompleteAnnotationType& stru)
{
  return (strm << stru.annotation_flag)
    && (strm << stru.header)
    && (strm << stru.member_seq);
}

bool operator>>(Serializer& strm, XTypes::CompleteAnnotationType& stru)
{
  return (strm >> stru.annotation_flag)
    && (strm >> stru.header)
    && (strm >> stru.member_seq);
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::MinimalAnnotationType& stru)
{
  primitive_serialized_size(encoding, size, stru.annotation_flag);
  serialized_size(encoding, size, stru.header);
  serialized_size(encoding, size, stru.member_seq);
}

bool operator<<(Serializer& strm, const XTypes::MinimalAnnotationType& stru)
{
  return (strm << stru.annotation_flag)
    && (strm << stru.header)
    && (strm << stru.member_seq);
}

bool operator>>(Serializer& strm, XTypes::MinimalAnnotationType& stru)
{
  return (strm >> stru.annotation_flag)
    && (strm >> stru.header)
    && (strm >> stru.member_seq);
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::CompleteAliasType& stru)
{
  primitive_serialized_size(encoding, size, stru.alias_flags);
  serialized_size(encoding, size, stru.header);
  serialized_size(encoding, size, stru.body);
}

bool operator<<(Serializer& strm, const XTypes::CompleteAliasType& stru)
{
  return (strm << stru.alias_flags)
    && (strm << stru.header)
    && (strm << stru.body);
}

bool operator>>(Serializer& strm, XTypes::CompleteAliasType& stru)
{
  return (strm >> stru.alias_flags)
    && (strm >> stru.header)
    && (strm >> stru.body);
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::MinimalAliasType& stru)
{
  primitive_serialized_size(encoding, size, stru.alias_flags);
  serialized_size(encoding, size, stru.header);
  serialized_size(encoding, size, stru.body);
}

bool operator<<(Serializer& strm, const XTypes::MinimalAliasType& stru)
{
  return (strm << stru.alias_flags)
    && (strm << stru.header)
    && (strm << stru.body);
}

bool operator>>(Serializer& strm, XTypes::MinimalAliasType& stru)
{
  return (strm >> stru.alias_flags)
    && (strm >> stru.header)
    && (strm >> stru.body);
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::CompleteSequenceType& stru)
{
  primitive_serialized_size(encoding, size, stru.collection_flag);
  serialized_size(encoding, size, stru.header);
  serialized_size(encoding, size, stru.element);
}

bool operator<<(Serializer& strm, const XTypes::CompleteSequenceType& stru)
{
  return (strm << stru.collection_flag)
    && (strm << stru.header)
    && (strm << stru.element);
}

bool operator>>(Serializer& strm, XTypes::CompleteSequenceType& stru)
{
  return (strm >> stru.collection_flag)
    && (strm >> stru.header)
    && (strm >> stru.element);
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::MinimalSequenceType& stru)
{
  primitive_serialized_size(encoding, size, stru.collection_flag);
  serialized_size(encoding, size, stru.header);
  serialized_size(encoding, size, stru.element);
}

bool operator<<(Serializer& strm, const XTypes::MinimalSequenceType& stru)
{
  return (strm << stru.collection_flag)
    && (strm << stru.header)
    && (strm << stru.element);
}

bool operator>>(Serializer& strm, XTypes::MinimalSequenceType& stru)
{
  return (strm >> stru.collection_flag)
    && (strm >> stru.header)
    && (strm >> stru.element);
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::CompleteArrayType& stru)
{
  serialized_size_delimiter(encoding, size);
  primitive_serialized_size(encoding, size, stru.collection_flag);
  serialized_size(encoding, size, stru.header);
  serialized_size(encoding, size, stru.element);
}

bool operator<<(Serializer& strm, const XTypes::CompleteArrayType& stru)
{
  size_t total_size = 0;
  serialized_size(strm.encoding(), total_size, stru);
  if (!strm.write_delimiter(total_size)) {
    return false;
  }

  return (strm << stru.collection_flag)
    && (strm << stru.header)
    && (strm << stru.element);
}

bool operator>>(Serializer& strm, XTypes::CompleteArrayType& stru)
{
  size_t total_size = 0;
  if (!strm.read_delimiter(total_size)) {
    return false;
  }

  const size_t start_pos = strm.rpos();

  const bool ret = (strm >> stru.collection_flag)
    && (strm >> stru.header)
    && (strm >> stru.element);

  if (ret && strm.rpos() - start_pos < total_size) {
    strm.skip(total_size - strm.rpos() + start_pos);
  }
  return ret;
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::MinimalArrayType& stru)
{
  primitive_serialized_size(encoding, size, stru.collection_flag);
  serialized_size(encoding, size, stru.header);
  serialized_size(encoding, size, stru.element);
}

bool operator<<(Serializer& strm, const XTypes::MinimalArrayType& stru)
{
  return (strm << stru.collection_flag)
    && (strm << stru.header)
    && (strm << stru.element);
}

bool operator>>(Serializer& strm, XTypes::MinimalArrayType& stru)
{
  return (strm >> stru.collection_flag)
    && (strm >> stru.header)
    && (strm >> stru.element);
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::CompleteMapType& stru)
{
  primitive_serialized_size(encoding, size, stru.collection_flag);
  serialized_size(encoding, size, stru.header);
  serialized_size(encoding, size, stru.key);
  serialized_size(encoding, size, stru.element);
}

bool operator<<(Serializer& strm, const XTypes::CompleteMapType& stru)
{
  return (strm << stru.collection_flag)
    && (strm << stru.header)
    && (strm << stru.key)
    && (strm << stru.element);
}

bool operator>>(Serializer& strm, XTypes::CompleteMapType& stru)
{
  return (strm >> stru.collection_flag)
    && (strm >> stru.header)
    && (strm >> stru.key)
    && (strm >> stru.element);
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::MinimalMapType& stru)
{
  primitive_serialized_size(encoding, size, stru.collection_flag);
  serialized_size(encoding, size, stru.header);
  serialized_size(encoding, size, stru.key);
  serialized_size(encoding, size, stru.element);
}

bool operator<<(Serializer& strm, const XTypes::MinimalMapType& stru)
{
  return (strm << stru.collection_flag)
    && (strm << stru.header)
    && (strm << stru.key)
    && (strm << stru.element);
}

bool operator>>(Serializer& strm, XTypes::MinimalMapType& stru)
{
  return (strm >> stru.collection_flag)
    && (strm >> stru.header)
    && (strm >> stru.key)
    && (strm >> stru.element);
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::CompleteEnumeratedHeader& stru)
{
  serialized_size_delimiter(encoding, size);
  serialized_size(encoding, size, stru.common);
  serialized_size(encoding, size, stru.detail);
}

bool operator<<(Serializer& strm, const XTypes::CompleteEnumeratedHeader& stru)
{
  size_t total_size = 0;
  serialized_size(strm.encoding(), total_size, stru);
  if (!strm.write_delimiter(total_size)) {
    return false;
  }

  return (strm << stru.common)
    && (strm << stru.detail);
}

bool operator>>(Serializer& strm, XTypes::CompleteEnumeratedHeader& stru)
{
  size_t total_size = 0;
  if (!strm.read_delimiter(total_size)) {
    return false;
  }

  const size_t start_pos = strm.rpos();

  const bool ret = (strm >> stru.common)
    && (strm >> stru.detail);

  if (ret && strm.rpos() - start_pos < total_size) {
    strm.skip(total_size - strm.rpos() + start_pos);
  }
  return ret;
}

void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::MinimalEnumeratedHeader& stru)
{
  serialized_size_delimiter(encoding, size);
  serialized_size(encoding, size, stru.common);
}

bool operator<<(Serializer& strm, const XTypes::MinimalEnumeratedHeader& stru)
{
  size_t total_size = 0;
  serialized_size(strm.encoding(), total_size, stru);
  if (!strm.write_delimiter(total_size)) {
    return false;
  }

  return (strm << stru.common);
}

bool operator>>(Serializer& strm, XTypes::MinimalEnumeratedHeader& stru)
{
  size_t total_size = 0;
  if (!strm.read_delimiter(total_size)) {
    return false;
  }

  const size_t start_pos = strm.rpos();

  const bool ret = (strm >> stru.common);

  if (ret && strm.rpos() - start_pos < total_size) {
    strm.skip(total_size - strm.rpos() + start_pos);
  }
  return ret;
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::CompleteEnumeratedType& stru)
{
  primitive_serialized_size(encoding, size, stru.enum_flags);
  serialized_size(encoding, size, stru.header);
  serialized_size(encoding, size, stru.literal_seq);
}

bool operator<<(Serializer& strm, const XTypes::CompleteEnumeratedType& stru)
{
  return (strm << stru.enum_flags)
    && (strm << stru.header)
    && (strm << stru.literal_seq);
}

bool operator>>(Serializer& strm, XTypes::CompleteEnumeratedType& stru)
{
  return (strm >> stru.enum_flags)
    && (strm >> stru.header)
    && (strm >> stru.literal_seq);
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::MinimalEnumeratedType& stru)
{
  primitive_serialized_size(encoding, size, stru.enum_flags);
  serialized_size(encoding, size, stru.header);
  serialized_size(encoding, size, stru.literal_seq);
}

bool operator<<(Serializer& strm, const XTypes::MinimalEnumeratedType& stru)
{
  return (strm << stru.enum_flags)
    && (strm << stru.header)
    && (strm << stru.literal_seq);
}

bool operator>>(Serializer& strm, XTypes::MinimalEnumeratedType& stru)
{
  return (strm >> stru.enum_flags)
    && (strm >> stru.header)
    && (strm >> stru.literal_seq);
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::CompleteBitmaskType& stru)
{
  serialized_size_delimiter(encoding, size);
  primitive_serialized_size(encoding, size, stru.bitmask_flags);
  serialized_size(encoding, size, stru.header);
  serialized_size(encoding, size, stru.flag_seq);
}

bool operator<<(Serializer& strm, const XTypes::CompleteBitmaskType& stru)
{
  size_t total_size = 0;
  serialized_size(strm.encoding(), total_size, stru);
  if (!strm.write_delimiter(total_size)) {
    return false;
  }

  return (strm << stru.bitmask_flags)
    && (strm << stru.header)
    && (strm << stru.flag_seq);
}

bool operator>>(Serializer& strm, XTypes::CompleteBitmaskType& stru)
{
  size_t total_size = 0;
  if (!strm.read_delimiter(total_size)) {
    return false;
  }

  const size_t start_pos = strm.rpos();

  const bool ret = (strm >> stru.bitmask_flags)
    && (strm >> stru.header)
    && (strm >> stru.flag_seq);

  if (ret && strm.rpos() - start_pos < total_size) {
    strm.skip(total_size - strm.rpos() + start_pos);
  }
  return ret;
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::MinimalBitmaskType& stru)
{
  serialized_size_delimiter(encoding, size);
  primitive_serialized_size(encoding, size, stru.bitmask_flags);
  serialized_size(encoding, size, stru.header);
  serialized_size(encoding, size, stru.flag_seq);
}

bool operator<<(Serializer& strm, const XTypes::MinimalBitmaskType& stru)
{
  size_t total_size = 0;
  serialized_size(strm.encoding(), total_size, stru);
  if (!strm.write_delimiter(total_size)) {
    return false;
  }

  return (strm << stru.bitmask_flags)
    && (strm << stru.header)
    && (strm << stru.flag_seq);
}

bool operator>>(Serializer& strm, XTypes::MinimalBitmaskType& stru)
{
  size_t total_size = 0;
  if (!strm.read_delimiter(total_size)) {
    return false;
  }

  const size_t start_pos = strm.rpos();

  const bool ret = (strm >> stru.bitmask_flags)
    && (strm >> stru.header)
    && (strm >> stru.flag_seq);

  if (ret && strm.rpos() - start_pos < total_size) {
    strm.skip(total_size - strm.rpos() + start_pos);
  }
  return ret;
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::CompleteBitsetType& stru)
{
  serialized_size_delimiter(encoding, size);
  primitive_serialized_size(encoding, size, stru.bitset_flags);
  serialized_size(encoding, size, stru.header);
  serialized_size(encoding, size, stru.field_seq);
}

bool operator<<(Serializer& strm, const XTypes::CompleteBitsetType& stru)
{
  size_t total_size = 0;
  serialized_size(strm.encoding(), total_size, stru);
  if (!strm.write_delimiter(total_size)) {
    return false;
  }

  return (strm << stru.bitset_flags)
    && (strm << stru.header)
    && (strm << stru.field_seq);
}

bool operator>>(Serializer& strm, XTypes::CompleteBitsetType& stru)
{
  size_t total_size = 0;
  if (!strm.read_delimiter(total_size)) {
    return false;
  }

  const size_t start_pos = strm.rpos();

  const bool ret = (strm >> stru.bitset_flags)
    && (strm >> stru.header)
    && (strm >> stru.field_seq);

  if (ret && strm.rpos() - start_pos < total_size) {
    strm.skip(total_size - strm.rpos() + start_pos);
  }
  return ret;
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::MinimalBitsetType& stru)
{
  serialized_size_delimiter(encoding, size);
  primitive_serialized_size(encoding, size, stru.bitset_flags);
  serialized_size(encoding, size, stru.header);
  serialized_size(encoding, size, stru.field_seq);
}

bool operator<<(Serializer& strm, const XTypes::MinimalBitsetType& stru)
{
  size_t total_size = 0;
  serialized_size(strm.encoding(), total_size, stru);
  if (!strm.write_delimiter(total_size)) {
    return false;
  }

  return (strm << stru.bitset_flags)
    && (strm << stru.header)
    && (strm << stru.field_seq);
}

bool operator>>(Serializer& strm, XTypes::MinimalBitsetType& stru)
{
  size_t total_size = 0;
  if (!strm.read_delimiter(total_size)) {
    return false;
  }

  const size_t start_pos = strm.rpos();

  const bool ret = (strm >> stru.bitset_flags)
    && (strm >> stru.header)
    && (strm >> stru.field_seq);

  if (ret && strm.rpos() - start_pos < total_size) {
    strm.skip(total_size - strm.rpos() + start_pos);
  }
  return ret;
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::CompleteExtendedType&)
{
  serialized_size_delimiter(encoding, size);
}

bool operator<<(Serializer& strm, const XTypes::CompleteExtendedType&)
{
  return XTypes::write_empty_xcdr2_nonfinal(strm);
}

bool operator>>(Serializer& strm, XTypes::CompleteExtendedType&)
{
  return XTypes::read_empty_xcdr2_nonfinal(strm);
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::TypeIdentifierWithSize& stru)
{
  serialized_size_delimiter(encoding, size);
  serialized_size(encoding, size, stru.type_id);
  primitive_serialized_size(encoding, size, stru.typeobject_serialized_size);
}

bool operator<<(Serializer& strm, const XTypes::TypeIdentifierWithSize& stru)
{
  size_t total_size = 0;
  serialized_size(strm.encoding(), total_size, stru);
  if (!strm.write_delimiter(total_size)) {
    return false;
  }

  return (strm << stru.type_id)
    && (strm << stru.typeobject_serialized_size);
}

bool operator>>(Serializer& strm, XTypes::TypeIdentifierWithSize& stru)
{
  size_t total_size = 0;
  if (!strm.read_delimiter(total_size)) {
    return false;
  }

  const size_t start_pos = strm.rpos();

  const bool ret = (strm >> stru.type_id)
    && (strm >> stru.typeobject_serialized_size);

  if (ret && strm.rpos() - start_pos < total_size) {
    strm.skip(total_size - strm.rpos() + start_pos);
  }
  return ret;
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::TypeIdentifierWithDependencies& stru)
{
  serialized_size_delimiter(encoding, size);
  serialized_size(encoding, size, stru.typeid_with_size);
  primitive_serialized_size(encoding, size, stru.dependent_typeid_count);
  serialized_size(encoding, size, stru.dependent_typeids);
}

bool operator<<(Serializer& strm, const XTypes::TypeIdentifierWithDependencies& stru)
{
  size_t total_size = 0;
  serialized_size(strm.encoding(), total_size, stru);
  if (!strm.write_delimiter(total_size)) {
    return false;
  }

  return (strm << stru.typeid_with_size)
    && (strm << stru.dependent_typeid_count)
    && (strm << stru.dependent_typeids);
}

bool operator>>(Serializer& strm, XTypes::TypeIdentifierWithDependencies& stru)
{
  size_t total_size = 0;
  if (!strm.read_delimiter(total_size)) {
    return false;
  }

  const size_t start_pos = strm.rpos();

  const bool ret = (strm >> stru.typeid_with_size)
    && (strm >> stru.dependent_typeid_count)
    && (strm >> stru.dependent_typeids);

  if (ret && strm.rpos() - start_pos < total_size) {
    strm.skip(total_size - strm.rpos() + start_pos);
  }
  return ret;
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::AppliedAnnotation& stru)
{
  serialized_size_delimiter(encoding, size);
  serialized_size(encoding, size, stru.annotation_typeid);
  serialized_size(encoding, size, stru.param_seq);
}

bool operator<<(Serializer& strm, const XTypes::AppliedAnnotation& stru)
{
  size_t total_size = 0;
  serialized_size(strm.encoding(), total_size, stru);
  if (!strm.write_delimiter(total_size)) {
    return false;
  }

  return (strm << stru.annotation_typeid)
    && (strm << stru.param_seq);
}

bool operator>>(Serializer& strm, XTypes::AppliedAnnotation& stru)
{
  size_t total_size = 0;
  if (!strm.read_delimiter(total_size)) {
    return false;
  }

  const size_t start_pos = strm.rpos();
  const bool ret = (strm >> stru.annotation_typeid)
    && (strm >> stru.param_seq);

  if (ret && strm.rpos() - start_pos < total_size) {
    strm.skip(total_size - strm.rpos() + start_pos);
  }
  return ret;
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::AppliedBuiltinTypeAnnotations& stru)
{
  serialized_size_delimiter(encoding, size);
  serialized_size(encoding, size, stru.verbatim);
}

bool operator<<(Serializer& strm, const XTypes::AppliedBuiltinTypeAnnotations& stru)
{
  size_t total_size = 0;
  serialized_size(strm.encoding(), total_size, stru);
  if (!strm.write_delimiter(total_size)) {
    return false;
  }

  return (strm << stru.verbatim);
}

bool operator>>(Serializer& strm, XTypes::AppliedBuiltinTypeAnnotations& stru)
{
  size_t total_size = 0;
  if (!strm.read_delimiter(total_size)) {
    return false;
  }

  const size_t start_pos = strm.rpos();

  const bool ret = (strm >> stru.verbatim);

  if (ret && strm.rpos() - start_pos < total_size) {
    strm.skip(total_size - strm.rpos() + start_pos);
  }
  return ret;
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::CompleteAliasBody& stru)
{
  serialized_size_delimiter(encoding, size);
  serialized_size(encoding, size, stru.common);
  serialized_size(encoding, size, stru.ann_builtin);
  serialized_size(encoding, size, stru.ann_custom);
}

bool operator<<(Serializer& strm, const XTypes::CompleteAliasBody& stru)
{
  size_t total_size = 0;
  serialized_size(strm.encoding(), total_size, stru);
  if (!strm.write_delimiter(total_size)) {
    return false;
  }

  return (strm << stru.common)
    && (strm << stru.ann_builtin)
    && (strm << stru.ann_custom);
}

bool operator>>(Serializer& strm, XTypes::CompleteAliasBody& stru)
{
  size_t total_size = 0;
  if (!strm.read_delimiter(total_size)) {
    return false;
  }

  const size_t start_pos = strm.rpos();

  const bool ret = (strm >> stru.common)
    && (strm >> stru.ann_builtin)
    && (strm >> stru.ann_custom);

  if (ret && strm.rpos() - start_pos < total_size) {
    strm.skip(total_size - strm.rpos() + start_pos);
  }
  return ret;
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::CompleteAliasHeader& stru)
{
  serialized_size_delimiter(encoding, size);
  serialized_size(encoding, size, stru.detail);
}

bool operator<<(Serializer& strm, const XTypes::CompleteAliasHeader& stru)
{
  size_t total_size = 0;
  serialized_size(strm.encoding(), total_size, stru);
  if (!strm.write_delimiter(total_size)) {
    return false;
  }

  return (strm << stru.detail);
}

bool operator>>(Serializer& strm, XTypes::CompleteAliasHeader& stru)
{
  size_t total_size = 0;
  if (!strm.read_delimiter(total_size)) {
    return false;
  }

  const size_t start_pos = strm.rpos();

  const bool ret = (strm >> stru.detail);

  if (ret && strm.rpos() - start_pos < total_size) {
    strm.skip(total_size - strm.rpos() + start_pos);
  }
  return ret;
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::CompleteAnnotationHeader& stru)
{
  serialized_size_delimiter(encoding, size);
  DCPS::primitive_serialized_size_ulong(encoding, size);
  size += ACE_OS::strlen(stru.annotation_name.c_str()) + 1;
}

bool operator<<(Serializer& strm, const XTypes::CompleteAnnotationHeader& stru)
{
  size_t total_size = 0;
  serialized_size(strm.encoding(), total_size, stru);
  if (!strm.write_delimiter(total_size)) {
    return false;
  }

  return (strm << Serializer::FromBoundedString<char>(stru.annotation_name, 256));
}

bool operator>>(Serializer& strm, XTypes::CompleteAnnotationHeader& stru)
{
  size_t total_size = 0;
  if (!strm.read_delimiter(total_size)) {
    return false;
  }

  const size_t start_pos = strm.rpos();

  const bool ret = (strm >> Serializer::ToBoundedString<char>(stru.annotation_name, 256));

  if (ret && strm.rpos() - start_pos < total_size) {
    strm.skip(total_size - strm.rpos() + start_pos);
  }
  return ret;
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::CompleteAnnotationParameter& stru)
{
  serialized_size_delimiter(encoding, size);
  serialized_size(encoding, size, stru.common);
  primitive_serialized_size_ulong(encoding, size);
  size += ACE_OS::strlen(stru.name.c_str()) + 1;
  serialized_size(encoding, size, stru.default_value);
}

bool operator<<(Serializer& strm, const XTypes::CompleteAnnotationParameter& stru)
{
  size_t total_size = 0;
  serialized_size(strm.encoding(), total_size, stru);
  if (!strm.write_delimiter(total_size)) {
    return false;
  }

  return (strm << stru.common)
    && (strm << Serializer::FromBoundedString<char>(stru.name, 256))
    && (strm << stru.default_value);
}

bool operator>>(Serializer& strm, XTypes::CompleteAnnotationParameter& stru)
{
  size_t total_size = 0;
  if (!strm.read_delimiter(total_size)) {
    return false;
  }

  const size_t start_pos = strm.rpos();

  const bool ret = (strm >> stru.common)
    && (strm >> Serializer::ToBoundedString<char>(stru.name, 256))
    && (strm >> stru.default_value);

  if (ret && strm.rpos() - start_pos < total_size) {
    strm.skip(total_size - strm.rpos() + start_pos);
  }
  return ret;
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::CompleteArrayHeader& stru)
{
  serialized_size_delimiter(encoding, size);
  serialized_size(encoding, size, stru.common);
  serialized_size(encoding, size, stru.detail);
}

bool operator<<(Serializer& strm, const XTypes::CompleteArrayHeader& stru)
{
  size_t total_size = 0;
  serialized_size(strm.encoding(), total_size, stru);
  if (!strm.write_delimiter(total_size)) {
    return false;
  }

  return (strm << stru.common)
    && (strm << stru.detail);
}

bool operator>>(Serializer& strm, XTypes::CompleteArrayHeader& stru)
{
  size_t total_size = 0;
  if (!strm.read_delimiter(total_size)) {
    return false;
  }

  const size_t start_pos = strm.rpos();

  const bool ret = (strm >> stru.common)
    && (strm >> stru.detail);

  if (ret && strm.rpos() - start_pos < total_size) {
    strm.skip(total_size - strm.rpos() + start_pos);
  }
  return ret;
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::CompleteBitfield& stru)
{
  serialized_size_delimiter(encoding, size);
  serialized_size(encoding, size, stru.common);
  serialized_size(encoding, size, stru.detail);
}

bool operator<<(Serializer& strm, const XTypes::CompleteBitfield& stru)
{
  size_t total_size = 0;
  serialized_size(strm.encoding(), total_size, stru);
  if (!strm.write_delimiter(total_size)) {
    return false;
  }

  return (strm << stru.common)
    && (strm << stru.detail);
}

bool operator>>(Serializer& strm, XTypes::CompleteBitfield& stru)
{
  size_t total_size = 0;
  if (!strm.read_delimiter(total_size)) {
    return false;
  }

  const size_t start_pos = strm.rpos();

  const bool ret = (strm >> stru.common)
    && (strm >> stru.detail);

  if (ret && strm.rpos() - start_pos < total_size) {
    strm.skip(total_size - strm.rpos() + start_pos);
  }
  return ret;
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::CompleteBitflag& stru)
{
  serialized_size_delimiter(encoding, size);
  serialized_size(encoding, size, stru.common);
  serialized_size(encoding, size, stru.detail);
}

bool operator<<(Serializer& strm, const XTypes::CompleteBitflag& stru)
{
  size_t total_size = 0;
  serialized_size(strm.encoding(), total_size, stru);
  if (!strm.write_delimiter(total_size)) {
    return false;
  }

  return (strm << stru.common)
    && (strm << stru.detail);
}

bool operator>>(Serializer& strm, XTypes::CompleteBitflag& stru)
{
  size_t total_size = 0;
  if (!strm.read_delimiter(total_size)) {
    return false;
  }

  const size_t start_pos = strm.rpos();

  const bool ret = (strm >> stru.common)
    && (strm >> stru.detail);

  if (ret && strm.rpos() - start_pos < total_size) {
    strm.skip(total_size - strm.rpos() + start_pos);
  }
  return ret;
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::CompleteBitsetHeader& stru)
{
  serialized_size_delimiter(encoding, size);
  serialized_size(encoding, size, stru.detail);
}

bool operator<<(Serializer& strm, const XTypes::CompleteBitsetHeader& stru)
{
  size_t total_size = 0;
  serialized_size(strm.encoding(), total_size, stru);
  if (!strm.write_delimiter(total_size)) {
    return false;
  }

  return (strm << stru.detail);
}

bool operator>>(Serializer& strm, XTypes::CompleteBitsetHeader& stru)
{
  size_t total_size = 0;
  if (!strm.read_delimiter(total_size)) {
    return false;
  }

  const size_t start_pos = strm.rpos();

  const bool ret = (strm >> stru.detail);

  if (ret && strm.rpos() - start_pos < total_size) {
    strm.skip(total_size - strm.rpos() + start_pos);
  }
  return ret;
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::CompleteCollectionElement& stru)
{
  serialized_size_delimiter(encoding, size);
  serialized_size(encoding, size, stru.common);
  serialized_size(encoding, size, stru.detail);
}

bool operator<<(Serializer& strm, const XTypes::CompleteCollectionElement& stru)
{
  size_t total_size = 0;
  serialized_size(strm.encoding(), total_size, stru);
  if (!strm.write_delimiter(total_size)) {
    return false;
  }

  return (strm << stru.common)
    && (strm << stru.detail);
}

bool operator>>(Serializer& strm, XTypes::CompleteCollectionElement& stru)
{
  size_t total_size = 0;
  if (!strm.read_delimiter(total_size)) {
    return false;
  }

  const size_t start_pos = strm.rpos();

  const bool ret = (strm >> stru.common)
    && (strm >> stru.detail);

  if (ret && strm.rpos() - start_pos < total_size) {
    strm.skip(total_size - strm.rpos() + start_pos);
  }
  return ret;
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::CompleteCollectionHeader& stru)
{
  serialized_size_delimiter(encoding, size);
  serialized_size(encoding, size, stru.common);
  serialized_size(encoding, size, stru.detail);
}

bool operator<<(Serializer& strm, const XTypes::CompleteCollectionHeader& stru)
{
  size_t total_size = 0;
  serialized_size(strm.encoding(), total_size, stru);
  if (!strm.write_delimiter(total_size)) {
    return false;
  }

  return (strm << stru.common)
    && (strm << stru.detail);
}

bool operator>>(Serializer& strm, XTypes::CompleteCollectionHeader& stru)
{
  size_t total_size = 0;
  if (!strm.read_delimiter(total_size)) {
    return false;
  }

  const size_t start_pos = strm.rpos();

  const bool ret = (strm >> stru.common)
    && (strm >> stru.detail);

  if (ret && strm.rpos() - start_pos < total_size) {
    strm.skip(total_size - strm.rpos() + start_pos);
  }
  return ret;
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::CompleteDiscriminatorMember& stru)
{
  serialized_size_delimiter(encoding, size);
  serialized_size(encoding, size, stru.common);
  serialized_size(encoding, size, stru.ann_builtin);
  serialized_size(encoding, size, stru.ann_custom);
}

bool operator<<(Serializer& strm, const XTypes::CompleteDiscriminatorMember& stru)
{
  size_t total_size = 0;
  serialized_size(strm.encoding(), total_size, stru);
  if (!strm.write_delimiter(total_size)) {
    return false;
  }

  return (strm << stru.common)
    && (strm << stru.ann_builtin)
    && (strm << stru.ann_custom);
}

bool operator>>(Serializer& strm, XTypes::CompleteDiscriminatorMember& stru)
{
  size_t total_size = 0;
  if (!strm.read_delimiter(total_size)) {
    return false;
  }

  const size_t start_pos = strm.rpos();

  const bool ret = (strm >> stru.common)
    && (strm >> stru.ann_builtin)
    && (strm >> stru.ann_custom);

  if (ret && strm.rpos() - start_pos < total_size) {
    strm.skip(total_size - strm.rpos() + start_pos);
  }
  return ret;
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::CompleteEnumeratedLiteral& stru)
{
  serialized_size_delimiter(encoding, size);
  serialized_size(encoding, size, stru.common);
  serialized_size(encoding, size, stru.detail);
}

bool operator<<(Serializer& strm, const XTypes::CompleteEnumeratedLiteral& stru)
{
  size_t total_size = 0;
  serialized_size(strm.encoding(), total_size, stru);
  if (!strm.write_delimiter(total_size)) {
    return false;
  }

  return (strm << stru.common)
    && (strm << stru.detail);
}

bool operator>>(Serializer& strm, XTypes::CompleteEnumeratedLiteral& stru)
{
  size_t total_size = 0;
  if (!strm.read_delimiter(total_size)) {
    return false;
  }

  const size_t start_pos = strm.rpos();

  const bool ret = (strm >> stru.common)
    && (strm >> stru.detail);

  if (ret && strm.rpos() - start_pos < total_size) {
    strm.skip(total_size - strm.rpos() + start_pos);
  }
  return ret;
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::CompleteStructMember& stru)
{
  serialized_size_delimiter(encoding, size);
  serialized_size(encoding, size, stru.common);
  serialized_size(encoding, size, stru.detail);
}

bool operator<<(Serializer& strm, const XTypes::CompleteStructMember& stru)
{
  size_t total_size = 0;
  serialized_size(strm.encoding(), total_size, stru);
  if (!strm.write_delimiter(total_size)) {
    return false;
  }

  return (strm << stru.common)
    && (strm << stru.detail);
}

bool operator>>(Serializer& strm, XTypes::CompleteStructMember& stru)
{
  size_t total_size = 0;
  if (!strm.read_delimiter(total_size)) {
    return false;
  }

  const size_t start_pos = strm.rpos();

  const bool ret = (strm >> stru.common)
    && (strm >> stru.detail);

  if (ret && strm.rpos() - start_pos < total_size) {
    strm.skip(total_size - strm.rpos() + start_pos);
  }

  return ret;
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::CompleteUnionHeader& stru)
{
  serialized_size_delimiter(encoding, size);
  serialized_size(encoding, size, stru.detail);
}

bool operator<<(Serializer& strm, const XTypes::CompleteUnionHeader& stru)
{
  size_t total_size = 0;
  serialized_size(strm.encoding(), total_size, stru);
  if (!strm.write_delimiter(total_size)) {
    return false;
  }

  return (strm << stru.detail);
}

bool operator>>(Serializer& strm, XTypes::CompleteUnionHeader& stru)
{
  size_t total_size = 0;
  if (!strm.read_delimiter(total_size)) {
    return false;
  }

  const size_t start_pos = strm.rpos();

  const bool ret = (strm >> stru.detail);

  if (ret && strm.rpos() - start_pos < total_size) {
    strm.skip(total_size - strm.rpos() + start_pos);
  }
  return ret;
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::CompleteUnionMember& stru)
{
  serialized_size_delimiter(encoding, size);
  serialized_size(encoding, size, stru.common);
  serialized_size(encoding, size, stru.detail);
}

bool operator<<(Serializer& strm, const XTypes::CompleteUnionMember& stru)
{
  size_t total_size = 0;
  serialized_size(strm.encoding(), total_size, stru);
  if (!strm.write_delimiter(total_size)) {
    return false;
  }

  return (strm << stru.common)
    && (strm << stru.detail);
}

bool operator>>(Serializer& strm, XTypes::CompleteUnionMember& stru)
{
  size_t total_size = 0;
  if (!strm.read_delimiter(total_size)) {
    return false;
  }

  const size_t start_pos = strm.rpos();

  const bool ret = (strm >> stru.common)
    && (strm >> stru.detail);

  if (ret && strm.rpos() - start_pos < total_size) {
    strm.skip(total_size - strm.rpos() + start_pos);
  }
  return ret;
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::MinimalAliasBody& stru)
{
  serialized_size_delimiter(encoding, size);
  serialized_size(encoding, size, stru.common);
}

bool operator<<(Serializer& strm, const XTypes::MinimalAliasBody& stru)
{
  size_t total_size = 0;
  serialized_size(strm.encoding(), total_size, stru);
  if (!strm.write_delimiter(total_size)) {
    return false;
  }

  return (strm << stru.common);
}

bool operator>>(Serializer& strm, XTypes::MinimalAliasBody& stru)
{
  size_t total_size = 0;
  if (!strm.read_delimiter(total_size)) {
    return false;
  }

  const size_t start_pos = strm.rpos();

  const bool ret = (strm >> stru.common);

  if (ret && strm.rpos() - start_pos < total_size) {
    strm.skip(total_size - strm.rpos() + start_pos);
  }
  return ret;
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::MinimalAliasHeader&)
{
  serialized_size_delimiter(encoding, size);
}

bool operator<<(Serializer& strm, const XTypes::MinimalAliasHeader&)
{
  return XTypes::write_empty_xcdr2_nonfinal(strm);
}

bool operator>>(Serializer& strm, XTypes::MinimalAliasHeader&)
{
  return XTypes::read_empty_xcdr2_nonfinal(strm);
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::MinimalAnnotationHeader&)
{
  serialized_size_delimiter(encoding, size);
}

bool operator<<(Serializer& strm, const XTypes::MinimalAnnotationHeader&)
{
  return XTypes::write_empty_xcdr2_nonfinal(strm);
}

bool operator>>(Serializer& strm, XTypes::MinimalAnnotationHeader&)
{
  return XTypes::read_empty_xcdr2_nonfinal(strm);
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::MinimalAnnotationParameter& stru)
{
  serialized_size_delimiter(encoding, size);
  XTypes::NameHash_forany stru_name_hash(const_cast<XTypes::NameHash_slice*>(stru.name_hash));
  serialized_size(encoding, size, stru.common);
  serialized_size(encoding, size, stru_name_hash);
  serialized_size(encoding, size, stru.default_value);
}

bool operator<<(Serializer& strm, const XTypes::MinimalAnnotationParameter& stru)
{
  size_t total_size = 0;
  serialized_size(strm.encoding(), total_size, stru);
  if (!strm.write_delimiter(total_size)) {
    return false;
  }

  XTypes::NameHash_forany stru_name_hash(const_cast<XTypes::NameHash_slice*>(stru.name_hash));
  return (strm << stru.common)
    && (strm << stru_name_hash)
    && (strm << stru.default_value);
}

bool operator>>(Serializer& strm, XTypes::MinimalAnnotationParameter& stru)
{
  XTypes::NameHash_forany stru_name_hash(const_cast<XTypes::NameHash_slice*>(stru.name_hash));
  size_t total_size = 0;
  if (!strm.read_delimiter(total_size)) {
    return false;
  }

  const size_t start_pos = strm.rpos();

  const bool ret = (strm >> stru.common)
    && (strm >> stru_name_hash)
    && (strm >> stru.default_value);

  if (ret && strm.rpos() - start_pos < total_size) {
    strm.skip(total_size - strm.rpos() + start_pos);
  }
  return ret;
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::MinimalArrayHeader& stru)
{
  serialized_size_delimiter(encoding, size);
  serialized_size(encoding, size, stru.common);
}

bool operator<<(Serializer& strm, const XTypes::MinimalArrayHeader& stru)
{
  size_t total_size = 0;
  serialized_size(strm.encoding(), total_size, stru);
  if (!strm.write_delimiter(total_size)) {
    return false;
  }

  return (strm << stru.common);
}

bool operator>>(Serializer& strm, XTypes::MinimalArrayHeader& stru)
{
  size_t total_size = 0;
  if (!strm.read_delimiter(total_size)) {
    return false;
  }

  const size_t start_pos = strm.rpos();

  const bool ret = (strm >> stru.common);

  if (ret && strm.rpos() - start_pos < total_size) {
    strm.skip(total_size - strm.rpos() + start_pos);
  }
  return ret;
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::MinimalBitfield& stru)
{
  serialized_size_delimiter(encoding, size);
  XTypes::NameHash_forany stru_name_hash(const_cast<XTypes::NameHash_slice*>(stru.name_hash));
  serialized_size(encoding, size, stru.common);
  serialized_size(encoding, size, stru_name_hash);
}

bool operator<<(Serializer& strm, const XTypes::MinimalBitfield& stru)
{
  size_t total_size = 0;
  serialized_size(strm.encoding(), total_size, stru);
  if (!strm.write_delimiter(total_size)) {
    return false;
  }

  XTypes::NameHash_forany stru_name_hash(const_cast<XTypes::NameHash_slice*>(stru.name_hash));
  return (strm << stru.common)
    && (strm << stru_name_hash);
}

bool operator>>(Serializer& strm, XTypes::MinimalBitfield& stru)
{
  XTypes::NameHash_forany stru_name_hash(const_cast<XTypes::NameHash_slice*>(stru.name_hash));
  size_t total_size = 0;
  if (!strm.read_delimiter(total_size)) {
    return false;
  }

  const size_t start_pos = strm.rpos();

  const bool ret = (strm >> stru.common)
    && (strm >> stru_name_hash);

  if (ret && strm.rpos() - start_pos < total_size) {
    strm.skip(total_size - strm.rpos() + start_pos);
  }
  return ret;
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::MinimalBitflag& stru)
{
  serialized_size_delimiter(encoding, size);
  serialized_size(encoding, size, stru.common);
  serialized_size(encoding, size, stru.detail);
}

bool operator<<(Serializer& strm, const XTypes::MinimalBitflag& stru)
{
  size_t total_size = 0;
  serialized_size(strm.encoding(), total_size, stru);
  if (!strm.write_delimiter(total_size)) {
    return false;
  }

  return (strm << stru.common)
    && (strm << stru.detail);
}

bool operator>>(Serializer& strm, XTypes::MinimalBitflag& stru)
{
  size_t total_size = 0;
  if (!strm.read_delimiter(total_size)) {
    return false;
  }

  const size_t start_pos = strm.rpos();

  const bool ret = (strm >> stru.common)
    && (strm >> stru.detail);

  if (ret && strm.rpos() - start_pos < total_size) {
    strm.skip(total_size - strm.rpos() + start_pos);
  }
  return ret;
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::MinimalBitsetHeader&)
{
  serialized_size_delimiter(encoding, size);
}

bool operator<<(Serializer& strm, const XTypes::MinimalBitsetHeader&)
{
  return XTypes::write_empty_xcdr2_nonfinal(strm);
}

bool operator>>(Serializer& strm, XTypes::MinimalBitsetHeader&)
{
  return XTypes::read_empty_xcdr2_nonfinal(strm);
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::MinimalCollectionElement& stru)
{
  serialized_size_delimiter(encoding, size);
  serialized_size(encoding, size, stru.common);
}

bool operator<<(Serializer& strm, const XTypes::MinimalCollectionElement& stru)
{
  size_t total_size = 0;
  serialized_size(strm.encoding(), total_size, stru);
  if (!strm.write_delimiter(total_size)) {
    return false;
  }

  return (strm << stru.common);
}

bool operator>>(Serializer& strm, XTypes::MinimalCollectionElement& stru)
{
  size_t total_size = 0;
  if (!strm.read_delimiter(total_size)) {
    return false;
  }

  const size_t start_pos = strm.rpos();

  const bool ret = (strm >> stru.common);

  if (ret && strm.rpos() - start_pos < total_size) {
    strm.skip(total_size - strm.rpos() + start_pos);
  }
  return ret;
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::MinimalCollectionHeader& stru)
{
  serialized_size_delimiter(encoding, size);
  serialized_size(encoding, size, stru.common);
}

bool operator<<(Serializer& strm, const XTypes::MinimalCollectionHeader& stru)
{
  size_t total_size = 0;
  serialized_size(strm.encoding(), total_size, stru);
  if (!strm.write_delimiter(total_size)) {
    return false;
  }

  return (strm << stru.common);
}

bool operator>>(Serializer& strm, XTypes::MinimalCollectionHeader& stru)
{
  size_t total_size = 0;
  if (!strm.read_delimiter(total_size)) {
    return false;
  }

  const size_t start_pos = strm.rpos();

  const bool ret = (strm >> stru.common);

  if (ret && strm.rpos() - start_pos < total_size) {
    strm.skip(total_size - strm.rpos() + start_pos);
  }
  return ret;
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::MinimalDiscriminatorMember& stru)
{
  serialized_size_delimiter(encoding, size);
  serialized_size(encoding, size, stru.common);
}

bool operator<<(Serializer& strm, const XTypes::MinimalDiscriminatorMember& stru)
{
  size_t total_size = 0;
  serialized_size(strm.encoding(), total_size, stru);
  if (!strm.write_delimiter(total_size)) {
    return false;
  }

  return (strm << stru.common);
}

bool operator>>(Serializer& strm, XTypes::MinimalDiscriminatorMember& stru)
{
  size_t total_size = 0;
  if (!strm.read_delimiter(total_size)) {
    return false;
  }

  const size_t start_pos = strm.rpos();

  const bool ret = (strm >> stru.common);

  if (ret && strm.rpos() - start_pos < total_size) {
    strm.skip(total_size - strm.rpos() + start_pos);
  }
  return ret;
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::MinimalEnumeratedLiteral& stru)
{
  serialized_size_delimiter(encoding, size);
  serialized_size(encoding, size, stru.common);
  serialized_size(encoding, size, stru.detail);
}

bool operator<<(Serializer& strm, const XTypes::MinimalEnumeratedLiteral& stru)
{
  size_t total_size = 0;
  serialized_size(strm.encoding(), total_size, stru);
  if (!strm.write_delimiter(total_size)) {
    return false;
  }

  return (strm << stru.common)
    && (strm << stru.detail);
}

bool operator>>(Serializer& strm, XTypes::MinimalEnumeratedLiteral& stru)
{
  size_t total_size = 0;
  if (!strm.read_delimiter(total_size)) {
    return false;
  }

  const size_t start_pos = strm.rpos();

  const bool ret = (strm >> stru.common)
    && (strm >> stru.detail);

  if (ret && strm.rpos() - start_pos < total_size) {
    strm.skip(total_size - strm.rpos() + start_pos);
  }
  return ret;
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::MinimalStructMember& stru)
{
  serialized_size_delimiter(encoding, size);
  serialized_size(encoding, size, stru.common);
  serialized_size(encoding, size, stru.detail);
}

bool operator<<(Serializer& strm, const XTypes::MinimalStructMember& stru)
{
  size_t total_size = 0;
  serialized_size(strm.encoding(), total_size, stru);
  if (!strm.write_delimiter(total_size)) {
    return false;
  }

  return (strm << stru.common)
    && (strm << stru.detail);
}

bool operator>>(Serializer& strm, XTypes::MinimalStructMember& stru)
{
  size_t total_size = 0;
  if (!strm.read_delimiter(total_size)) {
    return false;
  }

  const size_t start_pos = strm.rpos();

  const bool ret = (strm >> stru.common)
    && (strm >> stru.detail);

  if (ret && strm.rpos() - start_pos < total_size) {
    strm.skip(total_size - strm.rpos() + start_pos);
  }
  return ret;
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::MinimalUnionHeader& stru)
{
  serialized_size_delimiter(encoding, size);
  serialized_size(encoding, size, stru.detail);
}

bool operator<<(Serializer& strm, const XTypes::MinimalUnionHeader& stru)
{
  size_t total_size = 0;
  serialized_size(strm.encoding(), total_size, stru);
  if (!strm.write_delimiter(total_size)) {
    return false;
  }

  return (strm << stru.detail);
}

bool operator>>(Serializer& strm, XTypes::MinimalUnionHeader& stru)
{
  size_t total_size = 0;
  if (!strm.read_delimiter(total_size)) {
    return false;
  }

  const size_t start_pos = strm.rpos();

  const bool ret = (strm >> stru.detail);

  if (ret && strm.rpos() - start_pos < total_size) {
    strm.skip(total_size - strm.rpos() + start_pos);
  }
  return ret;
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::MinimalUnionMember& stru)
{
  serialized_size_delimiter(encoding, size);
  serialized_size(encoding, size, stru.common);
  serialized_size(encoding, size, stru.detail);
}

bool operator<<(Serializer& strm, const XTypes::MinimalUnionMember& stru)
{
  size_t total_size = 0;
  serialized_size(strm.encoding(), total_size, stru);
  if (!strm.write_delimiter(total_size)) {
    return false;
  }

  return (strm << stru.common)
    && (strm << stru.detail);
}

bool operator>>(Serializer& strm, XTypes::MinimalUnionMember& stru)
{
  size_t total_size = 0;
  if (!strm.read_delimiter(total_size)) {
    return false;
  }

  const size_t start_pos = strm.rpos();

  const bool ret = (strm >> stru.common)
    && (strm >> stru.detail);

  if (ret && strm.rpos() - start_pos < total_size) {
    strm.skip(total_size - strm.rpos() + start_pos);
  }
  return ret;
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::AnnotationParameterValue& uni)
{
  primitive_serialized_size(encoding, size, ACE_OutputCDR::from_octet(uni.kind));
  switch (uni.kind) {
  case XTypes::TK_BOOLEAN: {
    primitive_serialized_size(encoding, size, ACE_OutputCDR::from_boolean(uni.boolean_value));
    break;
  }
  case XTypes::TK_BYTE: {
    primitive_serialized_size(encoding, size, ACE_OutputCDR::from_octet(uni.byte_value));
    break;
  }
  case XTypes::TK_INT16: {
    primitive_serialized_size(encoding, size, uni.int16_value);
    break;
  }
  case XTypes::TK_UINT16: {
    primitive_serialized_size(encoding, size, uni.uint16_value);
    break;
  }
  case XTypes::TK_INT32: {
    primitive_serialized_size(encoding, size, uni.int32_value);
    break;
  }
  case XTypes::TK_UINT32: {
    primitive_serialized_size(encoding, size, uni.uint32_value);
    break;
  }
  case XTypes::TK_INT64: {
    primitive_serialized_size(encoding, size, uni.int64_value);
    break;
  }
  case XTypes::TK_UINT64: {
    primitive_serialized_size(encoding, size, uni.uint64_value);
    break;
  }
  case XTypes::TK_FLOAT32: {
    primitive_serialized_size(encoding, size, uni.float32_value);
    break;
  }
  case XTypes::TK_FLOAT64: {
    primitive_serialized_size(encoding, size, uni.float64_value);
    break;
  }
  case XTypes::TK_FLOAT128: {
    primitive_serialized_size(encoding, size, ACE_CDR::LongDouble());
    break;
  }
  case XTypes::TK_CHAR8: {
    primitive_serialized_size(encoding, size, ACE_OutputCDR::from_char(uni.char_value));
    break;
  }
  case XTypes::TK_CHAR16: {
    primitive_serialized_size(encoding, size, ACE_OutputCDR::from_wchar(uni.wchar_value));
    break;
  }
  case XTypes::TK_ENUM: {
    primitive_serialized_size(encoding, size, uni.enumerated_value);
    break;
  }
  case XTypes::TK_STRING8: {
    DCPS::primitive_serialized_size_ulong(encoding, size);
    size += ACE_OS::strlen(uni.string8_value.c_str()) + 1;
    break;
  }
  case XTypes::TK_STRING16: {
#ifdef DDS_HAS_WCHAR
    DCPS::primitive_serialized_size_ulong(encoding, size);
    size += ACE_OS::strlen(uni.string16_value.c_str()) * DCPS::char16_cdr_size;
#endif
    break;
  }
  default: {
    serialized_size(encoding, size, uni.extended_value);
    break;
  }
  }
}

bool operator<<(Serializer& strm, const XTypes::AnnotationParameterValue& uni)
{
  if (!(strm << ACE_OutputCDR::from_octet(uni.kind))) {
    return false;
  }
  switch (uni.kind) {
  case XTypes::TK_BOOLEAN: {
    return (strm << ACE_OutputCDR::from_boolean(uni.boolean_value));
  }
  case XTypes::TK_BYTE: {
    return (strm << ACE_OutputCDR::from_octet(uni.byte_value));
  }
  case XTypes::TK_INT16: {
    return (strm << uni.int16_value);
  }
  case XTypes::TK_UINT16: {
    return (strm << uni.uint16_value);
  }
  case XTypes::TK_INT32: {
    return (strm << uni.int32_value);
  }
  case XTypes::TK_UINT32: {
    return (strm << uni.uint32_value);
  }
  case XTypes::TK_INT64: {
    return (strm << uni.int64_value);
  }
  case XTypes::TK_UINT64: {
    return (strm << uni.uint64_value);
  }
  case XTypes::TK_FLOAT32: {
    return (strm << uni.float32_value);
  }
  case XTypes::TK_FLOAT64: {
    return (strm << uni.float64_value);
  }
  case XTypes::TK_FLOAT128: {
    return (strm << uni.float128_value);
  }
  case XTypes::TK_CHAR8: {
    return (strm << ACE_OutputCDR::from_char(uni.char_value));
  }
  case XTypes::TK_CHAR16: {
    return (strm << ACE_OutputCDR::from_wchar(uni.wchar_value));
  }
  case XTypes::TK_ENUM: {
    return (strm << uni.enumerated_value);
  }
  case XTypes::TK_STRING8: {
    return (strm << Serializer::FromBoundedString<char>(uni.string8_value, 128));
  }
  case XTypes::TK_STRING16: {
#ifdef DDS_HAS_WCHAR
    return (strm << Serializer::FromBoundedString<wchar_t>(uni.string16_value, 128));
#else
    return false;
#endif
  }
  default: {
    return (strm << uni.extended_value);
  }
  }
}

bool operator>>(Serializer& strm, XTypes::AnnotationParameterValue& uni)
{
  ACE_CDR::Octet kind;
  if (!(strm >> ACE_InputCDR::to_octet(kind))) {
    return false;
  }
  switch (kind) {
  case XTypes::TK_BOOLEAN: {
    ACE_CDR::Boolean tmp;
    if (strm >> ACE_InputCDR::to_boolean(tmp)) {
      uni.boolean_value = tmp;
      uni.kind = kind;
      return true;
    }
    return false;
  }
  case XTypes::TK_BYTE: {
    ACE_CDR::Octet tmp;
    if (strm >> ACE_InputCDR::to_octet(tmp)) {
      uni.byte_value = tmp;
      uni.kind = kind;
      return true;
    }
    return false;
  }
  case XTypes::TK_INT16: {
    ACE_CDR::Short tmp;
    if (strm >> tmp) {
      uni.int16_value = tmp;
      uni.kind = kind;
      return true;
    }
    return false;
  }
  case XTypes::TK_UINT16: {
    ACE_CDR::UShort tmp;
    if (strm >> tmp) {
      uni.uint16_value = tmp;
      uni.kind = kind;
      return true;
    }
    return false;
  }
  case XTypes::TK_INT32: {
    ACE_CDR::Long tmp;
    if (strm >> tmp) {
      uni.int32_value = tmp;
      uni.kind = kind;
      return true;
    }
    return false;
  }
  case XTypes::TK_UINT32: {
    ACE_CDR::ULong tmp;
    if (strm >> tmp) {
      uni.uint32_value = tmp;
      uni.kind = kind;
      return true;
    }
    return false;
  }
  case XTypes::TK_INT64: {
    ACE_CDR::LongLong tmp;
    if (strm >> tmp) {
      uni.int64_value = tmp;
      uni.kind = kind;
      return true;
    }
    return false;
  }
  case XTypes::TK_UINT64: {
    ACE_CDR::ULongLong tmp;
    if (strm >> tmp) {
      uni.uint64_value = tmp;
      uni.kind = kind;
      return true;
    }
    return false;
  }
  case XTypes::TK_FLOAT32: {
    ACE_CDR::Float tmp;
    if (strm >> tmp) {
      uni.float32_value = tmp;
      uni.kind = kind;
      return true;
    }
    return false;
  }
  case XTypes::TK_FLOAT64: {
    ACE_CDR::Double tmp;
    if (strm >> tmp) {
      uni.float64_value = tmp;
      uni.kind = kind;
      return true;
    }
    return false;
  }
  case XTypes::TK_FLOAT128: {
    ACE_CDR::LongDouble tmp;
    if (strm >> tmp) {
      uni.float128_value = tmp;
      uni.kind = kind;
      return true;
    }
    return false;
  }
  case XTypes::TK_CHAR8: {
    ACE_CDR::Char tmp;
    if (strm >> ACE_InputCDR::to_char(tmp)) {
      uni.char_value = tmp;
      uni.kind = kind;
      return true;
    }
    return false;
  }
  case XTypes::TK_CHAR16: {
    ACE_CDR::WChar tmp;
    if (strm >> ACE_InputCDR::to_wchar(tmp)) {
      uni.wchar_value = tmp;
      uni.kind = kind;
      return true;
    }
    return false;
  }
  case XTypes::TK_ENUM: {
    ACE_CDR::Long tmp;
    if (strm >> tmp) {
      uni.enumerated_value = tmp;
      uni.kind = kind;
      return true;
    }
    return false;
  }
  case XTypes::TK_STRING8: {
    OPENDDS_STRING tmp;
    if (strm >> Serializer::ToBoundedString<char>(tmp, 128)) {
      uni.string8_value = tmp;
      uni.kind = kind;
      return true;
    }
    return false;
  }
  case XTypes::TK_STRING16: {
#ifdef DDS_HAS_WCHAR
    OPENDDS_WSTRING tmp;
    if (strm >> Serializer::ToBoundedString<wchar_t>(tmp, 128)) {
      uni.string16_value = tmp;
      uni.kind = kind;
      return true;
    }
#endif
    return false;
  }
  default: {
    XTypes::ExtendedAnnotationParameterValue tmp;
    if (strm >> tmp) {
      uni.extended_value = tmp;
      uni.kind = kind;
      return true;
    }
    return false;
  }
  }
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::AppliedAnnotationParameter& stru)
{
  serialized_size_delimiter(encoding, size);
  XTypes::NameHash_forany stru_paramname_hash(const_cast<XTypes::NameHash_slice*>(stru.paramname_hash));
  serialized_size(encoding, size, stru_paramname_hash);
  serialized_size(encoding, size, stru.value);
}

bool operator<<(Serializer& strm, const XTypes::AppliedAnnotationParameter& stru)
{
  size_t total_size = 0;
  serialized_size(strm.encoding(), total_size, stru);
  if (!strm.write_delimiter(total_size)) {
    return false;
  }

  XTypes::NameHash_forany stru_paramname_hash(const_cast<XTypes::NameHash_slice*>(stru.paramname_hash));
  return (strm << stru_paramname_hash)
    && (strm << stru.value);
}

bool operator>>(Serializer& strm, XTypes::AppliedAnnotationParameter& stru)
{
  size_t total_size = 0;
  if (!strm.read_delimiter(total_size)) {
    return false;
  }

  const size_t start_pos = strm.rpos();

  XTypes::NameHash_forany stru_paramname_hash(const_cast<XTypes::NameHash_slice*>(stru.paramname_hash));
  const bool ret = (strm >> stru_paramname_hash)
    && (strm >> stru.value);

  if (ret && strm.rpos() - start_pos < total_size) {
    strm.skip(total_size - strm.rpos() + start_pos);
  }
  return ret;
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::AppliedBuiltinMemberAnnotations& stru)
{
  serialized_size_delimiter(encoding, size);

  size += DCPS::boolean_cdr_size;
  if (stru.unit.present) {
    DCPS::primitive_serialized_size_ulong(encoding, size);
    size += ACE_OS::strlen(stru.unit.value.c_str()) + 1;
  }

  serialized_size(encoding, size, stru.min);
  serialized_size(encoding, size, stru.max);

  size += DCPS::boolean_cdr_size;
  if (stru.hash_id.present) {
    DCPS::primitive_serialized_size_ulong(encoding, size);
    size += ACE_OS::strlen(stru.hash_id.value.c_str()) + 1;
  }
}

bool operator<<(Serializer& strm, const XTypes::AppliedBuiltinMemberAnnotations& stru)
{
  size_t total_size = 0;
  serialized_size(strm.encoding(), total_size, stru);
  if (!strm.write_delimiter(total_size)) {
    return false;
  }

  return (strm << stru.unit)
    && (strm << stru.min)
    && (strm << stru.max)
    && (strm << stru.hash_id);
}

bool operator>>(Serializer& strm, XTypes::AppliedBuiltinMemberAnnotations& stru)
{
  size_t total_size = 0;
  if (!strm.read_delimiter(total_size)) {
    return false;
  }

  const size_t start_pos = strm.rpos();

  const bool ret = (strm >> stru.unit)
    && (strm >> stru.min)
    && (strm >> stru.max)
    && (strm >> stru.hash_id);

  if (ret && strm.rpos() - start_pos < total_size) {
    strm.skip(total_size - strm.rpos() + start_pos);
  }
  return ret;
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::AppliedVerbatimAnnotation& stru)
{
  DCPS::primitive_serialized_size_ulong(encoding, size);
  size += ACE_OS::strlen(stru.placement.c_str()) + 1;
  DCPS::primitive_serialized_size_ulong(encoding, size);
  size += ACE_OS::strlen(stru.language.c_str()) + 1;
  DCPS::primitive_serialized_size_ulong(encoding, size);
  size += ACE_OS::strlen(stru.text.c_str()) + 1;
}

bool operator<<(Serializer& strm, const XTypes::AppliedVerbatimAnnotation& stru)
{
  return (strm << Serializer::FromBoundedString<char>(stru.placement, 32))
    && (strm << Serializer::FromBoundedString<char>(stru.language, 32))
    && (strm << stru.text);
}

bool operator>>(Serializer& strm, XTypes::AppliedVerbatimAnnotation& stru)
{
  return (strm >> Serializer::ToBoundedString<char>(stru.placement, 32))
    && (strm >> Serializer::ToBoundedString<char>(stru.language, 32))
    && (strm >> stru.text);
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::CommonAliasBody& stru)
{
  primitive_serialized_size(encoding, size, stru.related_flags);
  serialized_size(encoding, size, stru.related_type);
}

bool operator<<(Serializer& strm, const XTypes::CommonAliasBody& stru)
{
  return (strm << stru.related_flags)
    && (strm << stru.related_type);
}

bool operator>>(Serializer& strm, XTypes::CommonAliasBody& stru)
{
  return (strm >> stru.related_flags)
    && (strm >> stru.related_type);
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::CommonAnnotationParameter& stru)
{
  primitive_serialized_size(encoding, size, stru.member_flags);
  serialized_size(encoding, size, stru.member_type_id);
}

bool operator<<(Serializer& strm, const XTypes::CommonAnnotationParameter& stru)
{
  return (strm << stru.member_flags)
    && (strm << stru.member_type_id);
}

bool operator>>(Serializer& strm, XTypes::CommonAnnotationParameter& stru)
{
  return (strm >> stru.member_flags)
    && (strm >> stru.member_type_id);
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::CommonArrayHeader& stru)
{
  serialized_size(encoding, size, stru.bound_seq);
}

bool operator<<(Serializer& strm, const XTypes::CommonArrayHeader& stru)
{
  return (strm << stru.bound_seq);
}

bool operator>>(Serializer& strm, XTypes::CommonArrayHeader& stru)
{
  return (strm >> stru.bound_seq);
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::CommonBitfield& stru)
{
  primitive_serialized_size(encoding, size, stru.position);
  primitive_serialized_size(encoding, size, stru.flags);
  primitive_serialized_size(encoding, size, ACE_OutputCDR::from_octet(stru.bitcount));
  primitive_serialized_size(encoding, size, ACE_OutputCDR::from_octet(stru.holder_type));
}

bool operator<<(Serializer& strm, const XTypes::CommonBitfield& stru)
{
  return (strm << stru.position)
    && (strm << stru.flags)
    && (strm << ACE_OutputCDR::from_octet(stru.bitcount))
    && (strm << ACE_OutputCDR::from_octet(stru.holder_type));
}

bool operator>>(Serializer& strm, XTypes::CommonBitfield& stru)
{
  return (strm >> stru.position)
    && (strm >> stru.flags)
    && (strm >> ACE_InputCDR::to_octet(stru.bitcount))
    && (strm >> ACE_InputCDR::to_octet(stru.holder_type));
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::CommonBitflag& stru)
{
  primitive_serialized_size(encoding, size, stru.position);
  primitive_serialized_size(encoding, size, stru.flags);
}

bool operator<<(Serializer& strm, const XTypes::CommonBitflag& stru)
{
  return (strm << stru.position)
    && (strm << stru.flags);
}

bool operator>>(Serializer& strm, XTypes::CommonBitflag& stru)
{
  return (strm >> stru.position)
    && (strm >> stru.flags);
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::CommonCollectionElement& stru)
{
  primitive_serialized_size(encoding, size, stru.element_flags);
  serialized_size(encoding, size, stru.type);
}

bool operator<<(Serializer& strm, const XTypes::CommonCollectionElement& stru)
{
  return (strm << stru.element_flags)
    && (strm << stru.type);
}

bool operator>>(Serializer& strm, XTypes::CommonCollectionElement& stru)
{
  return (strm >> stru.element_flags)
    && (strm >> stru.type);
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::CommonCollectionHeader& stru)
{
  primitive_serialized_size(encoding, size, stru.bound);
}

bool operator<<(Serializer& strm, const XTypes::CommonCollectionHeader& stru)
{
  return (strm << stru.bound);
}

bool operator>>(Serializer& strm, XTypes::CommonCollectionHeader& stru)
{
  return (strm >> stru.bound);
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::CommonDiscriminatorMember& stru)
{
  primitive_serialized_size(encoding, size, stru.member_flags);
  serialized_size(encoding, size, stru.type_id);
}

bool operator<<(Serializer& strm, const XTypes::CommonDiscriminatorMember& stru)
{
  return (strm << stru.member_flags)
    && (strm << stru.type_id);
}

bool operator>>(Serializer& strm, XTypes::CommonDiscriminatorMember& stru)
{
  return (strm >> stru.member_flags)
    && (strm >> stru.type_id);
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::CommonEnumeratedHeader& stru)
{
  primitive_serialized_size(encoding, size, stru.bit_bound);
}

bool operator<<(Serializer& strm, const XTypes::CommonEnumeratedHeader& stru)
{
  return (strm << stru.bit_bound);
}

bool operator>>(Serializer& strm, XTypes::CommonEnumeratedHeader& stru)
{
  return (strm >> stru.bit_bound);
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::CommonEnumeratedLiteral& stru)
{
  serialized_size_delimiter(encoding, size);
  primitive_serialized_size(encoding, size, stru.value);
  primitive_serialized_size(encoding, size, stru.flags);
}

bool operator<<(Serializer& strm, const XTypes::CommonEnumeratedLiteral& stru)
{
  size_t total_size = 0;
  serialized_size(strm.encoding(), total_size, stru);
  if (!strm.write_delimiter(total_size)) {
    return false;
  }

  return (strm << stru.value)
    && (strm << stru.flags);
}

bool operator>>(Serializer& strm, XTypes::CommonEnumeratedLiteral& stru)
{
  size_t total_size = 0;
  if (!strm.read_delimiter(total_size)) {
    return false;
  }

  const size_t start_pos = strm.rpos();

  const bool ret = (strm >> stru.value)
    && (strm >> stru.flags);

  if (ret && strm.rpos() - start_pos < total_size) {
    strm.skip(total_size - strm.rpos() + start_pos);
  }
  return ret;
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::CommonStructMember& stru)
{
  primitive_serialized_size(encoding, size, stru.member_id);
  primitive_serialized_size(encoding, size, stru.member_flags);
  serialized_size(encoding, size, stru.member_type_id);
}

bool operator<<(Serializer& strm, const XTypes::CommonStructMember& stru)
{
  return (strm << stru.member_id)
    && (strm << stru.member_flags)
    && (strm << stru.member_type_id);
}

bool operator>>(Serializer& strm, XTypes::CommonStructMember& stru)
{
  return (strm >> stru.member_id)
    && (strm >> stru.member_flags)
    && (strm >> stru.member_type_id);
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::CommonUnionMember& stru)
{
  primitive_serialized_size(encoding, size, stru.member_id);
  primitive_serialized_size(encoding, size, stru.member_flags);
  serialized_size(encoding, size, stru.type_id);
  serialized_size(encoding, size, stru.label_seq);
}

bool operator<<(Serializer& strm, const XTypes::CommonUnionMember& stru)
{
  return (strm << stru.member_id)
    && (strm << stru.member_flags)
    && (strm << stru.type_id)
    && (strm << stru.label_seq);
}

bool operator>>(Serializer& strm, XTypes::CommonUnionMember& stru)
{
  return (strm >> stru.member_id)
    && (strm >> stru.member_flags)
    && (strm >> stru.type_id)
    && (strm >> stru.label_seq);
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::CompleteElementDetail& stru)
{
  serialized_size(encoding, size, stru.ann_builtin);
  serialized_size(encoding, size, stru.ann_custom);
}

bool operator<<(Serializer& strm, const XTypes::CompleteElementDetail& stru)
{
  return (strm << stru.ann_builtin)
    && (strm << stru.ann_custom);
}

bool operator>>(Serializer& strm, XTypes::CompleteElementDetail& stru)
{
  return (strm >> stru.ann_builtin)
    && (strm >> stru.ann_custom);
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::CompleteMemberDetail& stru)
{
  DCPS::primitive_serialized_size_ulong(encoding, size);
  size += ACE_OS::strlen(stru.name.c_str()) + 1;
  serialized_size(encoding, size, stru.ann_builtin);
  serialized_size(encoding, size, stru.ann_custom);
}

bool operator<<(Serializer& strm, const XTypes::CompleteMemberDetail& stru)
{
  return (strm << Serializer::FromBoundedString<char>(stru.name, 256))
    && (strm << stru.ann_builtin)
    && (strm << stru.ann_custom);
}

bool operator>>(Serializer& strm, XTypes::CompleteMemberDetail& stru)
{
  return (strm >> Serializer::ToBoundedString<char>(stru.name, 256))
    && (strm >> stru.ann_builtin)
    && (strm >> stru.ann_custom);
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::MinimalMemberDetail& stru)
{
  XTypes::NameHash_forany stru_name_hash(const_cast<XTypes::NameHash_slice*>(stru.name_hash));
  serialized_size(encoding, size, stru_name_hash);
}

bool operator<<(Serializer& strm, const XTypes::MinimalMemberDetail& stru)
{
  XTypes::NameHash_forany stru_name_hash(const_cast<XTypes::NameHash_slice*>(stru.name_hash));
  return (strm << stru_name_hash);
}

bool operator>>(Serializer& strm, XTypes::MinimalMemberDetail& stru)
{
  XTypes::NameHash_forany stru_name_hash(const_cast<XTypes::NameHash_slice*>(stru.name_hash));
  return (strm >> stru_name_hash);
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::ExtendedTypeDefn&)
{
  serialized_size_delimiter(encoding, size);
}

bool operator<<(Serializer& strm, const XTypes::ExtendedTypeDefn&)
{
  return XTypes::write_empty_xcdr2_nonfinal(strm);
}

bool operator>>(Serializer& strm, XTypes::ExtendedTypeDefn&)
{
  return XTypes::read_empty_xcdr2_nonfinal(strm);
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::PlainArrayLElemDefn& stru)
{
  serialized_size(encoding, size, stru.header);
  serialized_size(encoding, size, stru.array_bound_seq);
  serialized_size(encoding, size, *stru.element_identifier);
}

bool operator<<(Serializer& strm, const XTypes::PlainArrayLElemDefn& stru)
{
  return (strm << stru.header)
    && (strm << stru.array_bound_seq)
    && (strm << *stru.element_identifier);
}

bool operator>>(Serializer& strm, XTypes::PlainArrayLElemDefn& stru)
{
  return (strm >> stru.header)
    && (strm >> stru.array_bound_seq)
    && (strm >> *stru.element_identifier);
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::PlainArraySElemDefn& stru)
{
  serialized_size(encoding, size, stru.header);
  serialized_size(encoding, size, stru.array_bound_seq);
  serialized_size(encoding, size, *stru.element_identifier);
}

bool operator<<(Serializer& strm, const XTypes::PlainArraySElemDefn& stru)
{
  return (strm << stru.header)
    && (strm << stru.array_bound_seq)
    && (strm << *stru.element_identifier);
}

bool operator>>(Serializer& strm, XTypes::PlainArraySElemDefn& stru)
{
  return (strm >> stru.header)
    && (strm >> stru.array_bound_seq)
    && (strm >> *stru.element_identifier);
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::PlainMapLTypeDefn& stru)
{
  serialized_size(encoding, size, stru.header);
  primitive_serialized_size(encoding, size, stru.bound);
  serialized_size(encoding, size, *stru.element_identifier);
  primitive_serialized_size(encoding, size, stru.key_flags);
  serialized_size(encoding, size, *stru.key_identifier);
}

bool operator<<(Serializer& strm, const XTypes::PlainMapLTypeDefn& stru)
{
  return (strm << stru.header)
    && (strm << stru.bound)
    && (strm << *stru.element_identifier)
    && (strm << stru.key_flags)
    && (strm << *stru.key_identifier);
}

bool operator>>(Serializer& strm, XTypes::PlainMapLTypeDefn& stru)
{
  return (strm >> stru.header)
    && (strm >> stru.bound)
    && (strm >> *stru.element_identifier)
    && (strm >> stru.key_flags)
    && (strm >> *stru.key_identifier);
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::PlainMapSTypeDefn& stru)
{
  serialized_size(encoding, size, stru.header);
  primitive_serialized_size(encoding, size, ACE_OutputCDR::from_octet(stru.bound));
  serialized_size(encoding, size, *stru.element_identifier);
  primitive_serialized_size(encoding, size, stru.key_flags);
  serialized_size(encoding, size, *stru.key_identifier);
}

bool operator<<(Serializer& strm, const XTypes::PlainMapSTypeDefn& stru)
{
  return (strm << stru.header)
    && (strm << ACE_OutputCDR::from_octet(stru.bound))
    && (strm << *stru.element_identifier)
    && (strm << stru.key_flags)
    && (strm << *stru.key_identifier);
}

bool operator>>(Serializer& strm, XTypes::PlainMapSTypeDefn& stru)
{
  return (strm >> stru.header)
    && (strm >> ACE_InputCDR::to_octet(stru.bound))
    && (strm >> *stru.element_identifier)
    && (strm >> stru.key_flags)
    && (strm >> *stru.key_identifier);
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::PlainSequenceLElemDefn& stru)
{
  serialized_size(encoding, size, stru.header);
  primitive_serialized_size(encoding, size, stru.bound);
  serialized_size(encoding, size, *stru.element_identifier);
}

bool operator<<(Serializer& strm, const XTypes::PlainSequenceLElemDefn& stru)
{
  return (strm << stru.header)
    && (strm << stru.bound)
    && (strm << *stru.element_identifier);
}

bool operator>>(Serializer& strm, XTypes::PlainSequenceLElemDefn& stru)
{
  return (strm >> stru.header)
    && (strm >> stru.bound)
    && (strm >> *stru.element_identifier);
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::PlainSequenceSElemDefn& stru)
{
  serialized_size(encoding, size, stru.header);
  primitive_serialized_size(encoding, size, ACE_OutputCDR::from_octet(stru.bound));
  serialized_size(encoding, size, *stru.element_identifier);
}

bool operator<<(Serializer& strm, const XTypes::PlainSequenceSElemDefn& stru)
{
  return (strm << stru.header)
    && (strm << ACE_OutputCDR::from_octet(stru.bound))
    && (strm << *stru.element_identifier);
}

bool operator>>(Serializer& strm, XTypes::PlainSequenceSElemDefn& stru)
{
  return (strm >> stru.header)
    && (strm >> ACE_InputCDR::to_octet(stru.bound))
    && (strm >> *stru.element_identifier);
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::StringLTypeDefn& stru)
{
  primitive_serialized_size(encoding, size, stru.bound);
}

bool operator<<(Serializer& strm, const XTypes::StringLTypeDefn& stru)
{
  return (strm << stru.bound);
}

bool operator>>(Serializer& strm, XTypes::StringLTypeDefn& stru)
{
  return (strm >> stru.bound);
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::StringSTypeDefn& stru)
{
  primitive_serialized_size(encoding, size, ACE_OutputCDR::from_octet(stru.bound));
}

bool operator<<(Serializer& strm, const XTypes::StringSTypeDefn& stru)
{
  return (strm << ACE_OutputCDR::from_octet(stru.bound));
}

bool operator>>(Serializer& strm, XTypes::StringSTypeDefn& stru)
{
  return (strm >> ACE_InputCDR::to_octet(stru.bound));
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::StronglyConnectedComponentId& stru)
{
  serialized_size_delimiter(encoding, size);
  serialized_size(encoding, size, stru.sc_component_id);
  primitive_serialized_size(encoding, size, stru.scc_length);
  primitive_serialized_size(encoding, size, stru.scc_index);
}

bool operator<<(Serializer& strm, const XTypes::StronglyConnectedComponentId& stru)
{
  size_t total_size = 0;
  serialized_size(XTypes::get_typeobject_encoding(), total_size, stru);
  if (!strm.write_delimiter(total_size)) {
    return false;
  }

  return (strm << stru.sc_component_id)
    && (strm << stru.scc_length)
    && (strm << stru.scc_index);
}

bool operator>>(Serializer& strm, XTypes::StronglyConnectedComponentId& stru)
{
  size_t total_size = 0;
  if (!strm.read_delimiter(total_size)) {
    return false;
  }

  const size_t start_pos = strm.rpos();

  // appendable, but no need to handle truncated streams since
  // this struct is defined in the spec with the following members:
  const bool ret = (strm >> stru.sc_component_id)
    && (strm >> stru.scc_length)
    && (strm >> stru.scc_index);

  if (ret && strm.rpos() - start_pos < total_size) {
    strm.skip(total_size - strm.rpos() + start_pos);
  }

  return ret;
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::PlainCollectionHeader& stru)
{
  primitive_serialized_size(encoding, size, ACE_OutputCDR::from_octet(stru.equiv_kind));
  primitive_serialized_size(encoding, size, stru.element_flags);
}

bool operator<<(Serializer& strm, const XTypes::PlainCollectionHeader& stru)
{
  return (strm << ACE_OutputCDR::from_octet(stru.equiv_kind))
    && (strm << stru.element_flags);
}

bool operator>>(Serializer& strm, XTypes::PlainCollectionHeader& stru)
{
  return (strm >> ACE_InputCDR::to_octet(stru.equiv_kind))
    && (strm >> stru.element_flags);
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::TypeObjectHashId& uni)
{
  primitive_serialized_size(encoding, size, ACE_OutputCDR::from_octet(uni.kind));
  switch (uni.kind) {
  case XTypes::EK_MINIMAL:
  case XTypes::EK_COMPLETE: {
    XTypes::EquivalenceHash_forany uni_hash(const_cast<XTypes::EquivalenceHash_slice*>(uni.hash));
    serialized_size(encoding, size, uni_hash);
    break;
  }
  default:
    break;
  }
}

bool operator<<(Serializer& strm, const XTypes::TypeObjectHashId& uni)
{
  if (!(strm << ACE_OutputCDR::from_octet(uni.kind))) {
    return false;
  }
  switch (uni.kind) {
  case XTypes::EK_MINIMAL:
  case XTypes::EK_COMPLETE: {
    XTypes::EquivalenceHash_forany uni_hash(const_cast<XTypes::EquivalenceHash_slice*>(uni.hash));
    return strm << uni_hash;
  }
  default:
    return true;
  }
}

bool operator>>(Serializer& strm, XTypes::TypeObjectHashId& uni)
{
  if (!(strm >> ACE_InputCDR::to_octet(uni.kind))) {
    return false;
  }
  switch (uni.kind) {
  case XTypes::EK_MINIMAL:
  case XTypes::EK_COMPLETE: {
    XTypes::EquivalenceHash_forany uni_hash(uni.hash);
    return strm >> uni_hash;
  }
  default:
    return true;
  }
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::CompleteTypeObject& type_object)
{
  using namespace XTypes;
  primitive_serialized_size_octet(encoding, size); // discriminator

  switch (type_object.kind) {
  case TK_ALIAS:
    serialized_size(encoding, size, type_object.alias_type);
    return;
  case TK_ANNOTATION:
    serialized_size(encoding, size, type_object.annotation_type);
    return;
  case TK_STRUCTURE:
    serialized_size(encoding, size, type_object.struct_type);
    return;
  case TK_UNION:
    serialized_size(encoding, size, type_object.union_type);
    return;
  case TK_BITSET:
    serialized_size(encoding, size, type_object.bitset_type);
    return;
  case TK_SEQUENCE:
    serialized_size(encoding, size, type_object.sequence_type);
    return;
  case TK_ARRAY:
    serialized_size(encoding, size, type_object.array_type);
    return;
  case TK_MAP:
    serialized_size(encoding, size, type_object.map_type);
    return;
  case TK_ENUM:
    serialized_size(encoding, size, type_object.enumerated_type);
    return;
  case TK_BITMASK:
    serialized_size(encoding, size, type_object.bitmask_type);
    return;
  default:
    serialized_size(encoding, size, type_object.extended_type);
  }
}

bool operator<<(Serializer& ser, const XTypes::CompleteTypeObject& type_object)
{
  using namespace XTypes;
  if (!(ser << ACE_OutputCDR::from_octet(type_object.kind))) {
    return false;
  }

  switch (type_object.kind) {
  case TK_ALIAS:
    return ser << type_object.alias_type;
  case TK_ANNOTATION:
    return ser << type_object.annotation_type;
  case TK_STRUCTURE:
    return ser << type_object.struct_type;
  case TK_UNION:
    return ser << type_object.union_type;
  case TK_BITSET:
    return ser << type_object.bitset_type;
  case TK_SEQUENCE:
    return ser << type_object.sequence_type;
  case TK_ARRAY:
    return ser << type_object.array_type;
  case TK_MAP:
    return ser << type_object.map_type;
  case TK_ENUM:
    return ser << type_object.enumerated_type;
  case TK_BITMASK:
    return ser << type_object.bitmask_type;
  default:
    return ser << type_object.extended_type;
  }
}

bool operator>>(Serializer& ser, XTypes::CompleteTypeObject& type_object)
{
  using namespace XTypes;
  if (!(ser >> ACE_InputCDR::to_octet(type_object.kind))) {
    return false;
  }

  switch (type_object.kind) {
  case TK_ALIAS:
    return ser >> type_object.alias_type;
  case TK_ANNOTATION:
    return ser >> type_object.annotation_type;
  case TK_STRUCTURE:
    return ser >> type_object.struct_type;
  case TK_UNION:
    return ser >> type_object.union_type;
  case TK_BITSET:
    return ser >> type_object.bitset_type;
  case TK_SEQUENCE:
    return ser >> type_object.sequence_type;
  case TK_ARRAY:
    return ser >> type_object.array_type;
  case TK_MAP:
    return ser >> type_object.map_type;
  case TK_ENUM:
    return ser >> type_object.enumerated_type;
  case TK_BITMASK:
    return ser >> type_object.bitmask_type;
  default:
    return ser >> type_object.extended_type;
  }
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::MinimalTypeObject& type_object)
{
  using namespace XTypes;
  primitive_serialized_size_octet(encoding, size); // discriminator

  switch (type_object.kind) {
  case TK_ALIAS:
    return serialized_size(encoding, size, type_object.alias_type);
  case TK_ANNOTATION:
    return serialized_size(encoding, size, type_object.annotation_type);
  case TK_STRUCTURE:
    return serialized_size(encoding, size, type_object.struct_type);
  case TK_UNION:
    return serialized_size(encoding, size, type_object.union_type);
  case TK_BITSET:
    return serialized_size(encoding, size, type_object.bitset_type);
  case TK_SEQUENCE:
    return serialized_size(encoding, size, type_object.sequence_type);
  case TK_ARRAY:
    return serialized_size(encoding, size, type_object.array_type);
  case TK_MAP:
    return serialized_size(encoding, size, type_object.map_type);
  case TK_ENUM:
    return serialized_size(encoding, size, type_object.enumerated_type);
  case TK_BITMASK:
    return serialized_size(encoding, size, type_object.bitmask_type);
  default:
    return serialized_size(encoding, size, type_object.extended_type);
  }
}

bool operator<<(Serializer& ser, const XTypes::MinimalTypeObject& type_object)
{
  using namespace XTypes;
  if (!(ser << ACE_OutputCDR::from_octet(type_object.kind))) {
    return false;
  }

  switch (type_object.kind) {
  case TK_ALIAS:
    return ser << type_object.alias_type;
  case TK_ANNOTATION:
    return ser << type_object.annotation_type;
  case TK_STRUCTURE:
    return ser << type_object.struct_type;
  case TK_UNION:
    return ser << type_object.union_type;
  case TK_BITSET:
    return ser << type_object.bitset_type;
  case TK_SEQUENCE:
    return ser << type_object.sequence_type;
  case TK_ARRAY:
    return ser << type_object.array_type;
  case TK_MAP:
    return ser << type_object.map_type;
  case TK_ENUM:
    return ser << type_object.enumerated_type;
  case TK_BITMASK:
    return ser << type_object.bitmask_type;
  default:
    return ser << type_object.extended_type;
  }
}

bool operator>>(Serializer& ser, XTypes::MinimalTypeObject& type_object)
{
  using namespace XTypes;
  if (!(ser >> ACE_InputCDR::to_octet(type_object.kind))) {
    return false;
  }

  switch (type_object.kind) {
  case TK_ALIAS:
    return ser >> type_object.alias_type;
  case TK_ANNOTATION:
    return ser >> type_object.annotation_type;
  case TK_STRUCTURE:
    return ser >> type_object.struct_type;
  case TK_UNION:
    return ser >> type_object.union_type;
  case TK_BITSET:
    return ser >> type_object.bitset_type;
  case TK_SEQUENCE:
    return ser >> type_object.sequence_type;
  case TK_ARRAY:
    return ser >> type_object.array_type;
  case TK_MAP:
    return ser >> type_object.map_type;
  case TK_ENUM:
    return ser >> type_object.enumerated_type;
  case TK_BITMASK:
    return ser >> type_object.bitmask_type;
  default:
    return ser >> type_object.extended_type;
  }
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::TypeObject& type_object)
{
  using namespace XTypes;
  primitive_serialized_size_ulong(encoding, size); // DHEADER
  primitive_serialized_size_octet(encoding, size); // discriminator

  switch (type_object.kind) {
  case EK_COMPLETE:
    return serialized_size(encoding, size, type_object.complete);
  case EK_MINIMAL:
    return serialized_size(encoding, size, type_object.minimal);
  }
}

bool operator<<(Serializer& ser, const XTypes::TypeObject& type_object)
{
  size_t total_size = 0;
  serialized_size(ser.encoding(), total_size, type_object);
  if (!ser.write_delimiter(total_size)) {
    return false;
  }

  if (!(ser << ACE_OutputCDR::from_octet(type_object.kind))) {
    return false;
  }

  switch (type_object.kind) {
  case XTypes::EK_COMPLETE:
    return ser << type_object.complete;
  case XTypes::EK_MINIMAL:
    return ser << type_object.minimal;
  }

  return true;
}

bool operator>>(Serializer& ser, XTypes::TypeObject& type_object)
{
  size_t total_size = 0;
  if (!ser.read_delimiter(total_size)) {
    return false;
  }

  if (!(ser >> ACE_InputCDR::to_octet(type_object.kind))) {
    return false;
  }

  bool ret = true;
  switch (type_object.kind) {
  case XTypes::EK_COMPLETE:
    ret = ser >> type_object.complete;
    break;
  case XTypes::EK_MINIMAL:
    ret = ser >> type_object.minimal;
    break;
  }

  return ret;
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::MinimalExtendedType&)
{
  serialized_size_delimiter(encoding, size);
}

bool operator<<(Serializer& strm, const XTypes::MinimalExtendedType&)
{
  return XTypes::write_empty_xcdr2_nonfinal(strm);
}

bool operator>>(Serializer& strm, XTypes::MinimalExtendedType&)
{
  return XTypes::read_empty_xcdr2_nonfinal(strm);
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::TypeInformation& stru)
{
  size_t mutable_running_total = 0;
  serialized_size_delimiter(encoding, size);

  serialized_size_parameter_id(encoding, size, mutable_running_total);
  serialized_size(encoding, size, stru.minimal);

  serialized_size_parameter_id(encoding, size, mutable_running_total);
  serialized_size(encoding, size, stru.complete);

  serialized_size_list_end_parameter_id(encoding, size, mutable_running_total);
}

bool operator<<(Serializer& strm, const XTypes::TypeInformation& stru)
{
  const Encoding& encoding = strm.encoding();
  size_t total_size = 0;
  serialized_size(encoding, total_size, stru);
  if (!strm.write_delimiter(total_size)) {
    return false;
  }

  size_t size = 0;

  serialized_size(encoding, size, stru.minimal);
  if (!strm.write_parameter_id(4097, size)) {
    return false;
  }
  size = 0;
  if (!(strm << stru.minimal)) {
    return false;
  }

  serialized_size(encoding, size, stru.complete);
  if (!strm.write_parameter_id(4098, size)) {
    return false;
  }
  size = 0;
  if (!(strm << stru.complete)) {
    return false;
  }

  return true;
}

bool operator>>(Serializer& strm, XTypes::TypeInformation& stru)
{
  size_t total_size = 0;
  if (!strm.read_delimiter(total_size)) {
    return false;
  }

  const size_t start_pos = strm.rpos();

  unsigned member_id;
  size_t field_size;
  while (true) {

    if (strm.rpos() - start_pos >= total_size) {
      return true;
    }

    bool must_understand = false;
    if (!strm.read_parameter_id(member_id, field_size, must_understand)) {
      return false;
    }

    switch (member_id) {
    case 4097: {
      if (!(strm >> stru.minimal)) {
        return false;
      }
      break;
    }
    case 4098: {
      if (!(strm >> stru.complete)) {
        return false;
      }
      break;
    }
    default:
      if (must_understand) {
        if (DCPS_debug_level >= 8) {
          ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) unknown must_understand field(%u) in OpenDDS::XTypes::TypeInformation\n"), member_id));
        }
        return false;
      }
      strm.skip(field_size);
      break;
    }
  }
  return false;
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::TypeIdentifierTypeObjectPair& stru)
{
  serialized_size(encoding, size, stru.type_identifier);
  serialized_size(encoding, size, stru.type_object);
}

bool operator<<(Serializer& strm, const XTypes::TypeIdentifierTypeObjectPair& stru)
{
  return (strm << stru.type_identifier)
    && (strm << stru.type_object);
}

bool operator>>(Serializer& strm, XTypes::TypeIdentifierTypeObjectPair& stru)
{
  return (strm >> stru.type_identifier)
    && (strm >> stru.type_object);
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::TypeIdentifierPair& stru)
{
  serialized_size(encoding, size, stru.type_identifier1);
  serialized_size(encoding, size, stru.type_identifier2);
}

bool operator<<(Serializer& strm, const XTypes::TypeIdentifierPair& stru)
{
  return (strm << stru.type_identifier1)
    && (strm << stru.type_identifier2);
}

bool operator>>(Serializer& strm, XTypes::TypeIdentifierPair& stru)
{
  return (strm >> stru.type_identifier1)
    && (strm >> stru.type_identifier2);
}


} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
