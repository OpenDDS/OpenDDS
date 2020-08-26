/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "TypeObject.h"

#include "dds/DCPS/Message_Block_Ptr.h"
#include "dds/DCPS/Hash.h"

#include <dds/DdsDcpsCoreC.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {

using DCPS::Encoding;
using DCPS::serialized_size;
using DCPS::operator<<;

namespace XTypes {

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
  case XTypes::TI_STRING8_LARGE:
  case XTypes::TI_STRING16_LARGE:
    OPENDDS_BRANCH_ACTIVATE(StringLTypeDefn, string_ldefn);
  case XTypes::TI_PLAIN_SEQUENCE_SMALL:
    OPENDDS_BRANCH_ACTIVATE(PlainSequenceSElemDefn, seq_sdefn);
  case XTypes::TI_PLAIN_SEQUENCE_LARGE:
    OPENDDS_BRANCH_ACTIVATE(PlainSequenceLElemDefn, seq_ldefn);
  case XTypes::TI_PLAIN_ARRAY_SMALL:
    OPENDDS_BRANCH_ACTIVATE(PlainArraySElemDefn, array_sdefn);
  case XTypes::TI_PLAIN_ARRAY_LARGE:
    OPENDDS_BRANCH_ACTIVATE(PlainArrayLElemDefn, array_ldefn);
  case XTypes::TI_PLAIN_MAP_SMALL:
    OPENDDS_BRANCH_ACTIVATE(PlainMapSTypeDefn, map_sdefn);
  case XTypes::TI_PLAIN_MAP_LARGE:
    OPENDDS_BRANCH_ACTIVATE(PlainMapLTypeDefn, map_ldefn);
  case XTypes::TI_STRONGLY_CONNECTED_COMPONENT:
    OPENDDS_BRANCH_ACTIVATE(StronglyConnectedComponentId, sc_component_id);
  case XTypes::EK_COMPLETE:
  case XTypes::EK_MINIMAL:
    active_ = equivalence_hash_;
    if (other) {
      std::memcpy(equivalence_hash(), other->equivalence_hash(), sizeof(EquivalenceHash));
    }
    break;
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

void TypeIdentifier::kind(ACE_CDR::Octet k)
{
  if (kind_ != k) {
    reset();
    kind_ = k;
    activate();
  }
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
  case XTypes::TI_STRING8_LARGE:
  case XTypes::TI_STRING16_LARGE:
    OPENDDS_BRANCH_RESET(StringLTypeDefn);
  case XTypes::TI_PLAIN_SEQUENCE_SMALL:
    OPENDDS_BRANCH_RESET(PlainSequenceSElemDefn);
  case XTypes::TI_PLAIN_SEQUENCE_LARGE:
    OPENDDS_BRANCH_RESET(PlainSequenceLElemDefn);
  case XTypes::TI_PLAIN_ARRAY_SMALL:
    OPENDDS_BRANCH_RESET(PlainArraySElemDefn);
  case XTypes::TI_PLAIN_ARRAY_LARGE:
    OPENDDS_BRANCH_RESET(PlainArrayLElemDefn);
  case XTypes::TI_PLAIN_MAP_SMALL:
    OPENDDS_BRANCH_RESET(PlainMapSTypeDefn);
  case XTypes::TI_PLAIN_MAP_LARGE:
    OPENDDS_BRANCH_RESET(PlainMapLTypeDefn);
  case XTypes::TI_STRONGLY_CONNECTED_COMPONENT:
    OPENDDS_BRANCH_RESET(StronglyConnectedComponentId);
  case XTypes::EK_COMPLETE:
  case XTypes::EK_MINIMAL:
    break; // no-op, data is just an array of octets
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


TypeIdentifier makeTypeIdentifier(const TypeObject& type_object)
{
  const Encoding& encoding = get_typeobject_encoding();
  size_t size = serialized_size(encoding, type_object);
  ACE_Message_Block buff(size);
  DCPS::Serializer ser(&buff, encoding);
  ser << type_object;

  unsigned char result[16];
  DCPS::MD5Hash(result, buff.rd_ptr(), buff.length());

  // First 14 bytes of MD5 of the serialized TypeObject using XCDR
  // version 2 with Little Endian encoding
  EquivalenceHash eh;
  std::memcpy(eh, result, sizeof eh);

  if (type_object.kind == EK_MINIMAL || type_object.kind == EK_COMPLETE) {
    return TypeIdentifier::make(type_object.kind, eh);
  }

  return TypeIdentifier();
}

void serialize_type_info(const TypeInformation& type_info, DDS::OctetSeq& seq)
{
  seq.length(DCPS::serialized_size(XTypes::get_typeobject_encoding(), type_info));
  DCPS::MessageBlockHelper helper(seq);
  DCPS::Serializer serializer(helper, XTypes::get_typeobject_encoding());
  if (!(serializer << type_info)) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) serialize_type_info ")
              ACE_TEXT("serialization of type information failed.\n")));
  }
}

void deserialize_type_info(TypeInformation& type_info, const DDS::OctetSeq& seq)
{
  DCPS::MessageBlockHelper helper(seq);
  DCPS::Serializer serializer(helper, XTypes::get_typeobject_encoding());
  if (!(serializer >> type_info)) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) deserialize_type_info ")
              ACE_TEXT("deserialization of type information failed.\n")));
  }
}


} // namespace XTypes

namespace DCPS {

// Serialization support for TypeObject and its components

void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::TypeIdentifier& uni)
{
  max_serialized_size(encoding, size, ACE_OutputCDR::from_octet(uni.kind()));
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
  case XTypes::EK_MINIMAL: {
    XTypes::EquivalenceHash_forany uni_equivalence_hash(const_cast<XTypes::EquivalenceHash_slice*>(uni.equivalence_hash()));
    serialized_size(encoding, size, uni_equivalence_hash);
    break;
  }
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
  case XTypes::EK_MINIMAL: {
    XTypes::EquivalenceHash_forany uni_equivalence_hash(const_cast<XTypes::EquivalenceHash_slice*>(uni.equivalence_hash()));
    return (strm << uni_equivalence_hash);
  }
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
  case XTypes::EK_MINIMAL: {
    XTypes::EquivalenceHash_forany uni_equivalence_hash(uni.equivalence_hash());
    return (strm >> uni_equivalence_hash);
  }
  default:
    return (strm >> uni.extended_defn());
  }
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::LBoundSeq& seq)
{
  DCPS::serialized_size_ulong(encoding, size);
  if (seq.length() == 0) {
    return;
  }
  max_serialized_size(encoding, size, CORBA::ULong(), seq.length());
}

bool operator<<(Serializer& strm, const XTypes::LBoundSeq& seq)
{
  const CORBA::ULong length = seq.length();
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
  CORBA::ULong length;
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
  DCPS::serialized_size_ulong(encoding, size);
  if (seq.length() == 0) {
    return;
  }
  max_serialized_size_octet(encoding, size, seq.length());
}

bool operator<<(Serializer& strm, const XTypes::SBoundSeq& seq)
{
  const CORBA::ULong length = seq.length();
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
  CORBA::ULong length;
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
  DCPS::serialized_size_ulong(encoding, size);
  if (seq.length() == 0) {
    return;
  }
  max_serialized_size(encoding, size, CORBA::Long(), seq.length());
}

bool operator<<(Serializer& strm, const XTypes::UnionCaseLabelSeq& seq)
{
  const CORBA::ULong length = seq.length();
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
  CORBA::ULong length;
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
  const XTypes::NameHash_forany&)
{
  max_serialized_size_octet(encoding, size, 4);
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
  max_serialized_size_octet(encoding, size, 14);
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
  DCPS::serialized_size_ulong(encoding, size);
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
  serialized_size(encoding, size, stru.base_type);
  serialized_size(encoding, size, stru.detail);
}

bool operator<<(Serializer& strm, const XTypes::CompleteStructHeader& stru)
{
  return (strm << stru.base_type)
    && (strm << stru.detail);
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::MinimalStructHeader& stru)
{
  serialized_size(encoding, size, stru.base_type);
  serialized_size(encoding, size, stru.detail);
}

bool operator<<(Serializer& strm, const XTypes::MinimalStructHeader& stru)
{
  return (strm << stru.base_type)
    && (strm << stru.detail);
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::CompleteStructType& stru)
{
  max_serialized_size(encoding, size, stru.struct_flags);
  serialized_size(encoding, size, stru.header);
  serialized_size(encoding, size, stru.member_seq);
}

bool operator<<(Serializer& strm, const XTypes::CompleteStructType& stru)
{
  return (strm << stru.struct_flags)
    && (strm << stru.header)
    && (strm << stru.member_seq);
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::MinimalStructType& stru)
{
  max_serialized_size(encoding, size, stru.struct_flags);
  serialized_size(encoding, size, stru.header);
  serialized_size(encoding, size, stru.member_seq);
}

bool operator<<(Serializer& strm, const XTypes::MinimalStructType& stru)
{
  return (strm << stru.struct_flags)
    && (strm << stru.header)
    && (strm << stru.member_seq);
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::CompleteUnionType& stru)
{
  max_serialized_size(encoding, size, stru.union_flags);
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


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::MinimalUnionType& stru)
{
  max_serialized_size(encoding, size, stru.union_flags);
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


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::CompleteAnnotationType& stru)
{
  max_serialized_size(encoding, size, stru.annotation_flag);
  serialized_size(encoding, size, stru.header);
  serialized_size(encoding, size, stru.member_seq);
}

bool operator<<(Serializer& strm, const XTypes::CompleteAnnotationType& stru)
{
  return (strm << stru.annotation_flag)
    && (strm << stru.header)
    && (strm << stru.member_seq);
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::MinimalAnnotationType& stru)
{
  max_serialized_size(encoding, size, stru.annotation_flag);
  serialized_size(encoding, size, stru.header);
  serialized_size(encoding, size, stru.member_seq);
}

bool operator<<(Serializer& strm, const XTypes::MinimalAnnotationType& stru)
{
  return (strm << stru.annotation_flag)
    && (strm << stru.header)
    && (strm << stru.member_seq);
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::CompleteAliasType& stru)
{
  max_serialized_size(encoding, size, stru.alias_flags);
  serialized_size(encoding, size, stru.header);
  serialized_size(encoding, size, stru.body);
}

bool operator<<(Serializer& strm, const XTypes::CompleteAliasType& stru)
{
  return (strm << stru.alias_flags)
    && (strm << stru.header)
    && (strm << stru.body);
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::MinimalAliasType& stru)
{
  max_serialized_size(encoding, size, stru.alias_flags);
  serialized_size(encoding, size, stru.header);
  serialized_size(encoding, size, stru.body);
}

bool operator<<(Serializer& strm, const XTypes::MinimalAliasType& stru)
{
  return (strm << stru.alias_flags)
    && (strm << stru.header)
    && (strm << stru.body);
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::CompleteSequenceType& stru)
{
  max_serialized_size(encoding, size, stru.collection_flag);
  serialized_size(encoding, size, stru.header);
  serialized_size(encoding, size, stru.element);
}

bool operator<<(Serializer& strm, const XTypes::CompleteSequenceType& stru)
{
  return (strm << stru.collection_flag)
    && (strm << stru.header)
    && (strm << stru.element);
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::MinimalSequenceType& stru)
{
  max_serialized_size(encoding, size, stru.collection_flag);
  serialized_size(encoding, size, stru.header);
  serialized_size(encoding, size, stru.element);
}

bool operator<<(Serializer& strm, const XTypes::MinimalSequenceType& stru)
{
  return (strm << stru.collection_flag)
    && (strm << stru.header)
    && (strm << stru.element);
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::CompleteArrayType& stru)
{
  max_serialized_size(encoding, size, stru.collection_flag);
  serialized_size(encoding, size, stru.header);
  serialized_size(encoding, size, stru.element);
}

bool operator<<(Serializer& strm, const XTypes::CompleteArrayType& stru)
{
  return (strm << stru.collection_flag)
    && (strm << stru.header)
    && (strm << stru.element);
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::MinimalArrayType& stru)
{
  max_serialized_size(encoding, size, stru.collection_flag);
  serialized_size(encoding, size, stru.header);
  serialized_size(encoding, size, stru.element);
}

bool operator<<(Serializer& strm, const XTypes::MinimalArrayType& stru)
{
  return (strm << stru.collection_flag)
    && (strm << stru.header)
    && (strm << stru.element);
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::CompleteMapType& stru)
{
  max_serialized_size(encoding, size, stru.collection_flag);
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


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::MinimalMapType& stru)
{
  max_serialized_size(encoding, size, stru.collection_flag);
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

void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::CompleteEnumeratedHeader& stru)
{
  serialized_size(encoding, size, stru.common);
  serialized_size(encoding, size, stru.detail);
}

bool operator<<(Serializer& strm, const XTypes::CompleteEnumeratedHeader& stru)
{
  return (strm << stru.common)
    && (strm << stru.detail);
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::MinimalEnumeratedHeader& stru)
{
  serialized_size(encoding, size, stru.common);
}

bool operator<<(Serializer& strm, const XTypes::MinimalEnumeratedHeader& stru)
{
  return (strm << stru.common);
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::CompleteEnumeratedType& stru)
{
  max_serialized_size(encoding, size, stru.enum_flags);
  serialized_size(encoding, size, stru.header);
  serialized_size(encoding, size, stru.literal_seq);
}

bool operator<<(Serializer& strm, const XTypes::CompleteEnumeratedType& stru)
{
  return (strm << stru.enum_flags)
    && (strm << stru.header)
    && (strm << stru.literal_seq);
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::MinimalEnumeratedType& stru)
{
  max_serialized_size(encoding, size, stru.enum_flags);
  serialized_size(encoding, size, stru.header);
  serialized_size(encoding, size, stru.literal_seq);
}

bool operator<<(Serializer& strm, const XTypes::MinimalEnumeratedType& stru)
{
  return (strm << stru.enum_flags)
    && (strm << stru.header)
    && (strm << stru.literal_seq);
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::CompleteBitmaskType& stru)
{
  max_serialized_size(encoding, size, stru.bitmask_flags);
  serialized_size(encoding, size, stru.header);
  serialized_size(encoding, size, stru.flag_seq);
}

bool operator<<(Serializer& strm, const XTypes::CompleteBitmaskType& stru)
{
  return (strm << stru.bitmask_flags)
    && (strm << stru.header)
    && (strm << stru.flag_seq);
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::MinimalBitmaskType& stru)
{
  max_serialized_size(encoding, size, stru.bitmask_flags);
  serialized_size(encoding, size, stru.header);
  serialized_size(encoding, size, stru.flag_seq);
}

bool operator<<(Serializer& strm, const XTypes::MinimalBitmaskType& stru)
{
  return (strm << stru.bitmask_flags)
    && (strm << stru.header)
    && (strm << stru.flag_seq);
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::CompleteBitsetType& stru)
{
  max_serialized_size(encoding, size, stru.bitset_flags);
  serialized_size(encoding, size, stru.header);
  serialized_size(encoding, size, stru.field_seq);
}

bool operator<<(Serializer& strm, const XTypes::CompleteBitsetType& stru)
{
  return (strm << stru.bitset_flags)
    && (strm << stru.header)
    && (strm << stru.field_seq);
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::MinimalBitsetType& stru)
{
  max_serialized_size(encoding, size, stru.bitset_flags);
  serialized_size(encoding, size, stru.header);
  serialized_size(encoding, size, stru.field_seq);
}

bool operator<<(Serializer& strm, const XTypes::MinimalBitsetType& stru)
{
  return (strm << stru.bitset_flags)
    && (strm << stru.header)
    && (strm << stru.field_seq);
}

void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::TypeIdentifierWithSize& stru)
{
  serialized_size(encoding, size, stru.type_id);
  max_serialized_size(encoding, size, stru.typeobject_serialized_size);
}

bool operator<<(Serializer& strm, const XTypes::TypeIdentifierWithSize& stru)
{
  return (strm << stru.type_id)
    && (strm << stru.typeobject_serialized_size);
}

bool operator>>(Serializer& strm, XTypes::TypeIdentifierWithSize& stru)
{
  return (strm >> stru.type_id)
    && (strm >> stru.typeobject_serialized_size);
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::TypeIdentifierWithSizeSeq& seq)
{
  DCPS::serialized_size_ulong(encoding, size);
  for (CORBA::ULong i = 0; i < seq.length(); ++i) {
    serialized_size(encoding, size, seq[i]);
  }
}

bool operator<<(Serializer& strm, const XTypes::TypeIdentifierWithSizeSeq& seq)
{
  const CORBA::ULong length = seq.length();
  if (!(strm << length)) {
    return false;
  }
  for (CORBA::ULong i = 0; i < length; ++i) {
    if (!(strm << seq[i])) {
      return false;
    }
  }
  return true;
}

bool operator>>(Serializer& strm, XTypes::TypeIdentifierWithSizeSeq& seq)
{
  CORBA::ULong length;
  if (!(strm >> length)) {
    return false;
  }
  seq.length(length);
  for (CORBA::ULong i = 0; i < length; ++i) {
    if (!(strm >> seq[i])) {
      return false;
    }
  }
  return true;
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::TypeIdentifierWithDependencies& stru)
{
  serialized_size(encoding, size, stru.typeid_with_size);
  max_serialized_size(encoding, size, stru.dependent_typeid_count);
  serialized_size(encoding, size, stru.dependent_typeids);
}

bool operator<<(Serializer& strm, const XTypes::TypeIdentifierWithDependencies& stru)
{
  return (strm << stru.typeid_with_size)
    && (strm << stru.dependent_typeid_count)
    && (strm << stru.dependent_typeids);
}

bool operator>>(Serializer& strm, XTypes::TypeIdentifierWithDependencies& stru)
{
  return (strm >> stru.typeid_with_size)
    && (strm >> stru.dependent_typeid_count)
    && (strm >> stru.dependent_typeids);
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::AppliedAnnotation& stru)
{
  serialized_size(encoding, size, stru.annotation_typeid);
  serialized_size(encoding, size, stru.param_seq);
}

bool operator<<(Serializer& strm, const XTypes::AppliedAnnotation& stru)
{
  return (strm << stru.annotation_typeid)
    && (strm << stru.param_seq);
}

bool operator>>(Serializer& strm, XTypes::AppliedAnnotation& stru)
{
  return (strm >> stru.annotation_typeid)
    && (strm >> stru.param_seq);
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::AppliedBuiltinTypeAnnotations& stru)
{
  serialized_size(encoding, size, stru.verbatim);
}

bool operator<<(Serializer& strm, const XTypes::AppliedBuiltinTypeAnnotations& stru)
{
  return (strm << stru.verbatim);
}

bool operator>>(Serializer& strm, XTypes::AppliedBuiltinTypeAnnotations& stru)
{
  return (strm >> stru.verbatim);
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::CompleteAliasBody& stru)
{
  serialized_size(encoding, size, stru.common);
  serialized_size(encoding, size, stru.ann_builtin);
  serialized_size(encoding, size, stru.ann_custom);
}

bool operator<<(Serializer& strm, const XTypes::CompleteAliasBody& stru)
{
  return (strm << stru.common)
    && (strm << stru.ann_builtin)
    && (strm << stru.ann_custom);
}

bool operator>>(Serializer& strm, XTypes::CompleteAliasBody& stru)
{
  return (strm >> stru.common)
    && (strm >> stru.ann_builtin)
    && (strm >> stru.ann_custom);
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::CompleteAliasHeader& stru)
{
  serialized_size(encoding, size, stru.detail);
}

bool operator<<(Serializer& strm, const XTypes::CompleteAliasHeader& stru)
{
  return (strm << stru.detail);
}

bool operator>>(Serializer& strm, XTypes::CompleteAliasHeader& stru)
{
  return (strm >> stru.detail);
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::CompleteAnnotationHeader& stru)
{
  DCPS::serialized_size_ulong(encoding, size);
  size += ACE_OS::strlen(stru.annotation_name.c_str()) + 1;
}

bool operator<<(Serializer& strm, const XTypes::CompleteAnnotationHeader& stru)
{
  return (strm << Serializer::FromBoundedString<char>(stru.annotation_name, 256));
}

bool operator>>(Serializer& strm, XTypes::CompleteAnnotationHeader& stru)
{
  return (strm >> Serializer::ToBoundedString<char>(stru.annotation_name, 256));
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::CompleteAnnotationParameter& stru)
{
  serialized_size(encoding, size, stru.common);
  serialized_size_ulong(encoding, size);
  size += ACE_OS::strlen(stru.name.c_str()) + 1;
  serialized_size(encoding, size, stru.default_value);
}

bool operator<<(Serializer& strm, const XTypes::CompleteAnnotationParameter& stru)
{
  return (strm << stru.common)
    && (strm << Serializer::FromBoundedString<char>(stru.name, 256))
    && (strm << stru.default_value);
}

bool operator>>(Serializer& strm, XTypes::CompleteAnnotationParameter& stru)
{
  return (strm >> stru.common)
    && (strm >> Serializer::ToBoundedString<char>(stru.name, 256))
    && (strm >> stru.default_value);
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::CompleteArrayHeader& stru)
{
  serialized_size(encoding, size, stru.common);
  serialized_size(encoding, size, stru.detail);
}

bool operator<<(Serializer& strm, const XTypes::CompleteArrayHeader& stru)
{
  return (strm << stru.common)
    && (strm << stru.detail);
}

bool operator>>(Serializer& strm, XTypes::CompleteArrayHeader& stru)
{
  return (strm >> stru.common)
    && (strm >> stru.detail);
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::CompleteBitfield& stru)
{
  serialized_size(encoding, size, stru.common);
  serialized_size(encoding, size, stru.detail);
}

bool operator<<(Serializer& strm, const XTypes::CompleteBitfield& stru)
{
  return (strm << stru.common)
    && (strm << stru.detail);
}

bool operator>>(Serializer& strm, XTypes::CompleteBitfield& stru)
{
  return (strm >> stru.common)
    && (strm >> stru.detail);
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::CompleteBitflag& stru)
{
  serialized_size(encoding, size, stru.common);
  serialized_size(encoding, size, stru.detail);
}

bool operator<<(Serializer& strm, const XTypes::CompleteBitflag& stru)
{
  return (strm << stru.common)
    && (strm << stru.detail);
}

bool operator>>(Serializer& strm, XTypes::CompleteBitflag& stru)
{
  return (strm << stru.common)
    && (strm << stru.detail);
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::CompleteBitsetHeader& stru)
{
  serialized_size(encoding, size, stru.detail);
}

bool operator<<(Serializer& strm, const XTypes::CompleteBitsetHeader& stru)
{
  return (strm << stru.detail);
}

bool operator>>(Serializer& strm, XTypes::CompleteBitsetHeader& stru)
{
  return (strm >> stru.detail);
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::CompleteCollectionElement& stru)
{
  serialized_size(encoding, size, stru.common);
  serialized_size(encoding, size, stru.detail);
}

bool operator<<(Serializer& strm, const XTypes::CompleteCollectionElement& stru)
{
  return (strm << stru.common)
    && (strm << stru.detail);
}

bool operator>>(Serializer& strm, XTypes::CompleteCollectionElement& stru)
{
  return (strm >> stru.common)
    && (strm >> stru.detail);
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::CompleteCollectionHeader& stru)
{
  serialized_size(encoding, size, stru.common);
  serialized_size(encoding, size, stru.detail);
}

bool operator<<(Serializer& strm, const XTypes::CompleteCollectionHeader& stru)
{
  return (strm << stru.common)
    && (strm << stru.detail);
}

bool operator>>(Serializer& strm, XTypes::CompleteCollectionHeader& stru)
{
  return (strm >> stru.common)
    && (strm >> stru.detail);
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::CompleteDiscriminatorMember& stru)
{
  serialized_size(encoding, size, stru.common);
  serialized_size(encoding, size, stru.ann_builtin);
  serialized_size(encoding, size, stru.ann_custom);
}

bool operator<<(Serializer& strm, const XTypes::CompleteDiscriminatorMember& stru)
{
  return (strm << stru.common)
    && (strm << stru.ann_builtin)
    && (strm << stru.ann_custom);
}

bool operator>>(Serializer& strm, XTypes::CompleteDiscriminatorMember& stru)
{
  return (strm >> stru.common)
    && (strm >> stru.ann_builtin)
    && (strm >> stru.ann_custom);
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::CompleteEnumeratedLiteral& stru)
{
  serialized_size(encoding, size, stru.common);
  serialized_size(encoding, size, stru.detail);
}

bool operator<<(Serializer& strm, const XTypes::CompleteEnumeratedLiteral& stru)
{
  return (strm << stru.common)
    && (strm << stru.detail);
}

bool operator>>(Serializer& strm, XTypes::CompleteEnumeratedLiteral& stru)
{
  return (strm >> stru.common)
    && (strm >> stru.detail);
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::CompleteStructMember& stru)
{
  serialized_size(encoding, size, stru.common);
  serialized_size(encoding, size, stru.detail);
}

bool operator<<(Serializer& strm, const XTypes::CompleteStructMember& stru)
{
  return (strm << stru.common)
    && (strm << stru.detail);
}

bool operator>>(Serializer& strm, XTypes::CompleteStructMember& stru)
{
  return (strm >> stru.common)
    && (strm >> stru.detail);
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::CompleteUnionHeader& stru)
{
  serialized_size(encoding, size, stru.detail);
}

bool operator<<(Serializer& strm, const XTypes::CompleteUnionHeader& stru)
{
  return (strm << stru.detail);
}

bool operator>>(Serializer& strm, XTypes::CompleteUnionHeader& stru)
{
  return (strm >> stru.detail);
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::CompleteUnionMember& stru)
{
  serialized_size(encoding, size, stru.common);
  serialized_size(encoding, size, stru.detail);
}

bool operator<<(Serializer& strm, const XTypes::CompleteUnionMember& stru)
{
  return (strm << stru.common)
    && (strm << stru.detail);
}

bool operator>>(Serializer& strm, XTypes::CompleteUnionMember& stru)
{
  return (strm >> stru.common)
    && (strm >> stru.detail);
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::MinimalAliasBody& stru)
{
  serialized_size(encoding, size, stru.common);
}

bool operator<<(Serializer& strm, const XTypes::MinimalAliasBody& stru)
{
  return (strm << stru.common);
}

bool operator>>(Serializer& strm, XTypes::MinimalAliasBody& stru)
{
  return (strm >> stru.common);
}


void serialized_size(const Encoding&, size_t&,
  const XTypes::MinimalAliasHeader&)
{
}

bool operator<<(Serializer&, const XTypes::MinimalAliasHeader&)
{
  return true;
}

bool operator>>(Serializer&, XTypes::MinimalAliasHeader&)
{
  return true;
}


void serialized_size(const Encoding&, size_t&,
  const XTypes::MinimalAnnotationHeader&)
{
}

bool operator<<(Serializer&, const XTypes::MinimalAnnotationHeader&)
{
  return true;
}

bool operator>>(Serializer&, XTypes::MinimalAnnotationHeader&)
{
  return true;
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::MinimalAnnotationParameter& stru)
{
  XTypes::NameHash_forany stru_name_hash(const_cast<XTypes::NameHash_slice*>(stru.name_hash));
  serialized_size(encoding, size, stru.common);
  serialized_size(encoding, size, stru_name_hash);
  serialized_size(encoding, size, stru.default_value);
}

bool operator<<(Serializer& strm, const XTypes::MinimalAnnotationParameter& stru)
{
  XTypes::NameHash_forany stru_name_hash(const_cast<XTypes::NameHash_slice*>(stru.name_hash));
  return (strm << stru.common)
    && (strm << stru_name_hash)
    && (strm << stru.default_value);
}

bool operator>>(Serializer& strm, XTypes::MinimalAnnotationParameter& stru)
{
  XTypes::NameHash_forany stru_name_hash(const_cast<XTypes::NameHash_slice*>(stru.name_hash));
  return (strm >> stru.common)
    && (strm >> stru_name_hash)
    && (strm >> stru.default_value);
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::MinimalArrayHeader& stru)
{
  serialized_size(encoding, size, stru.common);
}

bool operator<<(Serializer& strm, const XTypes::MinimalArrayHeader& stru)
{
  return (strm << stru.common);
}

bool operator>>(Serializer& strm, XTypes::MinimalArrayHeader& stru)
{
  return (strm >> stru.common);
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::MinimalBitfield& stru)
{
  XTypes::NameHash_forany stru_name_hash(const_cast<XTypes::NameHash_slice*>(stru.name_hash));
  serialized_size(encoding, size, stru.common);
  serialized_size(encoding, size, stru_name_hash);
}

bool operator<<(Serializer& strm, const XTypes::MinimalBitfield& stru)
{
  XTypes::NameHash_forany stru_name_hash(const_cast<XTypes::NameHash_slice*>(stru.name_hash));
  return (strm << stru.common)
    && (strm << stru_name_hash);
}

bool operator>>(Serializer& strm, XTypes::MinimalBitfield& stru)
{
  XTypes::NameHash_forany stru_name_hash(const_cast<XTypes::NameHash_slice*>(stru.name_hash));
  return (strm >> stru.common)
    && (strm >> stru_name_hash);
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::MinimalBitflag& stru)
{
  serialized_size(encoding, size, stru.common);
  serialized_size(encoding, size, stru.detail);
}

bool operator<<(Serializer& strm, const XTypes::MinimalBitflag& stru)
{
  return (strm << stru.common)
    && (strm << stru.detail);
}

bool operator>>(Serializer& strm, XTypes::MinimalBitflag& stru)
{
  return (strm >> stru.common)
    && (strm >> stru.detail);
}


void serialized_size(const Encoding&, size_t&,
  const XTypes::MinimalBitsetHeader&)
{
}

bool operator<<(Serializer&, const XTypes::MinimalBitsetHeader&)
{
  return true;
}

bool operator>>(Serializer&, XTypes::MinimalBitsetHeader&)
{
  return true;
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::MinimalCollectionElement& stru)
{
  serialized_size(encoding, size, stru.common);
}

bool operator<<(Serializer& strm, const XTypes::MinimalCollectionElement& stru)
{
  return (strm << stru.common);
}

bool operator>>(Serializer& strm, XTypes::MinimalCollectionElement& stru)
{
  return (strm >> stru.common);
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::MinimalCollectionHeader& stru)
{
  serialized_size(encoding, size, stru.common);
}

bool operator<<(Serializer& strm, const XTypes::MinimalCollectionHeader& stru)
{
  return (strm << stru.common);
}

bool operator>>(Serializer& strm, XTypes::MinimalCollectionHeader& stru)
{
  return (strm >> stru.common);
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::MinimalDiscriminatorMember& stru)
{
  serialized_size(encoding, size, stru.common);
}

bool operator<<(Serializer& strm, const XTypes::MinimalDiscriminatorMember& stru)
{
  return (strm << stru.common);
}

bool operator>>(Serializer& strm, XTypes::MinimalDiscriminatorMember& stru)
{
  return (strm >> stru.common);
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::MinimalEnumeratedLiteral& stru)
{
  serialized_size(encoding, size, stru.common);
  serialized_size(encoding, size, stru.detail);
}

bool operator<<(Serializer& strm, const XTypes::MinimalEnumeratedLiteral& stru)
{
  return (strm << stru.common)
    && (strm << stru.detail);
}

bool operator>>(Serializer& strm, XTypes::MinimalEnumeratedLiteral& stru)
{
  return (strm >> stru.common)
    && (strm >> stru.detail);
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::MinimalStructMember& stru)
{
  serialized_size(encoding, size, stru.common);
  serialized_size(encoding, size, stru.detail);
}

bool operator<<(Serializer& strm, const XTypes::MinimalStructMember& stru)
{
  return (strm << stru.common)
    && (strm << stru.detail);
}

bool operator>>(Serializer& strm, XTypes::MinimalStructMember& stru)
{
  return (strm >> stru.common)
    && (strm >> stru.detail);
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::MinimalUnionHeader& stru)
{
  serialized_size(encoding, size, stru.detail);
}

bool operator<<(Serializer& strm, const XTypes::MinimalUnionHeader& stru)
{
  return (strm << stru.detail);
}

bool operator>>(Serializer& strm, XTypes::MinimalUnionHeader& stru)
{
  return (strm >> stru.detail);
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::MinimalUnionMember& stru)
{
  serialized_size(encoding, size, stru.common);
  serialized_size(encoding, size, stru.detail);
}

bool operator<<(Serializer& strm, const XTypes::MinimalUnionMember& stru)
{
  return (strm << stru.common)
    && (strm << stru.detail);
}

bool operator>>(Serializer& strm, XTypes::MinimalUnionMember& stru)
{
  return (strm >> stru.common)
    && (strm >> stru.detail);
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::AnnotationParameterValue& uni)
{
  max_serialized_size(encoding, size, ACE_OutputCDR::from_octet(uni.kind));
  switch (uni.kind) {
  case XTypes::TK_BOOLEAN: {
    max_serialized_size(encoding, size, ACE_OutputCDR::from_boolean(uni.boolean_value));
    break;
  }
  case XTypes::TK_BYTE: {
    max_serialized_size(encoding, size, ACE_OutputCDR::from_octet(uni.byte_value));
    break;
  }
  case XTypes::TK_INT16: {
    max_serialized_size(encoding, size, uni.int16_value);
    break;
  }
  case XTypes::TK_UINT16: {
    max_serialized_size(encoding, size, uni.uint_16_value);
    break;
  }
  case XTypes::TK_INT32: {
    max_serialized_size(encoding, size, uni.int32_value);
    break;
  }
  case XTypes::TK_UINT32: {
    max_serialized_size(encoding, size, uni.uint32_value);
    break;
  }
  case XTypes::TK_INT64: {
    max_serialized_size(encoding, size, uni.int64_value);
    break;
  }
  case XTypes::TK_UINT64: {
    max_serialized_size(encoding, size, uni.uint64_value);
    break;
  }
  case XTypes::TK_FLOAT32: {
    max_serialized_size(encoding, size, uni.float32_value);
    break;
  }
  case XTypes::TK_FLOAT64: {
    max_serialized_size(encoding, size, uni.float64_value);
    break;
  }
  case XTypes::TK_FLOAT128: {
    max_serialized_size(encoding, size, ACE_CDR::LongDouble());
    break;
  }
  case XTypes::TK_CHAR8: {
    max_serialized_size(encoding, size, ACE_OutputCDR::from_char(uni.char_value));
    break;
  }
  case XTypes::TK_CHAR16: {
    max_serialized_size(encoding, size, ACE_OutputCDR::from_wchar(uni.wchar_value));
    break;
  }
  case XTypes::TK_ENUM: {
    max_serialized_size(encoding, size, uni.enumerated_value);
    break;
  }
  case XTypes::TK_STRING8: {
    DCPS::serialized_size_ulong(encoding, size);
    size += ACE_OS::strlen(uni.string8_value.c_str()) + 1;
    break;
  }
  case XTypes::TK_STRING16: {
#ifdef DDS_HAS_WCHAR
    DCPS::serialized_size_ulong(encoding, size);
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
    return (strm << uni.uint_16_value);
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
    CORBA::Boolean tmp;
    if (strm >> ACE_InputCDR::to_boolean(tmp)) {
      uni.boolean_value = tmp;
      uni.kind = kind;
      return true;
    }
    return false;
  }
  case XTypes::TK_BYTE: {
    CORBA::Octet tmp;
    if (strm >> ACE_InputCDR::to_octet(tmp)) {
      uni.byte_value = tmp;
      uni.kind = kind;
      return true;
    }
    return false;
  }
  case XTypes::TK_INT16: {
    CORBA::Short tmp;
    if (strm >> tmp) {
      uni.int16_value = tmp;
      uni.kind = kind;
      return true;
    }
    return false;
  }
  case XTypes::TK_UINT16: {
    CORBA::UShort tmp;
    if (strm >> tmp) {
      uni.uint_16_value = tmp;
      uni.kind = kind;
      return true;
    }
    return false;
  }
  case XTypes::TK_INT32: {
    CORBA::Long tmp;
    if (strm >> tmp) {
      uni.int32_value = tmp;
      uni.kind = kind;
      return true;
    }
    return false;
  }
  case XTypes::TK_UINT32: {
    CORBA::ULong tmp;
    if (strm >> tmp) {
      uni.uint32_value = tmp;
      uni.kind = kind;
      return true;
    }
    return false;
  }
  case XTypes::TK_INT64: {
    CORBA::LongLong tmp;
    if (strm >> tmp) {
      uni.int64_value = tmp;
      uni.kind = kind;
      return true;
    }
    return false;
  }
  case XTypes::TK_UINT64: {
    CORBA::ULongLong tmp;
    if (strm >> tmp) {
      uni.uint64_value = tmp;
      uni.kind = kind;
      return true;
    }
    return false;
  }
  case XTypes::TK_FLOAT32: {
    CORBA::Float tmp;
    if (strm >> tmp) {
      uni.float32_value = tmp;
      uni.kind = kind;
      return true;
    }
    return false;
  }
  case XTypes::TK_FLOAT64: {
    CORBA::Double tmp;
    if (strm >> tmp) {
      uni.float64_value = tmp;
      uni.kind = kind;
      return true;
    }
    return false;
  }
  case XTypes::TK_FLOAT128: {
    CORBA::LongDouble tmp;
    if (strm >> tmp) {
      uni.float128_value = tmp;
      uni.kind = kind;
      return true;
    }
    return false;
  }
  case XTypes::TK_CHAR8: {
    CORBA::Char tmp;
    if (strm >> ACE_InputCDR::to_char(tmp)) {
      uni.char_value = tmp;
      uni.kind = kind;
      return true;
    }
    return false;
  }
  case XTypes::TK_CHAR16: {
    CORBA::WChar tmp;
    if (strm >> ACE_InputCDR::to_wchar(tmp)) {
      uni.wchar_value = tmp;
      uni.kind = kind;
      return true;
    }
    return false;
  }
  case XTypes::TK_ENUM: {
    CORBA::Long tmp;
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
  XTypes::NameHash_forany stru_paramname_hash(const_cast<XTypes::NameHash_slice*>(stru.paramname_hash));
  serialized_size(encoding, size, stru_paramname_hash);
  serialized_size(encoding, size, stru.value);
}

bool operator<<(Serializer& strm, const XTypes::AppliedAnnotationParameter& stru)
{
  XTypes::NameHash_forany stru_paramname_hash(const_cast<XTypes::NameHash_slice*>(stru.paramname_hash));
  return (strm << stru_paramname_hash)
    && (strm << stru.value);
}

bool operator>>(Serializer& strm, XTypes::AppliedAnnotationParameter& stru)
{
  XTypes::NameHash_forany stru_paramname_hash(const_cast<XTypes::NameHash_slice*>(stru.paramname_hash));
  return (strm >> stru_paramname_hash)
    && (strm >> stru.value);
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::AppliedBuiltinMemberAnnotations& stru)
{
  size += DCPS::boolean_cdr_size;
  if (stru.unit.present) {
    DCPS::serialized_size_ulong(encoding, size);
    size += ACE_OS::strlen(stru.unit.value.c_str()) + 1;
  }

  serialized_size(encoding, size, stru.min);
  serialized_size(encoding, size, stru.max);

  size += DCPS::boolean_cdr_size;
  if (stru.hash_id.present) {
    DCPS::serialized_size_ulong(encoding, size);
    size += ACE_OS::strlen(stru.hash_id.value.c_str()) + 1;
  }
}

bool operator<<(Serializer& strm, const XTypes::AppliedBuiltinMemberAnnotations& stru)
{
  return (strm << stru.unit)
    && (strm << stru.min)
    && (strm << stru.max)
    && (strm << stru.hash_id);
}

bool operator>>(Serializer& strm, XTypes::AppliedBuiltinMemberAnnotations& stru)
{
  return (strm >> stru.unit)
    && (strm >> stru.min)
    && (strm >> stru.max)
    && (strm >> stru.hash_id);
}

void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::AppliedVerbatimAnnotation& stru)
{
  DCPS::serialized_size_ulong(encoding, size);
  size += ACE_OS::strlen(stru.placement.c_str()) + 1;
  DCPS::serialized_size_ulong(encoding, size);
  size += ACE_OS::strlen(stru.language.c_str()) + 1;
  DCPS::serialized_size_ulong(encoding, size);
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
  max_serialized_size(encoding, size, stru.related_flags);
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
  max_serialized_size(encoding, size, stru.member_flags);
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
  max_serialized_size(encoding, size, stru.position);
  max_serialized_size(encoding, size, stru.flags);
  max_serialized_size(encoding, size, ACE_OutputCDR::from_octet(stru.bitcount));
  max_serialized_size(encoding, size, ACE_OutputCDR::from_octet(stru.holder_type));
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
  max_serialized_size(encoding, size, stru.position);
  max_serialized_size(encoding, size, stru.flags);
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
  max_serialized_size(encoding, size, stru.element_flags);
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
  max_serialized_size(encoding, size, stru.bound);
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
  max_serialized_size(encoding, size, stru.member_flags);
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
  max_serialized_size(encoding, size, stru.bit_bound);
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
  max_serialized_size(encoding, size, stru.value);
  max_serialized_size(encoding, size, stru.flags);
}

bool operator<<(Serializer& strm, const XTypes::CommonEnumeratedLiteral& stru)
{
  return (strm << stru.value)
    && (strm << stru.flags);
}

bool operator>>(Serializer& strm, XTypes::CommonEnumeratedLiteral& stru)
{
  return (strm >> stru.value)
    && (strm >> stru.flags);
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::CommonStructMember& stru)
{
  max_serialized_size(encoding, size, stru.member_id);
  max_serialized_size(encoding, size, stru.member_flags);
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
  max_serialized_size(encoding, size, stru.member_id);
  max_serialized_size(encoding, size, stru.member_flags);
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
  DCPS::serialized_size_ulong(encoding, size);
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
  max_serialized_size(encoding, size, stru.bound);
  serialized_size(encoding, size, *stru.element_identifier);
  max_serialized_size(encoding, size, stru.key_flags);
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
  max_serialized_size(encoding, size, ACE_OutputCDR::from_octet(stru.bound));
  serialized_size(encoding, size, *stru.element_identifier);
  max_serialized_size(encoding, size, stru.key_flags);
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
  max_serialized_size(encoding, size, stru.bound);
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
  max_serialized_size(encoding, size, ACE_OutputCDR::from_octet(stru.bound));
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
  max_serialized_size(encoding, size, stru.bound);
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
  max_serialized_size(encoding, size, ACE_OutputCDR::from_octet(stru.bound));
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
  serialized_size(encoding, size, stru.sc_component_id);
  max_serialized_size(encoding, size, stru.scc_length);
  max_serialized_size(encoding, size, stru.scc_index);
}

bool operator<<(Serializer& strm, const XTypes::StronglyConnectedComponentId& stru)
{
  return (strm << stru.sc_component_id)
    && (strm << stru.scc_length)
    && (strm << stru.scc_index);
}

bool operator>>(Serializer& strm, XTypes::StronglyConnectedComponentId& stru)
{
  return (strm >> stru.sc_component_id)
    && (strm >> stru.scc_length)
    && (strm >> stru.scc_index);
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::PlainCollectionHeader& stru)
{
  max_serialized_size(encoding, size, ACE_OutputCDR::from_octet(stru.equiv_kind));
  max_serialized_size(encoding, size, stru.element_flags);
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
  max_serialized_size(encoding, size, ACE_OutputCDR::from_octet(uni.kind));
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
  max_serialized_size_octet(encoding, size); // discriminator

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

void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::MinimalTypeObject& type_object)
{
  using namespace XTypes;
  max_serialized_size_octet(encoding, size); // discriminator

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

void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::TypeObject& type_object)
{
  using namespace XTypes;
  max_serialized_size_ulong(encoding, size); // DHEADER
  max_serialized_size_octet(encoding, size); // discriminator

  switch (type_object.kind) {
  case EK_COMPLETE:
    return serialized_size(encoding, size, type_object.complete);
  case EK_MINIMAL:
    return serialized_size(encoding, size, type_object.minimal);
  }
}

bool operator<<(Serializer& ser, const XTypes::TypeObject& type_object)
{
  using namespace XTypes;
  // XCDR2 Appendable Union: DELIMITED_CDR (7.4.2) = DHEADER + PLAIN_CDR2
  // DHEADER = UInt32 size of object that follows
  // subtracting the DHEADER's own size doesn't impact alignment since the
  // maximum alignment in PLAIN_CDR2 is 4
  size_t object_size = serialized_size(ser.encoding(), type_object);
  size_t dheader_size = 0;
  max_serialized_size_ulong(ser.encoding(), dheader_size);
  if (!ser.write_delimiter(object_size - dheader_size)) {
    return false;
  }

  if (!(ser << ACE_OutputCDR::from_octet(type_object.kind))) {
    return false;
  }

  switch (type_object.kind) {
  case EK_COMPLETE:
    return ser << type_object.complete;
  case EK_MINIMAL:
    return ser << type_object.minimal;
  }

  return true;
}

bool operator>>(Serializer& ser, XTypes::TypeObject& type_object)
{
  // TODO: needs correct implementation
  using namespace XTypes;
  if (!(ser << ACE_OutputCDR::from_octet(type_object.kind))) {
    return false;
  }

  switch (type_object.kind) {
  case EK_COMPLETE:
    return ser << type_object.complete;
  case EK_MINIMAL:
    return ser << type_object.minimal;
  }

  return true;
}

void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::TypeInformation& stru)
{
  serialized_size(encoding, size, stru.minimal);
  serialized_size(encoding, size, stru.complete);
}

bool operator<<(Serializer& strm, const XTypes::TypeInformation& stru)
{
  return (strm << stru.minimal)
    && (strm << stru.complete);
}

bool operator>>(Serializer& strm, XTypes::TypeInformation& stru)
{
  return (strm >> stru.minimal)
    && (strm >> stru.complete);
}

void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::TypeIdentifierTypeObjectPair& stru)
{
  // TODO: needs correct implementation
  serialized_size(encoding, size, stru.type_identifier);
  serialized_size(encoding, size, stru.type_object);
}

bool operator<<(Serializer& strm, const XTypes::TypeIdentifierTypeObjectPair& stru)
{
  // TODO: needs correct implementation
  return (strm << stru.type_identifier)
    && (strm << stru.type_object);
}

bool operator>>(Serializer& strm, XTypes::TypeIdentifierTypeObjectPair& stru)
{
  // TODO: needs correct implementation
  return (strm >> stru.type_identifier)
    && (strm >> stru.type_object);
}

void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::TypeIdentifierPair& stru)
{
  // TODO: needs correct implementation
  serialized_size(encoding, size, stru.type_identifier1);
  serialized_size(encoding, size, stru.type_identifier2);
}

bool operator<<(Serializer& strm, const XTypes::TypeIdentifierPair& stru)
{
  // TODO: needs correct implementation
  return (strm << stru.type_identifier1)
    && (strm << stru.type_identifier2);
}

bool operator>>(Serializer& strm, XTypes::TypeIdentifierPair& stru)
{
  // TODO: needs correct implementation
  return (strm >> stru.type_identifier1)
    && (strm >> stru.type_identifier2);
}

void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::TypeIdentifierPairSeq& seq)
{
  // TODO: needs correct implementation
  DCPS::serialized_size_ulong(encoding, size);
  for (CORBA::ULong i = 0; i < seq.length(); ++i) {
    serialized_size(encoding, size, seq[i]);
  }
}

bool operator<<(Serializer& strm, const XTypes::TypeIdentifierPairSeq& seq)
{
  // TODO: needs correct implementation
  const CORBA::ULong length = seq.length();
  if (!(strm << length)) {
    return false;
  }
  for (CORBA::ULong i = 0; i < length; ++i) {
    if (!(strm << seq[i])) {
      return false;
    }
  }
  return true;
}

bool operator>>(Serializer& strm, XTypes::TypeIdentifierPairSeq& seq)
{
  // TODO: needs correct implementation
  CORBA::ULong length;
  if (!(strm >> length)) {
    return false;
  }
  seq.length(length);
  for (CORBA::ULong i = 0; i < length; ++i) {
    if (!(strm >> seq[i])) {
      return false;
    }
  }
  return true;
}


template<>
XTypes::TypeIdentifier getMinimalTypeIdentifier<void>()
{
  static const XTypes::TypeIdentifier ti;
  return ti;
}

template<>
XTypes::TypeIdentifier getMinimalTypeIdentifier<ACE_CDR::Boolean>()
{
  static const XTypes::TypeIdentifier ti(XTypes::TK_BOOLEAN);
  return ti;
}

template<>
XTypes::TypeIdentifier getMinimalTypeIdentifier<ACE_CDR::Octet>()
{
  static const XTypes::TypeIdentifier ti(XTypes::TK_BYTE);
  return ti;
}

template<>
XTypes::TypeIdentifier getMinimalTypeIdentifier<ACE_CDR::Short>()
{
  static const XTypes::TypeIdentifier ti(XTypes::TK_INT16);
  return ti;
}

template<>
XTypes::TypeIdentifier getMinimalTypeIdentifier<ACE_CDR::Long>()
{
  static const XTypes::TypeIdentifier ti(XTypes::TK_INT32);
  return ti;
}

template<>
XTypes::TypeIdentifier getMinimalTypeIdentifier<ACE_CDR::LongLong>()
{
  static const XTypes::TypeIdentifier ti(XTypes::TK_INT64);
  return ti;
}

template<>
XTypes::TypeIdentifier getMinimalTypeIdentifier<ACE_CDR::UShort>()
{
  static const XTypes::TypeIdentifier ti(XTypes::TK_UINT16);
  return ti;
}

template<>
XTypes::TypeIdentifier getMinimalTypeIdentifier<ACE_CDR::ULong>()
{
  static const XTypes::TypeIdentifier ti(XTypes::TK_UINT32);
  return ti;
}

template<>
XTypes::TypeIdentifier getMinimalTypeIdentifier<ACE_CDR::ULongLong>()
{
  static const XTypes::TypeIdentifier ti(XTypes::TK_UINT64);
  return ti;
}

template<>
XTypes::TypeIdentifier getMinimalTypeIdentifier<ACE_CDR::Float>()
{
  static const XTypes::TypeIdentifier ti(XTypes::TK_FLOAT32);
  return ti;
}

template<>
XTypes::TypeIdentifier getMinimalTypeIdentifier<ACE_CDR::Double>()
{
  static const XTypes::TypeIdentifier ti(XTypes::TK_FLOAT64);
  return ti;
}

template<>
XTypes::TypeIdentifier getMinimalTypeIdentifier<ACE_CDR::LongDouble>()
{
  static const XTypes::TypeIdentifier ti(XTypes::TK_FLOAT128);
  return ti;
}

template<>
XTypes::TypeIdentifier getMinimalTypeIdentifier<ACE_CDR::Char>()
{
  static const XTypes::TypeIdentifier ti(XTypes::TK_CHAR8);
  return ti;
}

template<>
XTypes::TypeIdentifier getMinimalTypeIdentifier<ACE_OutputCDR::from_wchar>()
{
  static const XTypes::TypeIdentifier ti(XTypes::TK_CHAR16);
  return ti;
}

template<>
XTypes::TypeIdentifier getMinimalTypeIdentifier<ACE_CDR::Char*>()
{
  static const XTypes::TypeIdentifier ti = XTypes::TypeIdentifier::makeString(false, XTypes::StringSTypeDefn(XTypes::INVALID_SBOUND));
  return ti;
}

template<>
XTypes::TypeIdentifier getMinimalTypeIdentifier<ACE_CDR::WChar*>()
{
  static const XTypes::TypeIdentifier ti = XTypes::TypeIdentifier::makeString(true, XTypes::StringSTypeDefn(XTypes::INVALID_SBOUND));
  return ti;
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
