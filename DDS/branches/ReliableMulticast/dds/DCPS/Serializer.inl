// -*- C++ -*-
//
// $Id$

#include  <ace/Message_Block.h>
#include  <ace/CDR_Stream.h>
#include  "Serializer.h"


// NOTE: I use the ternary operators in here for conditionals to help
//       the compiler inline the code -- and it does end up fairly
//       tight...
ACE_INLINE size_t
TAO::DCPS::Serializer::doread (char* dest, size_t size, bool swap, size_t offset)
{
  //
  // Ensure we work only with buffer data.
  //
  if (this->current_ == 0)
    {
      this->good_bit_ = false;
      return size;
    }

  //
  // Determine how much data will remain to be read after the current
  // buffer has been entirely read.
  //
  register int remainder = size - offset - this->current_->length ();
  remainder = (remainder<0)? 0: remainder;

  //
  // Derive how much data we need to read from the current buffer.
  //
  register size_t initial = size-offset-remainder;

  //
  // Copy or swap the source data from the current buffer into the
  // destination.
  //
  swap? this->swapcpy (dest+remainder, this->current_->rd_ptr (), initial):
        this->smemcpy (dest+   offset, this->current_->rd_ptr (), initial);
  this->current_->rd_ptr (initial);

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
  this->current_->length ()==0? this->current_ = this->current_->cont () :0;

  //
  // Return the current location in the read.
  //
  return offset + initial;
}

ACE_INLINE void
TAO::DCPS::Serializer::buffer_read (char* dest, size_t size, bool swap)
{
  register size_t offset = 0;
  while (size > offset)
    {
      offset = this->doread (dest, size, swap, offset);
    }
}

// NOTE: I use the ternary operators in here for conditionals to help
//       the compiler inline the code -- and it does end up fairly
//       tight...
ACE_INLINE size_t
TAO::DCPS::Serializer::dowrite (const char* src, size_t size, bool swap, size_t offset)
{
  //
  // Ensure we work only with buffer data.
  //
  if (this->current_ == 0)
    {
      this->good_bit_ = false;
      return size;
    }

  //
  // Determine how much data will remain to be written after the current
  // buffer has been entirely filled.
  //
  register int remainder = size - offset - this->current_->space ();
  remainder = (remainder<0)? 0: remainder;

  //
  // Derive how much data we need to write to the current buffer.
  //
  register size_t initial = size-offset-remainder;

  //
  // Copy or swap the source data into the current buffer.
  //
  swap? this->swapcpy (this->current_->wr_ptr (), src+remainder, initial):
        this->smemcpy (this->current_->wr_ptr (), src+   offset, initial);
  this->current_->wr_ptr (initial);

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
  this->current_->space ()==0? this->current_ = this->current_->cont () :0;

  //
  // Return the current location in the write.
  //
  return offset + initial;
}

ACE_INLINE void
TAO::DCPS::Serializer::buffer_write (const char* src, size_t size, bool swap)
{
  register size_t offset = 0;
  while (size > offset)
    {
      offset = this->dowrite (src, size, swap, offset);
    }
}

ACE_INLINE ACE_Message_Block*
TAO::DCPS::Serializer::add_chain (ACE_Message_Block* chain)
{
  ACE_Message_Block* previous = this->start_;
  this->current_ = this->start_ = chain;
  return previous;
}

ACE_INLINE void
TAO::DCPS::Serializer::swap_bytes (bool do_swap)
{
  this->swap_bytes_ = do_swap;
}

ACE_INLINE bool
TAO::DCPS::Serializer::swap_bytes () const
{
  return this->swap_bytes_;
}

ACE_INLINE bool
TAO::DCPS::Serializer::good_bit () const
{
  return this->good_bit_;
}

ACE_INLINE void
TAO::DCPS::Serializer::read_array (char* x, size_t size, ACE_CDR::ULong length)
{
  //
  // Array reads will always honor the object swap state.
  //
  if (this->swap_bytes () == false)
    {
      //
      // No swap, copy direct.  This silently corrupts the data if there is
      // padding in the buffer.
      //
      this->buffer_read (x, size*length, false);

    }
  else
    {
      //
      // Swapping _must_ be done at 'size' boundaries, so we need to spin
      // through the array element by element.  This silently corrupts the
      // data if there is padding in the buffer.
      //
      while (length-- > 0)
        {
          this->buffer_read (x, size, true);
          x += size;
        }
    }
}

ACE_INLINE void
TAO::DCPS::Serializer::write_array (const char* x, size_t size, ACE_CDR::ULong length)
{
  //
  // Array writes will always honor the object swap state.
  //
  if (this->swap_bytes () == false)
    {
      //
      // No swap, copy direct.
      //
      this->buffer_write (x, size*length, false);

    }
  else
    {
      //
      // Swapping _must_ be done at 'size' boundaries, so we need to spin
      // through the array element by element.
      // NOTE: This assumes that there is _no_ padding between the array
      //       elements.  If this is not the case, do not use this
      //       method.
      //
      while (length-- > 0)
        {
          this->buffer_write (x, size, true);
          x += size;
        }
    }
}

ACE_INLINE ACE_CDR::Boolean
TAO::DCPS::Serializer::read_boolean_array (ACE_CDR::Boolean* x,
                                ACE_CDR::ULong length)
{
  this->read_array (reinterpret_cast<char*> (x), sizeof (ACE_CDR::Boolean), length);
  return static_cast<ACE_CDR::Boolean> (this->good_bit ());
}

ACE_INLINE ACE_CDR::Boolean
TAO::DCPS::Serializer::read_char_array (ACE_CDR::Char *x,
                             ACE_CDR::ULong length)
{
  this->read_array (reinterpret_cast<char*> (x), sizeof (ACE_CDR::Char), length);
  return static_cast<ACE_CDR::Boolean> (this->good_bit ());
}

ACE_INLINE ACE_CDR::Boolean
TAO::DCPS::Serializer::read_wchar_array (ACE_CDR::WChar* x,
                              ACE_CDR::ULong length)
{
  this->read_array (reinterpret_cast<char*> (x), sizeof (ACE_CDR::WChar), length);
  return static_cast<ACE_CDR::Boolean> (this->good_bit ());
}

ACE_INLINE ACE_CDR::Boolean
TAO::DCPS::Serializer::read_octet_array (ACE_CDR::Octet* x,
                              ACE_CDR::ULong length)
{
  this->read_array (reinterpret_cast<char*> (x), sizeof (ACE_CDR::Octet), length);
  return static_cast<ACE_CDR::Boolean> (this->good_bit ());
}

ACE_INLINE ACE_CDR::Boolean
TAO::DCPS::Serializer::read_short_array (ACE_CDR::Short *x,
                              ACE_CDR::ULong length)
{
  this->read_array (reinterpret_cast<char*> (x), sizeof (ACE_CDR::Short), length);
  return static_cast<ACE_CDR::Boolean> (this->good_bit ());
}

ACE_INLINE ACE_CDR::Boolean
TAO::DCPS::Serializer::read_ushort_array (ACE_CDR::UShort *x,
                               ACE_CDR::ULong length)
{
  this->read_array (reinterpret_cast<char*> (x), sizeof (ACE_CDR::UShort), length);
  return static_cast<ACE_CDR::Boolean> (this->good_bit ());
}

ACE_INLINE ACE_CDR::Boolean
TAO::DCPS::Serializer::read_long_array (ACE_CDR::Long *x,
                             ACE_CDR::ULong length)
{
  this->read_array (reinterpret_cast<char*> (x), sizeof (ACE_CDR::Long), length);
  return static_cast<ACE_CDR::Boolean> (this->good_bit ());
}

ACE_INLINE ACE_CDR::Boolean
TAO::DCPS::Serializer::read_ulong_array (ACE_CDR::ULong *x,
                              ACE_CDR::ULong length)
{
  this->read_array (reinterpret_cast<char*> (x), sizeof (ACE_CDR::ULong), length);
  return static_cast<ACE_CDR::Boolean> (this->good_bit ());
}

ACE_INLINE ACE_CDR::Boolean
TAO::DCPS::Serializer::read_longlong_array (ACE_CDR::LongLong* x,
                                 ACE_CDR::ULong length)
{
  this->read_array (reinterpret_cast<char*> (x), sizeof (ACE_CDR::LongLong), length);
  return static_cast<ACE_CDR::Boolean> (this->good_bit ());
}

ACE_INLINE ACE_CDR::Boolean
TAO::DCPS::Serializer::read_ulonglong_array (ACE_CDR::ULongLong* x,
                                  ACE_CDR::ULong length)
{
  this->read_array (reinterpret_cast<char*> (x), sizeof (ACE_CDR::ULongLong), length);
  return static_cast<ACE_CDR::Boolean> (this->good_bit ());
}

ACE_INLINE ACE_CDR::Boolean
TAO::DCPS::Serializer::read_float_array (ACE_CDR::Float *x,
                              ACE_CDR::ULong length)
{
  this->read_array (reinterpret_cast<char*> (x), sizeof (ACE_CDR::Float), length);
  return static_cast<ACE_CDR::Boolean> (this->good_bit ());
}

ACE_INLINE ACE_CDR::Boolean
TAO::DCPS::Serializer::read_double_array (ACE_CDR::Double *x,
                               ACE_CDR::ULong length)
{
  this->read_array (reinterpret_cast<char*> (x), sizeof (ACE_CDR::Double), length);
  return static_cast<ACE_CDR::Boolean> (this->good_bit ());
}

ACE_INLINE ACE_CDR::Boolean
TAO::DCPS::Serializer::read_longdouble_array (ACE_CDR::LongDouble* x,
                                   ACE_CDR::ULong length)
{
  this->read_array (reinterpret_cast<char*> (x), sizeof (ACE_CDR::LongDouble), length);
  return static_cast<ACE_CDR::Boolean> (this->good_bit ());
}


ACE_INLINE ACE_CDR::Boolean
TAO::DCPS::Serializer::write_boolean_array (const ACE_CDR::Boolean *x,
                                 ACE_CDR::ULong length)
{
  this->write_array (reinterpret_cast<const char*> (x), sizeof (ACE_CDR::Boolean), length);
  return static_cast<ACE_CDR::Boolean> (this->good_bit ());
}

ACE_INLINE ACE_CDR::Boolean
TAO::DCPS::Serializer::write_char_array (const ACE_CDR::Char *x,
                              ACE_CDR::ULong length)
{
  this->write_array (reinterpret_cast<const char*> (x), sizeof (ACE_CDR::Char), length);
  return static_cast<ACE_CDR::Boolean> (this->good_bit ());
}

ACE_INLINE ACE_CDR::Boolean
TAO::DCPS::Serializer::write_wchar_array (const ACE_CDR::WChar* x,
                               ACE_CDR::ULong length)
{
  this->write_array (reinterpret_cast<const char*> (x), sizeof (ACE_CDR::WChar), length);
  return static_cast<ACE_CDR::Boolean> (this->good_bit ());
}

ACE_INLINE ACE_CDR::Boolean
TAO::DCPS::Serializer::write_octet_array (const ACE_CDR::Octet* x,
                               ACE_CDR::ULong length)
{
  this->write_array (reinterpret_cast<const char*> (x), sizeof (ACE_CDR::Octet), length);
  return static_cast<ACE_CDR::Boolean> (this->good_bit ());
}

ACE_INLINE ACE_CDR::Boolean
TAO::DCPS::Serializer::write_short_array (const ACE_CDR::Short *x,
                               ACE_CDR::ULong length)
{
  this->write_array (reinterpret_cast<const char*> (x), sizeof (ACE_CDR::Short), length);
  return static_cast<ACE_CDR::Boolean> (this->good_bit ());
}

ACE_INLINE ACE_CDR::Boolean
TAO::DCPS::Serializer::write_ushort_array (const ACE_CDR::UShort *x,
                                ACE_CDR::ULong length)
{
  this->write_array (reinterpret_cast<const char*> (x), sizeof (ACE_CDR::UShort), length);
  return static_cast<ACE_CDR::Boolean> (this->good_bit ());
}

ACE_INLINE ACE_CDR::Boolean
TAO::DCPS::Serializer::write_long_array (const ACE_CDR::Long *x,
                              ACE_CDR::ULong length)
{
  this->write_array (reinterpret_cast<const char*> (x), sizeof (ACE_CDR::Long), length);
  return static_cast<ACE_CDR::Boolean> (this->good_bit ());
}

ACE_INLINE ACE_CDR::Boolean
TAO::DCPS::Serializer::write_ulong_array (const ACE_CDR::ULong *x,
                               ACE_CDR::ULong length)
{
  this->write_array (reinterpret_cast<const char*> (x), sizeof (ACE_CDR::ULong), length);
  return static_cast<ACE_CDR::Boolean> (this->good_bit ());
}

ACE_INLINE ACE_CDR::Boolean
TAO::DCPS::Serializer::write_longlong_array (const ACE_CDR::LongLong* x,
                                  ACE_CDR::ULong length)
{
  this->write_array (reinterpret_cast<const char*> (x), sizeof (ACE_CDR::LongLong), length);
  return static_cast<ACE_CDR::Boolean> (this->good_bit ());
}

ACE_INLINE ACE_CDR::Boolean
TAO::DCPS::Serializer::write_ulonglong_array (const ACE_CDR::ULongLong *x,
                                   ACE_CDR::ULong length)
{
  this->write_array (reinterpret_cast<const char*> (x), sizeof (ACE_CDR::ULongLong), length);
  return static_cast<ACE_CDR::Boolean> (this->good_bit ());
}

ACE_INLINE ACE_CDR::Boolean
TAO::DCPS::Serializer::write_float_array (const ACE_CDR::Float *x,
                               ACE_CDR::ULong length)
{
  this->write_array (reinterpret_cast<const char*> (x), sizeof (ACE_CDR::Float), length);
  return static_cast<ACE_CDR::Boolean> (this->good_bit ());
}

ACE_INLINE ACE_CDR::Boolean
TAO::DCPS::Serializer::write_double_array (const ACE_CDR::Double *x,
                                ACE_CDR::ULong length)
{
  this->write_array (reinterpret_cast<const char*> (x), sizeof (ACE_CDR::Double), length);
  return static_cast<ACE_CDR::Boolean> (this->good_bit ());
}

ACE_INLINE ACE_CDR::Boolean
TAO::DCPS::Serializer::write_longdouble_array (const ACE_CDR::LongDouble* x,
                                    ACE_CDR::ULong length)
{
  this->write_array (reinterpret_cast<const char*> (x), sizeof (ACE_CDR::LongDouble), length);
  return static_cast<ACE_CDR::Boolean> (this->good_bit ());
}

//
// The following insertion operators are done in the style of the
// ACE_CDR insertion operators instead of a stream abstraction.  This
// is done to allow their use in the same way as existing ACE_CDR
// inserters, rather than as a true stream abstraction (which would
// return the argument stream).
//

ACE_INLINE ACE_CDR::Boolean
operator<< (TAO::DCPS::Serializer& s, ACE_CDR::Char x)
{
  s.buffer_write (reinterpret_cast<char*> (&x), sizeof (ACE_CDR::Char), s.swap_bytes ());
  return static_cast<ACE_CDR::Boolean> (s.good_bit ());
}

ACE_INLINE ACE_CDR::Boolean
operator<< (TAO::DCPS::Serializer& s, ACE_CDR::Short x)
{
  s.buffer_write (reinterpret_cast<char*> (&x), sizeof (ACE_CDR::Short), s.swap_bytes ());
  return static_cast<ACE_CDR::Boolean> (s.good_bit ());
}

ACE_INLINE ACE_CDR::Boolean
operator<< (TAO::DCPS::Serializer& s, ACE_CDR::UShort x)
{
  s.buffer_write (reinterpret_cast<char*> (&x), sizeof (ACE_CDR::UShort), s.swap_bytes ());
  return static_cast<ACE_CDR::Boolean> (s.good_bit ());
}

ACE_INLINE ACE_CDR::Boolean
operator<< (TAO::DCPS::Serializer& s, ACE_CDR::Long x)
{
  s.buffer_write (reinterpret_cast<char*> (&x), sizeof (ACE_CDR::Long), s.swap_bytes ());
  return static_cast<ACE_CDR::Boolean> (s.good_bit ());
}

ACE_INLINE ACE_CDR::Boolean
operator<< (TAO::DCPS::Serializer& s, ACE_CDR::ULong x)
{
  s.buffer_write (reinterpret_cast<char*> (&x), sizeof (ACE_CDR::ULong), s.swap_bytes ());
  return static_cast<ACE_CDR::Boolean> (s.good_bit ());
}

ACE_INLINE ACE_CDR::Boolean
operator<< (TAO::DCPS::Serializer& s, ACE_CDR::LongLong x)
{
  s.buffer_write (reinterpret_cast<char*> (&x), sizeof (ACE_CDR::LongLong), s.swap_bytes ());
  return static_cast<ACE_CDR::Boolean> (s.good_bit ());
}

ACE_INLINE ACE_CDR::Boolean
operator<< (TAO::DCPS::Serializer& s, ACE_CDR::ULongLong x)
{
  s.buffer_write (reinterpret_cast<char*> (&x), sizeof (ACE_CDR::ULongLong), s.swap_bytes ());
  return static_cast<ACE_CDR::Boolean> (s.good_bit ());
}

ACE_INLINE ACE_CDR::Boolean
operator<< (TAO::DCPS::Serializer& s, ACE_CDR::LongDouble x)
{
  s.buffer_write (reinterpret_cast<char*> (&x), sizeof (ACE_CDR::LongDouble), s.swap_bytes ());
  return static_cast<ACE_CDR::Boolean> (s.good_bit ());
}

ACE_INLINE ACE_CDR::Boolean
operator<< (TAO::DCPS::Serializer& s, ACE_CDR::Float x)
{
  s.buffer_write (reinterpret_cast<char*> (&x), sizeof (ACE_CDR::Float), s.swap_bytes ());
  return static_cast<ACE_CDR::Boolean> (s.good_bit ());
}

ACE_INLINE ACE_CDR::Boolean
operator<< (TAO::DCPS::Serializer& s, ACE_CDR::Double x)
{
  s.buffer_write (reinterpret_cast<char*> (&x), sizeof (ACE_CDR::Double), s.swap_bytes ());
  return static_cast<ACE_CDR::Boolean> (s.good_bit ());
}

ACE_INLINE ACE_CDR::Boolean
operator<< (TAO::DCPS::Serializer& s, const ACE_CDR::Char* x)
{
  if (x != 0)
    {
      // Included the null termination in the serialized data.
      ACE_CDR::ULong stringlen = static_cast<ACE_CDR::ULong> (ACE_OS::strlen (x));
      s << stringlen;
      s.buffer_write (reinterpret_cast<const char*> (x), stringlen, false);
    }
  else
    {
      s << 0;
    }
  return static_cast<ACE_CDR::Boolean> (s.good_bit ());
}

ACE_INLINE ACE_CDR::Boolean
operator<< (TAO::DCPS::Serializer& s, const ACE_CDR::WChar* x)
{
  if (x != 0)
    {
      // Included the null termination in the serialized data.
      ACE_CDR::ULong stringlen = static_cast<ACE_CDR::ULong> (ACE_OS::strlen (x));
      s << stringlen;
      s.buffer_write (reinterpret_cast<const char*> (x), stringlen, false);
    }
  else
    {
      s << 0;
    }
  return static_cast<ACE_CDR::Boolean> (s.good_bit ());
}

ACE_INLINE ACE_CDR::Boolean
operator<< (TAO::DCPS::Serializer& s, ACE_OutputCDR::from_boolean x)
{
  s.buffer_write (reinterpret_cast<char*> (&x.val_), sizeof (ACE_CDR::Boolean), s.swap_bytes ());
  return static_cast<ACE_CDR::Boolean> (s.good_bit ());
}

ACE_INLINE ACE_CDR::Boolean
operator<< (TAO::DCPS::Serializer& s, ACE_OutputCDR::from_char x)
{
  s.buffer_write (reinterpret_cast<char*> (&x.val_), sizeof (ACE_CDR::Char), false);
  return static_cast<ACE_CDR::Boolean> (s.good_bit ());
}

ACE_INLINE ACE_CDR::Boolean
operator<< (TAO::DCPS::Serializer& s, ACE_OutputCDR::from_wchar x)
{
  s.buffer_write (reinterpret_cast<char*> (&x.val_), sizeof (ACE_CDR::WChar), false);
  return static_cast<ACE_CDR::Boolean> (s.good_bit ());
}

ACE_INLINE ACE_CDR::Boolean
operator<< (TAO::DCPS::Serializer& s, ACE_OutputCDR::from_octet x)
{
  s.buffer_write (reinterpret_cast<char*> (&x.val_), sizeof (ACE_CDR::Octet), false);
  return static_cast<ACE_CDR::Boolean> (s.good_bit ());
}

ACE_INLINE ACE_CDR::Boolean
operator<< (TAO::DCPS::Serializer& s, ACE_OutputCDR::from_string x)
{
  // Included the null termination in the serialized data.
  ACE_CDR::ULong stringlen = 0;
  if (x.val_ != 0)
    {
      stringlen = static_cast<ACE_CDR::ULong> (ACE_OS::strlen (x.val_));
      s << stringlen;
      s.buffer_write (reinterpret_cast<char*> (x.val_), stringlen, false);
    }
  else
    {
      s << 0;
    }
  return static_cast<ACE_CDR::Boolean> (s.good_bit ()) && ( (x.bound_ == 0) || (stringlen <= x.bound_));
}

ACE_INLINE ACE_CDR::Boolean
operator<< (TAO::DCPS::Serializer& s, ACE_OutputCDR::from_wstring x)
{
  // Included the null termination in the serialized data.
  ACE_CDR::ULong stringlen = 0;
  if (x.val_ != 0)
    {
      stringlen = static_cast<ACE_CDR::ULong> (ACE_OS::strlen (x.val_));
      s << stringlen;
      s.buffer_write (reinterpret_cast<char*> (x.val_), stringlen, false);
    }
  else
    {
      s << 0;
    }
  return static_cast<ACE_CDR::Boolean> (s.good_bit ()) && ( (x.bound_ == 0) || (stringlen <= x.bound_));
}

//
// The following extraction operators are done in the style of the
// ACE_CDR extraction operators instead of a stream abstraction.  This
// is done to allow their use in the same way as existing ACE_CDR
// extractors, rather than as a true stream abstraction (which would
// return the argument stream).
//

ACE_INLINE ACE_CDR::Boolean
operator>> (TAO::DCPS::Serializer& s, ACE_CDR::Char& x)
{
  s.buffer_read (reinterpret_cast<char*> (&x), sizeof (ACE_CDR::Char), s.swap_bytes ());
  return static_cast<ACE_CDR::Boolean> (s.good_bit ());
}

ACE_INLINE ACE_CDR::Boolean
operator>> (TAO::DCPS::Serializer& s, ACE_CDR::Short& x)
{
  s.buffer_read (reinterpret_cast<char*> (&x), sizeof (ACE_CDR::Short), s.swap_bytes ());
  return static_cast<ACE_CDR::Boolean> (s.good_bit ());
}

ACE_INLINE ACE_CDR::Boolean
operator>> (TAO::DCPS::Serializer& s, ACE_CDR::UShort& x)
{
  s.buffer_read (reinterpret_cast<char*> (&x), sizeof (ACE_CDR::UShort), s.swap_bytes ());
  return static_cast<ACE_CDR::Boolean> (s.good_bit ());
}

ACE_INLINE ACE_CDR::Boolean
operator>> (TAO::DCPS::Serializer& s, ACE_CDR::Long& x)
{
  s.buffer_read (reinterpret_cast<char*> (&x), sizeof (ACE_CDR::Long), s.swap_bytes ());
  return static_cast<ACE_CDR::Boolean> (s.good_bit ());
}

ACE_INLINE ACE_CDR::Boolean
operator>> (TAO::DCPS::Serializer& s, ACE_CDR::ULong& x)
{
  s.buffer_read (reinterpret_cast<char*> (&x), sizeof (ACE_CDR::ULong), s.swap_bytes ());
  return static_cast<ACE_CDR::Boolean> (s.good_bit ());
}

ACE_INLINE ACE_CDR::Boolean
operator>> (TAO::DCPS::Serializer& s, ACE_CDR::LongLong& x)
{
  s.buffer_read (reinterpret_cast<char*> (&x), sizeof (ACE_CDR::LongLong), s.swap_bytes ());
  return static_cast<ACE_CDR::Boolean> (s.good_bit ());
}

ACE_INLINE ACE_CDR::Boolean
operator>> (TAO::DCPS::Serializer& s, ACE_CDR::ULongLong& x)
{
  s.buffer_read (reinterpret_cast<char*> (&x), sizeof (ACE_CDR::ULongLong), s.swap_bytes ());
  return static_cast<ACE_CDR::Boolean> (s.good_bit ());
}

ACE_INLINE ACE_CDR::Boolean
operator>> (TAO::DCPS::Serializer& s, ACE_CDR::LongDouble& x)
{
  s.buffer_read (reinterpret_cast<char*> (&x), sizeof (ACE_CDR::LongDouble), s.swap_bytes ());
  return static_cast<ACE_CDR::Boolean> (s.good_bit ());
}

ACE_INLINE ACE_CDR::Boolean
operator>> (TAO::DCPS::Serializer& s, ACE_CDR::Float& x)
{
  s.buffer_read (reinterpret_cast<char*> (&x), sizeof (ACE_CDR::Float), s.swap_bytes ());
  return static_cast<ACE_CDR::Boolean> (s.good_bit ());
}

ACE_INLINE ACE_CDR::Boolean
operator>> (TAO::DCPS::Serializer& s, ACE_CDR::Double& x)
{
  s.buffer_read (reinterpret_cast<char*> (&x), sizeof (ACE_CDR::Double), s.swap_bytes ());
  return static_cast<ACE_CDR::Boolean> (s.good_bit ());
}

ACE_INLINE ACE_CDR::Boolean
operator>> (TAO::DCPS::Serializer& s, ACE_CDR::Char*& x)
{
  s.read_string (x);
  return static_cast<ACE_CDR::Boolean> (s.good_bit ());
}

ACE_INLINE ACE_CDR::Boolean
operator>> (TAO::DCPS::Serializer& s, ACE_CDR::WChar*& x)
{
  s.read_string (x);
  return static_cast<ACE_CDR::Boolean> (s.good_bit ());
}

ACE_INLINE ACE_CDR::Boolean
operator>> (TAO::DCPS::Serializer& s, ACE_InputCDR::to_boolean x)
{
  s.buffer_read (reinterpret_cast<char*> (&x.ref_), sizeof (ACE_CDR::Boolean), s.swap_bytes ());
  return static_cast<ACE_CDR::Boolean> (s.good_bit ());
}

ACE_INLINE ACE_CDR::Boolean
operator>> (TAO::DCPS::Serializer& s, ACE_InputCDR::to_char x)
{
  s.buffer_read (reinterpret_cast<char*> (&x.ref_), sizeof (ACE_CDR::Char), s.swap_bytes ());
  return static_cast<ACE_CDR::Boolean> (s.good_bit ());
}

ACE_INLINE ACE_CDR::Boolean
operator>> (TAO::DCPS::Serializer& s, ACE_InputCDR::to_wchar x)
{
  s.buffer_read (reinterpret_cast<char*> (&x.ref_), sizeof (ACE_CDR::WChar), false);
  return static_cast<ACE_CDR::Boolean> (s.good_bit ());
}

ACE_INLINE ACE_CDR::Boolean
operator>> (TAO::DCPS::Serializer& s, ACE_InputCDR::to_octet x)
{
  s.buffer_read (reinterpret_cast<char*> (&x.ref_), sizeof (ACE_CDR::Octet), false);
  return static_cast<ACE_CDR::Boolean> (s.good_bit ());
}

ACE_INLINE ACE_CDR::Boolean
operator>> (TAO::DCPS::Serializer& s, ACE_InputCDR::to_string x)
{
  s.read_string (const_cast<char*&> (x.val_));
  return static_cast<ACE_CDR::Boolean> (s.good_bit ())
           && ( (x.bound_ == 0) || (ACE_OS::strlen (x.val_) <= x.bound_));
}

ACE_INLINE ACE_CDR::Boolean
operator>> (TAO::DCPS::Serializer& s, ACE_InputCDR::to_wstring x)
{
  s.read_string (const_cast<ACE_CDR::WChar*&> (x.val_));
  return static_cast<ACE_CDR::Boolean> (s.good_bit ())
           && ( (x.bound_ == 0) || (ACE_OS::strlen (x.val_) <= x.bound_));
}

//----------------------------------------------------------------------------
// predefined type _dcps_max_marshaled_size methods
ACE_INLINE size_t _dcps_max_marshaled_size (const ACE_CDR::Short& x)
{
  ACE_UNUSED_ARG (x);
  return sizeof (ACE_CDR::Short);
}

ACE_INLINE size_t _dcps_max_marshaled_size (const ACE_CDR::UShort& x)
{
  ACE_UNUSED_ARG (x);
  return sizeof (ACE_CDR::UShort);
}

ACE_INLINE size_t _dcps_max_marshaled_size (const ACE_CDR::Long& x)
{
  ACE_UNUSED_ARG (x);
  return sizeof (ACE_CDR::Long);
}

ACE_INLINE size_t _dcps_max_marshaled_size (const ACE_CDR::ULong& x)
{
  ACE_UNUSED_ARG (x);
  return sizeof (ACE_CDR::ULong);
}

ACE_INLINE size_t _dcps_max_marshaled_size (const ACE_CDR::LongLong& x)
{
  ACE_UNUSED_ARG (x);
  return sizeof (ACE_CDR::LongLong);
}

ACE_INLINE size_t _dcps_max_marshaled_size (const ACE_CDR::ULongLong& x)
{
  ACE_UNUSED_ARG (x);
  return sizeof (ACE_CDR::ULongLong);
}

ACE_INLINE size_t _dcps_max_marshaled_size (const ACE_CDR::LongDouble& x)
{
  ACE_UNUSED_ARG (x);
  return sizeof (ACE_CDR::LongDouble);
}

ACE_INLINE size_t _dcps_max_marshaled_size (const ACE_CDR::Float& x)
{
  ACE_UNUSED_ARG (x);
  return sizeof (ACE_CDR::Float);
}

ACE_INLINE size_t _dcps_max_marshaled_size (const ACE_CDR::Double& x)
{
  ACE_UNUSED_ARG (x);
  return sizeof (ACE_CDR::Double);
}


// predefined type _dcps_max_marshaled_size method disambiguators.
ACE_INLINE size_t _dcps_max_marshaled_size (const ACE_OutputCDR::from_boolean x)
{
  ACE_UNUSED_ARG (x);
  return sizeof (ACE_CDR::Char);
}

ACE_INLINE size_t _dcps_max_marshaled_size (const ACE_OutputCDR::from_char x)
{
  ACE_UNUSED_ARG (x);
  return sizeof (ACE_CDR::Char);
}

ACE_INLINE size_t _dcps_max_marshaled_size (const ACE_OutputCDR::from_wchar x)
{
  ACE_UNUSED_ARG (x);
  return sizeof (ACE_CDR::WChar);
}

ACE_INLINE size_t _dcps_max_marshaled_size (const ACE_OutputCDR::from_octet x)
{
  ACE_UNUSED_ARG (x);
  return sizeof (ACE_CDR::Char);
}

// predefined type _dcps_max_marshaled_size method explicit disambiguators.
ACE_INLINE size_t _dcps_max_marshaled_size_boolean ()
{
  return sizeof (ACE_CDR::Char);
}

ACE_INLINE size_t _dcps_max_marshaled_size_char ()
{
  return sizeof (ACE_CDR::Char);
}

ACE_INLINE size_t _dcps_max_marshaled_size_wchar ()
{
  return sizeof (ACE_CDR::WChar);
}

ACE_INLINE size_t _dcps_max_marshaled_size_octet ()
{
  return sizeof (ACE_CDR::Char);
}

ACE_INLINE size_t _dcps_max_marshaled_size_ulong ()
{
  return sizeof (ACE_CDR::ULong);
}

