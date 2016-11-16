/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <ace/Message_Block.h>
#include <ace/CDR_Stream.h>
#include "Serializer.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

// NOTE: I use the ternary operators in here for conditionals to help
//       the compiler inline the code -- and it does end up fairly
//       tight...
ACE_INLINE size_t
Serializer::doread(char* dest, size_t size, bool swap, size_t offset)
{
  //
  // Ensure we work only with buffer data.
  //
  if (this->current_ == 0) {
    this->good_bit_ = false;
    return size;
  }

  //
  // Determine how much data will remain to be read after the current
  // buffer has been entirely read.
  //
  const size_t len = this->current_->length();
  register size_t remainder = (size - offset > len) ? size - offset - len : 0;

  //
  // Derive how much data we need to read from the current buffer.
  //
  register size_t initial = size - offset - remainder;

  //
  // Copy or swap the source data from the current buffer into the
  // destination.
  //
  swap
    ? this->swapcpy(dest + remainder, this->current_->rd_ptr(), initial)
    : this->smemcpy(dest + offset, this->current_->rd_ptr(), initial);
  this->current_->rd_ptr(initial);

  //   smemcpy
  //
  //   dest            b1   b2   b3        offset   remainder   initial
  //   xxxxxxxx        01   23   4567xx
  //                   ^                   0        6           2
  //   01xxxxxx        01   23   4567xx
  //                        ^              2        4           2
  //   0123xxxx        01   23   4567xx
  //                             ^         4        0           4
  //   01234567        01   23   4567xx
  //                                 ^

  //   swapcpy
  //
  //   dest            b1   b2   b3        offset   remainder   initial
  //   xxxxxxxx        01   23   4567xx
  //                   ^                   0        6           2
  //   xxxxxx10        01   23   4567xx
  //                        ^              2        4           2
  //   xxxx3210        01   23   4567xx
  //                             ^         4        0           4
  //   76543210        01   23   4567xx
  //                                 ^

  //
  // Move to the next chained block if this one is spent.
  //
  if (this->current_->length() == 0) {

    if (this->alignment_ == ALIGN_NONE) {
      this->current_ = this->current_->cont();
    } else {
      this->align_cont_r();
    }
  }
  //
  // Return the current location in the read.
  //
  return offset + initial;
}

ACE_INLINE void
Serializer::buffer_read(char* dest, size_t size, bool swap)
{
  register size_t offset = 0;

  while (size > offset) {
    offset = this->doread(dest, size, swap, offset);
  }
}

// NOTE: I use the ternary operators in here for conditionals to help
//       the compiler inline the code -- and it does end up fairly
//       tight...
ACE_INLINE size_t
Serializer::dowrite(const char* src, size_t size, bool swap, size_t offset)
{
  //
  // Ensure we work only with buffer data.
  //
  if (this->current_ == 0) {
    this->good_bit_ = false;
    return size;
  }

  //
  // Determine how much data will remain to be written after the current
  // buffer has been entirely filled.
  //
  const size_t spc = this->current_->space();
  register size_t remainder = (size - offset > spc) ? size - offset - spc : 0;

  //
  // Derive how much data we need to write to the current buffer.
  //
  register size_t initial = size - offset - remainder;

  //
  // Copy or swap the source data into the current buffer.
  //
  swap
    ? this->swapcpy(this->current_->wr_ptr(), src + remainder, initial)
    : this->smemcpy(this->current_->wr_ptr(), src + offset, initial);
  this->current_->wr_ptr(initial);

  //   smemcpy
  //
  //   src             b1   b2   b3        offset   remainder   initial
  //   01234567        xx   xx   xxxxxx
  //                   ^                   0        6           2
  //                   01   xx   xxxxxx
  //                        ^              2        4           2
  //                   01   23   xxxxxx
  //                             ^         4        0           4
  //                   01   23   4567xx
  //                                 ^

  //   swapcpy
  //
  //   src             b1   b2   b3        offset   remainder   initial
  //   01234567        xx   xx   xxxxxx
  //                   ^                   0        6           2
  //                   76   xx   xxxxxx
  //                        ^              2        4           2
  //                   76   54   xxxxxx
  //                             ^         4        0           4
  //                   76   54   3210xx
  //                                 ^

  //
  // Move to the next chained block if this one is spent.
  //
  if (this->current_->space() == 0) {

    if (this->alignment_ == ALIGN_NONE) {
      this->current_ = this->current_->cont();
    } else {
      this->align_cont_w();
    }
  }

  //
  // Return the current location in the write.
  //
  return offset + initial;
}

ACE_INLINE void
Serializer::buffer_write(const char* src, size_t size, bool swap)
{
  register size_t offset = 0;

  while (size > offset) {
    offset = this->dowrite(src, size, swap, offset);
  }
}

ACE_INLINE void
Serializer::swap_bytes(bool do_swap)
{
  this->swap_bytes_ = do_swap;
}

ACE_INLINE bool
Serializer::swap_bytes() const
{
  return this->swap_bytes_;
}

ACE_INLINE Serializer::Alignment
Serializer::alignment() const
{
  return this->alignment_;
}

ACE_INLINE bool
Serializer::good_bit() const
{
  return this->good_bit_;
}

ACE_INLINE bool
Serializer::skip(ACE_CDR::UShort n, int size)
{
  if (size > 1 && this->alignment_ != ALIGN_NONE) {
    this->align_r(size_t(size) > MAX_ALIGN ? MAX_ALIGN : size_t(size));
  }
  for (size_t len = static_cast<size_t>(n * size); len;) {
    if (!this->current_) {
      this->good_bit_ = false;
      return false;
    }
    const size_t cur_len = this->current_->length();
    if (cur_len <= len) {
      len -= cur_len;
      this->current_->rd_ptr(this->current_->wr_ptr());
      this->align_cont_r();
    } else {
      this->current_->rd_ptr(len);
      break;
    }
  }
  return this->good_bit();
}

ACE_INLINE void
Serializer::read_array(char* x, size_t size, ACE_CDR::ULong length)
{
  this->read_array(x, size, length, this->swap_bytes_);
}

ACE_INLINE void
Serializer::read_array(char* x, size_t size,
                       ACE_CDR::ULong length, bool swap)
{
  if (!swap || size == 1) {
    //
    // No swap, copy direct.  This silently corrupts the data if there is
    // padding in the buffer.
    //
    this->buffer_read(x, size * length, false);

  } else {
    //
    // Swapping _must_ be done at 'size' boundaries, so we need to spin
    // through the array element by element.  This silently corrupts the
    // data if there is padding in the buffer.
    //
    while (length-- > 0) {
      this->buffer_read(x, size, true);
      x += size;
    }
  }
}

ACE_INLINE void
Serializer::write_array(const char* x, size_t size, ACE_CDR::ULong length)
{
  this->write_array(x, size, length, this->swap_bytes_);
}

ACE_INLINE void
Serializer::write_array(const char* x, size_t size,
                        ACE_CDR::ULong length, bool swap)
{
  if (!swap || size == 1) {
    //
    // No swap, copy direct.
    //
    this->buffer_write(x, size * length, false);

  } else {
    //
    // Swapping _must_ be done at 'size' boundaries, so we need to spin
    // through the array element by element.
    // NOTE: This assumes that there is _no_ padding between the array
    //       elements.  If this is not the case, do not use this
    //       method.
    //
    while (length-- > 0) {
      this->buffer_write(x, size, true);
      x += size;
    }
  }
}

ACE_INLINE bool
Serializer::read_boolean_array(ACE_CDR::Boolean* x, ACE_CDR::ULong length)
{
  this->read_array(reinterpret_cast<char*>(x), sizeof(ACE_CDR::Boolean), length);
  return this->good_bit();
}

ACE_INLINE bool
Serializer::read_char_array(ACE_CDR::Char* x, ACE_CDR::ULong length)
{
  this->read_array(reinterpret_cast<char*>(x), sizeof(ACE_CDR::Char), length);
  return this->good_bit();
}

ACE_INLINE bool
Serializer::read_wchar_array(ACE_CDR::WChar* x, ACE_CDR::ULong length)
{
  for (ACE_CDR::ULong i = 0; i < length; ++i) {
    if (!((*this) >> ACE_InputCDR::to_wchar(x[i]))) {
      break;
    }
  }
  return this->good_bit();
}

ACE_INLINE bool
Serializer::read_octet_array(ACE_CDR::Octet* x, ACE_CDR::ULong length)
{
  this->read_array(reinterpret_cast<char*>(x), sizeof(ACE_CDR::Octet), length);
  return this->good_bit();
}

ACE_INLINE bool
Serializer::read_short_array(ACE_CDR::Short* x, ACE_CDR::ULong length)
{
  this->alignment() == Serializer::ALIGN_NONE ? 0 : this->align_r(sizeof(ACE_CDR::Short));
  this->read_array(reinterpret_cast<char*>(x), sizeof(ACE_CDR::Short), length);
  return this->good_bit();
}

ACE_INLINE bool
Serializer::read_ushort_array(ACE_CDR::UShort* x, ACE_CDR::ULong length)
{
  this->alignment() == Serializer::ALIGN_NONE ? 0 : this->align_r(sizeof(ACE_CDR::UShort));
  this->read_array(reinterpret_cast<char*>(x), sizeof(ACE_CDR::UShort), length);
  return this->good_bit();
}

ACE_INLINE bool
Serializer::read_long_array(ACE_CDR::Long* x, ACE_CDR::ULong length)
{
  this->alignment() == Serializer::ALIGN_NONE ? 0 : this->align_r(sizeof(ACE_CDR::Long));
  this->read_array(reinterpret_cast<char*>(x), sizeof(ACE_CDR::Long), length);
  return this->good_bit();
}

ACE_INLINE bool
Serializer::read_ulong_array(ACE_CDR::ULong* x, ACE_CDR::ULong length)
{
  this->alignment() == Serializer::ALIGN_NONE ? 0 : this->align_r(sizeof(ACE_CDR::ULong));
  this->read_array(reinterpret_cast<char*>(x), sizeof(ACE_CDR::ULong), length);
  return this->good_bit();
}

ACE_INLINE bool
Serializer::read_longlong_array(ACE_CDR::LongLong* x, ACE_CDR::ULong length)
{
  this->alignment() == Serializer::ALIGN_NONE ? 0 : this->align_r(sizeof(ACE_CDR::LongLong));
  this->read_array(reinterpret_cast<char*>(x), sizeof(ACE_CDR::LongLong), length);
  return this->good_bit();
}

ACE_INLINE bool
Serializer::read_ulonglong_array(ACE_CDR::ULongLong* x, ACE_CDR::ULong length)
{
  this->alignment() == Serializer::ALIGN_NONE ? 0 : this->align_r(sizeof(ACE_CDR::ULongLong));
  this->read_array(reinterpret_cast<char*>(x), sizeof(ACE_CDR::ULongLong), length);
  return this->good_bit();
}

ACE_INLINE bool
Serializer::read_float_array(ACE_CDR::Float* x, ACE_CDR::ULong length)
{
  this->alignment() == Serializer::ALIGN_NONE ? 0 : this->align_r(sizeof(ACE_CDR::Float));
  this->read_array(reinterpret_cast<char*>(x), sizeof(ACE_CDR::Float), length);
  return this->good_bit();
}

ACE_INLINE bool
Serializer::read_double_array(ACE_CDR::Double* x, ACE_CDR::ULong length)
{
  this->alignment() == Serializer::ALIGN_NONE ? 0 : this->align_r(sizeof(ACE_CDR::Double));
  this->read_array(reinterpret_cast<char*>(x), sizeof(ACE_CDR::Double), length);
  return this->good_bit();
}

ACE_INLINE bool
Serializer::read_longdouble_array(ACE_CDR::LongDouble* x, ACE_CDR::ULong length)
{
  this->alignment() == Serializer::ALIGN_NONE ? 0 : this->align_r(8);
  this->read_array(reinterpret_cast<char*>(x), sizeof(ACE_CDR::LongDouble), length);
  return this->good_bit();
}

ACE_INLINE bool
Serializer::write_boolean_array(const ACE_CDR::Boolean* x,
                                ACE_CDR::ULong length)
{
  this->write_array(reinterpret_cast<const char*>(x), sizeof(ACE_CDR::Boolean), length);
  return this->good_bit();
}

ACE_INLINE bool
Serializer::write_char_array(const ACE_CDR::Char* x, ACE_CDR::ULong length)
{
  this->write_array(reinterpret_cast<const char*>(x), sizeof(ACE_CDR::Char), length);
  return this->good_bit();
}

ACE_INLINE bool
Serializer::write_wchar_array(const ACE_CDR::WChar* x, ACE_CDR::ULong length)
{
  for (ACE_CDR::ULong i = 0; i < length; ++i) {
    if (!((*this) << ACE_OutputCDR::from_wchar(x[i]))) {
      break;
    }
  }
  return this->good_bit();
}

ACE_INLINE bool
Serializer::write_octet_array(const ACE_CDR::Octet* x, ACE_CDR::ULong length)
{
  this->write_array(reinterpret_cast<const char*>(x), sizeof(ACE_CDR::Octet), length);
  return this->good_bit();
}

ACE_INLINE bool
Serializer::write_short_array(const ACE_CDR::Short* x, ACE_CDR::ULong length)
{
  this->alignment() == Serializer::ALIGN_NONE ? 0 : this->align_w(sizeof(ACE_CDR::Short));
  this->write_array(reinterpret_cast<const char*>(x), sizeof(ACE_CDR::Short), length);
  return this->good_bit();
}

ACE_INLINE bool
Serializer::write_ushort_array(const ACE_CDR::UShort* x, ACE_CDR::ULong length)
{
  this->alignment() == Serializer::ALIGN_NONE ? 0 : this->align_w(sizeof(ACE_CDR::UShort));
  this->write_array(reinterpret_cast<const char*>(x), sizeof(ACE_CDR::UShort), length);
  return this->good_bit();
}

ACE_INLINE bool
Serializer::write_long_array(const ACE_CDR::Long* x, ACE_CDR::ULong length)
{
  this->alignment() == Serializer::ALIGN_NONE ? 0 : this->align_w(sizeof(ACE_CDR::Long));
  this->write_array(reinterpret_cast<const char*>(x), sizeof(ACE_CDR::Long), length);
  return this->good_bit();
}

ACE_INLINE bool
Serializer::write_ulong_array(const ACE_CDR::ULong* x, ACE_CDR::ULong length)
{
  this->alignment() == Serializer::ALIGN_NONE ? 0 : this->align_w(sizeof(ACE_CDR::ULong));
  this->write_array(reinterpret_cast<const char*>(x), sizeof(ACE_CDR::ULong), length);
  return this->good_bit();
}

ACE_INLINE bool
Serializer::write_longlong_array(const ACE_CDR::LongLong* x,
                                 ACE_CDR::ULong length)
{
  this->alignment() == Serializer::ALIGN_NONE ? 0 : this->align_w(sizeof(ACE_CDR::LongLong));
  this->write_array(reinterpret_cast<const char*>(x), sizeof(ACE_CDR::LongLong), length);
  return this->good_bit();
}

ACE_INLINE bool
Serializer::write_ulonglong_array(const ACE_CDR::ULongLong* x,
                                  ACE_CDR::ULong length)
{
  this->alignment() == Serializer::ALIGN_NONE ? 0 : this->align_w(sizeof(ACE_CDR::ULongLong));
  this->write_array(reinterpret_cast<const char*>(x), sizeof(ACE_CDR::ULongLong), length);
  return this->good_bit();
}

ACE_INLINE bool
Serializer::write_float_array(const ACE_CDR::Float* x, ACE_CDR::ULong length)
{
  this->alignment() == Serializer::ALIGN_NONE ? 0 : this->align_w(sizeof(ACE_CDR::Float));
  this->write_array(reinterpret_cast<const char*>(x), sizeof(ACE_CDR::Float), length);
  return this->good_bit();
}

ACE_INLINE bool
Serializer::write_double_array(const ACE_CDR::Double* x, ACE_CDR::ULong length)
{
  this->alignment() == Serializer::ALIGN_NONE ? 0 : this->align_w(sizeof(ACE_CDR::Double));
  this->write_array(reinterpret_cast<const char*>(x), sizeof(ACE_CDR::Double), length);
  return this->good_bit();
}

ACE_INLINE bool
Serializer::write_longdouble_array(const ACE_CDR::LongDouble* x,
                                   ACE_CDR::ULong length)
{
  this->alignment() == Serializer::ALIGN_NONE ? 0 : this->align_w(8);
  this->write_array(reinterpret_cast<const char*>(x), sizeof(ACE_CDR::LongDouble), length);
  return this->good_bit();
}

ACE_INLINE int
Serializer::align_r(size_t al)
{
  const size_t len =
    (al - ptrdiff_t(this->current_->rd_ptr()) + this->align_rshift_) % al;
  this->skip(static_cast<ACE_CDR::UShort>(len));
  return 0;
}

ACE_INLINE int
Serializer::align_w(size_t al)
{
  size_t len =
    (al - ptrdiff_t(this->current_->wr_ptr()) + this->align_wshift_) % al;
  while (len) {
    if (!this->current_) {
      this->good_bit_ = false;
      break;
    }
    const size_t cur_spc = this->current_->space();
    if (cur_spc <= len) {
      len -= cur_spc;
      if (this->alignment_ == ALIGN_INITIALIZE) {
        this->smemcpy(this->current_->wr_ptr(), ALIGN_PAD, cur_spc);
      }
      this->current_->wr_ptr(cur_spc);
      this->align_cont_w();
    } else {
      if (this->alignment_ == ALIGN_INITIALIZE) {
        this->smemcpy(this->current_->wr_ptr(), ALIGN_PAD, len);
      }
      this->current_->wr_ptr(len);
      break;
    }
  }
  return 0;
}


ACE_INLINE void
Serializer::align_cont_r()
{
  const size_t thisblock =
    (ptrdiff_t(this->current_->rd_ptr()) - this->align_rshift_) % MAX_ALIGN;

  this->current_ = this->current_->cont();

  if (this->current_) {
    this->align_rshift_ =
      (ptrdiff_t(this->current_->rd_ptr()) - thisblock) % MAX_ALIGN;
  }
}

ACE_INLINE void
Serializer::align_cont_w()
{
  const size_t thisblock =
    (ptrdiff_t(this->current_->wr_ptr()) - this->align_wshift_) % MAX_ALIGN;

  this->current_ = this->current_->cont();

  if (this->current_) {
    this->align_wshift_ =
      (ptrdiff_t(this->current_->wr_ptr()) - thisblock) % MAX_ALIGN;
  }
}

//
// The following insertion operators are done in the style of the
// ACE_CDR insertion operators instead of a stream abstraction.  This
// is done to allow their use in the same way as existing ACE_CDR
// inserters, rather than as a true stream abstraction (which would
// return the argument stream).
//

ACE_INLINE bool
operator<<(Serializer& s, ACE_CDR::Char x)
{
  s.buffer_write(reinterpret_cast<char*>(&x), sizeof(ACE_CDR::Char), s.swap_bytes());
  return s.good_bit();
}

ACE_INLINE bool
operator<<(Serializer& s, ACE_CDR::Short x)
{
  s.alignment() == Serializer::ALIGN_NONE ? 0 : s.align_w(sizeof(ACE_CDR::Short));
  s.buffer_write(reinterpret_cast<char*>(&x), sizeof(ACE_CDR::Short), s.swap_bytes());
  return s.good_bit();
}

ACE_INLINE bool
operator<<(Serializer& s, ACE_CDR::UShort x)
{
  s.alignment() == Serializer::ALIGN_NONE ? 0 : s.align_w(sizeof(ACE_CDR::UShort));
  s.buffer_write(reinterpret_cast<char*>(&x), sizeof(ACE_CDR::UShort), s.swap_bytes());
  return s.good_bit();
}

ACE_INLINE bool
operator<<(Serializer& s, ACE_CDR::Long x)
{
  s.alignment() == Serializer::ALIGN_NONE ? 0 : s.align_w(sizeof(ACE_CDR::Long));
  s.buffer_write(reinterpret_cast<char*>(&x), sizeof(ACE_CDR::Long), s.swap_bytes());
  return s.good_bit();
}

ACE_INLINE bool
operator<<(Serializer& s, ACE_CDR::ULong x)
{
  s.alignment() == Serializer::ALIGN_NONE ? 0 : s.align_w(sizeof(ACE_CDR::ULong));
  s.buffer_write(reinterpret_cast<char*>(&x), sizeof(ACE_CDR::ULong), s.swap_bytes());
  return s.good_bit();
}

ACE_INLINE bool
operator<<(Serializer& s, ACE_CDR::LongLong x)
{
  s.alignment() == Serializer::ALIGN_NONE ? 0 : s.align_w(sizeof(ACE_CDR::LongLong));
  s.buffer_write(reinterpret_cast<char*>(&x), sizeof(ACE_CDR::LongLong), s.swap_bytes());
  return s.good_bit();
}

ACE_INLINE bool
operator<<(Serializer& s, ACE_CDR::ULongLong x)
{
  s.alignment() == Serializer::ALIGN_NONE ? 0 : s.align_w(sizeof(ACE_CDR::ULongLong));
  s.buffer_write(reinterpret_cast<char*>(&x), sizeof(ACE_CDR::ULongLong), s.swap_bytes());
  return s.good_bit();
}

ACE_INLINE bool
operator<<(Serializer& s, ACE_CDR::LongDouble x)
{
  s.alignment() == Serializer::ALIGN_NONE ? 0 : s.align_w(8);
  s.buffer_write(reinterpret_cast<char*>(&x), sizeof(ACE_CDR::LongDouble), s.swap_bytes());
  return s.good_bit();
}

ACE_INLINE bool
operator<<(Serializer& s, ACE_CDR::Float x)
{
  s.alignment() == Serializer::ALIGN_NONE ? 0 : s.align_w(sizeof(ACE_CDR::Float));
  s.buffer_write(reinterpret_cast<char*>(&x), sizeof(ACE_CDR::Float), s.swap_bytes());
  return s.good_bit();
}

ACE_INLINE bool
operator<<(Serializer& s, ACE_CDR::Double x)
{
  s.alignment() == Serializer::ALIGN_NONE ? 0 : s.align_w(sizeof(ACE_CDR::Double));
  s.buffer_write(reinterpret_cast<char*>(&x), sizeof(ACE_CDR::Double), s.swap_bytes());
  return s.good_bit();
}

ACE_INLINE bool
operator<<(Serializer& s, const ACE_CDR::Char* x)
{
  if (x != 0) {
    // Include the null termination in the serialized data.
    const ACE_CDR::ULong stringlen =
      1 + static_cast<ACE_CDR::ULong>(ACE_OS::strlen(x));
    s << stringlen;
    s.buffer_write(reinterpret_cast<const char*>(x), stringlen, false);

  } else {
    s << ACE_CDR::ULong(0);
  }

  return s.good_bit();
}

ACE_INLINE bool
operator<<(Serializer& s, const ACE_CDR::WChar* x)
{
  //NOTE: Serializing wchar/wstring uses UTF-16BE
  if (x != 0) {
    // Do not include the null terminatator in the serialized data.
    const ACE_CDR::ULong length = static_cast<ACE_CDR::ULong>(ACE_OS::strlen(x));
    s << ACE_CDR::ULong(length * Serializer::WCHAR_SIZE);

#if ACE_SIZEOF_WCHAR == 2
    s.write_array(reinterpret_cast<const char*>(x), Serializer::WCHAR_SIZE,
                  length, Serializer::SWAP_BE);
#else
    for (size_t i = 0; i < length && s.good_bit(); ++i) {
      const ACE_UINT16 as_utf16 = static_cast<ACE_UINT16>(x[i]);
      if (as_utf16 != x[i]) { // not currently handling surrogates
        s.good_bit_ = false;
        break;
      }
      s.buffer_write(reinterpret_cast<const char*>(&as_utf16), Serializer::WCHAR_SIZE, Serializer::SWAP_BE);
    }
#endif
  } else {
    s << ACE_CDR::ULong(0);
  }

  return s.good_bit();
}

ACE_INLINE bool
operator<<(Serializer& s, ACE_OutputCDR::from_boolean x)
{
  s.buffer_write(reinterpret_cast<char*>(&x.val_), sizeof(ACE_CDR::Boolean), s.swap_bytes());
  return s.good_bit();
}

ACE_INLINE bool
operator<<(Serializer& s, ACE_OutputCDR::from_char x)
{
  s.buffer_write(reinterpret_cast<char*>(&x.val_), sizeof(ACE_CDR::Char), false);
  return s.good_bit();
}

ACE_INLINE bool
operator<<(Serializer& s, ACE_OutputCDR::from_wchar x)
{
  // CDR wchar format: 1 octet for # of bytes in wchar, followed by those bytes
  static const ACE_CDR::Octet wchar_bytes = Serializer::WCHAR_SIZE;
  s.buffer_write(reinterpret_cast<const char*>(&wchar_bytes), 1, false);
#if ACE_SIZEOF_WCHAR == 2
  s.buffer_write(reinterpret_cast<char*>(&x.val_), Serializer::WCHAR_SIZE, Serializer::SWAP_BE);
#else
  const ACE_UINT16 as_utf16 = static_cast<ACE_UINT16>(x.val_);
  if (as_utf16 != x.val_) { // not currently handling surrogates
    s.good_bit_ = false;
  } else {
    s.buffer_write(reinterpret_cast<const char*>(&as_utf16), Serializer::WCHAR_SIZE, Serializer::SWAP_BE);
  }
#endif
  return s.good_bit();
}

ACE_INLINE bool
operator<<(Serializer& s, ACE_OutputCDR::from_octet x)
{
  s.buffer_write(reinterpret_cast<char*>(&x.val_), sizeof(ACE_CDR::Octet), false);
  return s.good_bit();
}

ACE_INLINE bool
operator<<(Serializer& s, ACE_OutputCDR::from_string x)
{
  // Include the null termination in the serialized data.
  ACE_CDR::ULong stringlen = 0;

  if (x.val_ != 0) {
    stringlen = 1 + static_cast<ACE_CDR::ULong>(ACE_OS::strlen(x.val_));
    s << stringlen;
    s.buffer_write(reinterpret_cast<char*>(x.val_), stringlen, false);

  } else {
    s << ACE_CDR::ULong(0);
  }

  return s.good_bit() && ((x.bound_ == 0) || (stringlen - 1 <= x.bound_));
}

ACE_INLINE bool
operator<<(Serializer& s, ACE_OutputCDR::from_wstring x)
{
  s << x.val_;
  const ACE_CDR::ULong stringlen =
    x.bound_ ? static_cast<ACE_CDR::ULong>(ACE_OS::strlen(x.val_)) : 0;
  return s.good_bit() && ((x.bound_ == 0) || (stringlen <= x.bound_));
}

//
// The following extraction operators are done in the style of the
// ACE_CDR extraction operators instead of a stream abstraction.  This
// is done to allow their use in the same way as existing ACE_CDR
// extractors, rather than as a true stream abstraction (which would
// return the argument stream).
//

ACE_INLINE bool
operator>>(Serializer& s, ACE_CDR::Char& x)
{
  s.buffer_read(reinterpret_cast<char*>(&x), sizeof(ACE_CDR::Char), s.swap_bytes());
  return s.good_bit();
}

ACE_INLINE bool
operator>>(Serializer& s, ACE_CDR::Short& x)
{
  s.alignment() == Serializer::ALIGN_NONE ? 0 : s.align_r(sizeof(ACE_CDR::Short));
  s.buffer_read(reinterpret_cast<char*>(&x), sizeof(ACE_CDR::Short), s.swap_bytes());
  return s.good_bit();
}

ACE_INLINE bool
operator>>(Serializer& s, ACE_CDR::UShort& x)
{
  s.alignment() == Serializer::ALIGN_NONE ? 0 : s.align_r(sizeof(ACE_CDR::UShort));
  s.buffer_read(reinterpret_cast<char*>(&x), sizeof(ACE_CDR::UShort), s.swap_bytes());
  return s.good_bit();
}

ACE_INLINE bool
operator>>(Serializer& s, ACE_CDR::Long& x)
{
  s.alignment() == Serializer::ALIGN_NONE ? 0 : s.align_r(sizeof(ACE_CDR::Long));
  s.buffer_read(reinterpret_cast<char*>(&x), sizeof(ACE_CDR::Long), s.swap_bytes());
  return s.good_bit();
}

ACE_INLINE bool
operator>>(Serializer& s, ACE_CDR::ULong& x)
{
  s.alignment() == Serializer::ALIGN_NONE ? 0 : s.align_r(sizeof(ACE_CDR::ULong));
  s.buffer_read(reinterpret_cast<char*>(&x), sizeof(ACE_CDR::ULong), s.swap_bytes());
  return s.good_bit();
}

ACE_INLINE bool
operator>>(Serializer& s, ACE_CDR::LongLong& x)
{
  s.alignment() == Serializer::ALIGN_NONE ? 0 : s.align_r(sizeof(ACE_CDR::LongLong));
  s.buffer_read(reinterpret_cast<char*>(&x), sizeof(ACE_CDR::LongLong), s.swap_bytes());
  return s.good_bit();
}

ACE_INLINE bool
operator>>(Serializer& s, ACE_CDR::ULongLong& x)
{
  s.alignment() == Serializer::ALIGN_NONE ? 0 : s.align_r(sizeof(ACE_CDR::ULongLong));
  s.buffer_read(reinterpret_cast<char*>(&x), sizeof(ACE_CDR::ULongLong), s.swap_bytes());
  return s.good_bit();
}

ACE_INLINE bool
operator>>(Serializer& s, ACE_CDR::LongDouble& x)
{
  s.alignment() == Serializer::ALIGN_NONE ? 0 : s.align_r(8);
  s.buffer_read(reinterpret_cast<char*>(&x), sizeof(ACE_CDR::LongDouble), s.swap_bytes());
  return s.good_bit();
}

ACE_INLINE bool
operator>>(Serializer& s, ACE_CDR::Float& x)
{
  s.alignment() == Serializer::ALIGN_NONE ? 0 : s.align_r(sizeof(ACE_CDR::Float));
  s.buffer_read(reinterpret_cast<char*>(&x), sizeof(ACE_CDR::Float), s.swap_bytes());
  return s.good_bit();
}

ACE_INLINE bool
operator>>(Serializer& s, ACE_CDR::Double& x)
{
  s.alignment() == Serializer::ALIGN_NONE ? 0 : s.align_r(sizeof(ACE_CDR::Double));
  s.buffer_read(reinterpret_cast<char*>(&x), sizeof(ACE_CDR::Double), s.swap_bytes());
  return s.good_bit();
}

ACE_INLINE bool
operator>>(Serializer& s, ACE_CDR::Char*& x)
{
  s.read_string(x);
  return s.good_bit();
}

ACE_INLINE bool
operator>>(Serializer& s, ACE_CDR::WChar*& x)
{
  s.read_string(x);
  return s.good_bit();
}

ACE_INLINE bool
operator>>(Serializer& s, ACE_InputCDR::to_boolean x)
{
  s.buffer_read(reinterpret_cast<char*>(&x.ref_), sizeof(ACE_CDR::Boolean), s.swap_bytes());
  return s.good_bit();
}

ACE_INLINE bool
operator>>(Serializer& s, ACE_InputCDR::to_char x)
{
  s.buffer_read(reinterpret_cast<char*>(&x.ref_), sizeof(ACE_CDR::Char), s.swap_bytes());
  return s.good_bit();
}

ACE_INLINE bool
operator>>(Serializer& s, ACE_InputCDR::to_wchar x)
{
  ACE_CDR::Octet len;
  s.buffer_read(reinterpret_cast<char*>(&len), 1, false);
  if (len != Serializer::WCHAR_SIZE) {
    s.good_bit_ = false;
  } else {
#if ACE_SIZEOF_WCHAR == 2
    s.buffer_read(reinterpret_cast<char*>(&x.ref_), Serializer::WCHAR_SIZE, Serializer::SWAP_BE);
#else
    ACE_UINT16 as_utf16;
    s.buffer_read(reinterpret_cast<char*>(&as_utf16), Serializer::WCHAR_SIZE, Serializer::SWAP_BE);
    x.ref_ = as_utf16;
#endif
  }
  return s.good_bit();
}

ACE_INLINE bool
operator>>(Serializer& s, ACE_InputCDR::to_octet x)
{
  s.buffer_read(reinterpret_cast<char*>(&x.ref_), sizeof(ACE_CDR::Octet), false);
  return s.good_bit();
}

ACE_INLINE bool
operator>>(Serializer& s, ACE_InputCDR::to_string x)
{
  s.read_string(const_cast<char*&>(x.val_));
  return s.good_bit()
         && ((x.bound_ == 0) || (ACE_OS::strlen(x.val_) <= x.bound_));
}

ACE_INLINE bool
operator>>(Serializer& s, ACE_InputCDR::to_wstring x)
{
  s.read_string(const_cast<ACE_CDR::WChar*&>(x.val_));
  return s.good_bit()
         && ((x.bound_ == 0) || (ACE_OS::strlen(x.val_) <= x.bound_));
}

//----------------------------------------------------------------------------
// predefined type gen_max_marshaled_size methods
ACE_INLINE size_t gen_max_marshaled_size(const ACE_CDR::Short& /* x */)
{
  return sizeof(ACE_CDR::Short);
}

ACE_INLINE size_t gen_max_marshaled_size(const ACE_CDR::UShort& /* x */)
{
  return sizeof(ACE_CDR::UShort);
}

ACE_INLINE size_t gen_max_marshaled_size(const ACE_CDR::Long& /* x */)
{
  return sizeof(ACE_CDR::Long);
}

ACE_INLINE size_t gen_max_marshaled_size(const ACE_CDR::ULong& /* x */)
{
  return sizeof(ACE_CDR::ULong);
}

ACE_INLINE size_t gen_max_marshaled_size(const ACE_CDR::LongLong& /* x */)
{
  return sizeof(ACE_CDR::LongLong);
}

ACE_INLINE size_t gen_max_marshaled_size(const ACE_CDR::ULongLong& /* x */)
{
  return sizeof(ACE_CDR::ULongLong);
}

ACE_INLINE size_t gen_max_marshaled_size(const ACE_CDR::LongDouble& /* x */)
{
  return sizeof(ACE_CDR::LongDouble);
}

ACE_INLINE size_t gen_max_marshaled_size(const ACE_CDR::Float& /* x */)
{
  return sizeof(ACE_CDR::Float);
}

ACE_INLINE size_t gen_max_marshaled_size(const ACE_CDR::Double& /* x */)
{
  return sizeof(ACE_CDR::Double);
}

// predefined type gen_max_marshaled_size method disambiguators.
ACE_INLINE size_t gen_max_marshaled_size(const ACE_OutputCDR::from_boolean /* x */)
{
  return sizeof(ACE_CDR::Char);
}

ACE_INLINE size_t gen_max_marshaled_size(const ACE_OutputCDR::from_char /* x */)
{
  return sizeof(ACE_CDR::Char);
}

ACE_INLINE size_t gen_max_marshaled_size(const ACE_OutputCDR::from_wchar /* x */)
{
  return Serializer::WCHAR_SIZE + 1; //CDR encoding adds 1 octet for length
}

ACE_INLINE size_t gen_max_marshaled_size(const ACE_OutputCDR::from_octet /* x */)
{
  return sizeof(ACE_CDR::Char);
}

// predefined type gen_max_marshaled_size method explicit disambiguators.
ACE_INLINE size_t max_marshaled_size_boolean()
{
  return sizeof(ACE_CDR::Char);
}

ACE_INLINE size_t max_marshaled_size_char()
{
  return sizeof(ACE_CDR::Char);
}

ACE_INLINE size_t max_marshaled_size_wchar()
{
  return Serializer::WCHAR_SIZE + 1; //CDR encoding adds 1 octet for length
}

ACE_INLINE size_t max_marshaled_size_octet()
{
  return sizeof(ACE_CDR::Char);
}

ACE_INLINE size_t max_marshaled_size_ulong()
{
  return sizeof(ACE_CDR::ULong);
}

ACE_INLINE void find_size_ulong(size_t& size, size_t& padding)
{
  const size_t sz = sizeof(ACE_CDR::ULong);
  if ((size + padding) % sz) {
    padding += sz - ((size + padding) % sz);
  }
  size += sz;
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
