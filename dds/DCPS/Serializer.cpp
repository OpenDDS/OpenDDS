/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "Serializer.h"

#if !defined (__ACE_INLINE__)
# include "Serializer.inl"
#endif /* !__ACE_INLINE__ */

#include "SafetyProfileStreams.h"

#ifndef NOTAO
#include <tao/String_Alloc.h>
#endif

#include <ace/OS_NS_string.h>
#include <ace/OS_Memory.h>

#include <cstdlib>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

#ifdef NOTAO
namespace {

ACE_CDR::Char* string_alloc(ACE_CDR::ULong size)
{
  return static_cast<ACE_CDR::Char*>(std::malloc(size + 1));
}

void string_free(ACE_CDR::Char* ptr)
{
  std::free(ptr);
}

ACE_CDR::WChar* wstring_alloc(ACE_CDR::ULong size)
{
  return static_cast<ACE_CDR::WChar*>(std::malloc(sizeof(ACE_CDR::WChar) * (size + 1)));
}

void wstring_free(ACE_CDR::WChar* ptr)
{
  std::free(ptr);
}

}
#endif

String endianness_to_string(Endianness endianness)
{
  switch (endianness) {
  case ENDIAN_BIG:
    return "big-endian ("
#ifdef ACE_LITTLE_ENDIAN
      "non-"
#endif
      "native)";
  case ENDIAN_LITTLE:
    return "little-endian ("
#ifndef ACE_LITTLE_ENDIAN
      "non-"
#endif
      "native)";
  default:
    return "invalid endianness";
  }
}

Encoding::Encoding()
  : endianness_(ENDIAN_NATIVE)
  , skip_sequence_dheader_(false)
{
  kind(KIND_XCDR1);
}

Encoding::Encoding(Kind k, Endianness endianness)
  : endianness_(endianness)
  , skip_sequence_dheader_(false)
{
  kind(k);
}

Encoding::Encoding(Kind k, bool swap_bytes)
  : endianness_(swap_bytes ? ENDIAN_NONNATIVE : ENDIAN_NATIVE)
  , skip_sequence_dheader_(false)
{
  kind(k);
}

EncapsulationHeader::EncapsulationHeader(Kind kind, ACE_CDR::UShort options)
  : kind_(kind)
  , options_(options)
{
}

EncapsulationHeader::EncapsulationHeader(const Encoding& enc, Extensibility ext, ACE_CDR::UShort options)
  : kind_(KIND_INVALID)
  , options_(options)
{
   if (!from_encoding(enc, ext)) {
     kind_ = KIND_INVALID;
   }
}


bool EncapsulationHeader::from_encoding(
  const Encoding& encoding, Extensibility extensibility)
{
  const bool big = encoding.endianness() == ENDIAN_BIG;
  switch (encoding.kind()) {
  case Encoding::KIND_XCDR1:
    switch (extensibility) {
    case FINAL:
    case APPENDABLE:
      kind_ = big ? KIND_CDR_BE : KIND_CDR_LE;
      break;
    case MUTABLE:
      kind_ = big ? KIND_PL_CDR_BE : KIND_PL_CDR_LE;
      break;
    }
    break;
  case Encoding::KIND_XCDR2:
    switch (extensibility) {
    case FINAL:
      kind_ = big ? KIND_CDR2_BE : KIND_CDR2_LE;
      break;
    case APPENDABLE:
      kind_ = big ? KIND_D_CDR2_BE : KIND_D_CDR2_LE;
      break;
    case MUTABLE:
      kind_ = big ? KIND_PL_CDR2_BE : KIND_PL_CDR2_LE;
      break;
    }
    break;
  default:
    if (DCPS_debug_level > 0) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR EncapsulationHeader::from_encoding: ")
        ACE_TEXT("Got Encoding With Unsupported Kind: %C\n"),
        Encoding::kind_to_string(encoding.kind()).c_str()));
    }
    return false;
  }
  return true;
}

bool EncapsulationHeader::to_encoding(
  Encoding& encoding, Extensibility expected_extensibility)
{
  bool wrong_extensibility = true;
  switch (kind_) {
  case KIND_CDR_BE:
    encoding.kind(Encoding::KIND_XCDR1);
    encoding.endianness(ENDIAN_BIG);
    wrong_extensibility = expected_extensibility == MUTABLE;
    break;

  case KIND_CDR_LE:
    encoding.kind(Encoding::KIND_XCDR1);
    encoding.endianness(ENDIAN_LITTLE);
    wrong_extensibility = expected_extensibility == MUTABLE;
    break;

  case KIND_PL_CDR_BE:
    encoding.kind(Encoding::KIND_XCDR1);
    encoding.endianness(ENDIAN_BIG);
    wrong_extensibility = expected_extensibility != MUTABLE;
    break;

  case KIND_PL_CDR_LE:
    encoding.kind(Encoding::KIND_XCDR1);
    encoding.endianness(ENDIAN_LITTLE);
    wrong_extensibility = expected_extensibility != MUTABLE;
    break;

  case KIND_CDR2_BE:
    encoding.kind(Encoding::KIND_XCDR2);
    encoding.endianness(ENDIAN_BIG);
    wrong_extensibility = expected_extensibility != FINAL;
    break;

  case KIND_CDR2_LE:
    encoding.kind(Encoding::KIND_XCDR2);
    encoding.endianness(ENDIAN_LITTLE);
    wrong_extensibility = expected_extensibility != FINAL;
    break;

  case KIND_D_CDR2_BE:
    encoding.kind(Encoding::KIND_XCDR2);
    encoding.endianness(ENDIAN_BIG);
    wrong_extensibility = expected_extensibility != APPENDABLE;
    break;

  case KIND_D_CDR2_LE:
    encoding.kind(Encoding::KIND_XCDR2);
    encoding.endianness(ENDIAN_LITTLE);
    wrong_extensibility = expected_extensibility != APPENDABLE;
    break;

  case KIND_PL_CDR2_BE:
    encoding.kind(Encoding::KIND_XCDR2);
    encoding.endianness(ENDIAN_BIG);
    wrong_extensibility = expected_extensibility != MUTABLE;
    break;

  case KIND_PL_CDR2_LE:
    encoding.kind(Encoding::KIND_XCDR2);
    encoding.endianness(ENDIAN_LITTLE);
    wrong_extensibility = expected_extensibility != MUTABLE;
    break;

  default:
    if (DCPS_debug_level > 0) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR EncapsulationHeader::to_encoding: ")
        ACE_TEXT("Unsupported Encoding: %C\n"), to_string().c_str()));
    }
    return false;
  }

  if (wrong_extensibility) {
    if (DCPS_debug_level > 0) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR EncapsulationHeader::to_encoding: ")
        ACE_TEXT("Unexpected Extensibility Encoding: %C\n"),
        to_string().c_str()));
    }
    return false;
  }

  return true;
}

bool EncapsulationHeader::set_encapsulation_options(Message_Block_Ptr& mb)
{
  if (mb->length() < padding_marker_byte_index + 1) {
    if (DCPS_debug_level > 0) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR EncapsulationHeader::set_encapsulation_options: ")
        ACE_TEXT("Insufficient buffer size %d\n"), mb->length()));
    }
    return false;
  }

  mb->rd_ptr()[padding_marker_byte_index] |= ((padding_marker_alignment - mb->length() % padding_marker_alignment) & 0x03);
  return true;
}

String EncapsulationHeader::to_string() const
{
  switch (kind_) {
  case KIND_CDR_BE:
    return "CDR/XCDR1 Big Endian Plain";
  case KIND_CDR_LE:
    return "CDR/XCDR1 Little Endian Plain";
  case KIND_PL_CDR_BE:
    return "CDR/XCDR1 Big Endian Parameter List";
  case KIND_PL_CDR_LE:
    return "CDR/XCDR1 Little Endian Parameter List";
  case KIND_CDR2_BE:
    return "XCDR2 Big Endian Plain";
  case KIND_CDR2_LE:
    return "XCDR2 Little Endian Plain";
  case KIND_D_CDR2_BE:
    return "XCDR2 Big Endian Delimited";
  case KIND_D_CDR2_LE:
    return "XCDR2 Little Endian Delimited";
  case KIND_PL_CDR2_BE:
    return "XCDR2 Big Endian Parameter List";
  case KIND_PL_CDR2_LE:
    return "XCDR2 Little Endian Parameter List";
  case KIND_XML:
    return "XML";
  case KIND_INVALID:
    return "Invalid";
  default:
    return "Unknown: " + to_dds_string(static_cast<unsigned>(kind_), true);
  }
}

bool operator>>(Serializer& s, EncapsulationHeader& value)
{
  ACE_CDR::Octet data[EncapsulationHeader::serialized_size];
  if (!s.read_octet_array(&data[0], EncapsulationHeader::serialized_size)) {
    return false;
  }
  value.kind(static_cast<EncapsulationHeader::Kind>(
    (static_cast<ACE_UINT16>(data[0]) << 8) | data[1]));
  value.options((static_cast<ACE_UINT16>(data[2]) << 8) | data[3]);
  s.reset_alignment();
  return true;
}

bool operator<<(Serializer& s, const EncapsulationHeader& value)
{
  if (!value.is_good()) {
    return false;
  }
  ACE_CDR::Octet data[EncapsulationHeader::serialized_size];
  data[0] = (value.kind() >> 8) & 0xff;
  data[1] = value.kind() & 0xff;
  data[2] = (value.options() >> 8) & 0xff;
  data[3] = value.options() & 0xff;
  const bool ok = s.write_octet_array(&data[0],
    EncapsulationHeader::serialized_size);
  s.reset_alignment();
  return ok;
}

String Encoding::kind_to_string(Kind value)
{
  switch (value) {
  case KIND_XCDR1:
    return "CDR/XCDR1";
  case KIND_XCDR2:
    return "XCDR2";
  case KIND_UNALIGNED_CDR:
    return "Unaligned CDR";
  default:
    return "Unknown: " + to_dds_string(static_cast<unsigned>(value), true);
  }
}

String Encoding::to_string() const
{
  String rv = Encoding::kind_to_string(kind_) + ", " +
    endianness_to_string(endianness_);
  if (!zero_init_padding_) {
    rv += ", non-initialized padding";
  }
  return rv;
}

const char Serializer::ALIGN_PAD[] = {0};

Serializer::Serializer(ACE_Message_Block* chain, const Encoding& enc)
  : current_(chain)
  , good_bit_(true)
  , construction_status_(ConstructionSuccessful)
  , align_rshift_(0)
  , align_wshift_(0)
  , rpos_(0)
  , wpos_(0)
{
  encoding(enc);
  reset_alignment();
}

Serializer::Serializer(ACE_Message_Block* chain, Encoding::Kind kind,
  Endianness endianness)
  : current_(chain)
  , good_bit_(true)
  , construction_status_(ConstructionSuccessful)
  , align_rshift_(0)
  , align_wshift_(0)
  , rpos_(0)
  , wpos_(0)
{
  encoding(Encoding(kind, endianness));
  reset_alignment();
}

Serializer::Serializer(ACE_Message_Block* chain,
  Encoding::Kind kind, bool swap_bytes)
  : current_(chain)
  , good_bit_(true)
  , construction_status_(ConstructionSuccessful)
  , align_rshift_(0)
  , align_wshift_(0)
  , rpos_(0)
  , wpos_(0)
{
  encoding(Encoding(kind, swap_bytes));
  reset_alignment();
}

Serializer::~Serializer()
{
}

Serializer::ScopedAlignmentContext::ScopedAlignmentContext(Serializer& ser, size_t min_read)
  : ser_(ser)
  , max_align_(ser.encoding().max_align())
  , start_rpos_(ser.rpos())
  , rblock_(max_align_ ? (ptrdiff_t(ser.current_->rd_ptr()) - ser.align_rshift_) % max_align_ : 0)
  , min_read_(min_read)
  , start_wpos_(ser.wpos())
  , wblock_(max_align_ ? (ptrdiff_t(ser.current_->wr_ptr()) - ser.align_wshift_) % max_align_ : 0)
{
  ser_.reset_alignment();
}

void
Serializer::ScopedAlignmentContext::restore(Serializer& ser) const
{
  if (min_read_ != 0 && (ser.rpos() - start_rpos_) < min_read_) {
    ser.skip(min_read_ - (ser.rpos() - start_rpos_));
  }

  if (ser.current_ && max_align_) {
    ser.align_rshift_ = offset(ser.current_->rd_ptr(), ser.rpos() - start_rpos_ + rblock_, max_align_);
    ser.align_wshift_ = offset(ser.current_->wr_ptr(), ser.wpos() - start_wpos_ + wblock_, max_align_);
  }
}

bool
Serializer::peek(ACE_CDR::ULong& t)
{
  // save
  const size_t rpos = rpos_;
  const unsigned char align_rshift = align_rshift_;
  ACE_Message_Block* const current = current_;

  // read
  if (!peek_helper(current_, 2 * uint32_cdr_size, t)) {
    return false;
  }

  // reset
  current_ = current;
  align_rshift_ = align_rshift;
  rpos_ = rpos;
  return true;
}

void
Serializer::reset_alignment()
{
  const ptrdiff_t align = encoding().max_align();
  if (current_ && align) {
    align_rshift_ = offset(current_->rd_ptr(), 0, align);
    align_wshift_ = offset(current_->wr_ptr(), 0, align);
  }
}

void
Serializer::smemcpy(char* to, const char* from, size_t n)
{
  OPENDDS_ASSERT(from);
  (void) ACE_OS::memcpy(to, from, n);
}

void
Serializer::swapcpy(char* to, const char* from, size_t n)
{
  // Unroll the loop...
  switch (n) {               // 2   4   8   16
  case 16:
    to[ 15] = from[ n - 16]; // x   x   x    0
    // fallthrough
  case 15:
    to[ 14] = from[ n - 15]; // x   x   x    1
    // fallthrough
  case 14:
    to[ 13] = from[ n - 14]; // x   x   x    2
    // fallthrough
  case 13:
    to[ 12] = from[ n - 13]; // x   x   x    3
    // fallthrough
  case 12:
    to[ 11] = from[ n - 12]; // x   x   x    4
    // fallthrough
  case 11:
    to[ 10] = from[ n - 11]; // x   x   x    5
    // fallthrough
  case 10:
    to[  9] = from[ n - 10]; // x   x   x    6
    // fallthrough
  case  9:
    to[  8] = from[ n -  9]; // x   x   x    7
    // fallthrough
  case  8:
    to[  7] = from[ n -  8]; // x   x   0    8
    // fallthrough
  case  7:
    to[  6] = from[ n -  7]; // x   x   1    9
    // fallthrough
  case  6:
    to[  5] = from[ n -  6]; // x   x   2   10
    // fallthrough
  case  5:
    to[  4] = from[ n -  5]; // x   x   3   11
    // fallthrough
  case  4:
    to[  3] = from[ n -  4]; // x   0   4   12
    // fallthrough
  case  3:
    to[  2] = from[ n -  3]; // x   1   5   13
    // fallthrough
  case  2:
    to[  1] = from[ n -  2]; // 0   2   6   14
    // fallthrough
  case  1:
    to[  0] = from[ n -  1]; // 1   3   7   15
    // fallthrough
  case  0:
    return;
  default:
    good_bit_ = false;
  }
}

size_t
Serializer::read_string(ACE_CDR::Char*& dest,
                        StrAllocate str_alloc,
                        StrFree str_free)
{
  if (str_alloc == 0) {
#ifdef NOTAO
    str_alloc = string_alloc;
#else
    str_alloc = CORBA::string_alloc;
#endif
  }

  //
  // Ensure no bad values leave the routine.
  //
  free_string(dest, str_free);
  dest = 0;

  //
  // Extract the string length.
  //
  ACE_CDR::ULong length; // includes the null
  if (!(*this >> length)) {
    return 0;
  }

  if (length == 0) {
    // not legal CDR, but we need to accept it since other implementations may generate this
    dest = str_alloc(0);
    return 0;
  }

  //
  // NOTE: Maintain the ACE implementation where the length check is
  //       done here before the allocation even though it will be
  //       checked during the actual read as well.
  //
  if (length <= current_->total_length()) {

    dest = str_alloc(length - 1);

    if (dest == 0) {
      good_bit_ = false;

    } else {
      //
      // Extract the string.
      //
      read_char_array(dest, length);
    }

    if (!good_bit_) {
      free_string(dest, str_free);
      dest = 0;
    }

  } else {
    good_bit_ = false;
  }

  return length - 1;
}

void
Serializer::free_string(ACE_CDR::Char* str,
                        StrFree str_free)
{
  if (str_free == 0) {
#ifdef NOTAO
    str_free = string_free;
#else
    str_free = CORBA::string_free;
#endif
  }
  str_free(str);
}

size_t
Serializer::read_string(ACE_CDR::WChar*& dest,
                        WStrAllocate str_alloc,
                        WStrFree str_free)
{
  if (str_alloc == 0) {
#ifdef NOTAO
    str_alloc = wstring_alloc;
#else
    str_alloc = CORBA::wstring_alloc;
#endif
  }

  //
  // Ensure no bad values leave the routine.
  //
  free_string(dest, str_free);
  dest = 0;

  //
  // Extract the string size.
  //
  ACE_CDR::ULong bytecount;
  if (!(*this >> bytecount)) {
    return 0;
  }

  //
  // NOTE: Maintain the ACE implementation where the length check is
  //       done here before the allocation even though it will be
  //       checked during the actual read as well.
  //
  ACE_CDR::ULong length = 0;
  if (bytecount <= current_->total_length()) {
    length = bytecount / char16_cdr_size;
    dest = str_alloc(length);

    if (dest == 0) {
      good_bit_ = false;
      return 0;
    }

#if ACE_SIZEOF_WCHAR == 2
    read_array(reinterpret_cast<char*>(dest), char16_cdr_size, length, swap_bytes());
#else
    for (size_t i = 0; i < length && good_bit_; ++i) {
      ACE_UINT16 as_utf16;
      buffer_read(reinterpret_cast<char*>(&as_utf16), char16_cdr_size, swap_bytes());
      if (good_bit_) {
        dest[i] = as_utf16;
      }
    }
#endif

    if (good_bit_) {
      //
      // Null terminate the string.
      //
      dest[length] = L'\0';
    } else {
      free_string(dest, str_free);
      dest = 0;
      length = 0;
    }

  } else {
    good_bit_ = false;
  }

  return length;
}

void
Serializer::free_string(ACE_CDR::WChar* str,
                        WStrFree str_free)
{
  if (str_free == 0) {
#ifdef NOTAO
    str_free = wstring_free;
#else
    str_free = CORBA::wstring_free;
#endif
  }
  str_free(str);
}

bool Serializer::read_parameter_id(unsigned& id, size_t& size, bool& must_understand)
{
  const Encoding::XcdrVersion xcdr = encoding().xcdr_version();
  if (xcdr == Encoding::XCDR_VERSION_1) {
    // Get the "short" id and size
    if (!align_r(xcdr1_pid_alignment)) {
      return false;
    }
    ACE_CDR::UShort pid;
    if (!(*this >> pid)) {
      return false;
    }
    const ACE_CDR::UShort short_id = pid & 0x3fff;
    ACE_CDR::UShort short_size;
    if (!(*this >> short_size)) {
      return false;
    }

    // TODO(iguessthislldo): handle PID flags
    must_understand = false; // Placeholder

    // If extended, get the "long" id and size
    if (short_id == pid_extended) {
      ACE_CDR::ULong long_id, long_size;
      if (!(*this >> long_id) || !(*this >> long_size)) {
        return false;
      }
      const unsigned short_size_left = short_size - 8;
      if (short_size_left) {
        skip(short_size_left);
      }
      id = long_id;
      size = long_size;
    } else {
      id = short_id;
      size = short_size;
    }

    reset_alignment();
  } else if (xcdr == Encoding::XCDR_VERSION_2) {
    ACE_CDR::ULong emheader;
    if (!(*this >> emheader)) {
      return false;
    }

    must_understand = emheader & emheader_must_understand;

    // Get Size
    const unsigned short lc = (emheader >> 28) & 0x7;
    if (lc < 4) {
      size = size_t(1) << lc;
    } else {
      ACE_CDR::ULong next_int;
      if (lc == 4 ? !(*this >> next_int) : !peek(next_int)) {
        return false;
      }
      if (lc == 6) {
        size = next_int * 4;
      } else if (lc == 7) {
        size = next_int * 8;
      } else { // 4 or 5
        size = next_int;
      }
    }
    id = emheader & 0xfffffff;
  }

  return true;
}

bool Serializer::write_parameter_id(const unsigned id, const size_t size, const bool must_understand)
{
  const Encoding::XcdrVersion xcdr = encoding().xcdr_version();
  if (xcdr == Encoding::XCDR_VERSION_1) {
    if (!align_w(xcdr1_pid_alignment)) {
      return false;
    }

    // Determine if we need to use a short or long PID
    const bool long_pid = id > (1 << 14) || size > (1 << 16);

    // Write the short part of the PID
    /*
     * TODO(iguessthislldo): Control when to use "must understand" and "impl
     * extension"?
     *
     * The XTypes CDR rules seem to imply they're alway here but that doesn't
     * sound quite right.
     *
     * Also see TODOs above and the TODO below.
     */
    const ACE_CDR::UShort pid_id =
      pid_impl_extension + pid_must_understand +
      (long_pid ? pid_extended : static_cast<ACE_CDR::UShort>(id));
    if (!(*this << pid_id)) {
      return false;
    }
    const ACE_CDR::UShort pid_size = long_pid ? 8 : ACE_CDR::UShort(size);
    if (!(*this << pid_size)) {
      return false;
    }

    // If PID is long, write the extended/long part.
    if (long_pid && (
          !(*this << static_cast<ACE_CDR::ULong>(id)) ||
          !(*this << static_cast<ACE_CDR::ULong>(size)))) {
      return false;
    }

    reset_alignment();
  } else if (xcdr == Encoding::XCDR_VERSION_2) {
    // Compute Length Code, write EM Header and NEXTINT
    const ACE_CDR::ULong lc = (size == 1 ? 0 : size == 2 ? 1 : size == 4 ? 2 : size == 8 ? 3 : 4);
    const ACE_CDR::ULong emheader = (lc << 28) | id | (must_understand ? emheader_must_understand : 0);
    if (!(*this << emheader)) {
      return false;
    }
    if (lc == 4) {
      return *this << ACE_CDR::ULong(size);
    }
  }
  return true;
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
