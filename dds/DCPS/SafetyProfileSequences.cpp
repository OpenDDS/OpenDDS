/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <DCPS/DdsDcps_pch.h> // Only the _pch include should start with DCPS/

#ifdef OPENDDS_SAFETY_PROFILE

#include "SafetyProfileSequences.h"

#include "debug.h"

#include <tao/Basic_Types.h>
#include <tao/CORBA_String.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL
namespace OpenDDS { namespace DCPS {

void serialized_size(const Encoding& encoding, size_t& size, const CORBASeq::BooleanSeq& seq)
{
  ACE_UNUSED_ARG(encoding);
  ACE_UNUSED_ARG(size);
  ACE_UNUSED_ARG(seq);
  primitive_serialized_size_ulong(encoding, size);
  if (seq.length() == 0) {
    return;
  }
  primitive_serialized_size_boolean(encoding, size, seq.length());
}

bool operator<<(Serializer& strm, const CORBASeq::BooleanSeq& seq)
{
  ACE_UNUSED_ARG(strm);
  ACE_UNUSED_ARG(seq);
  const Encoding& encoding = strm.encoding();
  ACE_UNUSED_ARG(encoding);
  const CORBA::ULong length = seq.length();
  if (!(strm << length)) {
    return false;
  }
  if (length == 0) {
    return true;
  }
  return strm.write_boolean_array(seq.get_buffer(), length);
}

bool operator>>(Serializer& strm, CORBASeq::BooleanSeq& seq)
{
  ACE_UNUSED_ARG(strm);
  ACE_UNUSED_ARG(seq);
  const Encoding& encoding = strm.encoding();
  ACE_UNUSED_ARG(encoding);
  CORBA::ULong length;
  if (!(strm >> length)) {
    return false;
  }
  if (length > strm.length()) {
    if (DCPS_debug_level >= 8) {
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Invalid sequence length (%u)\n"), length));
    }
    return false;
  }
  CORBA::ULong new_length = length;
  seq.length(new_length);
  if (length == 0) {
    return true;
  }
  return strm.read_boolean_array(seq.get_buffer(), length);
}

void serialized_size(const Encoding& encoding, size_t& size, const CORBASeq::CharSeq& seq)
{
  ACE_UNUSED_ARG(encoding);
  ACE_UNUSED_ARG(size);
  ACE_UNUSED_ARG(seq);
  primitive_serialized_size_ulong(encoding, size);
  if (seq.length() == 0) {
    return;
  }
  primitive_serialized_size_char(encoding, size, seq.length());
}

bool operator<<(Serializer& strm, const CORBASeq::CharSeq& seq)
{
  ACE_UNUSED_ARG(strm);
  ACE_UNUSED_ARG(seq);
  const Encoding& encoding = strm.encoding();
  ACE_UNUSED_ARG(encoding);
  const CORBA::ULong length = seq.length();
  if (!(strm << length)) {
    return false;
  }
  if (length == 0) {
    return true;
  }
  return strm.write_char_array(seq.get_buffer(), length);
}

bool operator>>(Serializer& strm, CORBASeq::CharSeq& seq)
{
  ACE_UNUSED_ARG(strm);
  ACE_UNUSED_ARG(seq);
  const Encoding& encoding = strm.encoding();
  ACE_UNUSED_ARG(encoding);
  CORBA::ULong length;
  if (!(strm >> length)) {
    return false;
  }
  if (length > strm.length()) {
    if (DCPS_debug_level >= 8) {
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Invalid sequence length (%u)\n"), length));
    }
    return false;
  }
  CORBA::ULong new_length = length;
  seq.length(new_length);
  if (length == 0) {
    return true;
  }
  return strm.read_char_array(seq.get_buffer(), length);
}

void serialized_size(const Encoding& encoding, size_t& size, const CORBASeq::DoubleSeq& seq)
{
  ACE_UNUSED_ARG(encoding);
  ACE_UNUSED_ARG(size);
  ACE_UNUSED_ARG(seq);
  primitive_serialized_size_ulong(encoding, size);
  if (seq.length() == 0) {
    return;
  }
  encoding.align(size, 8);
  primitive_serialized_size(encoding, size, CORBA::Double(), seq.length());
}

bool operator<<(Serializer& strm, const CORBASeq::DoubleSeq& seq)
{
  ACE_UNUSED_ARG(strm);
  ACE_UNUSED_ARG(seq);
  const Encoding& encoding = strm.encoding();
  ACE_UNUSED_ARG(encoding);
  const CORBA::ULong length = seq.length();
  if (!(strm << length)) {
    return false;
  }
  if (length == 0) {
    return true;
  }
  return strm.write_double_array(seq.get_buffer(), length);
}

bool operator>>(Serializer& strm, CORBASeq::DoubleSeq& seq)
{
  ACE_UNUSED_ARG(strm);
  ACE_UNUSED_ARG(seq);
  const Encoding& encoding = strm.encoding();
  ACE_UNUSED_ARG(encoding);
  CORBA::ULong length;
  if (!(strm >> length)) {
    return false;
  }
  if (length > strm.length()) {
    if (DCPS_debug_level >= 8) {
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Invalid sequence length (%u)\n"), length));
    }
    return false;
  }
  CORBA::ULong new_length = length;
  seq.length(new_length);
  if (length == 0) {
    return true;
  }
  return strm.read_double_array(seq.get_buffer(), length);
}

void serialized_size(const Encoding& encoding, size_t& size, const CORBASeq::FloatSeq& seq)
{
  ACE_UNUSED_ARG(encoding);
  ACE_UNUSED_ARG(size);
  ACE_UNUSED_ARG(seq);
  primitive_serialized_size_ulong(encoding, size);
  if (seq.length() == 0) {
    return;
  }
  primitive_serialized_size(encoding, size, CORBA::Float(), seq.length());
}

bool operator<<(Serializer& strm, const CORBASeq::FloatSeq& seq)
{
  ACE_UNUSED_ARG(strm);
  ACE_UNUSED_ARG(seq);
  const Encoding& encoding = strm.encoding();
  ACE_UNUSED_ARG(encoding);
  const CORBA::ULong length = seq.length();
  if (!(strm << length)) {
    return false;
  }
  if (length == 0) {
    return true;
  }
  return strm.write_float_array(seq.get_buffer(), length);
}

bool operator>>(Serializer& strm, CORBASeq::FloatSeq& seq)
{
  ACE_UNUSED_ARG(strm);
  ACE_UNUSED_ARG(seq);
  const Encoding& encoding = strm.encoding();
  ACE_UNUSED_ARG(encoding);
  CORBA::ULong length;
  if (!(strm >> length)) {
    return false;
  }
  if (length > strm.length()) {
    if (DCPS_debug_level >= 8) {
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Invalid sequence length (%u)\n"), length));
    }
    return false;
  }
  CORBA::ULong new_length = length;
  seq.length(new_length);
  if (length == 0) {
    return true;
  }
  return strm.read_float_array(seq.get_buffer(), length);
}

void serialized_size(const Encoding& encoding, size_t& size, const CORBASeq::Int8Seq& seq)
{
  ACE_UNUSED_ARG(encoding);
  ACE_UNUSED_ARG(size);
  ACE_UNUSED_ARG(seq);
  primitive_serialized_size_ulong(encoding, size);
  if (seq.length() == 0) {
    return;
  }
  primitive_serialized_size_int8(encoding, size, seq.length());
}

bool operator<<(Serializer& strm, const CORBASeq::Int8Seq& seq)
{
  ACE_UNUSED_ARG(strm);
  ACE_UNUSED_ARG(seq);
  const Encoding& encoding = strm.encoding();
  ACE_UNUSED_ARG(encoding);
  const CORBA::ULong length = seq.length();
  if (!(strm << length)) {
    return false;
  }
  if (length == 0) {
    return true;
  }
  return strm.write_int8_array(seq.get_buffer(), length);
}

bool operator>>(Serializer& strm, CORBASeq::Int8Seq& seq)
{
  ACE_UNUSED_ARG(strm);
  ACE_UNUSED_ARG(seq);
  const Encoding& encoding = strm.encoding();
  ACE_UNUSED_ARG(encoding);
  CORBA::ULong length;
  if (!(strm >> length)) {
    return false;
  }
  if (length > strm.length()) {
    if (DCPS_debug_level >= 8) {
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Invalid sequence length (%u)\n"), length));
    }
    return false;
  }
  CORBA::ULong new_length = length;
  seq.length(new_length);
  if (length == 0) {
    return true;
  }
  return strm.read_int8_array(seq.get_buffer(), length);
}

void serialized_size(const Encoding& encoding, size_t& size, const CORBASeq::LongDoubleSeq& seq)
{
  ACE_UNUSED_ARG(encoding);
  ACE_UNUSED_ARG(size);
  ACE_UNUSED_ARG(seq);
  primitive_serialized_size_ulong(encoding, size);
  if (seq.length() == 0) {
    return;
  }
  encoding.align(size, 8);
  primitive_serialized_size(encoding, size, CORBA::LongDouble(), seq.length());
}

bool operator<<(Serializer& strm, const CORBASeq::LongDoubleSeq& seq)
{
  ACE_UNUSED_ARG(strm);
  ACE_UNUSED_ARG(seq);
  const Encoding& encoding = strm.encoding();
  ACE_UNUSED_ARG(encoding);
  const CORBA::ULong length = seq.length();
  if (!(strm << length)) {
    return false;
  }
  if (length == 0) {
    return true;
  }
  return strm.write_longdouble_array(seq.get_buffer(), length);
}

bool operator>>(Serializer& strm, CORBASeq::LongDoubleSeq& seq)
{
  ACE_UNUSED_ARG(strm);
  ACE_UNUSED_ARG(seq);
  const Encoding& encoding = strm.encoding();
  ACE_UNUSED_ARG(encoding);
  CORBA::ULong length;
  if (!(strm >> length)) {
    return false;
  }
  if (length > strm.length()) {
    if (DCPS_debug_level >= 8) {
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Invalid sequence length (%u)\n"), length));
    }
    return false;
  }
  CORBA::ULong new_length = length;
  seq.length(new_length);
  if (length == 0) {
    return true;
  }
  return strm.read_longdouble_array(seq.get_buffer(), length);
}

void serialized_size(const Encoding& encoding, size_t& size, const CORBASeq::LongLongSeq& seq)
{
  ACE_UNUSED_ARG(encoding);
  ACE_UNUSED_ARG(size);
  ACE_UNUSED_ARG(seq);
  primitive_serialized_size_ulong(encoding, size);
  if (seq.length() == 0) {
    return;
  }
  encoding.align(size, 8);
  primitive_serialized_size(encoding, size, CORBA::LongLong(), seq.length());
}

bool operator<<(Serializer& strm, const CORBASeq::LongLongSeq& seq)
{
  ACE_UNUSED_ARG(strm);
  ACE_UNUSED_ARG(seq);
  const Encoding& encoding = strm.encoding();
  ACE_UNUSED_ARG(encoding);
  const CORBA::ULong length = seq.length();
  if (!(strm << length)) {
    return false;
  }
  if (length == 0) {
    return true;
  }
  return strm.write_longlong_array(seq.get_buffer(), length);
}

bool operator>>(Serializer& strm, CORBASeq::LongLongSeq& seq)
{
  ACE_UNUSED_ARG(strm);
  ACE_UNUSED_ARG(seq);
  const Encoding& encoding = strm.encoding();
  ACE_UNUSED_ARG(encoding);
  CORBA::ULong length;
  if (!(strm >> length)) {
    return false;
  }
  if (length > strm.length()) {
    if (DCPS_debug_level >= 8) {
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Invalid sequence length (%u)\n"), length));
    }
    return false;
  }
  CORBA::ULong new_length = length;
  seq.length(new_length);
  if (length == 0) {
    return true;
  }
  return strm.read_longlong_array(seq.get_buffer(), length);
}

void serialized_size(const Encoding& encoding, size_t& size, const CORBASeq::LongSeq& seq)
{
  ACE_UNUSED_ARG(encoding);
  ACE_UNUSED_ARG(size);
  ACE_UNUSED_ARG(seq);
  primitive_serialized_size_ulong(encoding, size);
  if (seq.length() == 0) {
    return;
  }
  primitive_serialized_size(encoding, size, CORBA::Long(), seq.length());
}

bool operator<<(Serializer& strm, const CORBASeq::LongSeq& seq)
{
  ACE_UNUSED_ARG(strm);
  ACE_UNUSED_ARG(seq);
  const Encoding& encoding = strm.encoding();
  ACE_UNUSED_ARG(encoding);
  const CORBA::ULong length = seq.length();
  if (!(strm << length)) {
    return false;
  }
  if (length == 0) {
    return true;
  }
  return strm.write_long_array(seq.get_buffer(), length);
}

bool operator>>(Serializer& strm, CORBASeq::LongSeq& seq)
{
  ACE_UNUSED_ARG(strm);
  ACE_UNUSED_ARG(seq);
  const Encoding& encoding = strm.encoding();
  ACE_UNUSED_ARG(encoding);
  CORBA::ULong length;
  if (!(strm >> length)) {
    return false;
  }
  if (length > strm.length()) {
    if (DCPS_debug_level >= 8) {
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Invalid sequence length (%u)\n"), length));
    }
    return false;
  }
  CORBA::ULong new_length = length;
  seq.length(new_length);
  if (length == 0) {
    return true;
  }
  return strm.read_long_array(seq.get_buffer(), length);
}

void serialized_size(const Encoding& encoding, size_t& size, const CORBASeq::OctetSeq& seq)
{
  ACE_UNUSED_ARG(encoding);
  ACE_UNUSED_ARG(size);
  ACE_UNUSED_ARG(seq);
  primitive_serialized_size_ulong(encoding, size);
  if (seq.length() == 0) {
    return;
  }
  primitive_serialized_size_octet(encoding, size, seq.length());
}

bool operator<<(Serializer& strm, const CORBASeq::OctetSeq& seq)
{
  ACE_UNUSED_ARG(strm);
  ACE_UNUSED_ARG(seq);
  const Encoding& encoding = strm.encoding();
  ACE_UNUSED_ARG(encoding);
  const CORBA::ULong length = seq.length();
  if (!(strm << length)) {
    return false;
  }
  if (length == 0) {
    return true;
  }
  return strm.write_octet_array(seq.get_buffer(), length);
}

bool operator>>(Serializer& strm, CORBASeq::OctetSeq& seq)
{
  ACE_UNUSED_ARG(strm);
  ACE_UNUSED_ARG(seq);
  const Encoding& encoding = strm.encoding();
  ACE_UNUSED_ARG(encoding);
  CORBA::ULong length;
  if (!(strm >> length)) {
    return false;
  }
  if (length > strm.length()) {
    if (DCPS_debug_level >= 8) {
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Invalid sequence length (%u)\n"), length));
    }
    return false;
  }
  CORBA::ULong new_length = length;
  seq.length(new_length);
  if (length == 0) {
    return true;
  }
  return strm.read_octet_array(seq.get_buffer(), length);
}

void serialized_size(const Encoding& encoding, size_t& size, const CORBASeq::ShortSeq& seq)
{
  ACE_UNUSED_ARG(encoding);
  ACE_UNUSED_ARG(size);
  ACE_UNUSED_ARG(seq);
  primitive_serialized_size_ulong(encoding, size);
  if (seq.length() == 0) {
    return;
  }
  primitive_serialized_size(encoding, size, CORBA::Short(), seq.length());
}

bool operator<<(Serializer& strm, const CORBASeq::ShortSeq& seq)
{
  ACE_UNUSED_ARG(strm);
  ACE_UNUSED_ARG(seq);
  const Encoding& encoding = strm.encoding();
  ACE_UNUSED_ARG(encoding);
  const CORBA::ULong length = seq.length();
  if (!(strm << length)) {
    return false;
  }
  if (length == 0) {
    return true;
  }
  return strm.write_short_array(seq.get_buffer(), length);
}

bool operator>>(Serializer& strm, CORBASeq::ShortSeq& seq)
{
  ACE_UNUSED_ARG(strm);
  ACE_UNUSED_ARG(seq);
  const Encoding& encoding = strm.encoding();
  ACE_UNUSED_ARG(encoding);
  CORBA::ULong length;
  if (!(strm >> length)) {
    return false;
  }
  if (length > strm.length()) {
    if (DCPS_debug_level >= 8) {
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Invalid sequence length (%u)\n"), length));
    }
    return false;
  }
  CORBA::ULong new_length = length;
  seq.length(new_length);
  if (length == 0) {
    return true;
  }
  return strm.read_short_array(seq.get_buffer(), length);
}

void serialized_size(const Encoding& encoding, size_t& size, const CORBASeq::StringSeq& seq)
{
  ACE_UNUSED_ARG(encoding);
  ACE_UNUSED_ARG(size);
  ACE_UNUSED_ARG(seq);
  if (encoding.xcdr_version() == Encoding::XCDR_VERSION_2) {
    serialized_size_delimiter(encoding, size);
  }
  primitive_serialized_size_ulong(encoding, size);
  if (seq.length() == 0) {
    return;
  }
  for (CORBA::ULong i = 0; i < seq.length(); ++i) {
    primitive_serialized_size_ulong(encoding, size);
    if (seq[i]) {
      size += ACE_OS::strlen(seq[i]) + 1;
    }
  }
}

bool operator<<(Serializer& strm, const CORBASeq::StringSeq& seq)
{
  ACE_UNUSED_ARG(strm);
  ACE_UNUSED_ARG(seq);
  const Encoding& encoding = strm.encoding();
  ACE_UNUSED_ARG(encoding);
  size_t total_size = 0;
  if (encoding.xcdr_version() == Encoding::XCDR_VERSION_2) {
    serialized_size(encoding, total_size, seq);
    if (!strm.write_delimiter(total_size)) {
      return false;
    }
  }
  const CORBA::ULong length = seq.length();
  if (!(strm << length)) {
    return false;
  }
  if (length == 0) {
    return true;
  }
  for (CORBA::ULong i = 0; i < length; ++i) {
    if (!(strm << seq[i])) {
      return false;
    }
  }
  return true;
}

bool operator>>(Serializer& strm, CORBASeq::StringSeq& seq)
{
  ACE_UNUSED_ARG(strm);
  ACE_UNUSED_ARG(seq);
  const Encoding& encoding = strm.encoding();
  ACE_UNUSED_ARG(encoding);
  size_t total_size = 0;
  if (encoding.xcdr_version() == Encoding::XCDR_VERSION_2) {
    if (!strm.read_delimiter(total_size)) {
      return false;
    }
  }
  const size_t end_of_seq = strm.rpos() + total_size;
  CORBA::ULong length;
  if (!(strm >> length)) {
    return false;
  }
  if (length > strm.length()) {
    if (DCPS_debug_level >= 8) {
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Invalid sequence length (%u)\n"), length));
    }
    return false;
  }
  CORBA::ULong new_length = length;
  seq.length(new_length);
  for (CORBA::ULong i = 0; i < new_length; ++i) {
     if (!(strm >> seq[i])) {
      strm.set_construction_status(Serializer::ElementConstructionFailure);
      if (encoding.xcdr_version() == Encoding::XCDR_VERSION_2) {
        strm.skip(end_of_seq - strm.rpos());
      } else {
        CORBASeq::StringSeq tempvar;
        tempvar.length(1);
        for (CORBA::ULong j = i + 1; j < length; ++j) {
          strm >> tempvar[0];
        }
      }
      return false;
    }
  }
  if (new_length != length) {
    if (encoding.xcdr_version() == Encoding::XCDR_VERSION_2) {
      strm.skip(end_of_seq - strm.rpos());
    } else {
      CORBASeq::StringSeq tempvar;
      tempvar.length(1);
      for (CORBA::ULong j = new_length + 1; j < length; ++j) {
        strm >> tempvar[0];
      }
    }
    strm.set_construction_status(Serializer::BoundConstructionFailure);
    return false;
  }
  return true;
}

void serialized_size(const Encoding& encoding, size_t& size, const CORBASeq::UInt8Seq& seq)
{
  ACE_UNUSED_ARG(encoding);
  ACE_UNUSED_ARG(size);
  ACE_UNUSED_ARG(seq);
  primitive_serialized_size_ulong(encoding, size);
  if (seq.length() == 0) {
    return;
  }
  primitive_serialized_size_uint8(encoding, size, seq.length());
}

bool operator<<(Serializer& strm, const CORBASeq::UInt8Seq& seq)
{
  ACE_UNUSED_ARG(strm);
  ACE_UNUSED_ARG(seq);
  const Encoding& encoding = strm.encoding();
  ACE_UNUSED_ARG(encoding);
  const CORBA::ULong length = seq.length();
  if (!(strm << length)) {
    return false;
  }
  if (length == 0) {
    return true;
  }
  return strm.write_uint8_array(seq.get_buffer(), length);
}

bool operator>>(Serializer& strm, CORBASeq::UInt8Seq& seq)
{
  ACE_UNUSED_ARG(strm);
  ACE_UNUSED_ARG(seq);
  const Encoding& encoding = strm.encoding();
  ACE_UNUSED_ARG(encoding);
  CORBA::ULong length;
  if (!(strm >> length)) {
    return false;
  }
  if (length > strm.length()) {
    if (DCPS_debug_level >= 8) {
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Invalid sequence length (%u)\n"), length));
    }
    return false;
  }
  CORBA::ULong new_length = length;
  seq.length(new_length);
  if (length == 0) {
    return true;
  }
  return strm.read_uint8_array(seq.get_buffer(), length);
}

void serialized_size(const Encoding& encoding, size_t& size, const CORBASeq::ULongLongSeq& seq)
{
  ACE_UNUSED_ARG(encoding);
  ACE_UNUSED_ARG(size);
  ACE_UNUSED_ARG(seq);
  primitive_serialized_size_ulong(encoding, size);
  if (seq.length() == 0) {
    return;
  }
  encoding.align(size, 8);
  primitive_serialized_size(encoding, size, CORBA::ULongLong(), seq.length());
}

bool operator<<(Serializer& strm, const CORBASeq::ULongLongSeq& seq)
{
  ACE_UNUSED_ARG(strm);
  ACE_UNUSED_ARG(seq);
  const Encoding& encoding = strm.encoding();
  ACE_UNUSED_ARG(encoding);
  const CORBA::ULong length = seq.length();
  if (!(strm << length)) {
    return false;
  }
  if (length == 0) {
    return true;
  }
  return strm.write_ulonglong_array(seq.get_buffer(), length);
}

bool operator>>(Serializer& strm, CORBASeq::ULongLongSeq& seq)
{
  ACE_UNUSED_ARG(strm);
  ACE_UNUSED_ARG(seq);
  const Encoding& encoding = strm.encoding();
  ACE_UNUSED_ARG(encoding);
  CORBA::ULong length;
  if (!(strm >> length)) {
    return false;
  }
  if (length > strm.length()) {
    if (DCPS_debug_level >= 8) {
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Invalid sequence length (%u)\n"), length));
    }
    return false;
  }
  CORBA::ULong new_length = length;
  seq.length(new_length);
  if (length == 0) {
    return true;
  }
  return strm.read_ulonglong_array(seq.get_buffer(), length);
}

void serialized_size(const Encoding& encoding, size_t& size, const CORBASeq::ULongSeq& seq)
{
  ACE_UNUSED_ARG(encoding);
  ACE_UNUSED_ARG(size);
  ACE_UNUSED_ARG(seq);
  primitive_serialized_size_ulong(encoding, size);
  if (seq.length() == 0) {
    return;
  }
  primitive_serialized_size(encoding, size, CORBA::ULong(), seq.length());
}

bool operator<<(Serializer& strm, const CORBASeq::ULongSeq& seq)
{
  ACE_UNUSED_ARG(strm);
  ACE_UNUSED_ARG(seq);
  const Encoding& encoding = strm.encoding();
  ACE_UNUSED_ARG(encoding);
  const CORBA::ULong length = seq.length();
  if (!(strm << length)) {
    return false;
  }
  if (length == 0) {
    return true;
  }
  return strm.write_ulong_array(seq.get_buffer(), length);
}

bool operator>>(Serializer& strm, CORBASeq::ULongSeq& seq)
{
  ACE_UNUSED_ARG(strm);
  ACE_UNUSED_ARG(seq);
  const Encoding& encoding = strm.encoding();
  ACE_UNUSED_ARG(encoding);
  CORBA::ULong length;
  if (!(strm >> length)) {
    return false;
  }
  if (length > strm.length()) {
    if (DCPS_debug_level >= 8) {
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Invalid sequence length (%u)\n"), length));
    }
    return false;
  }
  CORBA::ULong new_length = length;
  seq.length(new_length);
  if (length == 0) {
    return true;
  }
  return strm.read_ulong_array(seq.get_buffer(), length);
}

void serialized_size(const Encoding& encoding, size_t& size, const CORBASeq::UShortSeq& seq)
{
  ACE_UNUSED_ARG(encoding);
  ACE_UNUSED_ARG(size);
  ACE_UNUSED_ARG(seq);
  primitive_serialized_size_ulong(encoding, size);
  if (seq.length() == 0) {
    return;
  }
  primitive_serialized_size(encoding, size, CORBA::UShort(), seq.length());
}

bool operator<<(Serializer& strm, const CORBASeq::UShortSeq& seq)
{
  ACE_UNUSED_ARG(strm);
  ACE_UNUSED_ARG(seq);
  const Encoding& encoding = strm.encoding();
  ACE_UNUSED_ARG(encoding);
  const CORBA::ULong length = seq.length();
  if (!(strm << length)) {
    return false;
  }
  if (length == 0) {
    return true;
  }
  return strm.write_ushort_array(seq.get_buffer(), length);
}

bool operator>>(Serializer& strm, CORBASeq::UShortSeq& seq)
{
  ACE_UNUSED_ARG(strm);
  ACE_UNUSED_ARG(seq);
  const Encoding& encoding = strm.encoding();
  ACE_UNUSED_ARG(encoding);
  CORBA::ULong length;
  if (!(strm >> length)) {
    return false;
  }
  if (length > strm.length()) {
    if (DCPS_debug_level >= 8) {
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Invalid sequence length (%u)\n"), length));
    }
    return false;
  }
  CORBA::ULong new_length = length;
  seq.length(new_length);
  if (length == 0) {
    return true;
  }
  return strm.read_ushort_array(seq.get_buffer(), length);
}

} }
OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif
