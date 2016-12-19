#ifndef FACE_TYPES_HPP_HEADER_FILE
#define FACE_TYPES_HPP_HEADER_FILE

#include <ace/CDR_Base.h>
#include "OpenDDS_FACE_Export.h"
#include "dds/Versioned_Namespace.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL
namespace OpenDDS {
  namespace FaceTypes {
    template<typename CharT> class String_var;
    template<typename CharT> class String_out;
    template<typename CharT> class StringManager;
  }
}
OPENDDS_END_VERSIONED_NAMESPACE_DECL

namespace FACE {
  typedef ACE_CDR::Boolean Boolean;
  typedef ACE_CDR::Octet Octet;
  typedef ACE_CDR::Short Short;
  typedef ACE_CDR::UShort UnsignedShort;
  typedef ACE_CDR::Long Long;
  typedef ACE_CDR::ULong UnsignedLong;
  typedef ACE_CDR::LongLong LongLong;
  typedef ACE_CDR::ULongLong UnsignedLongLong;
  typedef ACE_CDR::Float Float;
  typedef ACE_CDR::Double Double;
  typedef ACE_CDR::LongDouble LongDouble;
  typedef ACE_CDR::Char Char;
  typedef ACE_CDR::WChar WChar;

  typedef Boolean& Boolean_out;
  typedef Octet& Octet_out;
  typedef Short& Short_out;
  typedef UnsignedShort& UnsignedShort_out;
  typedef Long& Long_out;
  typedef UnsignedLong& UnsignedLong_out;
  typedef LongLong& LongLong_out;
  typedef UnsignedLongLong& UnsignedLongLong_out;
  typedef Float& Float_out;
  typedef Double& Double_out;
  typedef LongDouble& LongDouble_out;
  typedef Char& Char_out;
  typedef WChar& WChar_out;

  typedef OpenDDS::FaceTypes::String_var<Char> String_var;
  typedef OpenDDS::FaceTypes::String_out<Char> String_out;
  typedef OpenDDS::FaceTypes::String_var<WChar> WString_var;
  typedef OpenDDS::FaceTypes::String_out<WChar> WString_out;

  OpenDDS_FACE_Export Char* string_alloc(UnsignedLong len);
  OpenDDS_FACE_Export Char* string_dup(const Char* str);
  OpenDDS_FACE_Export void string_free(Char* str);

  OpenDDS_FACE_Export WChar* wstring_alloc(UnsignedLong len);
  OpenDDS_FACE_Export WChar* wstring_dup(const WChar* str);
  OpenDDS_FACE_Export void wstring_free(WChar* str);
}

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL
namespace OpenDDS {
  namespace FaceTypes {
    typedef StringManager<FACE::Char> String_mgr;
    typedef StringManager<FACE::WChar> WString_mgr;
  }
}
OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* FACE_TYPES_HPP_HEADER_FILE */
