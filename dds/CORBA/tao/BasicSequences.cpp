#include "tao/corba.h"

#define SEQUENCE_I(ELEM, BASE, ARG)                                     \
  ELEM##Seq::ELEM##Seq() {}                                             \
  ELEM##Seq::ELEM##Seq(ULong max) : BASE(ARG)(max) {}                   \
  ELEM##Seq::ELEM##Seq(ULong max, ULong len, ELEM* buffer, Boolean release) \
  : BASE(ARG)(max, len, buffer, release) {}                             \
  ELEM##Seq::ELEM##Seq(const ELEM##Seq& seq) : BASE(ARG)(seq) {}        \
  ELEM##Seq::~ELEM##Seq() {}                                            \

#define NONSTRINGBASE(ELEM) TAO::unbounded_value_sequence<ELEM>
#define SEQUENCE(ELEM) SEQUENCE_I(ELEM, NONSTRINGBASE, ELEM)

#define STRINGBASE(CHARTYPE) TAO::unbounded_basic_string_sequence<CHARTYPE>
#define STRSEQ(ELEM, CHARTYPE) SEQUENCE_I(ELEM, STRINGBASE, CHARTYPE)

#define OPERATORS(ELEM) \
  CORBA::Boolean operator<<(TAO_OutputCDR&, const CORBA::ELEM##Seq&) \
  { return false; }                                                  \
  CORBA::Boolean operator>>(TAO_InputCDR&, CORBA::ELEM##Seq&)        \
  { return false; }                                                  \

namespace CORBA
{
  SEQUENCE(Boolean)
  SEQUENCE(Char)
  SEQUENCE(Double)
  SEQUENCE(Float)
  SEQUENCE(LongDouble)
  SEQUENCE(LongLong)
  SEQUENCE(Long)
  SEQUENCE(Short)
  SEQUENCE(ULongLong)
  SEQUENCE(ULong)
  SEQUENCE(UShort)
  SEQUENCE(WChar)

  typedef Char* String;
  STRSEQ(String, Char)

  typedef WChar* WString;
  STRSEQ(WString, WChar)
}

OPERATORS(Boolean)
OPERATORS(Char)
OPERATORS(Double)
OPERATORS(Float)
OPERATORS(LongDouble)
OPERATORS(LongLong)
OPERATORS(Long)
OPERATORS(Short)
OPERATORS(String)
OPERATORS(ULongLong)
OPERATORS(ULong)
OPERATORS(UShort)
OPERATORS(WChar)
OPERATORS(WString)
