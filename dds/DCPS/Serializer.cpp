/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "Serializer.h"
#include <tao/String_Alloc.h>
#include <ace/OS_NS_string.h>
#include <ace/OS_Memory.h>

#if !defined (__ACE_INLINE__)
# include "Serializer.inl"
#endif /* !__ACE_INLINE__ */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

const char Serializer::ALIGN_PAD[] = {0};

Serializer::Serializer(ACE_Message_Block* chain,
                       bool swap_bytes, Alignment align)
  : current_(chain)
  , swap_bytes_(swap_bytes)
  , good_bit_(true)
  , alignment_(align)
  , align_rshift_(chain ? ptrdiff_t(chain->rd_ptr()) % MAX_ALIGN : 0)
  , align_wshift_(chain ? ptrdiff_t(chain->wr_ptr()) % MAX_ALIGN : 0)
{
}

Serializer::~Serializer()
{
}

void
Serializer::reset_alignment()
{
  align_rshift_ = current_ ? ptrdiff_t(current_->rd_ptr()) % MAX_ALIGN : 0;
  align_wshift_ = current_ ? ptrdiff_t(current_->wr_ptr()) % MAX_ALIGN : 0;
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
  this->alignment_ == ALIGN_NONE ? 0 : this->align_r(sizeof(ACE_CDR::ULong));
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
  this->alignment_ == ALIGN_NONE ? 0 : this->align_r(sizeof(ACE_CDR::ULong));
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

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
