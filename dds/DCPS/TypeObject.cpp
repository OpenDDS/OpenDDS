/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/

#include "Hash.h"
#include "TypeObject.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace XTypes {

MinimalMemberDetail MinimalMemberDetail::make(const std::string& name)
{
  unsigned char result[16];
  DCPS::MD5Hash(result, name.c_str(), name.size());

  MinimalMemberDetail m;
  std::memcpy(m.name_hash, result, sizeof m.name_hash);
  return m;
}


TypeIdentifierPtr makeTypeIdentifier(const TypeObject& type_object)
{
  ACE_Message_Block buff(64 * 1024); //TODO: find size
  DCPS::Serializer ser(&buff); //TODO: XCDR2_LE
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

bool operator<<(DCPS::Serializer& ser, const TypeObject& type_object)
{
  //TODO
  return true;
}

}
namespace DCPS {

template<>
RcHandle<XTypes::TypeIdentifier> getTypeIdentifier<void>()
{
  static RcHandle<XTypes::TypeIdentifier> ti = XTypes::TypeIdentifier::make(XTypes::TK_NONE);
  return ti;
}

template<>
RcHandle<XTypes::TypeIdentifier> getTypeIdentifier<ACE_CDR::Boolean>()
{
  static RcHandle<XTypes::TypeIdentifier> ti = XTypes::TypeIdentifier::make(XTypes::TK_BOOLEAN);
  return ti;
}

template<>
RcHandle<XTypes::TypeIdentifier> getTypeIdentifier<ACE_CDR::Octet>()
{
  static RcHandle<XTypes::TypeIdentifier> ti = XTypes::TypeIdentifier::make(XTypes::TK_BYTE);
  return ti;
}

template<>
RcHandle<XTypes::TypeIdentifier> getTypeIdentifier<ACE_CDR::Short>()
{
  static RcHandle<XTypes::TypeIdentifier> ti = XTypes::TypeIdentifier::make(XTypes::TK_INT16);
  return ti;
}

template<>
RcHandle<XTypes::TypeIdentifier> getTypeIdentifier<ACE_CDR::Long>()
{
  static RcHandle<XTypes::TypeIdentifier> ti = XTypes::TypeIdentifier::make(XTypes::TK_INT32);
  return ti;
}

template<>
RcHandle<XTypes::TypeIdentifier> getTypeIdentifier<ACE_CDR::LongLong>()
{
  static RcHandle<XTypes::TypeIdentifier> ti = XTypes::TypeIdentifier::make(XTypes::TK_INT64);
  return ti;
}

template<>
RcHandle<XTypes::TypeIdentifier> getTypeIdentifier<ACE_CDR::UShort>()
{
  static RcHandle<XTypes::TypeIdentifier> ti = XTypes::TypeIdentifier::make(XTypes::TK_UINT16);
  return ti;
}

template<>
RcHandle<XTypes::TypeIdentifier> getTypeIdentifier<ACE_CDR::ULong>()
{
  static RcHandle<XTypes::TypeIdentifier> ti = XTypes::TypeIdentifier::make(XTypes::TK_UINT32);
  return ti;
}

template<>
RcHandle<XTypes::TypeIdentifier> getTypeIdentifier<ACE_CDR::ULongLong>()
{
  static RcHandle<XTypes::TypeIdentifier> ti = XTypes::TypeIdentifier::make(XTypes::TK_UINT64);
  return ti;
}

template<>
RcHandle<XTypes::TypeIdentifier> getTypeIdentifier<ACE_CDR::Float>()
{
  static RcHandle<XTypes::TypeIdentifier> ti = XTypes::TypeIdentifier::make(XTypes::TK_FLOAT32);
  return ti;
}

template<>
RcHandle<XTypes::TypeIdentifier> getTypeIdentifier<ACE_CDR::Double>()
{
  static RcHandle<XTypes::TypeIdentifier> ti = XTypes::TypeIdentifier::make(XTypes::TK_FLOAT64);
  return ti;
}

template<>
RcHandle<XTypes::TypeIdentifier> getTypeIdentifier<ACE_CDR::LongDouble>()
{
  static RcHandle<XTypes::TypeIdentifier> ti = XTypes::TypeIdentifier::make(XTypes::TK_FLOAT128);
  return ti;
}

template<>
RcHandle<XTypes::TypeIdentifier> getTypeIdentifier<ACE_CDR::Char>()
{
  static RcHandle<XTypes::TypeIdentifier> ti = XTypes::TypeIdentifier::make(XTypes::TK_CHAR8);
  return ti;
}

template<>
RcHandle<XTypes::TypeIdentifier> getTypeIdentifier<ACE_CDR::WChar>()
{
  static RcHandle<XTypes::TypeIdentifier> ti = XTypes::TypeIdentifier::make(XTypes::TK_CHAR16);
  return ti;
}

template<>
RcHandle<XTypes::TypeIdentifier> getTypeIdentifier<ACE_CDR::Char*>()
{
  static RcHandle<XTypes::TypeIdentifier> ti = XTypes::TypeIdentifier::make(XTypes::TI_STRING8_SMALL);
  return ti;
}

template<>
RcHandle<XTypes::TypeIdentifier> getTypeIdentifier<ACE_CDR::WChar*>()
{
  static RcHandle<XTypes::TypeIdentifier> ti = XTypes::TypeIdentifier::make(XTypes::TI_STRING16_SMALL);
  return ti;
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
