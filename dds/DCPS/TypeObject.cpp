/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/

#include "TypeObject.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {
namespace XTypes {

TypeIdentifierPtr makeTypeIdentifier(const TypeObject& type_object)
{
  ACE_Message_Block buff(64 * 1024);
  DCPS::Serializer ser(&buff);
  ser << type_object;

  MD5_CTX ctx;
  MD5_Init(&ctx);
  MD5_Update(&ctx, buff.rd_ptr(), buff.length());
  unsigned char result[16];
  MD5_Final(result, &ctx);

  // First 14 bytes of MD5 of the serialized TypeObject using XCDR
  // version 2 with Little Endian encoding
  EquivalenceHash eh;
  std::memcpy(eh, result, sizeof eh);

  switch (type_object.kind) {
  case EK_COMPLETE:
    return TypeIdentifier::make(EK_COMPLETE, eh);
    break;
  case EK_MINIMAL:
    return TypeIdentifier::make(EK_MINIMAL, eh);
    break;
  }
}

}

template<>
RcHandle<XTypes::TypeIdentifier> getTypeIdentifier<XTypes::TkNone>()
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

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
