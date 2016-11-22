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

bool Serializer::use_rti_serialization_(false);

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
    (void) ACE_OS::memcpy(
      reinterpret_cast<void*>(to),
      reinterpret_cast<const void*>(from),
      n);
}

void
Serializer::swapcpy(char* to, const char* from, size_t n)
{
  // Unroll the loop...
  switch (n) {               // 2   4   8   16
  case 16:
    to[ 15] = from[ n - 16]; // x   x   x    0
  case 15:
    to[ 14] = from[ n - 15]; // x   x   x    1
  case 14:
    to[ 13] = from[ n - 14]; // x   x   x    2
  case 13:
    to[ 12] = from[ n - 13]; // x   x   x    3
  case 12:
    to[ 11] = from[ n - 12]; // x   x   x    4
  case 11:
    to[ 10] = from[ n - 11]; // x   x   x    5
  case 10:
    to[  9] = from[ n - 10]; // x   x   x    6
  case  9:
    to[  8] = from[ n -  9]; // x   x   x    7
  case  8:
    to[  7] = from[ n -  8]; // x   x   0    8
  case  7:
    to[  6] = from[ n -  7]; // x   x   1    9
  case  6:
    to[  5] = from[ n -  6]; // x   x   2   10
  case  5:
    to[  4] = from[ n -  5]; // x   x   3   11
  case  4:
    to[  3] = from[ n -  4]; // x   0   4   12
  case  3:
    to[  2] = from[ n -  3]; // x   1   5   13
  case  2:
    to[  1] = from[ n -  2]; // 0   2   6   14
  case  1:
    to[  0] = from[ n -  1]; // 1   3   7   15
  case  0:
    return;
  default:
    this->good_bit_ = false;
  }
}

void
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
    return;
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
  }
}

void
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
    return;
  }

  //
  // NOTE: Maintain the ACE implementation where the length check is
  //       done here before the allocation even though it will be
  //       checked during the actual read as well.
  //
  if (bytecount <= this->current_->total_length()) {

    const ACE_CDR::ULong length = bytecount / WCHAR_SIZE;

    dest = str_alloc(length);

    if (dest == 0) {
      this->good_bit_ = false;
      return;
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
    }
  }
}

void
Serializer::set_use_rti_serialization(bool should_use)
{
  use_rti_serialization_ = should_use;
}

bool
Serializer::use_rti_serialization()
{
  return use_rti_serialization_;
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
