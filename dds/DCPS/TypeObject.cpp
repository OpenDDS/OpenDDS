/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/

#include "TypeObject.h"

#include "Hash.h"

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

MinimalMemberDetail::MinimalMemberDetail(const std::string& name)
{
  unsigned char result[16];
  DCPS::MD5Hash(result, name.c_str(), name.size());

  std::memcpy(name_hash, result, sizeof name_hash);
}


TypeIdentifierPtr makeTypeIdentifier(const TypeObject& type_object)
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

  return TypeIdentifierPtr();
}

} // namespace XTypes

namespace DCPS {

// Serialization support for TypeObject and its components

void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::TypeIdentifier& stru)
{
}

bool operator<<(Serializer& ser, const XTypes::TypeIdentifier& stru)
{
  return true;
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::CompleteStructType& stru)
{
}

bool operator<<(Serializer& ser, const XTypes::CompleteStructType& stru)
{
  return true;
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::MinimalStructType& stru)
{
}

bool operator<<(Serializer& ser, const XTypes::MinimalStructType& stru)
{
  return true;
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::CompleteUnionType& stru)
{
}

bool operator<<(Serializer& ser, const XTypes::CompleteUnionType& stru)
{
  return true;
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::MinimalUnionType& stru)
{
}

bool operator<<(Serializer& ser, const XTypes::MinimalUnionType& stru)
{
  return true;
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::CompleteAnnotationType& stru)
{
}

bool operator<<(Serializer& ser, const XTypes::CompleteAnnotationType& stru)
{
  return true;
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::MinimalAnnotationType& stru)
{
}

bool operator<<(Serializer& ser, const XTypes::MinimalAnnotationType& stru)
{
  return true;
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::CompleteAliasType& stru)
{
}

bool operator<<(Serializer& ser, const XTypes::CompleteAliasType& stru)
{
  return true;
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::MinimalAliasType& stru)
{
}

bool operator<<(Serializer& ser, const XTypes::MinimalAliasType& stru)
{
  return true;
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::CompleteSequenceType& stru)
{
}

bool operator<<(Serializer& ser, const XTypes::CompleteSequenceType& stru)
{
  return true;
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::MinimalSequenceType& stru)
{
}

bool operator<<(Serializer& ser, const XTypes::MinimalSequenceType& stru)
{
  return true;
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::CompleteArrayType& stru)
{
}

bool operator<<(Serializer& ser, const XTypes::CompleteArrayType& stru)
{
  return true;
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::MinimalArrayType& stru)
{
}

bool operator<<(Serializer& ser, const XTypes::MinimalArrayType& stru)
{
  return true;
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::CompleteMapType& stru)
{
}

bool operator<<(Serializer& ser, const XTypes::CompleteMapType& stru)
{
  return true;
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::MinimalMapType& stru)
{
}

bool operator<<(Serializer& ser, const XTypes::MinimalMapType& stru)
{
  return true;
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::CompleteEnumeratedType& stru)
{
}

bool operator<<(Serializer& ser, const XTypes::CompleteEnumeratedType& stru)
{
  return true;
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::MinimalEnumeratedType& stru)
{
}

bool operator<<(Serializer& ser, const XTypes::MinimalEnumeratedType& stru)
{
  return true;
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::CompleteBitmaskType& stru)
{
}

bool operator<<(Serializer& ser, const XTypes::CompleteBitmaskType& stru)
{
  return true;
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::MinimalBitmaskType& stru)
{
}

bool operator<<(Serializer& ser, const XTypes::MinimalBitmaskType& stru)
{
  return true;
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::CompleteBitsetType& stru)
{
}

bool operator<<(Serializer& ser, const XTypes::CompleteBitsetType& stru)
{
  return true;
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::MinimalBitsetType& stru)
{
}

bool operator<<(Serializer& ser, const XTypes::MinimalBitsetType& stru)
{
  return true;
}

void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::TypeIdentifierWithSize& stru)
{
}

bool operator<<(Serializer& ser, const XTypes::TypeIdentifierWithSize& stru)
{
  return true;
}

void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::TypeIdentifierWithSizeSeq& stru)
{
}

bool operator<<(Serializer& ser, const XTypes::TypeIdentifierWithSizeSeq& stru)
{
  return true;
}

void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::TypeIdentifierWithDependencies& stru)
{
}

bool operator<<(Serializer& ser, const XTypes::TypeIdentifierWithDependencies& stru)
{
  return true;
}


void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::CompleteTypeObject& type_object)
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

void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::TypeInformation& type_info)
{
  //TODO
}

bool operator<<(Serializer& ser, const XTypes::TypeInformation& type_info)
{
  return true; //TODO
}
bool operator>>(Serializer& ser, XTypes::TypeInformation& type_info)
{
  return true; //TODO
}

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
