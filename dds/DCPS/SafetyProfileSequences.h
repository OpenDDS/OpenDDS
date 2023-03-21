/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_SAFETY_PROFILE_SEQUENCES_H
#define OPENDDS_DCPS_SAFETY_PROFILE_SEQUENCES_H
#ifdef OPENDDS_SAFETY_PROFILE

#include "SafetyProfileSequence.h"
#include "SafetyProfileSequenceVar.h"

#include <tao/Basic_Types_IDLv4.h>
#include <tao/Seq_Out_T.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL
namespace OpenDDS {
namespace CORBASeq {

class BooleanSeq;
typedef SafetyProfile::SequenceVar<BooleanSeq> BooleanSeq_var;
typedef ::TAO_Seq_Out_T<BooleanSeq> BooleanSeq_out;

class OpenDDS_Dcps_Export BooleanSeq : public SafetyProfile::Sequence< CORBA::Boolean, SafetyProfile::Unbounded > {
public:
  typedef BooleanSeq_var _var_type;
  typedef BooleanSeq_out _out_type;

  BooleanSeq() {}
  BooleanSeq(const BooleanSeq& seq) : SafetyProfile::Sequence< CORBA::Boolean, SafetyProfile::Unbounded >(seq) {}
  friend void swap(BooleanSeq& a, BooleanSeq& b) { a.swap(b); }
  BooleanSeq& operator=(const BooleanSeq& rhs)
  {
    BooleanSeq tmp(rhs);
    swap(tmp);
    return *this;
  }
  BooleanSeq(CORBA::ULong maximum)
    : SafetyProfile::Sequence< CORBA::Boolean, SafetyProfile::Unbounded >(maximum, 0u, 0, true) {}
  BooleanSeq(CORBA::ULong maximum, CORBA::ULong length, CORBA::Boolean* data, CORBA::Boolean release = false)
    : SafetyProfile::Sequence< CORBA::Boolean, SafetyProfile::Unbounded >(maximum, length, data, release) {}
};

class CharSeq;
typedef SafetyProfile::SequenceVar<CharSeq> CharSeq_var;
typedef ::TAO_Seq_Out_T<CharSeq> CharSeq_out;

class OpenDDS_Dcps_Export CharSeq : public SafetyProfile::Sequence< CORBA::Char, SafetyProfile::Unbounded > {
public:
  typedef CharSeq_var _var_type;
  typedef CharSeq_out _out_type;

  CharSeq() {}
  CharSeq(const CharSeq& seq) : SafetyProfile::Sequence< CORBA::Char, SafetyProfile::Unbounded >(seq) {}
  friend void swap(CharSeq& a, CharSeq& b) { a.swap(b); }
  CharSeq& operator=(const CharSeq& rhs)
  {
    CharSeq tmp(rhs);
    swap(tmp);
    return *this;
  }
  CharSeq(CORBA::ULong maximum)
    : SafetyProfile::Sequence< CORBA::Char, SafetyProfile::Unbounded >(maximum, 0u, 0, true) {}
  CharSeq(CORBA::ULong maximum, CORBA::ULong length, CORBA::Char* data, CORBA::Boolean release = false)
    : SafetyProfile::Sequence< CORBA::Char, SafetyProfile::Unbounded >(maximum, length, data, release) {}
};

class DoubleSeq;
typedef SafetyProfile::SequenceVar<DoubleSeq> DoubleSeq_var;
typedef ::TAO_Seq_Out_T<DoubleSeq> DoubleSeq_out;

class OpenDDS_Dcps_Export DoubleSeq : public SafetyProfile::Sequence< CORBA::Double, SafetyProfile::Unbounded > {
public:
  typedef DoubleSeq_var _var_type;
  typedef DoubleSeq_out _out_type;

  DoubleSeq() {}
  DoubleSeq(const DoubleSeq& seq) : SafetyProfile::Sequence< CORBA::Double, SafetyProfile::Unbounded >(seq) {}
  friend void swap(DoubleSeq& a, DoubleSeq& b) { a.swap(b); }
  DoubleSeq& operator=(const DoubleSeq& rhs)
  {
    DoubleSeq tmp(rhs);
    swap(tmp);
    return *this;
  }
  DoubleSeq(CORBA::ULong maximum)
    : SafetyProfile::Sequence< CORBA::Double, SafetyProfile::Unbounded >(maximum, 0u, 0, true) {}
  DoubleSeq(CORBA::ULong maximum, CORBA::ULong length, CORBA::Double* data, CORBA::Boolean release = false)
    : SafetyProfile::Sequence< CORBA::Double, SafetyProfile::Unbounded >(maximum, length, data, release) {}
};

class FloatSeq;
typedef SafetyProfile::SequenceVar<FloatSeq> FloatSeq_var;
typedef ::TAO_Seq_Out_T<FloatSeq> FloatSeq_out;

class OpenDDS_Dcps_Export FloatSeq : public SafetyProfile::Sequence< CORBA::Float, SafetyProfile::Unbounded > {
public:
  typedef FloatSeq_var _var_type;
  typedef FloatSeq_out _out_type;

  FloatSeq() {}
  FloatSeq(const FloatSeq& seq) : SafetyProfile::Sequence< CORBA::Float, SafetyProfile::Unbounded >(seq) {}
  friend void swap(FloatSeq& a, FloatSeq& b) { a.swap(b); }
  FloatSeq& operator=(const FloatSeq& rhs)
  {
    FloatSeq tmp(rhs);
    swap(tmp);
    return *this;
  }
  FloatSeq(CORBA::ULong maximum)
    : SafetyProfile::Sequence< CORBA::Float, SafetyProfile::Unbounded >(maximum, 0u, 0, true) {}
  FloatSeq(CORBA::ULong maximum, CORBA::ULong length, CORBA::Float* data, CORBA::Boolean release = false)
    : SafetyProfile::Sequence< CORBA::Float, SafetyProfile::Unbounded >(maximum, length, data, release) {}
};

class Int8Seq;
typedef SafetyProfile::SequenceVar<Int8Seq> Int8Seq_var;
typedef ::TAO_Seq_Out_T<Int8Seq> Int8Seq_out;

class OpenDDS_Dcps_Export Int8Seq : public SafetyProfile::Sequence< CORBA::IDLv4::Int8, SafetyProfile::Unbounded > {
public:
  typedef Int8Seq_var _var_type;
  typedef Int8Seq_out _out_type;

  Int8Seq() {}
  Int8Seq(const Int8Seq& seq) : SafetyProfile::Sequence< CORBA::IDLv4::Int8, SafetyProfile::Unbounded >(seq) {}
  friend void swap(Int8Seq& a, Int8Seq& b) { a.swap(b); }
  Int8Seq& operator=(const Int8Seq& rhs)
  {
    Int8Seq tmp(rhs);
    swap(tmp);
    return *this;
  }
  Int8Seq(CORBA::ULong maximum)
    : SafetyProfile::Sequence< CORBA::IDLv4::Int8, SafetyProfile::Unbounded >(maximum, 0u, 0, true) {}
  Int8Seq(CORBA::ULong maximum, CORBA::ULong length, CORBA::IDLv4::Int8* data, CORBA::Boolean release = false)
    : SafetyProfile::Sequence< CORBA::IDLv4::Int8, SafetyProfile::Unbounded >(maximum, length, data, release) {}
};

class LongDoubleSeq;
typedef SafetyProfile::SequenceVar<LongDoubleSeq> LongDoubleSeq_var;
typedef ::TAO_Seq_Out_T<LongDoubleSeq> LongDoubleSeq_out;

class OpenDDS_Dcps_Export LongDoubleSeq : public SafetyProfile::Sequence< CORBA::LongDouble, SafetyProfile::Unbounded > {
public:
  typedef LongDoubleSeq_var _var_type;
  typedef LongDoubleSeq_out _out_type;

  LongDoubleSeq() {}
  LongDoubleSeq(const LongDoubleSeq& seq) : SafetyProfile::Sequence< CORBA::LongDouble, SafetyProfile::Unbounded >(seq) {}
  friend void swap(LongDoubleSeq& a, LongDoubleSeq& b) { a.swap(b); }
  LongDoubleSeq& operator=(const LongDoubleSeq& rhs)
  {
    LongDoubleSeq tmp(rhs);
    swap(tmp);
    return *this;
  }
  LongDoubleSeq(CORBA::ULong maximum)
    : SafetyProfile::Sequence< CORBA::LongDouble, SafetyProfile::Unbounded >(maximum, 0u, 0, true) {}
  LongDoubleSeq(CORBA::ULong maximum, CORBA::ULong length, CORBA::LongDouble* data, CORBA::Boolean release = false)
    : SafetyProfile::Sequence< CORBA::LongDouble, SafetyProfile::Unbounded >(maximum, length, data, release) {}
};

class LongLongSeq;
typedef SafetyProfile::SequenceVar<LongLongSeq> LongLongSeq_var;
typedef ::TAO_Seq_Out_T<LongLongSeq> LongLongSeq_out;

class OpenDDS_Dcps_Export LongLongSeq : public SafetyProfile::Sequence< CORBA::LongLong, SafetyProfile::Unbounded > {
public:
  typedef LongLongSeq_var _var_type;
  typedef LongLongSeq_out _out_type;

  LongLongSeq() {}
  LongLongSeq(const LongLongSeq& seq) : SafetyProfile::Sequence< CORBA::LongLong, SafetyProfile::Unbounded >(seq) {}
  friend void swap(LongLongSeq& a, LongLongSeq& b) { a.swap(b); }
  LongLongSeq& operator=(const LongLongSeq& rhs)
  {
    LongLongSeq tmp(rhs);
    swap(tmp);
    return *this;
  }
  LongLongSeq(CORBA::ULong maximum)
    : SafetyProfile::Sequence< CORBA::LongLong, SafetyProfile::Unbounded >(maximum, 0u, 0, true) {}
  LongLongSeq(CORBA::ULong maximum, CORBA::ULong length, CORBA::LongLong* data, CORBA::Boolean release = false)
    : SafetyProfile::Sequence< CORBA::LongLong, SafetyProfile::Unbounded >(maximum, length, data, release) {}
};

class LongSeq;
typedef SafetyProfile::SequenceVar<LongSeq> LongSeq_var;
typedef ::TAO_Seq_Out_T<LongSeq> LongSeq_out;

class OpenDDS_Dcps_Export LongSeq : public SafetyProfile::Sequence< CORBA::Long, SafetyProfile::Unbounded > {
public:
  typedef LongSeq_var _var_type;
  typedef LongSeq_out _out_type;

  LongSeq() {}
  LongSeq(const LongSeq& seq) : SafetyProfile::Sequence< CORBA::Long, SafetyProfile::Unbounded >(seq) {}
  friend void swap(LongSeq& a, LongSeq& b) { a.swap(b); }
  LongSeq& operator=(const LongSeq& rhs)
  {
    LongSeq tmp(rhs);
    swap(tmp);
    return *this;
  }
  LongSeq(CORBA::ULong maximum)
    : SafetyProfile::Sequence< CORBA::Long, SafetyProfile::Unbounded >(maximum, 0u, 0, true) {}
  LongSeq(CORBA::ULong maximum, CORBA::ULong length, CORBA::Long* data, CORBA::Boolean release = false)
    : SafetyProfile::Sequence< CORBA::Long, SafetyProfile::Unbounded >(maximum, length, data, release) {}
};

class OctetSeq;
typedef SafetyProfile::SequenceVar<OctetSeq> OctetSeq_var;
typedef ::TAO_Seq_Out_T<OctetSeq> OctetSeq_out;

class OpenDDS_Dcps_Export OctetSeq : public SafetyProfile::Sequence< CORBA::Octet, SafetyProfile::Unbounded > {
public:
  typedef OctetSeq_var _var_type;
  typedef OctetSeq_out _out_type;

  OctetSeq() {}
  OctetSeq(const OctetSeq& seq) : SafetyProfile::Sequence< CORBA::Octet, SafetyProfile::Unbounded >(seq) {}
  friend void swap(OctetSeq& a, OctetSeq& b) { a.swap(b); }
  OctetSeq& operator=(const OctetSeq& rhs)
  {
    OctetSeq tmp(rhs);
    swap(tmp);
    return *this;
  }
  OctetSeq(CORBA::ULong maximum)
    : SafetyProfile::Sequence< CORBA::Octet, SafetyProfile::Unbounded >(maximum, 0u, 0, true) {}
  OctetSeq(CORBA::ULong maximum, CORBA::ULong length, CORBA::Octet* data, CORBA::Boolean release = false)
    : SafetyProfile::Sequence< CORBA::Octet, SafetyProfile::Unbounded >(maximum, length, data, release) {}
};

class ShortSeq;
typedef SafetyProfile::SequenceVar<ShortSeq> ShortSeq_var;
typedef ::TAO_Seq_Out_T<ShortSeq> ShortSeq_out;

class OpenDDS_Dcps_Export ShortSeq : public SafetyProfile::Sequence< CORBA::Short, SafetyProfile::Unbounded > {
public:
  typedef ShortSeq_var _var_type;
  typedef ShortSeq_out _out_type;

  ShortSeq() {}
  ShortSeq(const ShortSeq& seq) : SafetyProfile::Sequence< CORBA::Short, SafetyProfile::Unbounded >(seq) {}
  friend void swap(ShortSeq& a, ShortSeq& b) { a.swap(b); }
  ShortSeq& operator=(const ShortSeq& rhs)
  {
    ShortSeq tmp(rhs);
    swap(tmp);
    return *this;
  }
  ShortSeq(CORBA::ULong maximum)
    : SafetyProfile::Sequence< CORBA::Short, SafetyProfile::Unbounded >(maximum, 0u, 0, true) {}
  ShortSeq(CORBA::ULong maximum, CORBA::ULong length, CORBA::Short* data, CORBA::Boolean release = false)
    : SafetyProfile::Sequence< CORBA::Short, SafetyProfile::Unbounded >(maximum, length, data, release) {}
};

class StringSeq;
typedef SafetyProfile::SequenceVar<StringSeq> StringSeq_var;
typedef ::TAO_Seq_Out_T<StringSeq> StringSeq_out;

class OpenDDS_Dcps_Export StringSeq : public SafetyProfile::Sequence< CORBA::Char*, SafetyProfile::Unbounded, SafetyProfile::StringEltPolicy< CORBA::Char> > {
public:
  typedef StringSeq_var _var_type;
  typedef StringSeq_out _out_type;

  StringSeq() {}
  StringSeq(const StringSeq& seq) : SafetyProfile::Sequence< CORBA::Char*, SafetyProfile::Unbounded, SafetyProfile::StringEltPolicy< CORBA::Char> >(seq) {}
  friend void swap(StringSeq& a, StringSeq& b) { a.swap(b); }
  StringSeq& operator=(const StringSeq& rhs)
  {
    StringSeq tmp(rhs);
    swap(tmp);
    return *this;
  }
  StringSeq(CORBA::ULong maximum)
    : SafetyProfile::Sequence< CORBA::Char*, SafetyProfile::Unbounded, SafetyProfile::StringEltPolicy< CORBA::Char> >(maximum, 0u, 0, true) {}
  StringSeq(CORBA::ULong maximum, CORBA::ULong length, CORBA::Char** data, CORBA::Boolean release = false)
    : SafetyProfile::Sequence< CORBA::Char*, SafetyProfile::Unbounded, SafetyProfile::StringEltPolicy< CORBA::Char> >(maximum, length, data, release) {}
};

class UInt8Seq;
typedef SafetyProfile::SequenceVar<UInt8Seq> UInt8Seq_var;
typedef ::TAO_Seq_Out_T<UInt8Seq> UInt8Seq_out;

class OpenDDS_Dcps_Export UInt8Seq : public SafetyProfile::Sequence< CORBA::IDLv4::UInt8, SafetyProfile::Unbounded > {
public:
  typedef UInt8Seq_var _var_type;
  typedef UInt8Seq_out _out_type;

  UInt8Seq() {}
  UInt8Seq(const UInt8Seq& seq) : SafetyProfile::Sequence< CORBA::IDLv4::UInt8, SafetyProfile::Unbounded >(seq) {}
  friend void swap(UInt8Seq& a, UInt8Seq& b) { a.swap(b); }
  UInt8Seq& operator=(const UInt8Seq& rhs)
  {
    UInt8Seq tmp(rhs);
    swap(tmp);
    return *this;
  }
  UInt8Seq(CORBA::ULong maximum)
    : SafetyProfile::Sequence< CORBA::IDLv4::UInt8, SafetyProfile::Unbounded >(maximum, 0u, 0, true) {}
  UInt8Seq(CORBA::ULong maximum, CORBA::ULong length, CORBA::IDLv4::UInt8* data, CORBA::Boolean release = false)
    : SafetyProfile::Sequence< CORBA::IDLv4::UInt8, SafetyProfile::Unbounded >(maximum, length, data, release) {}
};

class ULongLongSeq;
typedef SafetyProfile::SequenceVar<ULongLongSeq> ULongLongSeq_var;
typedef ::TAO_Seq_Out_T<ULongLongSeq> ULongLongSeq_out;

class OpenDDS_Dcps_Export ULongLongSeq : public SafetyProfile::Sequence< CORBA::ULongLong, SafetyProfile::Unbounded > {
public:
  typedef ULongLongSeq_var _var_type;
  typedef ULongLongSeq_out _out_type;

  ULongLongSeq() {}
  ULongLongSeq(const ULongLongSeq& seq) : SafetyProfile::Sequence< CORBA::ULongLong, SafetyProfile::Unbounded >(seq) {}
  friend void swap(ULongLongSeq& a, ULongLongSeq& b) { a.swap(b); }
  ULongLongSeq& operator=(const ULongLongSeq& rhs)
  {
    ULongLongSeq tmp(rhs);
    swap(tmp);
    return *this;
  }
  ULongLongSeq(CORBA::ULong maximum)
    : SafetyProfile::Sequence< CORBA::ULongLong, SafetyProfile::Unbounded >(maximum, 0u, 0, true) {}
  ULongLongSeq(CORBA::ULong maximum, CORBA::ULong length, CORBA::ULongLong* data, CORBA::Boolean release = false)
    : SafetyProfile::Sequence< CORBA::ULongLong, SafetyProfile::Unbounded >(maximum, length, data, release) {}
};

class ULongSeq;
typedef SafetyProfile::SequenceVar<ULongSeq> ULongSeq_var;
typedef ::TAO_Seq_Out_T<ULongSeq> ULongSeq_out;

class OpenDDS_Dcps_Export ULongSeq : public SafetyProfile::Sequence< CORBA::ULong, SafetyProfile::Unbounded > {
public:
  typedef ULongSeq_var _var_type;
  typedef ULongSeq_out _out_type;

  ULongSeq() {}
  ULongSeq(const ULongSeq& seq) : SafetyProfile::Sequence< CORBA::ULong, SafetyProfile::Unbounded >(seq) {}
  friend void swap(ULongSeq& a, ULongSeq& b) { a.swap(b); }
  ULongSeq& operator=(const ULongSeq& rhs)
  {
    ULongSeq tmp(rhs);
    swap(tmp);
    return *this;
  }
  ULongSeq(CORBA::ULong maximum)
    : SafetyProfile::Sequence< CORBA::ULong, SafetyProfile::Unbounded >(maximum, 0u, 0, true) {}
  ULongSeq(CORBA::ULong maximum, CORBA::ULong length, CORBA::ULong* data, CORBA::Boolean release = false)
    : SafetyProfile::Sequence< CORBA::ULong, SafetyProfile::Unbounded >(maximum, length, data, release) {}
};

class UShortSeq;
typedef SafetyProfile::SequenceVar<UShortSeq> UShortSeq_var;
typedef ::TAO_Seq_Out_T<UShortSeq> UShortSeq_out;

class OpenDDS_Dcps_Export UShortSeq : public SafetyProfile::Sequence< CORBA::UShort, SafetyProfile::Unbounded > {
public:
  typedef UShortSeq_var _var_type;
  typedef UShortSeq_out _out_type;

  UShortSeq() {}
  UShortSeq(const UShortSeq& seq) : SafetyProfile::Sequence< CORBA::UShort, SafetyProfile::Unbounded >(seq) {}
  friend void swap(UShortSeq& a, UShortSeq& b) { a.swap(b); }
  UShortSeq& operator=(const UShortSeq& rhs)
  {
    UShortSeq tmp(rhs);
    swap(tmp);
    return *this;
  }
  UShortSeq(CORBA::ULong maximum)
    : SafetyProfile::Sequence< CORBA::UShort, SafetyProfile::Unbounded >(maximum, 0u, 0, true) {}
  UShortSeq(CORBA::ULong maximum, CORBA::ULong length, CORBA::UShort* data, CORBA::Boolean release = false)
    : SafetyProfile::Sequence< CORBA::UShort, SafetyProfile::Unbounded >(maximum, length, data, release) {}
};

}

namespace DCPS {

OpenDDS_Dcps_Export
void serialized_size(const Encoding& encoding, size_t& size, const CORBASeq::BooleanSeq& seq);

OpenDDS_Dcps_Export
bool operator<<(Serializer& strm, const CORBASeq::BooleanSeq& seq);

OpenDDS_Dcps_Export
bool operator>>(Serializer& strm, CORBASeq::BooleanSeq& seq);

OpenDDS_Dcps_Export
void serialized_size(const Encoding& encoding, size_t& size, const CORBASeq::CharSeq& seq);

OpenDDS_Dcps_Export
bool operator<<(Serializer& strm, const CORBASeq::CharSeq& seq);

OpenDDS_Dcps_Export
bool operator>>(Serializer& strm, CORBASeq::CharSeq& seq);

OpenDDS_Dcps_Export
void serialized_size(const Encoding& encoding, size_t& size, const CORBASeq::DoubleSeq& seq);

OpenDDS_Dcps_Export
bool operator<<(Serializer& strm, const CORBASeq::DoubleSeq& seq);

OpenDDS_Dcps_Export
bool operator>>(Serializer& strm, CORBASeq::DoubleSeq& seq);

OpenDDS_Dcps_Export
void serialized_size(const Encoding& encoding, size_t& size, const CORBASeq::FloatSeq& seq);

OpenDDS_Dcps_Export
bool operator<<(Serializer& strm, const CORBASeq::FloatSeq& seq);

OpenDDS_Dcps_Export
bool operator>>(Serializer& strm, CORBASeq::FloatSeq& seq);

OpenDDS_Dcps_Export
void serialized_size(const Encoding& encoding, size_t& size, const CORBASeq::Int8Seq& seq);

OpenDDS_Dcps_Export
bool operator<<(Serializer& strm, const CORBASeq::Int8Seq& seq);

OpenDDS_Dcps_Export
bool operator>>(Serializer& strm, CORBASeq::Int8Seq& seq);

OpenDDS_Dcps_Export
void serialized_size(const Encoding& encoding, size_t& size, const CORBASeq::LongDoubleSeq& seq);

OpenDDS_Dcps_Export
bool operator<<(Serializer& strm, const CORBASeq::LongDoubleSeq& seq);

OpenDDS_Dcps_Export
bool operator>>(Serializer& strm, CORBASeq::LongDoubleSeq& seq);

OpenDDS_Dcps_Export
void serialized_size(const Encoding& encoding, size_t& size, const CORBASeq::LongLongSeq& seq);

OpenDDS_Dcps_Export
bool operator<<(Serializer& strm, const CORBASeq::LongLongSeq& seq);

OpenDDS_Dcps_Export
bool operator>>(Serializer& strm, CORBASeq::LongLongSeq& seq);

OpenDDS_Dcps_Export
void serialized_size(const Encoding& encoding, size_t& size, const CORBASeq::LongSeq& seq);

OpenDDS_Dcps_Export
bool operator<<(Serializer& strm, const CORBASeq::LongSeq& seq);

OpenDDS_Dcps_Export
bool operator>>(Serializer& strm, CORBASeq::LongSeq& seq);

OpenDDS_Dcps_Export
void serialized_size(const Encoding& encoding, size_t& size, const CORBASeq::OctetSeq& seq);

OpenDDS_Dcps_Export
bool operator<<(Serializer& strm, const CORBASeq::OctetSeq& seq);

OpenDDS_Dcps_Export
bool operator>>(Serializer& strm, CORBASeq::OctetSeq& seq);

OpenDDS_Dcps_Export
void serialized_size(const Encoding& encoding, size_t& size, const CORBASeq::ShortSeq& seq);

OpenDDS_Dcps_Export
bool operator<<(Serializer& strm, const CORBASeq::ShortSeq& seq);

OpenDDS_Dcps_Export
bool operator>>(Serializer& strm, CORBASeq::ShortSeq& seq);

OpenDDS_Dcps_Export
void serialized_size(const Encoding& encoding, size_t& size, const CORBASeq::StringSeq& seq);

OpenDDS_Dcps_Export
bool operator<<(Serializer& strm, const CORBASeq::StringSeq& seq);

OpenDDS_Dcps_Export
bool operator>>(Serializer& strm, CORBASeq::StringSeq& seq);

OpenDDS_Dcps_Export
void serialized_size(const Encoding& encoding, size_t& size, const CORBASeq::UInt8Seq& seq);

OpenDDS_Dcps_Export
bool operator<<(Serializer& strm, const CORBASeq::UInt8Seq& seq);

OpenDDS_Dcps_Export
bool operator>>(Serializer& strm, CORBASeq::UInt8Seq& seq);

OpenDDS_Dcps_Export
void serialized_size(const Encoding& encoding, size_t& size, const CORBASeq::ULongLongSeq& seq);

OpenDDS_Dcps_Export
bool operator<<(Serializer& strm, const CORBASeq::ULongLongSeq& seq);

OpenDDS_Dcps_Export
bool operator>>(Serializer& strm, CORBASeq::ULongLongSeq& seq);

OpenDDS_Dcps_Export
void serialized_size(const Encoding& encoding, size_t& size, const CORBASeq::ULongSeq& seq);

OpenDDS_Dcps_Export
bool operator<<(Serializer& strm, const CORBASeq::ULongSeq& seq);

OpenDDS_Dcps_Export
bool operator>>(Serializer& strm, CORBASeq::ULongSeq& seq);

OpenDDS_Dcps_Export
void serialized_size(const Encoding& encoding, size_t& size, const CORBASeq::UShortSeq& seq);

OpenDDS_Dcps_Export
bool operator<<(Serializer& strm, const CORBASeq::UShortSeq& seq);

OpenDDS_Dcps_Export
bool operator>>(Serializer& strm, CORBASeq::UShortSeq& seq);


} }
OPENDDS_END_VERSIONED_NAMESPACE_DECL


#endif
#endif
