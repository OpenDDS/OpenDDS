/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/

#include "Hash.h"
#include "TypeObject.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace XTypes {

MinimalMemberDetail::MinimalMemberDetail(const std::string& name)
{
  unsigned char result[16];
  DCPS::MD5Hash(result, name.c_str(), name.size());

  std::memcpy(name_hash, result, sizeof name_hash);
}


TypeIdentifierPtr makeTypeIdentifier(const TypeObject& type_object)
{
  ACE_Message_Block buff(find_size(type_object));
  DCPS::Serializer ser(&buff, DCPS::Encoding::KIND_XCDR2_PLAIN, DCPS::ENDIAN_LITTLE);
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

  return TypeIdentifierPtr();
}


// Serialization support for TypeObject and its components

size_t find_size(const TypeIdentifier& stru, size_t& size)
{
  return size;
}

bool operator<<(DCPS::Serializer& ser, const TypeIdentifier& stru)
{
  return true;
}


size_t find_size(const CompleteStructType& stru, size_t& size)
{
  return size;
}

bool operator<<(DCPS::Serializer& ser, const CompleteStructType& stru)
{
  return true;
}


size_t find_size(const MinimalStructType& stru, size_t& size)
{
  return size;
}

bool operator<<(DCPS::Serializer& ser, const MinimalStructType& stru)
{
  return true;
}


size_t find_size(const CompleteUnionType& stru, size_t& size)
{
  return size;
}

bool operator<<(DCPS::Serializer& ser, const CompleteUnionType& stru)
{
  return true;
}


size_t find_size(const MinimalUnionType& stru, size_t& size)
{
  return size;
}

bool operator<<(DCPS::Serializer& ser, const MinimalUnionType& stru)
{
  return true;
}


size_t find_size(const CompleteAnnotationType& stru, size_t& size)
{
  return size;
}

bool operator<<(DCPS::Serializer& ser, const CompleteAnnotationType& stru)
{
  return true;
}


size_t find_size(const MinimalAnnotationType& stru, size_t& size)
{
  return size;
}

bool operator<<(DCPS::Serializer& ser, const MinimalAnnotationType& stru)
{
  return true;
}


size_t find_size(const CompleteAliasType& stru, size_t& size)
{
  return size;
}

bool operator<<(DCPS::Serializer& ser, const CompleteAliasType& stru)
{
  return true;
}


size_t find_size(const MinimalAliasType& stru, size_t& size)
{
  return size;
}

bool operator<<(DCPS::Serializer& ser, const MinimalAliasType& stru)
{
  return true;
}


size_t find_size(const CompleteSequenceType& stru, size_t& size)
{
  return size;
}

bool operator<<(DCPS::Serializer& ser, const CompleteSequenceType& stru)
{
  return true;
}


size_t find_size(const MinimalSequenceType& stru, size_t& size)
{
  return size;
}

bool operator<<(DCPS::Serializer& ser, const MinimalSequenceType& stru)
{
  return true;
}


size_t find_size(const CompleteArrayType& stru, size_t& size)
{
  return size;
}

bool operator<<(DCPS::Serializer& ser, const CompleteArrayType& stru)
{
  return true;
}


size_t find_size(const MinimalArrayType& stru, size_t& size)
{
  return size;
}

bool operator<<(DCPS::Serializer& ser, const MinimalArrayType& stru)
{
  return true;
}


size_t find_size(const CompleteMapType& stru, size_t& size)
{
  return size;
}

bool operator<<(DCPS::Serializer& ser, const CompleteMapType& stru)
{
  return true;
}


size_t find_size(const MinimalMapType& stru, size_t& size)
{
  return size;
}

bool operator<<(DCPS::Serializer& ser, const MinimalMapType& stru)
{
  return true;
}


size_t find_size(const CompleteEnumeratedType& stru, size_t& size)
{
  return size;
}

bool operator<<(DCPS::Serializer& ser, const CompleteEnumeratedType& stru)
{
  return true;
}


size_t find_size(const MinimalEnumeratedType& stru, size_t& size)
{
  return size;
}

bool operator<<(DCPS::Serializer& ser, const MinimalEnumeratedType& stru)
{
  return true;
}


size_t find_size(const CompleteBitmaskType& stru, size_t& size)
{
  return size;
}

bool operator<<(DCPS::Serializer& ser, const CompleteBitmaskType& stru)
{
  return true;
}


size_t find_size(const MinimalBitmaskType& stru, size_t& size)
{
  return size;
}

bool operator<<(DCPS::Serializer& ser, const MinimalBitmaskType& stru)
{
  return true;
}


size_t find_size(const CompleteBitsetType& stru, size_t& size)
{
  return size;
}

bool operator<<(DCPS::Serializer& ser, const CompleteBitsetType& stru)
{
  return true;
}


size_t find_size(const MinimalBitsetType& stru, size_t& size)
{
  return size;
}

bool operator<<(DCPS::Serializer& ser, const MinimalBitsetType& stru)
{
  return true;
}

size_t find_size(const TypeIdentifierWithSize& stru, size_t& size)
{
  return size;
}

bool operator<<(DCPS::Serializer& ser, const TypeIdentifierWithSize& stru)
{
  return true;
}

size_t find_size(const TypeIdentifierWithSizeSeq& stru, size_t& size)
{
  return size;
}

bool operator<<(DCPS::Serializer& ser, const TypeIdentifierWithSizeSeq& stru)
{
  return true;
}

size_t find_size(const TypeIdentifierWithDependencies& stru, size_t& size)
{
  return size;
}

bool operator<<(DCPS::Serializer& ser, const TypeIdentifierWithDependencies& stru)
{
  return true;
}


size_t find_size(const CompleteTypeObject& type_object, size_t& size)
{
  size += DCPS::max_marshaled_size_octet(); // discriminator

  switch (type_object.kind) {
  case TK_ALIAS:
    return find_size(type_object.alias_type, size);
  case TK_ANNOTATION:
    return find_size(type_object.annotation_type, size);
  case TK_STRUCTURE:
    return find_size(type_object.struct_type, size);
  case TK_UNION:
    return find_size(type_object.union_type, size);
  case TK_BITSET:
    return find_size(type_object.bitset_type, size);
  case TK_SEQUENCE:
    return find_size(type_object.sequence_type, size);
  case TK_ARRAY:
    return find_size(type_object.array_type, size);
  case TK_MAP:
    return find_size(type_object.map_type, size);
  case TK_ENUM:
    return find_size(type_object.enumerated_type, size);
  case TK_BITMASK:
    return find_size(type_object.bitmask_type, size);
  default:
    return find_size(type_object.extended_type, size);
  }
}

bool operator<<(DCPS::Serializer& ser, const CompleteTypeObject& type_object)
{
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

size_t find_size(const MinimalTypeObject& type_object, size_t& size)
{
  size += DCPS::max_marshaled_size_octet(); // discriminator

  switch (type_object.kind) {
  case TK_ALIAS:
    return find_size(type_object.alias_type, size);
  case TK_ANNOTATION:
    return find_size(type_object.annotation_type, size);
  case TK_STRUCTURE:
    return find_size(type_object.struct_type, size);
  case TK_UNION:
    return find_size(type_object.union_type, size);
  case TK_BITSET:
    return find_size(type_object.bitset_type, size);
  case TK_SEQUENCE:
    return find_size(type_object.sequence_type, size);
  case TK_ARRAY:
    return find_size(type_object.array_type, size);
  case TK_MAP:
    return find_size(type_object.map_type, size);
  case TK_ENUM:
    return find_size(type_object.enumerated_type, size);
  case TK_BITMASK:
    return find_size(type_object.bitmask_type, size);
  default:
    return find_size(type_object.extended_type, size);
  }
}

bool operator<<(DCPS::Serializer& ser, const MinimalTypeObject& type_object)
{
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

size_t find_size(const TypeObject& type_object)
{
  size_t size = DCPS::max_marshaled_size_ulong(); // DHEADER
  size += DCPS::max_marshaled_size_octet(); // discriminator

  switch (type_object.kind) {
  case EK_COMPLETE:
    return find_size(type_object.complete, size);
  case EK_MINIMAL:
    return find_size(type_object.minimal, size);
  }

  return size;
}

bool operator<<(DCPS::Serializer& ser, const TypeObject& type_object)
{
  // XCDR2 Appendable Union: DELIMITED_CDR (7.4.2) = DHEADER + PLAIN_CDR2
  // DHEADER = UInt32 size of object that follows
  // subtracting the DHEADER's own size doesn't impact alignment since the
  // maximum alignment in PLAIN_CDR2 is 4
  const size_t dheader = find_size(type_object) - DCPS::max_marshaled_size_ulong();
  if (!(ser << ACE_CDR::ULong(dheader))) {
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

size_t find_size(const TypeInformation& type_info)
{
  return 0; //TODO
}

bool operator<<(DCPS::Serializer& ser, const TypeInformation& type_info)
{
  return true; //TODO
}
bool operator>>(DCPS::Serializer& ser, TypeInformation& type_info)
{
  return true; //TODO
}
}
namespace DCPS {

template<>
RcHandle<XTypes::TypeIdentifier> getMinimalTypeIdentifier<void>()
{
  static const RcHandle<XTypes::TypeIdentifier> ti = XTypes::TypeIdentifier::make(XTypes::TK_NONE);
  return ti;
}

template<>
RcHandle<XTypes::TypeIdentifier> getMinimalTypeIdentifier<ACE_CDR::Boolean>()
{
  static const RcHandle<XTypes::TypeIdentifier> ti = XTypes::TypeIdentifier::make(XTypes::TK_BOOLEAN);
  return ti;
}

template<>
RcHandle<XTypes::TypeIdentifier> getMinimalTypeIdentifier<ACE_CDR::Octet>()
{
  static const RcHandle<XTypes::TypeIdentifier> ti = XTypes::TypeIdentifier::make(XTypes::TK_BYTE);
  return ti;
}

template<>
RcHandle<XTypes::TypeIdentifier> getMinimalTypeIdentifier<ACE_CDR::Short>()
{
  static const RcHandle<XTypes::TypeIdentifier> ti = XTypes::TypeIdentifier::make(XTypes::TK_INT16);
  return ti;
}

template<>
RcHandle<XTypes::TypeIdentifier> getMinimalTypeIdentifier<ACE_CDR::Long>()
{
  static const RcHandle<XTypes::TypeIdentifier> ti = XTypes::TypeIdentifier::make(XTypes::TK_INT32);
  return ti;
}

template<>
RcHandle<XTypes::TypeIdentifier> getMinimalTypeIdentifier<ACE_CDR::LongLong>()
{
  static const RcHandle<XTypes::TypeIdentifier> ti = XTypes::TypeIdentifier::make(XTypes::TK_INT64);
  return ti;
}

template<>
RcHandle<XTypes::TypeIdentifier> getMinimalTypeIdentifier<ACE_CDR::UShort>()
{
  static const RcHandle<XTypes::TypeIdentifier> ti = XTypes::TypeIdentifier::make(XTypes::TK_UINT16);
  return ti;
}

template<>
RcHandle<XTypes::TypeIdentifier> getMinimalTypeIdentifier<ACE_CDR::ULong>()
{
  static const RcHandle<XTypes::TypeIdentifier> ti = XTypes::TypeIdentifier::make(XTypes::TK_UINT32);
  return ti;
}

template<>
RcHandle<XTypes::TypeIdentifier> getMinimalTypeIdentifier<ACE_CDR::ULongLong>()
{
  static const RcHandle<XTypes::TypeIdentifier> ti = XTypes::TypeIdentifier::make(XTypes::TK_UINT64);
  return ti;
}

template<>
RcHandle<XTypes::TypeIdentifier> getMinimalTypeIdentifier<ACE_CDR::Float>()
{
  static const RcHandle<XTypes::TypeIdentifier> ti = XTypes::TypeIdentifier::make(XTypes::TK_FLOAT32);
  return ti;
}

template<>
RcHandle<XTypes::TypeIdentifier> getMinimalTypeIdentifier<ACE_CDR::Double>()
{
  static const RcHandle<XTypes::TypeIdentifier> ti = XTypes::TypeIdentifier::make(XTypes::TK_FLOAT64);
  return ti;
}

template<>
RcHandle<XTypes::TypeIdentifier> getMinimalTypeIdentifier<ACE_CDR::LongDouble>()
{
  static const RcHandle<XTypes::TypeIdentifier> ti = XTypes::TypeIdentifier::make(XTypes::TK_FLOAT128);
  return ti;
}

template<>
RcHandle<XTypes::TypeIdentifier> getMinimalTypeIdentifier<ACE_CDR::Char>()
{
  static const RcHandle<XTypes::TypeIdentifier> ti = XTypes::TypeIdentifier::make(XTypes::TK_CHAR8);
  return ti;
}

template<>
RcHandle<XTypes::TypeIdentifier> getMinimalTypeIdentifier<ACE_OutputCDR::from_wchar>()
{
  static const RcHandle<XTypes::TypeIdentifier> ti = XTypes::TypeIdentifier::make(XTypes::TK_CHAR16);
  return ti;
}

template<>
RcHandle<XTypes::TypeIdentifier> getMinimalTypeIdentifier<ACE_CDR::Char*>()
{
  static const RcHandle<XTypes::TypeIdentifier> ti = XTypes::TypeIdentifier::makeString(false, XTypes::StringSTypeDefn(XTypes::INVALID_SBOUND));
  return ti;
}

template<>
RcHandle<XTypes::TypeIdentifier> getMinimalTypeIdentifier<ACE_CDR::WChar*>()
{
  static const RcHandle<XTypes::TypeIdentifier> ti = XTypes::TypeIdentifier::makeString(true, XTypes::StringSTypeDefn(XTypes::INVALID_SBOUND));
  return ti;
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
