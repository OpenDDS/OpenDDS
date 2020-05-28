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
  const Encoding encoding(Encoding::KIND_XCDR2_PLAIN, DCPS::ENDIAN_LITTLE);
  size_t size = 0;
  serialized_size(encoding, size, type_object);
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

  return TypeIdentifierPtr();
}


// Serialization support for TypeObject and its components

void serialized_size(const Encoding& encoding, size_t& size,
  const TypeIdentifier& stru)
{
}

bool operator<<(DCPS::Serializer& ser, const TypeIdentifier& stru)
{
  return true;
}


void serialized_size(const Encoding& encoding, size_t& size,
  const CompleteStructType& stru)
{
}

bool operator<<(DCPS::Serializer& ser, const CompleteStructType& stru)
{
  return true;
}


void serialized_size(const Encoding& encoding, size_t& size,
  const MinimalStructType& stru)
{
}

bool operator<<(DCPS::Serializer& ser, const MinimalStructType& stru)
{
  return true;
}


void serialized_size(const Encoding& encoding, size_t& size,
  const CompleteUnionType& stru)
{
}

bool operator<<(DCPS::Serializer& ser, const CompleteUnionType& stru)
{
  return true;
}


void serialized_size(const Encoding& encoding, size_t& size,
  const MinimalUnionType& stru)
{
}

bool operator<<(DCPS::Serializer& ser, const MinimalUnionType& stru)
{
  return true;
}


void serialized_size(const Encoding& encoding, size_t& size,
  const CompleteAnnotationType& stru)
{
}

bool operator<<(DCPS::Serializer& ser, const CompleteAnnotationType& stru)
{
  return true;
}


void serialized_size(const Encoding& encoding, size_t& size,
  const MinimalAnnotationType& stru)
{
}

bool operator<<(DCPS::Serializer& ser, const MinimalAnnotationType& stru)
{
  return true;
}


void serialized_size(const Encoding& encoding, size_t& size,
  const CompleteAliasType& stru)
{
}

bool operator<<(DCPS::Serializer& ser, const CompleteAliasType& stru)
{
  return true;
}


void serialized_size(const Encoding& encoding, size_t& size,
  const MinimalAliasType& stru)
{
}

bool operator<<(DCPS::Serializer& ser, const MinimalAliasType& stru)
{
  return true;
}


void serialized_size(const Encoding& encoding, size_t& size,
  const CompleteSequenceType& stru)
{
}

bool operator<<(DCPS::Serializer& ser, const CompleteSequenceType& stru)
{
  return true;
}


void serialized_size(const Encoding& encoding, size_t& size,
  const MinimalSequenceType& stru)
{
}

bool operator<<(DCPS::Serializer& ser, const MinimalSequenceType& stru)
{
  return true;
}


void serialized_size(const Encoding& encoding, size_t& size,
  const CompleteArrayType& stru)
{
}

bool operator<<(DCPS::Serializer& ser, const CompleteArrayType& stru)
{
  return true;
}


void serialized_size(const Encoding& encoding, size_t& size,
  const MinimalArrayType& stru)
{
}

bool operator<<(DCPS::Serializer& ser, const MinimalArrayType& stru)
{
  return true;
}


void serialized_size(const Encoding& encoding, size_t& size,
  const CompleteMapType& stru)
{
}

bool operator<<(DCPS::Serializer& ser, const CompleteMapType& stru)
{
  return true;
}


void serialized_size(const Encoding& encoding, size_t& size, const MinimalMapType& stru)
{
}

bool operator<<(DCPS::Serializer& ser, const MinimalMapType& stru)
{
  return true;
}


void serialized_size(const Encoding& encoding, size_t& size, const CompleteEnumeratedType& stru)
{
}

bool operator<<(DCPS::Serializer& ser, const CompleteEnumeratedType& stru)
{
  return true;
}


void serialized_size(const Encoding& encoding, size_t& size,
  const MinimalEnumeratedType& stru)
{
}

bool operator<<(DCPS::Serializer& ser, const MinimalEnumeratedType& stru)
{
  return true;
}


void serialized_size(const Encoding& encoding, size_t& size, const CompleteBitmaskType& stru)
{
}

bool operator<<(DCPS::Serializer& ser, const CompleteBitmaskType& stru)
{
  return true;
}


void serialized_size(const Encoding& encoding, size_t& size,
  const MinimalBitmaskType& stru)
{
}

bool operator<<(DCPS::Serializer& ser, const MinimalBitmaskType& stru)
{
  return true;
}


void serialized_size(const Encoding& encoding, size_t& size,
  const CompleteBitsetType& stru)
{
}

bool operator<<(DCPS::Serializer& ser, const CompleteBitsetType& stru)
{
  return true;
}


void serialized_size(const Encoding& encoding, size_t& size,
  const MinimalBitsetType& stru)
{
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


void serialized_size(const Encoding& encoding, size_t& size,
  const CompleteTypeObject& type_object)
{
  DCPS::max_serialized_size_octet(encoding, size); // discriminator

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

void serialized_size(const Encoding& encoding, size_t& size,
  const MinimalTypeObject& type_object)
{
  DCPS::max_serialized_size_octet(encoding, size); // discriminator

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

void serialized_size(const Encoding& encoding, size_t& size,
  const TypeObject& type_object)
{
  DCPS::max_serialized_size_ulong(encoding, size); // DHEADER
  DCPS::max_serialized_size_octet(encoding, size); // discriminator

  switch (type_object.kind) {
  case EK_COMPLETE:
    return serialized_size(encoding, size, type_object.complete);
  case EK_MINIMAL:
    return serialized_size(encoding, size, type_object.minimal);
  }
}

bool operator<<(DCPS::Serializer& ser, const TypeObject& type_object)
{
  // XCDR2 Appendable Union: DELIMITED_CDR (7.4.2) = DHEADER + PLAIN_CDR2
  // DHEADER = UInt32 size of object that follows
  // subtracting the DHEADER's own size doesn't impact alignment since the
  // maximum alignment in PLAIN_CDR2 is 4
  size_t object_size = 0;
  serialized_size(ser.encoding(), object_size, type_object);
  size_t dheader_size = 0;
  DCPS::max_serialized_size_ulong(ser.encoding(), dheader_size);
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

void serialized_size(const Encoding& encoding, size_t& size,
  const TypeInformation& type_info)
{
  //TODO
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
