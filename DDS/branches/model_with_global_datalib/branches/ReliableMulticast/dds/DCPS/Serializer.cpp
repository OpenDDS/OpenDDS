// -*- C++ -*-
//
// $Id$

#include  "DCPS/DdsDcps_pch.h"
#include  "Serializer.h"
#include  <ace/OS.h>

#if !defined (__ACE_INLINE__)
# include "Serializer.inl"
#endif /* ! __ACE_INLINE__ */


// Default constructor
TAO::DCPS::Serializer::Serializer (
  ACE_Message_Block* chain,
  bool               swap_bytes)
 : start_ (chain)
 , current_ (chain)
 , swap_bytes_ (swap_bytes)
 , good_bit_ (true)
{
}

// Default destructor
TAO::DCPS::Serializer::~Serializer (void)
{
}

void
TAO::DCPS::Serializer::smemcpy (char* to, const char* from, size_t n)
{
  // Unroll the loop...
  switch (n)
    {
      case 16: to[ 15] = from[ 15];
      case 15: to[ 14] = from[ 14];
      case 14: to[ 13] = from[ 13];
      case 13: to[ 12] = from[ 12];
      case 12: to[ 11] = from[ 11];
      case 11: to[ 10] = from[ 10];
      case 10: to[  9] = from[  9];
      case  9: to[  8] = from[  8];
      case  8: to[  7] = from[  7];
      case  7: to[  6] = from[  6];
      case  6: to[  5] = from[  5];
      case  5: to[  4] = from[  4];
      case  4: to[  3] = from[  3];
      case  3: to[  2] = from[  2];
      case  2: to[  1] = from[  1];
      case  1: to[  0] = from[  0];
      case  0: return;
      default: (void) ACE_OS::memcpy(
                        reinterpret_cast<void*>(to),
                        reinterpret_cast<const void*>(from),
                        n
                      );
    }
}

void
TAO::DCPS::Serializer::swapcpy (char* to, const char* from, size_t n)
{
  // Unroll the loop...
  switch (n)                             // 2   4   8   16
    {
      case 16: to[ 15] = from[ n - 16]; // x   x   x    0
      case 15: to[ 14] = from[ n - 15]; // x   x   x    1
      case 14: to[ 13] = from[ n - 14]; // x   x   x    2
      case 13: to[ 12] = from[ n - 13]; // x   x   x    3
      case 12: to[ 11] = from[ n - 12]; // x   x   x    4
      case 11: to[ 10] = from[ n - 11]; // x   x   x    5
      case 10: to[  9] = from[ n - 10]; // x   x   x    6
      case  9: to[  8] = from[ n -  9]; // x   x   x    7
      case  8: to[  7] = from[ n -  8]; // x   x   0    8
      case  7: to[  6] = from[ n -  7]; // x   x   1    9
      case  6: to[  5] = from[ n -  6]; // x   x   2   10
      case  5: to[  4] = from[ n -  5]; // x   x   3   11
      case  4: to[  3] = from[ n -  4]; // x   0   4   12
      case  3: to[  2] = from[ n -  3]; // x   1   5   13
      case  2: to[  1] = from[ n -  2]; // 0   2   6   14
      case  1: to[  0] = from[ n -  1]; // 1   3   7   15
      case  0: return;
      default: this->good_bit_ = false;
    }
}

void
TAO::DCPS::Serializer::read_string (ACE_CDR::Char*& dest)
{
  //
  // Ensure no bad values leave the routine.
  //
  dest = 0;

  //
  // Extract the string length.
  //
  ACE_CDR::ULong length;
  this->buffer_read (reinterpret_cast<char*> (&length), sizeof (ACE_CDR::ULong), this->swap_bytes());
  if (this->good_bit () == false)
    {
      return;
    }

  //
  // NOTE: Maintain the ACE implementation where the length check is
  //       done here before the allocation even though it will be
  //       checked during the actual read as well.
  //
  if (length <= this->current_->total_length())
    {
      //
      // Allocate the destination.
      //
      ACE_NEW_NORETURN (dest, ACE_CDR::Char[ length+1]);
      if (dest == 0)
        {
          this->good_bit_ = false;
        }
      else
        {
          //
          // Extract the string.
          //
          this->read_char_array (dest, length);
        }

      if (this->good_bit() == true)
        {
          //
          // Null terminate the string.
          //
          dest[length] = '\0';
        }
      else
        {
          delete [] dest;
          dest = 0;
        }
    }

  //
  // Save the status.
  //
  this->good_bit_ = (dest != 0);
}

void
TAO::DCPS::Serializer::read_string (ACE_CDR::WChar*& dest)
{
  //
  // Ensure no bad values leave the routine.
  //
  dest = 0;

  //
  // Extract the string length.
  //
  ACE_CDR::ULong length;
  this->buffer_read (reinterpret_cast<char*> (&length), sizeof (ACE_CDR::ULong), this->swap_bytes());
  if (this->good_bit () == false)
    {
      return;
    }

  //
  // NOTE: Maintain the ACE implementation where the length check is
  //       done here before the allocation even though it will be
  //       checked during the actual read as well.
  //
  if (length <= this->current_->total_length ())
    {
      //
      // Allocate the destination.
      //
      ACE_NEW_NORETURN (dest, ACE_CDR::WChar[ length+1]);
      if (dest == 0)
        {
          this->good_bit_ = false;
        }
      else
        {
          //
          // Extract the string.
          //
          this->read_wchar_array (dest, length);
        }

      if (this->good_bit () == true)
        {
          //
          // Null terminate the string.
          //
          dest[length] = '\x00';
        }
      else
        {
          delete [] dest;
          dest = 0;
        }
    }

  //
  // Save the status.
  //
  this->good_bit_ = (dest != 0);
}

