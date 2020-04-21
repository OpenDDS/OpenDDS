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
  // XCDR2 Appendable Union: DELIMITED_CDR (7.4.2) = DHEADER + PLAIN_CDR2
  // DHEADER = UInt32 size of object that follows
  // Union = discriminator followed by active member (if any)
  return true;
}

}
namespace DCPS {

template<>
RcHandle<XTypes::TypeIdentifier> getTypeIdentifier<void>()
{
  static const RcHandle<XTypes::TypeIdentifier> ti = XTypes::TypeIdentifier::make(XTypes::TK_NONE);
  return ti;
}

template<>
RcHandle<XTypes::TypeIdentifier> getTypeIdentifier<ACE_CDR::Boolean>()
{
  static const RcHandle<XTypes::TypeIdentifier> ti = XTypes::TypeIdentifier::make(XTypes::TK_BOOLEAN);
  return ti;
}

template<>
RcHandle<XTypes::TypeIdentifier> getTypeIdentifier<ACE_CDR::Octet>()
{
  static const RcHandle<XTypes::TypeIdentifier> ti = XTypes::TypeIdentifier::make(XTypes::TK_BYTE);
  return ti;
}

template<>
RcHandle<XTypes::TypeIdentifier> getTypeIdentifier<ACE_CDR::Short>()
{
  static const RcHandle<XTypes::TypeIdentifier> ti = XTypes::TypeIdentifier::make(XTypes::TK_INT16);
  return ti;
}

template<>
RcHandle<XTypes::TypeIdentifier> getTypeIdentifier<ACE_CDR::Long>()
{
  static const RcHandle<XTypes::TypeIdentifier> ti = XTypes::TypeIdentifier::make(XTypes::TK_INT32);
  return ti;
}

template<>
RcHandle<XTypes::TypeIdentifier> getTypeIdentifier<ACE_CDR::LongLong>()
{
  static const RcHandle<XTypes::TypeIdentifier> ti = XTypes::TypeIdentifier::make(XTypes::TK_INT64);
  return ti;
}

template<>
RcHandle<XTypes::TypeIdentifier> getTypeIdentifier<ACE_CDR::UShort>()
{
  static const RcHandle<XTypes::TypeIdentifier> ti = XTypes::TypeIdentifier::make(XTypes::TK_UINT16);
  return ti;
}

template<>
RcHandle<XTypes::TypeIdentifier> getTypeIdentifier<ACE_CDR::ULong>()
{
  static const RcHandle<XTypes::TypeIdentifier> ti = XTypes::TypeIdentifier::make(XTypes::TK_UINT32);
  return ti;
}

template<>
RcHandle<XTypes::TypeIdentifier> getTypeIdentifier<ACE_CDR::ULongLong>()
{
  static const RcHandle<XTypes::TypeIdentifier> ti = XTypes::TypeIdentifier::make(XTypes::TK_UINT64);
  return ti;
}

template<>
RcHandle<XTypes::TypeIdentifier> getTypeIdentifier<ACE_CDR::Float>()
{
  static const RcHandle<XTypes::TypeIdentifier> ti = XTypes::TypeIdentifier::make(XTypes::TK_FLOAT32);
  return ti;
}

template<>
RcHandle<XTypes::TypeIdentifier> getTypeIdentifier<ACE_CDR::Double>()
{
  static const RcHandle<XTypes::TypeIdentifier> ti = XTypes::TypeIdentifier::make(XTypes::TK_FLOAT64);
  return ti;
}

template<>
RcHandle<XTypes::TypeIdentifier> getTypeIdentifier<ACE_CDR::LongDouble>()
{
  static const RcHandle<XTypes::TypeIdentifier> ti = XTypes::TypeIdentifier::make(XTypes::TK_FLOAT128);
  return ti;
}

template<>
RcHandle<XTypes::TypeIdentifier> getTypeIdentifier<ACE_CDR::Char>()
{
  static const RcHandle<XTypes::TypeIdentifier> ti = XTypes::TypeIdentifier::make(XTypes::TK_CHAR8);
  return ti;
}

template<>
RcHandle<XTypes::TypeIdentifier> getTypeIdentifier<ACE_OutputCDR::from_wchar>()
{
  static const RcHandle<XTypes::TypeIdentifier> ti = XTypes::TypeIdentifier::make(XTypes::TK_CHAR16);
  return ti;
}

template<>
RcHandle<XTypes::TypeIdentifier> getTypeIdentifier<ACE_CDR::Char*>()
{
  static const RcHandle<XTypes::TypeIdentifier> ti =
    XTypes::TypeIdentifier::make(XTypes::TI_STRING8_SMALL,
                                 XTypes::StringSTypeDefn::make(0));
  return ti;
}

template<>
RcHandle<XTypes::TypeIdentifier> getTypeIdentifier<ACE_CDR::WChar*>()
{
  static const RcHandle<XTypes::TypeIdentifier> ti =
    XTypes::TypeIdentifier::make(XTypes::TI_STRING16_SMALL,
                                 XTypes::StringSTypeDefn::make(0));
  return ti;
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
