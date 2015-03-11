#ifndef FACE_TYPES_HPP_HEADER_FILE
#define FACE_TYPES_HPP_HEADER_FILE

#include <ace/CDR_Base.h>

namespace FACE
{
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

  // _out types
  // {,W}String{_var,_out}
}

#endif /* FACE_TYPES_HPP_HEADER_FILE */
