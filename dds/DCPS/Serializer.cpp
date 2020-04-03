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

Encoding::Encoding()
: endianness_(ENDIAN_NATIVE)
{
  kind(KIND_UNKNOWN);
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

bool operator>>(Serializer& s, Encoding& encoding)
{
  ACE_CDR::Octet data[4];
  if (!s.read_octet_array(&data[0], sizeof(data))) {
    return false;
  }
  const ACE_CDR::UShort raw_kind =
    (static_cast<ACE_CDR::UShort>(data[0]) << 8) | data[1];
  const Encoding::Kind kind = static_cast<Encoding::Kind>(raw_kind & 0xfffe);
  switch (kind) {
  case Encoding::KIND_CDR_PLAIN: // fallthrough
  case Encoding::KIND_CDR_PARAMLIST: // fallthrough
  case Encoding::KIND_XCDR2_PLAIN: // fallthrough
  case Encoding::KIND_XCDR2_PARAMLIST: // fallthrough
  case Encoding::KIND_XCDR2_DELIMITED:
    encoding.endianness(static_cast<Endianness>(data[1] & 1));
    encoding.kind(kind);
    break;

  default:
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR ")
      ACE_TEXT("operator>>(Serializer&, Encoding&): ")
      ACE_TEXT("Unsupported Encoding Kind: %C\n"),
      encoding.encoding_value_to_string(raw_kind).c_str()));
    return false;
  }
  if (DCPS_debug_level && (data[2] || data[3])) {
    ACE_DEBUG((LM_WARNING, ACE_TEXT("(%P|%t) WARNING ")
      ACE_TEXT("operator>>(Serializer&, Encoding&): ")
      ACE_TEXT("Unexpected non-zero CDR header options: %C\n"),
      to_hex_dds_string(&data[2], 2).c_str()));
  }
  s.reset_alignment();
  return true;
}

bool operator<<(Serializer& s, const Encoding& encoding)
{
  ACE_CDR::Octet data[4];
  Encoding::Kind k = encoding.kind();
  data[0] = k >> 8;
  data[1] = (k & 0xfe) | encoding.endianness();
  // Encoding "Options" Currently Reserved, Set to Zero
  data[2] = 0;
  data[3] = 0;
  const bool ok = s.write_octet_array(&data[0], sizeof(data));
  s.reset_alignment();
  return ok;
}

OPENDDS_STRING Encoding::encoding_value_to_string(ACE_CDR::UShort value)
{
  ACE_CDR::Octet data[2];
  switch (static_cast<Kind>(value)) {
  case KIND_CDR_PLAIN:
    return "CDR/XCDR1 Plain";
  case KIND_CDR_PARAMLIST:
    return "CDR/XCDR1 Parameter List";
  case KIND_XCDR2_PLAIN:
    return "XCDR2 Plain";
  case KIND_XCDR2_PARAMLIST:
    return "XCDR2 Parameter List";
  case KIND_XCDR2_DELIMITED:
    return "XCDR2 Delimited";
  case KIND_XML:
    return "XML";
  default:
    data[0] = value >> 8;
    data[1] = value & 0xff;
    return to_hex_dds_string(&data[0], 2);
  }
}

const char Serializer::ALIGN_PAD[] = {0};

Serializer::Serializer(ACE_Message_Block* chain, const Encoding& encoding)
  : current_(chain)
  , good_bit_(true)
  , align_rshift_(0)
  , align_wshift_(0)
{
  this->encoding(encoding);
}

Serializer::Serializer(ACE_Message_Block* chain, Encoding::Kind kind,
  Endianness endianness)
  : current_(chain)
  , good_bit_(true)
  , align_rshift_(0)
  , align_wshift_(0)
{
  encoding(Encoding(kind, endianness));
}

Serializer::Serializer(ACE_Message_Block* chain, bool has_cdr_header,
  bool is_little_endian)
  : current_(chain)
  , good_bit_(true)
  , align_rshift_(0)
  , align_wshift_(0)
{
  Encoding enc;
  bool ok = true;
  if (has_cdr_header) {
    ok = *this >> enc;
  } else {
    enc.kind(Encoding::KIND_CDR_UNALIGNED);
    enc.endianness(is_little_endian ? ENDIAN_LITTLE : ENDIAN_BIG );
  }
  if (ok) {
    encoding(enc);
  }
}

Serializer::Serializer(ACE_Message_Block* chain, bool swap_bytes,
  Encoding::Alignment align, bool zero_init_padding)
  : current_(chain)
  , good_bit_(true)
  , align_rshift_(0)
  , align_wshift_(0)
{
  Encoding enc(Encoding::KIND_UNKNOWN, swap_bytes);
  enc.alignment(align);
  enc.zero_init_padding(zero_init_padding);
  encoding(enc);
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
  if (encoding().alignment()) {
    align_r(sizeof(ACE_CDR::ULong));
  }

  //
  // Ensure no bad values leave the routine.
  //
  str_free(dest);
  dest = 0;

  //
  // Extract the string length.
  //
  ACE_CDR::ULong length; // includes the null
  this->buffer_read(reinterpret_cast<char*>(&length), sizeof(ACE_CDR::ULong), this->swap_bytes());

  if (!this->good_bit_) {
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
  if (encoding().alignment()) {
    align_r(sizeof(ACE_CDR::ULong));
  }

  //
  // Ensure no bad values leave the routine.
  //
  str_free(dest);
  dest = 0;

  //
  // Extract the string length.
  //
  ACE_CDR::ULong bytecount = 0;
  this->buffer_read(reinterpret_cast<char*>(&bytecount),
                    sizeof(ACE_CDR::ULong), this->swap_bytes());

  if (!this->good_bit_) {
    return 0;
  }

  //
  // NOTE: Maintain the ACE implementation where the length check is
  //       done here before the allocation even though it will be
  //       checked during the actual read as well.
  //
  ACE_CDR::ULong length = 0;
  if (bytecount <= this->current_->total_length()) {
    length = bytecount / WCHAR_SIZE;
    dest = str_alloc(length);

    if (dest == 0) {
      this->good_bit_ = false;
      return 0;
    }

#if ACE_SIZEOF_WCHAR == 2
    this->read_array(reinterpret_cast<char*>(dest), WCHAR_SIZE, length, SWAP_BE);
#else
    for (size_t i = 0; i < length && this->good_bit_; ++i) {
      ACE_UINT16 as_utf16;
      this->buffer_read(reinterpret_cast<char*>(&as_utf16), WCHAR_SIZE, SWAP_BE);
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

bool Serializer::read_parameter_id(unsigned& id, unsigned& size)
{
  const Encoding::XcdrVersion xcdr = encoding().xcdr_version();
  if (xcdr == Encoding::XCDR1) {
    // Get the "short" id and size
    align_r(4);
    ACE_CDR::UShort pid;
    if (!(*this >> pid)) {
      return false;
    }
    const ACE_CDR::UShort short_id = pid & 0x3fff;
    ACE_CDR::UShort short_size;
    if (!(*this >> short_size)) {
      return false;
    }

    // TODO: handle PID flags

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

    // TODO: Handle Must Understand Flag

    // Get Size
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
  } else {
    return false;
  }

  return true;
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
