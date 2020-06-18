/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/

#include "Serializer.h"

#include "SafetyProfileStreams.h"

#include <tao/String_Alloc.h>

#include <ace/OS_NS_string.h>
#include <ace/OS_Memory.h>

#if !defined (__ACE_INLINE__)
# include "Serializer.inl"
#endif /* !__ACE_INLINE__ */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

OPENDDS_STRING endianness_to_string(Endianness endianness)
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
{
  kind(KIND_XCDR1);
}

Encoding::Encoding(Encoding::Kind kind, Endianness endianness)
: endianness_(endianness)
{
  this->kind(kind);
}

Encoding::Encoding(Encoding::Kind kind, bool swap_bytes)
: endianness_(swap_bytes ? ENDIAN_NONNATIVE : ENDIAN_NATIVE)
{
  this->kind(kind);
}

EncapsulationHeader::EncapsulationHeader()
: kind_(KIND_CDR_BE)
, options_(0)
{
}

// TODO(iguessthidlldo) Support End Padding Described By XTypes 7.6.3.1.2
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
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR EncapsulationHeader::from_encoding: ")
      ACE_TEXT("Got Encoding With Unsupported Kind: %s\n"),
      Encoding::kind_to_string(encoding.kind()).c_str()));
    return false;
  }
  return true;
}

// TODO(iguessthidlldo) Support End Padding Described By XTypes 7.6.3.1.2
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
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR EncapsulationHeader::to_encoding: ")
      ACE_TEXT("Unsupported Encoding: %C\n"), to_string().c_str()));
    return false;
  }

  if (wrong_extensibility) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR EncapsulationHeader::to_encoding: ")
      ACE_TEXT("Unexpected Extensibility Encoding: %C\n"),
      to_string().c_str()));
    return false;
  }

  return true;
}

OPENDDS_STRING EncapsulationHeader::to_string() const
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
  value.options(static_cast<EncapsulationHeader::Kind>(
    (static_cast<ACE_UINT16>(data[2]) << 8) | data[3]));
  s.reset_alignment();
  return true;
}

bool operator<<(Serializer& s, const EncapsulationHeader& value)
{
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

OPENDDS_STRING Encoding::kind_to_string(Kind value)
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

OPENDDS_STRING Encoding::to_string() const
{
  OPENDDS_STRING rv = Encoding::kind_to_string(kind_) + ", " +
    endianness_to_string(endianness_);
  if (!zero_init_padding_ ) {
    rv += ", non-initialized padding";
  }
  return rv;
}

const char Serializer::ALIGN_PAD[] = {0};

Serializer::Serializer(ACE_Message_Block* chain, const Encoding& encoding)
  : current_(chain)
  , good_bit_(true)
  , align_rshift_(0)
  , align_wshift_(0)
{
  this->encoding(encoding);
  reset_alignment();
}

Serializer::Serializer(ACE_Message_Block* chain, Encoding::Kind kind,
  Endianness endianness)
  : current_(chain)
  , good_bit_(true)
  , align_rshift_(0)
  , align_wshift_(0)
{
  encoding(Encoding(kind, endianness));
  reset_alignment();
}

Serializer::Serializer(ACE_Message_Block* chain,
  Encoding::Kind kind, bool swap_bytes)
  : current_(chain)
  , good_bit_(true)
  , align_rshift_(0)
  , align_wshift_(0)
{
  encoding(Encoding(kind, swap_bytes));
  reset_alignment();
}

Serializer::~Serializer()
{
}

void
Serializer::reset_alignment()
{
  const ptrdiff_t align = encoding().max_align();
  if (current_ && align) {
    align_rshift_ = ptrdiff_t(current_->rd_ptr()) % align;
    align_wshift_ = ptrdiff_t(current_->wr_ptr()) % align;
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
    this->good_bit_ = false;
  }
}

size_t
Serializer::read_string(ACE_CDR::Char*& dest,
    ACE_CDR::Char* str_alloc(ACE_CDR::ULong),
    void str_free(ACE_CDR::Char*))
{
  //
  // Ensure no bad values leave the routine.
  //
  str_free(dest);
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
  if (length <= this->current_->total_length()) {

    dest = str_alloc(length - 1);

    if (dest == 0) {
      this->good_bit_ = false;

    } else {
      //
      // Extract the string.
      //
      this->read_char_array(dest, length);
    }

    if (!this->good_bit_) {
      str_free(dest);
      dest = 0;
    }

  } else {
    good_bit_ = false;
  }

  return length - 1;
}

size_t
Serializer::read_string(ACE_CDR::WChar*& dest,
    ACE_CDR::WChar* str_alloc(ACE_CDR::ULong),
    void str_free(ACE_CDR::WChar*))
{
  //
  // Ensure no bad values leave the routine.
  //
  str_free(dest);
  dest = 0;

  //
  // Extract the string size.
  //
  ACE_CDR::ULong bytecount; // includes the null
  if (!(*this >> bytecount)) {
    return 0;
  }

  //
  // NOTE: Maintain the ACE implementation where the length check is
  //       done here before the allocation even though it will be
  //       checked during the actual read as well.
  //
  ACE_CDR::ULong length = 0;
  if (bytecount <= this->current_->total_length()) {
    length = bytecount / char16_cdr_size;
    dest = str_alloc(length);

    if (dest == 0) {
      this->good_bit_ = false;
      return 0;
    }

#if ACE_SIZEOF_WCHAR == 2
    read_array(reinterpret_cast<char*>(dest), char16_cdr_size, length, swap_bytes());
#else
    for (size_t i = 0; i < length && this->good_bit_; ++i) {
      ACE_UINT16 as_utf16;
      buffer_read(reinterpret_cast<char*>(&as_utf16), char16_cdr_size, swap_bytes());
      if (this->good_bit_) {
        dest[i] = as_utf16;
      }
    }
#endif

    if (this->good_bit_) {
      //
      // Null terminate the string.
      //
      dest[length] = L'\0';
    } else {
      str_free(dest);
      dest = 0;
      length = 0;
    }

  } else {
    good_bit_ = false;
  }

  return length;
}

bool Serializer::read_parameter_id(unsigned& id, size_t& size)
{
  const Encoding::XcdrVersion xcdr = encoding().xcdr_version();
  if (xcdr == Encoding::XCDR1) {
    // Get the "short" id and size
    if (!align_r(4)) {
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
  } else if (xcdr == Encoding::XCDR2) {
    ACE_CDR::ULong emheader;
    if (!(*this >> emheader)) {
      return false;
    }

    // TODO(iguessthislldo): Handle Must Understand Flag

    // Get Size
    // TODO(iguessthislldo) LC
    const unsigned short lc = (emheader >> 28) & 0x7;
    if (lc < 4) {
      size = 1 << lc;
    } else {
      ACE_CDR::ULong next_int;
      if (!(*this >> next_int)) {
        return false;
      }
      if (lc == 6) {
        size = 4 * next_int;
      } else if (lc == 7) {
        size = 8 * next_int;
      } else { // 4 or 5
        size = next_int;
      }
    }

    id = emheader & 0xfffffff;
  } else { // Not XCDR or something we're not prepared for.
    return false;
  }

  return true;
}

bool Serializer::write_parameter_id(unsigned id, size_t size)
{
  const Encoding::XcdrVersion xcdr = encoding().xcdr_version();
  if (xcdr == Encoding::XCDR1) {
    if (!align_w(4)) {
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
    const ACE_CDR::UShort pid_size = long_pid ? 8 : size;
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
  } else if (xcdr == Encoding::XCDR2) {

    ACE_CDR::ULong emheader = id;
    // TODO(iguessthislldo): Conditionally insert must understand flag?
    id += emheader_must_understand;
    // TODO(iguessthislldo) LC
    if (!(*this << emheader)) {
      return false;
    }

  } else { // Not XCDR or something we're not prepared for.
    return false;
  }

  return true;
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
