/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "Serializer.h"
#include "debug.h"

#include <ace/Message_Block.h>

#include <algorithm>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

ACE_INLINE
void align(size_t& value, size_t by)
{
  // TODO(iguessthislldo): If this is always alignment by a power of two, it
  // might benefit from a bitwise version.
  const size_t offset_by = value % by;
  if (offset_by) {
    value += by - offset_by;
  }
}

ACE_INLINE
Encoding::Kind Encoding::kind() const
{
  return kind_;
}

ACE_INLINE
void Encoding::kind(Kind value)
{
  zero_init_padding(true);

  switch (value) {
  case KIND_XCDR1:
    alignment(ALIGN_CDR);
    xcdr_version(XCDR_VERSION_1);
    break;

  case KIND_XCDR2:
    alignment(ALIGN_XCDR2);
    xcdr_version(XCDR_VERSION_2);
    break;

  case KIND_UNALIGNED_CDR:
    alignment(ALIGN_NONE);
    xcdr_version(XCDR_VERSION_NONE);
    break;

  default:
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: Encoding::kind: Invalid Argument: %u\n"), value));
  }

  kind_ = value;
}

ACE_INLINE
Endianness Encoding::endianness() const
{
  return endianness_;
}

ACE_INLINE
void Encoding::endianness(Endianness value)
{
  endianness_ = value;
}

ACE_INLINE
Encoding::Alignment Encoding::alignment() const
{
  return alignment_;
}

ACE_INLINE
void Encoding::alignment(Alignment value)
{
  alignment_ = value;
}

ACE_INLINE
bool Encoding::zero_init_padding() const
{
  return zero_init_padding_;
}

ACE_INLINE
void Encoding::zero_init_padding(bool value)
{
  zero_init_padding_ = value;
}

ACE_INLINE
bool Encoding::skip_sequence_dheader() const
{
  return skip_sequence_dheader_;
}

ACE_INLINE
void Encoding::skip_sequence_dheader(bool value)
{
  skip_sequence_dheader_ = value;
}

ACE_INLINE
size_t Encoding::max_align() const
{
  return static_cast<size_t>(alignment_);
}

ACE_INLINE
void Encoding::align(size_t& value, size_t by) const
{
  const size_t max_alignment = max_align();
  if (max_alignment) {
    DCPS::align(value, (std::min)(max_alignment, by));
  }
}

ACE_INLINE
Encoding::XcdrVersion Encoding::xcdr_version() const
{
  return xcdr_version_;
}

ACE_INLINE
void Encoding::xcdr_version(XcdrVersion value)
{
  xcdr_version_ = value;
}

ACE_INLINE
bool Encoding::is_encapsulated(Kind kind)
{
  switch (kind) {
  case KIND_XCDR1:
  case KIND_XCDR2:
    return true;
  case KIND_UNALIGNED_CDR:
    return false;
  default:
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: Encoding::is_encapsulated: ")
      ACE_TEXT("Invalid Argument: %u\n"), kind));
    return false;
  }
}

ACE_INLINE
bool Encoding::is_encapsulated() const
{
  return is_encapsulated(kind_);
}

ACE_INLINE
EncapsulationHeader::Kind EncapsulationHeader::kind() const
{
  return kind_;
}

ACE_INLINE
void EncapsulationHeader::kind(Kind value)
{
  kind_ = value;
}

ACE_INLINE
bool EncapsulationHeader::is_good() const
{
  return kind_ != KIND_INVALID;
}

ACE_INLINE
ACE_UINT16 EncapsulationHeader::options() const
{
  return options_;
}

ACE_INLINE
void EncapsulationHeader::options(ACE_CDR::UShort value)
{
  options_ = value;
}

ACE_INLINE
bool serialized_size(const Encoding& /*encoding*/, size_t& size,
  const EncapsulationHeader& /*value*/)
{
  size += EncapsulationHeader::serialized_size;
  return true;
}

ACE_INLINE
const Encoding& Serializer::encoding() const
{
  return encoding_;
}

ACE_INLINE
void Serializer::encoding(const Encoding& value)
{
  encoding_ = value;
  swap_bytes_ = value.endianness() != ENDIAN_NATIVE;
}

ACE_INLINE
void Serializer::endianness(Endianness value)
{
  Encoding enc = encoding();
  enc.endianness(value);
  encoding(enc);
}

ACE_INLINE
Endianness Serializer::endianness() const
{
  return encoding_.endianness();
}


// NOTE: I use the ternary operators in here for conditionals to help
//       the compiler inline the code -- and it does end up fairly
//       tight...
ACE_INLINE size_t
Serializer::doread(char* dest, size_t size, bool swap, size_t offset)
{
  //
  // Ensure we work only with buffer data.
  //
  if (current_ == 0) {
    good_bit_ = false;
    return size;
  }

  //
  // Determine how much data will remain to be read after the current
  // buffer has been entirely read.
  //
  const size_t len = current_->length();
  const size_t remainder = (size - offset > len) ? size - offset - len : 0;

  //
  // Derive how much data we need to read from the current buffer.
  //
  const size_t initial = size - offset - remainder;

  //
  // Copy or swap the source data from the current buffer into the
  // destination.
  //
  swap
    ? swapcpy(dest + remainder, current_->rd_ptr(), initial)
    : smemcpy(dest + offset, current_->rd_ptr(), initial);
  current_->rd_ptr(initial);

  // Update the logical reading position in the stream.
  rpos_ += initial;

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
  if (current_->length() == 0) {
    if (encoding().alignment()) {
      align_cont_r();
    } else {
      current_ = current_->cont();
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
  size_t offset = 0;

  while (size > offset) {
    offset = doread(dest, size, swap, offset);
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
  if (current_ == 0) {
    good_bit_ = false;
    return size;
  }

  //
  // Determine how much data will remain to be written after the current
  // buffer has been entirely filled.
  //
  const size_t spc = current_->space();
  const size_t remainder = (size - offset > spc) ? size - offset - spc : 0;

  //
  // Derive how much data we need to write to the current buffer.
  //
  const size_t initial = size - offset - remainder;

  //
  // Copy or swap the source data into the current buffer.
  //
  swap
    ? swapcpy(current_->wr_ptr(), src + remainder, initial)
    : smemcpy(current_->wr_ptr(), src + offset, initial);
  current_->wr_ptr(initial);

  // Update the logical writing position in the stream.
  wpos_ += initial;

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
  if (current_->space() == 0) {
    if (encoding().alignment()) {
      align_cont_w();
    } else {
      current_ = current_->cont();
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
  size_t offset = 0;

  while (size > offset) {
    offset = dowrite(src, size, swap, offset);
  }
}

ACE_INLINE
void Serializer::swap_bytes(bool do_swap)
{
  Encoding enc = encoding_;
  enc.endianness(do_swap ? ENDIAN_NONNATIVE : ENDIAN_NATIVE);
  encoding(enc);
}

ACE_INLINE bool
Serializer::swap_bytes() const
{
  return swap_bytes_;
}

ACE_INLINE
Encoding::Alignment Serializer::alignment() const
{
  return encoding().alignment();
}

ACE_INLINE
void Serializer::alignment(Encoding::Alignment value)
{
  Encoding enc = encoding_;
  enc.alignment(value);
  encoding(enc);
}

ACE_INLINE bool
Serializer::good_bit() const
{
  return good_bit_;
}

ACE_INLINE size_t
Serializer::length() const
{
  return good_bit_ && current_ ? current_->total_length() : 0;
}

ACE_INLINE bool
Serializer::skip(size_t n, int size)
{
  if (size > 1 && !align_r((std::min)(size_t(size), encoding().max_align()))) {
    return false;
  }

  for (size_t len = static_cast<size_t>(n * size); len;) {
    if (!current_) {
      good_bit_ = false;
      return false;
    }
    const size_t cur_len = current_->length();
    if (cur_len <= len) {
      len -= cur_len;
      current_->rd_ptr(current_->wr_ptr());
      align_cont_r();
    } else {
      current_->rd_ptr(len);
      break;
    }
  }

  if (good_bit_) {
    rpos_ += n * size;
  }
  return good_bit();
}

ACE_INLINE void
Serializer::read_array(char* x, size_t size, ACE_CDR::ULong length)
{
  read_array(x, size, length, swap_bytes_);
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
    buffer_read(x, size * length, false);

  } else {
    //
    // Swapping _must_ be done at 'size' boundaries, so we need to spin
    // through the array element by element.  This silently corrupts the
    // data if there is padding in the buffer.
    //
    while (length-- > 0) {
      buffer_read(x, size, true);
      x += size;
    }
  }
}

ACE_INLINE void
Serializer::write_array(const char* x, size_t size, ACE_CDR::ULong length)
{
  write_array(x, size, length, swap_bytes_);
}

ACE_INLINE void
Serializer::write_array(const char* x, size_t size,
                        ACE_CDR::ULong length, bool swap)
{
  if (!swap || size == 1) {
    //
    // No swap, copy direct.
    //
    buffer_write(x, size * length, false);

  } else {
    //
    // Swapping _must_ be done at 'size' boundaries, so we need to spin
    // through the array element by element.
    // NOTE: This assumes that there is _no_ padding between the array
    //       elements.  If this is not the case, do not use this
    //       method.
    //
    while (length-- > 0) {
      buffer_write(x, size, true);
      x += size;
    }
  }
}

ACE_INLINE bool
Serializer::read_boolean_array(ACE_CDR::Boolean* x, ACE_CDR::ULong length)
{
  read_array(reinterpret_cast<char*>(x), boolean_cdr_size, length);
  return good_bit();
}

ACE_INLINE bool
Serializer::read_char_array(ACE_CDR::Char* x, ACE_CDR::ULong length)
{
  read_array(reinterpret_cast<char*>(x), char8_cdr_size, length);
  return good_bit();
}

ACE_INLINE bool
Serializer::read_wchar_array(ACE_CDR::WChar* x, ACE_CDR::ULong length)
{
  for (ACE_CDR::ULong i = 0; i < length; ++i) {
    if (!((*this) >> ACE_InputCDR::to_wchar(x[i]))) {
      break;
    }
  }
  return good_bit();
}

ACE_INLINE bool
Serializer::read_octet_array(ACE_CDR::Octet* x, ACE_CDR::ULong length)
{
  read_array(reinterpret_cast<char*>(x), byte_cdr_size, length);
  return good_bit();
}

#if OPENDDS_HAS_EXPLICIT_INTS
ACE_INLINE
bool Serializer::read_int8_array(ACE_CDR::Int8* x, ACE_CDR::ULong length)
{
  read_array(reinterpret_cast<char*>(x), int8_cdr_size, length);
  return good_bit();
}

ACE_INLINE
bool Serializer::read_uint8_array(ACE_CDR::UInt8* x, ACE_CDR::ULong length)
{
  read_array(reinterpret_cast<char*>(x), uint8_cdr_size, length);
  return good_bit();
}
#endif

ACE_INLINE bool
Serializer::read_short_array(ACE_CDR::Short* x, ACE_CDR::ULong length)
{
  if (!align_r(int16_cdr_size)) {
    return false;
  }
  read_array(reinterpret_cast<char*>(x), int16_cdr_size, length);
  return good_bit();
}

ACE_INLINE bool
Serializer::read_ushort_array(ACE_CDR::UShort* x, ACE_CDR::ULong length)
{
  if (!align_r(uint16_cdr_size)) {
    return false;
  }
  read_array(reinterpret_cast<char*>(x), uint16_cdr_size, length);
  return good_bit();
}

ACE_INLINE bool
Serializer::read_long_array(ACE_CDR::Long* x, ACE_CDR::ULong length)
{
  if (!align_r(int32_cdr_size)) {
    return false;
  }
  read_array(reinterpret_cast<char*>(x), int32_cdr_size, length);
  return good_bit();
}

ACE_INLINE bool
Serializer::read_ulong_array(ACE_CDR::ULong* x, ACE_CDR::ULong length)
{
  if (!align_r(uint32_cdr_size)) {
    return false;
  }
  read_array(reinterpret_cast<char*>(x), uint32_cdr_size, length);
  return good_bit();
}

ACE_INLINE bool
Serializer::read_longlong_array(ACE_CDR::LongLong* x, ACE_CDR::ULong length)
{
  if (!align_r(int64_cdr_size)) {
    return false;
  }
  read_array(reinterpret_cast<char*>(x), int64_cdr_size, length);
  return good_bit();
}

ACE_INLINE bool
Serializer::read_ulonglong_array(ACE_CDR::ULongLong* x, ACE_CDR::ULong length)
{
  if (!align_r(uint64_cdr_size)) {
    return false;
  }
  read_array(reinterpret_cast<char*>(x), uint64_cdr_size, length);
  return good_bit();
}

ACE_INLINE bool
Serializer::read_float_array(ACE_CDR::Float* x, ACE_CDR::ULong length)
{
  if (!align_r(float32_cdr_size)) {
    return false;
  }
  read_array(reinterpret_cast<char*>(x), float32_cdr_size, length);
  return good_bit();
}

ACE_INLINE bool
Serializer::read_double_array(ACE_CDR::Double* x, ACE_CDR::ULong length)
{
  if (!align_r(float64_cdr_size)) {
    return false;
  }
  read_array(reinterpret_cast<char*>(x), float64_cdr_size, length);
  return good_bit();
}

ACE_INLINE bool
Serializer::read_longdouble_array(ACE_CDR::LongDouble* x, ACE_CDR::ULong length)
{
  if (!align_r(float128_cdr_size)) {
    return false;
  }
  read_array(reinterpret_cast<char*>(x), float128_cdr_size, length);
  return good_bit();
}

ACE_INLINE bool
Serializer::write_boolean_array(const ACE_CDR::Boolean* x,
                                ACE_CDR::ULong length)
{
  write_array(reinterpret_cast<const char*>(x), boolean_cdr_size, length);
  return good_bit();
}

ACE_INLINE bool
Serializer::write_char_array(const ACE_CDR::Char* x, ACE_CDR::ULong length)
{
  write_array(reinterpret_cast<const char*>(x), char8_cdr_size, length);
  return good_bit();
}

ACE_INLINE bool
Serializer::write_wchar_array(const ACE_CDR::WChar* x, ACE_CDR::ULong length)
{
  for (ACE_CDR::ULong i = 0; i < length; ++i) {
    if (!((*this) << ACE_OutputCDR::from_wchar(x[i]))) {
      break;
    }
  }
  return good_bit();
}

ACE_INLINE bool
Serializer::write_octet_array(const ACE_CDR::Octet* x, ACE_CDR::ULong length)
{
  write_array(reinterpret_cast<const char*>(x), byte_cdr_size, length);
  return good_bit();
}

#if OPENDDS_HAS_EXPLICIT_INTS
ACE_INLINE
bool Serializer::write_int8_array(const ACE_CDR::Int8* x, ACE_CDR::ULong length)
{
  write_array(reinterpret_cast<const char*>(x), int8_cdr_size, length);
  return good_bit();
}

ACE_INLINE
bool Serializer::write_uint8_array(const ACE_CDR::UInt8* x, ACE_CDR::ULong length)
{
  write_array(reinterpret_cast<const char*>(x), uint8_cdr_size, length);
  return good_bit();
}
#endif

ACE_INLINE bool
Serializer::write_short_array(const ACE_CDR::Short* x, ACE_CDR::ULong length)
{
  if (!align_w(int16_cdr_size)) {
    return false;
  }
  write_array(reinterpret_cast<const char*>(x), int16_cdr_size, length);
  return good_bit();
}

ACE_INLINE bool
Serializer::write_ushort_array(const ACE_CDR::UShort* x, ACE_CDR::ULong length)
{
  if (!align_w(uint16_cdr_size)) {
    return false;
  }
  write_array(reinterpret_cast<const char*>(x), uint16_cdr_size, length);
  return good_bit();
}

ACE_INLINE bool
Serializer::write_long_array(const ACE_CDR::Long* x, ACE_CDR::ULong length)
{
  if (!align_w(int32_cdr_size)) {
    return false;
  }
  write_array(reinterpret_cast<const char*>(x), int32_cdr_size, length);
  return good_bit();
}

ACE_INLINE bool
Serializer::write_ulong_array(const ACE_CDR::ULong* x, ACE_CDR::ULong length)
{
  if (!align_w(uint32_cdr_size)) {
    return false;
  }
  write_array(reinterpret_cast<const char*>(x), uint32_cdr_size, length);
  return good_bit();
}

ACE_INLINE bool
Serializer::write_longlong_array(const ACE_CDR::LongLong* x,
                                 ACE_CDR::ULong length)
{
  if (!align_w(int64_cdr_size)) {
    return false;
  }
  write_array(reinterpret_cast<const char*>(x), int64_cdr_size, length);
  return good_bit();
}

ACE_INLINE bool
Serializer::write_ulonglong_array(const ACE_CDR::ULongLong* x,
                                  ACE_CDR::ULong length)
{
  if (!align_w(uint64_cdr_size)) {
    return false;
  }
  write_array(reinterpret_cast<const char*>(x), uint64_cdr_size, length);
  return good_bit();
}

ACE_INLINE bool
Serializer::write_float_array(const ACE_CDR::Float* x, ACE_CDR::ULong length)
{
  if (!align_w(float32_cdr_size)) {
    return false;
  }
  write_array(reinterpret_cast<const char*>(x), float32_cdr_size, length);
  return good_bit();
}

ACE_INLINE bool
Serializer::write_double_array(const ACE_CDR::Double* x, ACE_CDR::ULong length)
{
  if (!align_w(float64_cdr_size)) {
    return false;
  }
  write_array(reinterpret_cast<const char*>(x), float64_cdr_size, length);
  return good_bit();
}

ACE_INLINE bool
Serializer::write_longdouble_array(const ACE_CDR::LongDouble* x,
                                   ACE_CDR::ULong length)
{
  if (!align_w(float128_cdr_size)) {
    return false;
  }
  write_array(reinterpret_cast<const char*>(x), float128_cdr_size, length);
  return good_bit();
}

ACE_INLINE
bool Serializer::align_r(size_t al)
{
  if (!alignment()) {
    return true;
  }
  if (!current_) {
    good_bit_ = false;
    return false;
  }
  al = (std::min)(al, encoding().max_align());
  const size_t len =
    (al - ptrdiff_t(current_->rd_ptr()) + align_rshift_) % al;

  return skip(static_cast<ACE_CDR::UShort>(len));
}

ACE_INLINE
bool Serializer::align_w(size_t al)
{
  if (!alignment()) {
    return true;
  }
  if (!current_) {
    good_bit_ = false;
    return false;
  }
  al = (std::min)(al, encoding().max_align());
  size_t len =
    (al - ptrdiff_t(current_->wr_ptr()) + align_wshift_) % al;
  while (len) {
    if (!current_) {
      good_bit_ = false;
      break;
    }
    const size_t cur_spc = current_->space();
    if (cur_spc <= len) {
      len -= cur_spc;
      if (encoding().zero_init_padding()) {
        smemcpy(current_->wr_ptr(), ALIGN_PAD, cur_spc);
      }
      current_->wr_ptr(cur_spc);
      wpos_ += cur_spc;
      align_cont_w();
    } else {
      if (encoding().zero_init_padding()) {
        smemcpy(current_->wr_ptr(), ALIGN_PAD, len);
      }
      current_->wr_ptr(len);
      wpos_ += len;
      break;
    }
  }
  return good_bit_;
}

ACE_INLINE unsigned char
Serializer::offset(char* index, size_t start, size_t align)
{
  return static_cast<unsigned char>((ptrdiff_t(index) - start) % align);
}

ACE_INLINE void
Serializer::align_cont_r()
{
  const size_t max_align = encoding().max_align();
  const size_t thisblock =
    max_align ? (ptrdiff_t(current_->rd_ptr()) - align_rshift_) % max_align : 0;

  current_ = current_->cont();

  if (current_ && max_align) {
    align_rshift_ = offset(current_->rd_ptr(), thisblock, max_align);
  }
}

ACE_INLINE void
Serializer::align_cont_w()
{
  const size_t max_align = encoding().max_align();
  const size_t thisblock =
    max_align ? (ptrdiff_t(current_->wr_ptr()) - align_wshift_) % max_align : 0;

  current_ = current_->cont();

  if (current_ && max_align) {
    align_wshift_ = offset(current_->wr_ptr(), thisblock, max_align);
  }
}

ACE_INLINE
bool Serializer::read_delimiter(size_t& size)
{
  if (encoding().xcdr_version() == Encoding::XCDR_VERSION_2) {
    ACE_CDR::ULong dheader;
    if (!(*this >> dheader)) {
      return false;
    }
    size = dheader;
  }
  return true;
}

ACE_INLINE
bool Serializer::write_delimiter(size_t size)
{
  if (encoding().xcdr_version() == Encoding::XCDR_VERSION_2) {
    return *this << static_cast<ACE_CDR::ULong>(size - uint32_cdr_size);
  }
  return true;
}

ACE_INLINE
bool Serializer::write_list_end_parameter_id()
{
  if (encoding().xcdr_version() == Encoding::XCDR_VERSION_1) {
    return align_w(xcdr1_pid_alignment) && *this << pid_list_end && *this << ACE_CDR::UShort(0);
  }
  return true;
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
  s.buffer_write(reinterpret_cast<char*>(&x), char8_cdr_size, s.swap_bytes());
  return s.good_bit();
}

ACE_INLINE bool
operator<<(Serializer& s, ACE_CDR::Short x)
{
  if (!s.align_w(int16_cdr_size)) {
    return false;
  }
  s.buffer_write(reinterpret_cast<char*>(&x), int16_cdr_size, s.swap_bytes());
  return s.good_bit();
}

ACE_INLINE bool
operator<<(Serializer& s, ACE_CDR::UShort x)
{
  if (!s.align_w(uint16_cdr_size)) {
    return false;
  }
  s.buffer_write(reinterpret_cast<char*>(&x), uint16_cdr_size, s.swap_bytes());
  return s.good_bit();
}

ACE_INLINE bool
operator<<(Serializer& s, ACE_CDR::Long x)
{
  if (!s.align_w(int32_cdr_size)) {
    return false;
  }
  s.buffer_write(reinterpret_cast<char*>(&x), int32_cdr_size, s.swap_bytes());
  return s.good_bit();
}

ACE_INLINE bool
operator<<(Serializer& s, ACE_CDR::ULong x)
{
  if (!s.align_w(uint32_cdr_size)) {
    return false;
  }
  s.buffer_write(reinterpret_cast<char*>(&x), uint32_cdr_size, s.swap_bytes());
  return s.good_bit();
}

ACE_INLINE bool
operator<<(Serializer& s, ACE_CDR::LongLong x)
{
  if (!s.align_w(int64_cdr_size)) {
    return false;
  }
  s.buffer_write(reinterpret_cast<char*>(&x), int64_cdr_size, s.swap_bytes());
  return s.good_bit();
}

ACE_INLINE bool
operator<<(Serializer& s, ACE_CDR::ULongLong x)
{
  if (!s.align_w(uint64_cdr_size)) {
    return false;
  }
  s.buffer_write(reinterpret_cast<char*>(&x), uint64_cdr_size, s.swap_bytes());
  return s.good_bit();
}

ACE_INLINE bool
operator<<(Serializer& s, ACE_CDR::Float x)
{
  if (!s.align_w(float32_cdr_size)) {
    return false;
  }
  s.buffer_write(reinterpret_cast<char*>(&x), float32_cdr_size, s.swap_bytes());
  return s.good_bit();
}

ACE_INLINE bool
operator<<(Serializer& s, ACE_CDR::Double x)
{
  if (!s.align_w(float64_cdr_size)) {
    return false;
  }
  s.buffer_write(reinterpret_cast<char*>(&x), float64_cdr_size, s.swap_bytes());
  return s.good_bit();
}

ACE_INLINE bool
operator<<(Serializer& s, ACE_CDR::LongDouble x)
{
  if (!s.align_w(float128_cdr_size)) {
    return false;
  }
  s.buffer_write(reinterpret_cast<char*>(&x), float128_cdr_size, s.swap_bytes());
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
  if (x != 0) {
    // Do not include the null terminatator in the serialized data.
    const ACE_CDR::ULong length = static_cast<ACE_CDR::ULong>(ACE_OS::strlen(x));
    s << ACE_CDR::ULong(length * char16_cdr_size);

#if ACE_SIZEOF_WCHAR == 2
    s.write_array(reinterpret_cast<const char*>(x), char16_cdr_size, length,
      s.swap_bytes());
#else
    for (size_t i = 0; i < length && s.good_bit(); ++i) {
      const ACE_UINT16 as_utf16 = static_cast<ACE_UINT16>(x[i]);
      if (as_utf16 != x[i]) { // not currently handling surrogates
        s.good_bit_ = false;
        break;
      }
      s.buffer_write(reinterpret_cast<const char*>(&as_utf16), char16_cdr_size,
        s.swap_bytes());
    }
#endif
  } else {
    s << ACE_CDR::ULong(0);
  }

  return s.good_bit();
}

#ifdef NONNATIVE_LONGDOUBLE
ACE_INLINE bool
operator<<(Serializer& s, long double x)
{
  if (!s.align_w(float128_cdr_size)) {
    return false;
  }
  ACE_CDR::LongDouble ld;
  ACE_CDR_LONG_DOUBLE_ASSIGNMENT(ld, x);
  return s << ld;
}
#endif

ACE_INLINE bool
operator<<(Serializer& s, ACE_OutputCDR::from_boolean x)
{
  s.buffer_write(reinterpret_cast<char*>(&x.val_), boolean_cdr_size, false);
  return s.good_bit();
}

ACE_INLINE bool
operator<<(Serializer& s, ACE_OutputCDR::from_char x)
{
  s.buffer_write(reinterpret_cast<char*>(&x.val_), char8_cdr_size, false);
  return s.good_bit();
}

ACE_INLINE bool
operator<<(Serializer& s, ACE_OutputCDR::from_wchar x)
{
  if (!s.align_w(char16_cdr_size)) {
    return false;
  }
#if ACE_SIZEOF_WCHAR == 2
  s.buffer_write(reinterpret_cast<char*>(&x.val_), char16_cdr_size, s.swap_bytes());
#else
  const ACE_UINT16 as_utf16 = static_cast<ACE_UINT16>(x.val_);
  if (as_utf16 != x.val_) { // not currently handling surrogates
    if (DCPS_debug_level) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ")
        ACE_TEXT("operator<<(Serializer&, ACE_OutputCDR::from_wchar): ")
        ACE_TEXT("failure to convert UTF-32 to UTF-16.\n")));
    }
    s.good_bit_ = false;
  } else {
    s.buffer_write(reinterpret_cast<const char*>(&as_utf16), char16_cdr_size,
      s.swap_bytes());
  }
#endif
  return s.good_bit();
}

ACE_INLINE bool
operator<<(Serializer& s, const String& x)
{
  return s << x.c_str();
}

ACE_INLINE bool
operator<<(Serializer& s, Serializer::FromBoundedString<char> x)
{
  return (x.bound_ == 0 || x.str_.size() <= x.bound_) && s << x.str_;
}

#ifdef DDS_HAS_WCHAR
ACE_INLINE bool
operator<<(Serializer& s, const WString& x)
{
  return s << x.c_str();
}

ACE_INLINE bool
operator<<(Serializer& s, Serializer::FromBoundedString<wchar_t> x)
{
  return (x.bound_ == 0 || x.str_.size() <= x.bound_) && s << x.str_;
}
#endif /* DDS_HAS_WCHAR */

ACE_INLINE bool
operator<<(Serializer& s, ACE_OutputCDR::from_octet x)
{
  s.buffer_write(reinterpret_cast<char*>(&x.val_), byte_cdr_size, false);
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

#if OPENDDS_HAS_EXPLICIT_INTS
ACE_INLINE
bool operator<<(Serializer& s, ACE_OutputCDR::from_uint8 x)
{
  s.buffer_write(reinterpret_cast<const char*>(&x.val_), uint8_cdr_size, false);
  return s.good_bit();
}

ACE_INLINE
bool operator<<(Serializer& s, ACE_OutputCDR::from_int8 x)
{
  s.buffer_write(reinterpret_cast<const char*>(&x.val_), int8_cdr_size, false);
  return s.good_bit();
}
#endif

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
  s.buffer_read(reinterpret_cast<char*>(&x), char8_cdr_size, s.swap_bytes());
  return s.good_bit();
}

ACE_INLINE bool
operator>>(Serializer& s, ACE_CDR::Short& x)
{
  if (!s.align_r(int16_cdr_size)) {
    return false;
  }
  s.buffer_read(reinterpret_cast<char*>(&x), int16_cdr_size, s.swap_bytes());
  return s.good_bit();
}

ACE_INLINE bool
operator>>(Serializer& s, ACE_CDR::UShort& x)
{
  if (!s.align_r(uint16_cdr_size)) {
    return false;
  }
  s.buffer_read(reinterpret_cast<char*>(&x), uint16_cdr_size, s.swap_bytes());
  return s.good_bit();
}

ACE_INLINE bool
operator>>(Serializer& s, ACE_CDR::Long& x)
{
  if (!s.align_r(int32_cdr_size)) {
    return false;
  }
  s.buffer_read(reinterpret_cast<char*>(&x), int32_cdr_size, s.swap_bytes());
  return s.good_bit();
}

ACE_INLINE bool
operator>>(Serializer& s, ACE_CDR::ULong& x)
{
  if (!s.align_r(uint32_cdr_size)) {
    return false;
  }
  s.buffer_read(reinterpret_cast<char*>(&x), uint32_cdr_size, s.swap_bytes());
  return s.good_bit();
}

ACE_INLINE bool
operator>>(Serializer& s, ACE_CDR::LongLong& x)
{
  if (!s.align_r(int64_cdr_size)) {
    return false;
  }
  s.buffer_read(reinterpret_cast<char*>(&x), int64_cdr_size, s.swap_bytes());
  return s.good_bit();
}

ACE_INLINE bool
operator>>(Serializer& s, ACE_CDR::ULongLong& x)
{
  if (!s.align_r(uint64_cdr_size)) {
    return false;
  }
  s.buffer_read(reinterpret_cast<char*>(&x), uint64_cdr_size, s.swap_bytes());
  return s.good_bit();
}

ACE_INLINE bool
operator>>(Serializer& s, ACE_CDR::Float& x)
{
  if (!s.align_r(float32_cdr_size)) {
    return false;
  }
  s.buffer_read(reinterpret_cast<char*>(&x), float32_cdr_size, s.swap_bytes());
  return s.good_bit();
}

ACE_INLINE bool
operator>>(Serializer& s, ACE_CDR::Double& x)
{
  if (!s.align_r(float64_cdr_size)) {
    return false;
  }
  s.buffer_read(reinterpret_cast<char*>(&x), float64_cdr_size, s.swap_bytes());
  return s.good_bit();
}

ACE_INLINE bool
operator>>(Serializer& s, ACE_CDR::LongDouble& x)
{
  if (!s.align_r(float128_cdr_size)) {
    return false;
  }
  s.buffer_read(reinterpret_cast<char*>(&x), float128_cdr_size, s.swap_bytes());
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

#ifdef NONNATIVE_LONGDOUBLE
ACE_INLINE bool
operator>>(Serializer& s, long double& x)
{
  if (!s.align_r(float128_cdr_size)) {
    return false;
  }
  ACE_CDR::LongDouble ld;
  if (s >> ld) {
    x = ld;
    return true;
  }
  return false;
}
#endif

ACE_INLINE bool
operator>>(Serializer& s, ACE_InputCDR::to_boolean x)
{
  s.buffer_read(reinterpret_cast<char*>(&x.ref_), boolean_cdr_size, s.swap_bytes());
  return s.good_bit();
}

ACE_INLINE bool
operator>>(Serializer& s, ACE_InputCDR::to_char x)
{
  s.buffer_read(reinterpret_cast<char*>(&x.ref_), char8_cdr_size, s.swap_bytes());
  return s.good_bit();
}

ACE_INLINE bool
operator>>(Serializer& s, ACE_InputCDR::to_wchar x)
{
  if (!s.align_r(char16_cdr_size)) {
    return false;
  }
#if ACE_SIZEOF_WCHAR == 2
  s.buffer_read(reinterpret_cast<char*>(&x.ref_), char16_cdr_size,
    s.swap_bytes());
#else
  ACE_UINT16 as_utf16;
  s.buffer_read(reinterpret_cast<char*>(&as_utf16), char16_cdr_size,
    s.swap_bytes());
  x.ref_ = as_utf16;
#endif
  return s.good_bit();
}

ACE_INLINE bool
operator>>(Serializer& s, ACE_InputCDR::to_octet x)
{
  s.buffer_read(reinterpret_cast<char*>(&x.ref_), byte_cdr_size, false);
  return s.good_bit();
}

ACE_INLINE bool
operator>>(Serializer& s, ACE_InputCDR::to_string x)
{
  const size_t length = s.read_string(const_cast<char*&>(x.val_));
  if (s.good_bit() && (x.bound_ != 0) && (length > x.bound_)) {
    s.set_construction_status(Serializer::BoundConstructionFailure);
    return false;
  }
  return s.good_bit();
}

ACE_INLINE bool
operator>>(Serializer& s, ACE_InputCDR::to_wstring x)
{
  const size_t length = s.read_string(const_cast<ACE_CDR::WChar*&>(x.val_));
  if (s.good_bit() && (x.bound_ != 0) && (length > x.bound_)) {
    s.set_construction_status(Serializer::BoundConstructionFailure);
    return false;
  }
  return s.good_bit();
}

#if OPENDDS_HAS_EXPLICIT_INTS
ACE_INLINE
bool operator>>(Serializer& s, ACE_InputCDR::to_uint8 x)
{
  s.buffer_read(reinterpret_cast<char*>(&x.ref_), uint8_cdr_size, false);
  return s.good_bit();
}

ACE_INLINE
bool operator>>(Serializer& s, ACE_InputCDR::to_int8 x)
{
  s.buffer_read(reinterpret_cast<char*>(&x.ref_), int8_cdr_size, false);
  return s.good_bit();
}
#endif

ACE_INLINE bool
operator>>(Serializer& s, String& x)
{
  char* buf = 0;
  const size_t length = s.read_string(buf);
  x.assign(buf, length);
  s.free_string(buf);
  return s.good_bit();
}

ACE_INLINE bool
operator>>(Serializer& s, Serializer::ToBoundedString<char> x)
{
  if (s >> x.str_) {
    if ((x.bound_ != 0) && (x.str_.size() > x.bound_)) {
      s.set_construction_status(Serializer::BoundConstructionFailure);
      return false;
    }
    return true;
  }
  return false;
}

#ifdef DDS_HAS_WCHAR
ACE_INLINE bool
operator>>(Serializer& s, WString& x)
{
  ACE_CDR::WChar* buf = 0;
  const size_t length = s.read_string(buf);
  x.assign(buf, length);
  s.free_string(buf);
  return s.good_bit();
}

ACE_INLINE bool
operator>>(Serializer& s, Serializer::ToBoundedString<wchar_t> x)
{
  if (s >> x.str_) {
    if ((x.bound_ != 0) && (x.str_.size() > x.bound_)) {
      s.set_construction_status(Serializer::BoundConstructionFailure);
      return false;
    }
    return true;
  }
  return false;
}
#endif /* DDS_HAS_WCHAR */

//----------------------------------------------------------------------------
// predefined type methods

ACE_INLINE
bool primitive_serialized_size(
  const Encoding& encoding, size_t& size,
  const ACE_CDR::Short& /*value*/, size_t count)
{
  encoding.align(size, int16_cdr_size);
  size += int16_cdr_size * count;
  return true;
}

ACE_INLINE
bool primitive_serialized_size(
  const Encoding& encoding, size_t& size,
  const ACE_CDR::UShort& /*value*/, size_t count)
{
  encoding.align(size, uint16_cdr_size);
  size += uint16_cdr_size * count;
  return true;
}

ACE_INLINE
bool primitive_serialized_size(
  const Encoding& encoding, size_t& size,
  const ACE_CDR::Long& /*value*/, size_t count)
{
  encoding.align(size, int32_cdr_size);
  size += int32_cdr_size * count;
  return true;
}

ACE_INLINE
bool primitive_serialized_size(
  const Encoding& encoding, size_t& size,
  const ACE_CDR::ULong& /*value*/, size_t count)
{
  encoding.align(size, uint32_cdr_size);
  size += uint32_cdr_size * count;
  return true;
}

ACE_INLINE
bool primitive_serialized_size(
  const Encoding& encoding, size_t& size,
  const ACE_CDR::LongLong& /*value*/, size_t count)
{
  encoding.align(size, int64_cdr_size);
  size += int64_cdr_size * count;
  return true;
}

ACE_INLINE
bool primitive_serialized_size(
  const Encoding& encoding, size_t& size,
  const ACE_CDR::ULongLong& /*value*/, size_t count)
{
  encoding.align(size, uint64_cdr_size);
  size += uint64_cdr_size * count;
  return true;
}

ACE_INLINE
bool primitive_serialized_size(
  const Encoding& encoding, size_t& size,
  const ACE_CDR::Float& /*value*/, size_t count)
{
  encoding.align(size, float32_cdr_size);
  size += float32_cdr_size * count;
  return true;
}

ACE_INLINE
bool primitive_serialized_size(
  const Encoding& encoding, size_t& size,
  const ACE_CDR::Double& /*value*/, size_t count)
{
  encoding.align(size, float64_cdr_size);
  size += float64_cdr_size * count;
  return true;
}

ACE_INLINE
bool primitive_serialized_size(
  const Encoding& encoding, size_t& size,
  const ACE_CDR::LongDouble& /*value*/, size_t count)
{
  encoding.align(size, float128_cdr_size);
  size += float128_cdr_size * count;
  return true;
}

// predefined type method disambiguators.
ACE_INLINE
bool primitive_serialized_size(
  const Encoding& /*encoding*/, size_t& size,
  const ACE_OutputCDR::from_boolean /*value*/, size_t count)
{
  size += boolean_cdr_size * count;
  return true;
}

ACE_INLINE
bool primitive_serialized_size(
  const Encoding& /*encoding*/, size_t& size,
  const ACE_OutputCDR::from_char /*value*/, size_t count)
{
  size += char8_cdr_size * count;
  return true;
}

ACE_INLINE
bool primitive_serialized_size(
  const Encoding& encoding, size_t& size,
  const ACE_OutputCDR::from_wchar /*value*/, size_t count)
{
  encoding.align(size, char16_cdr_size);
  size += char16_cdr_size * count;
  return true;
}

ACE_INLINE
bool primitive_serialized_size(
  const Encoding& /*encoding*/, size_t& size,
  const ACE_OutputCDR::from_octet /*value*/, size_t count)
{
  size += byte_cdr_size * count;
  return true;
}

#if OPENDDS_HAS_EXPLICIT_INTS
ACE_INLINE
bool primitive_serialized_size(
  const Encoding& /*encoding*/, size_t& size,
  const ACE_OutputCDR::from_uint8 /*value*/, size_t count)
{
  size += uint8_cdr_size * count;
  return true;
}

ACE_INLINE
bool primitive_serialized_size(
  const Encoding& /*encoding*/, size_t& size,
  const ACE_OutputCDR::from_int8 /*value*/, size_t count)
{
  size += int8_cdr_size * count;
  return true;
}
#endif

// predefined type method explicit disambiguators.

ACE_INLINE
void primitive_serialized_size_boolean(const Encoding& /*encoding*/, size_t& size,
  size_t count)
{
  size += boolean_cdr_size * count;
}

ACE_INLINE
void primitive_serialized_size_char(const Encoding& /*encoding*/, size_t& size,
  size_t count)
{
  size += char8_cdr_size * count;
}

ACE_INLINE
void primitive_serialized_size_wchar(const Encoding& encoding, size_t& size,
  size_t count)
{
  encoding.align(size, char16_cdr_size);
  size += char16_cdr_size * count;
}

ACE_INLINE
void primitive_serialized_size_octet(const Encoding& /*encoding*/, size_t& size,
  size_t count)
{
  size += byte_cdr_size * count;
}

ACE_INLINE
void primitive_serialized_size_ulong(const Encoding& encoding, size_t& size,
  size_t count)
{
  encoding.align(size, uint32_cdr_size);
  size += uint32_cdr_size * count;
}

#if OPENDDS_HAS_EXPLICIT_INTS
ACE_INLINE
void primitive_serialized_size_uint8(const Encoding& /*encoding*/, size_t& size,
  size_t count)
{
  size += uint8_cdr_size * count;
}

ACE_INLINE
void primitive_serialized_size_int8(const Encoding& /*encoding*/, size_t& size,
  size_t count)
{
  size += int8_cdr_size * count;
}
#endif

ACE_INLINE
void serialized_size_delimiter(const Encoding& encoding, size_t& size)
{
  if (encoding.xcdr_version() == Encoding::XCDR_VERSION_2) {
    primitive_serialized_size_ulong(encoding, size);
  }
}

ACE_INLINE
void serialized_size_parameter_id(
  const Encoding& encoding, size_t& size, size_t& running_size)
{
  const Encoding::XcdrVersion xcdr = encoding.xcdr_version();
  if (xcdr == Encoding::XCDR_VERSION_1) {
    encoding.align(size, xcdr1_pid_alignment);
    size += uint16_cdr_size * 2;
    // TODO(iguessthislldo): Extended PID

    // Save and Zero Size to Reset the Alignment
    running_size += size;
    size = 0;
  } else if (xcdr == Encoding::XCDR_VERSION_2) {
    if (running_size != 0 && size != 1 && size != 2 && size != 4 && size != 8) {
      size += uint32_cdr_size; // nextint
    }
    encoding.align(size, uint32_cdr_size);
    size += uint32_cdr_size; // emheader
    running_size += size;
    size = 0;
  }
}

ACE_INLINE
void serialized_size_list_end_parameter_id(
  const Encoding& encoding, size_t& size, size_t& running_size)
{
  if (encoding.xcdr_version() == Encoding::XCDR_VERSION_1) {
    /*
     * TODO(iguessthislldo): See how DDSXTY14-23 is resolved.
     * https://github.com/objectcomputing/OpenDDS/pull/1722#discussion_r447165924
     */
    encoding.align(size, xcdr1_pid_alignment);
    size += uint16_cdr_size * 2;

    // Restore Saved Totals from Alignment Resets
    size += running_size;
  } else if (encoding.xcdr_version() == Encoding::XCDR_VERSION_2) {
    if (running_size != 0 && size != 1 && size != 2 && size != 4 && size != 8) {
      size += uint32_cdr_size; // nextint
    }
    size += running_size;
  }
}

ACE_INLINE
Serializer::ConstructionStatus Serializer::get_construction_status() const
{
  return construction_status_;
}

ACE_INLINE
void Serializer::set_construction_status(ConstructionStatus cs)
{
  construction_status_ = cs;
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
