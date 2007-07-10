// -*- C++ -*-
//
// $Id$
#ifndef TAO_DDS_DCPS_SERIALIZER_H
#define TAO_DDS_DCPS_SERIALIZER_H

#include "ace/CDR_Base.h"
#include "dcps_export.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

class ACE_Message_Block ;

//This must stay in namespace "TAO" until the tao_idl compiler is changed
namespace TAO 
{
  namespace DCPS 
  {
    /**
     * @class Serializer
     *
     * @brief Class to serialize and deserialize data for DDS.
     *
     * This class provides a mechanism to insert and extract data to and
     * from an ACE_Message_Block chain that represents the data which
     * can be transported on the wire to other DDS service participants.
     */
    class OpenDDS_Dcps_Export Serializer
    {
    public:
      /**
       * Constructor with a message block chain.  This installs the
       * message block chain and sets the current block to the first in
       * the chain.  Memory management is the reponsibility of the owner
       * of this object, and is not performed internally.  Ownership of
       * the message block chain is retained by the owner of this object
       * and the lifetime of the chain must be longer than the use of
       * this object.
       *
       * Bytes are swapped when either reading or writing from the
       * message chain if the swap_bytes argument is true.  It is the
       * reponsibility of the owner of this object to determine whether
       * this should be performed or not.
       */
      Serializer (ACE_Message_Block* chain = 0,
                  bool               swap_bytes = false);

      /// Destructor 
      virtual ~Serializer (void);

      /// Add the new chain as the contained chain, return any
      /// previously held chain.
      ACE_Message_Block* add_chain (ACE_Message_Block* chain);

      /// Establish byte swaping behavior.
      void swap_bytes (bool do_swap);

      /// Examine byte swaping behavior.
      bool swap_bytes() const ;

      /// Examine the state of the stream abstraction.
      bool good_bit() const ;

      /// Read a C string.
      void read_string (ACE_CDR::Char*& dest);

      /// Read a WChar string.
      void read_string (ACE_CDR::WChar*& dest);

      /**
       * The buffer @a x must be large enough to contain @a length
       * elements.
       * Return @c false on failure and @c true on success.
       */
      //@{ @name Read basic IDL types arrays
      ACE_CDR::Boolean read_boolean_array (ACE_CDR::Boolean* x,
                                           ACE_CDR::ULong length);
      ACE_CDR::Boolean read_char_array (ACE_CDR::Char *x,
                                        ACE_CDR::ULong length);
      ACE_CDR::Boolean read_wchar_array (ACE_CDR::WChar* x,
                                         ACE_CDR::ULong length);
      ACE_CDR::Boolean read_octet_array (ACE_CDR::Octet* x,
                                         ACE_CDR::ULong length);
      ACE_CDR::Boolean read_short_array (ACE_CDR::Short *x,
                                         ACE_CDR::ULong length);
      ACE_CDR::Boolean read_ushort_array (ACE_CDR::UShort *x,
                                          ACE_CDR::ULong length);
      ACE_CDR::Boolean read_long_array (ACE_CDR::Long *x,
                                        ACE_CDR::ULong length);
      ACE_CDR::Boolean read_ulong_array (ACE_CDR::ULong *x,
                                         ACE_CDR::ULong length);
      ACE_CDR::Boolean read_longlong_array (ACE_CDR::LongLong* x,
                                            ACE_CDR::ULong length);
      ACE_CDR::Boolean read_ulonglong_array (ACE_CDR::ULongLong* x,
                                             ACE_CDR::ULong length);
      ACE_CDR::Boolean read_float_array (ACE_CDR::Float *x,
                                         ACE_CDR::ULong length);
      ACE_CDR::Boolean read_double_array (ACE_CDR::Double *x,
                                          ACE_CDR::ULong length);
      ACE_CDR::Boolean read_longdouble_array (ACE_CDR::LongDouble* x,
                                              ACE_CDR::ULong length);
      //@}

      /// Note: the portion written starts at x and ends
      ///    at x + length.
      /// The length is *NOT* stored into the CDR stream.
      //@{ @name Array write operations
      ACE_CDR::Boolean write_boolean_array (const ACE_CDR::Boolean *x,
                                            ACE_CDR::ULong length);
      ACE_CDR::Boolean write_char_array (const ACE_CDR::Char *x,
                                         ACE_CDR::ULong length);
      ACE_CDR::Boolean write_wchar_array (const ACE_CDR::WChar* x,
                                          ACE_CDR::ULong length);
      ACE_CDR::Boolean write_octet_array (const ACE_CDR::Octet* x,
                                          ACE_CDR::ULong length);
      ACE_CDR::Boolean write_short_array (const ACE_CDR::Short *x,
                                          ACE_CDR::ULong length);
      ACE_CDR::Boolean write_ushort_array (const ACE_CDR::UShort *x,
                                           ACE_CDR::ULong length);
      ACE_CDR::Boolean write_long_array (const ACE_CDR::Long *x,
                                         ACE_CDR::ULong length);
      ACE_CDR::Boolean write_ulong_array (const ACE_CDR::ULong *x,
                                          ACE_CDR::ULong length);
      ACE_CDR::Boolean write_longlong_array (const ACE_CDR::LongLong* x,
                                             ACE_CDR::ULong length);
      ACE_CDR::Boolean write_ulonglong_array (const ACE_CDR::ULongLong *x,
                                              ACE_CDR::ULong length);
      ACE_CDR::Boolean write_float_array (const ACE_CDR::Float *x,
                                          ACE_CDR::ULong length);
      ACE_CDR::Boolean write_double_array (const ACE_CDR::Double *x,
                                           ACE_CDR::ULong length);
      ACE_CDR::Boolean write_longdouble_array (const ACE_CDR::LongDouble* x,
                                               ACE_CDR::ULong length);
      //@}

      /// Read from the chain into a destination buffer.
      void buffer_read (char* dest, size_t size, bool swap);

      /// Read an array of values from the chain.
      /// NOTE: This assumes that the buffer contains elements that are
      ///       properly aligned.  The buffer must have padding if the
      ///       elements are not naturally aligned; or this routine should
      ///       not be used.
      void read_array (char* x,
                       size_t size,
                       ACE_CDR::ULong length);

      /// Write to the chain from a source buffer.
      void buffer_write (const char* src, size_t size, bool swap);

      /// Write an array of values to the chain.
      /// NOTE: This assumes that there is _no_ padding between the array
      ///       elements.  If this is not the case, do not use this
      ///       method.  If padding exists in the array, it will be
      ///       written when _not_ swapping, and will _not_ be written
      ///       when swapping, resulting in corrupted data.
      void write_array (const char *x,
                        size_t size,
                        ACE_CDR::ULong length);

    private:
      /// Efficient straight copy for quad words and shorter.  This is
      /// an instance method to match the swapcpy semantics.
      void smemcpy (char* to, const char* from, size_t n);

      /// Efficient swaping copy for quad words and shorter.  This is an
      /// instance method to allow clearing the good_bit_ on error.
      void swapcpy (char* to, const char* from, size_t n);

      /// Implementation of the actual read from the chain.
      size_t doread (char* dest, size_t size, bool swap, size_t offset);

      /// Implementation of the actual write to the chain.
      size_t dowrite (const char* dest, size_t size, bool swap, size_t offset);

      /// Start of message block chain.
      ACE_Message_Block* start_ ;

      /// Currently active message block in chain.
      ACE_Message_Block* current_ ;

      /// Indicates whether bytes will be swapped for this stream.
      bool swap_bytes_ ;

      /// Indicates the current state of the stream abstraction.
      bool good_bit_ ;

    };

  } // namespace DCPS

} // namespace OpenDDS

#if defined (__ACE_INLINE__)
# include "Serializer.inl"
#else  /* __ACE_INLINE__ */

#include <ace/CDR_Stream.h>

// Insertion operators.
extern OpenDDS_Dcps_Export
ACE_CDR::Boolean operator<< (TAO::DCPS::Serializer& s, ACE_CDR::Char x);
extern OpenDDS_Dcps_Export 
ACE_CDR::Boolean operator<< (TAO::DCPS::Serializer& s, ACE_CDR::Short x);
extern OpenDDS_Dcps_Export 
ACE_CDR::Boolean operator<< (TAO::DCPS::Serializer& s, ACE_CDR::UShort x);
extern OpenDDS_Dcps_Export 
ACE_CDR::Boolean operator<< (TAO::DCPS::Serializer& s, ACE_CDR::Long x);
extern OpenDDS_Dcps_Export 
ACE_CDR::Boolean operator<< (TAO::DCPS::Serializer& s, ACE_CDR::ULong x);
extern OpenDDS_Dcps_Export 
ACE_CDR::Boolean operator<< (TAO::DCPS::Serializer& s, ACE_CDR::LongLong x);
extern OpenDDS_Dcps_Export 
ACE_CDR::Boolean operator<< (TAO::DCPS::Serializer& s, ACE_CDR::ULongLong x);
extern OpenDDS_Dcps_Export 
ACE_CDR::Boolean operator<< (TAO::DCPS::Serializer& s, ACE_CDR::LongDouble x);
extern OpenDDS_Dcps_Export 
ACE_CDR::Boolean operator<< (TAO::DCPS::Serializer& s, ACE_CDR::Float x);
extern OpenDDS_Dcps_Export 
ACE_CDR::Boolean operator<< (TAO::DCPS::Serializer& s, ACE_CDR::Double x);
extern OpenDDS_Dcps_Export 
ACE_CDR::Boolean operator<< (TAO::DCPS::Serializer& s, const ACE_CDR::Char* x);
extern OpenDDS_Dcps_Export 
ACE_CDR::Boolean operator<< (TAO::DCPS::Serializer& s, const ACE_CDR::WChar* x);

// Using the ACE CDR Stream disambiguators.
extern OpenDDS_Dcps_Export 
ACE_CDR::Boolean operator<< (TAO::DCPS::Serializer& s, ACE_OutputCDR::from_boolean x);
extern OpenDDS_Dcps_Export 
ACE_CDR::Boolean operator<< (TAO::DCPS::Serializer& s, ACE_OutputCDR::from_char x);
extern OpenDDS_Dcps_Export 
ACE_CDR::Boolean operator<< (TAO::DCPS::Serializer& s, ACE_OutputCDR::from_wchar x);
extern OpenDDS_Dcps_Export 
ACE_CDR::Boolean operator<< (TAO::DCPS::Serializer& s, ACE_OutputCDR::from_octet x);
extern OpenDDS_Dcps_Export 
ACE_CDR::Boolean operator<< (TAO::DCPS::Serializer& s, ACE_OutputCDR::from_string x);
extern OpenDDS_Dcps_Export 
ACE_CDR::Boolean operator<< (TAO::DCPS::Serializer& s, ACE_OutputCDR::from_wstring x);

// Extraction operators.
extern OpenDDS_Dcps_Export 
ACE_CDR::Boolean operator>> (TAO::DCPS::Serializer& s, ACE_CDR::Char& x);
extern OpenDDS_Dcps_Export 
ACE_CDR::Boolean operator>> (TAO::DCPS::Serializer& s, ACE_CDR::Short& x);
extern OpenDDS_Dcps_Export 
ACE_CDR::Boolean operator>> (TAO::DCPS::Serializer& s, ACE_CDR::UShort& x);
extern OpenDDS_Dcps_Export 
ACE_CDR::Boolean operator>> (TAO::DCPS::Serializer& s, ACE_CDR::Long& x);
extern OpenDDS_Dcps_Export 
ACE_CDR::Boolean operator>> (TAO::DCPS::Serializer& s, ACE_CDR::ULong& x);
extern OpenDDS_Dcps_Export 
ACE_CDR::Boolean operator>> (TAO::DCPS::Serializer& s, ACE_CDR::LongLong& x);
extern OpenDDS_Dcps_Export 
ACE_CDR::Boolean operator>> (TAO::DCPS::Serializer& s, ACE_CDR::ULongLong& x);
extern OpenDDS_Dcps_Export 
ACE_CDR::Boolean operator>> (TAO::DCPS::Serializer& s, ACE_CDR::LongDouble& x);
extern OpenDDS_Dcps_Export 
ACE_CDR::Boolean operator>> (TAO::DCPS::Serializer& s, ACE_CDR::Float& x);
extern OpenDDS_Dcps_Export 
ACE_CDR::Boolean operator>> (TAO::DCPS::Serializer& s, ACE_CDR::Double& x);
extern OpenDDS_Dcps_Export 
ACE_CDR::Boolean operator>> (TAO::DCPS::Serializer& s, ACE_CDR::Char*& x);
extern OpenDDS_Dcps_Export 
ACE_CDR::Boolean operator>> (TAO::DCPS::Serializer& s, ACE_CDR::WChar*& x);

// Using the ACE CDR Stream disambiguators.
extern OpenDDS_Dcps_Export 
ACE_CDR::Boolean operator>> (TAO::DCPS::Serializer& s, ACE_InputCDR::to_boolean x);
extern OpenDDS_Dcps_Export 
ACE_CDR::Boolean operator>> (TAO::DCPS::Serializer& s, ACE_InputCDR::to_char x);
extern OpenDDS_Dcps_Export 
ACE_CDR::Boolean operator>> (TAO::DCPS::Serializer& s, ACE_InputCDR::to_wchar x);
extern OpenDDS_Dcps_Export 
ACE_CDR::Boolean operator>> (TAO::DCPS::Serializer& s, ACE_InputCDR::to_octet x);
extern OpenDDS_Dcps_Export 
ACE_CDR::Boolean operator>> (TAO::DCPS::Serializer& s, ACE_InputCDR::to_string x);
extern OpenDDS_Dcps_Export 
ACE_CDR::Boolean operator>> (TAO::DCPS::Serializer& s, ACE_InputCDR::to_wstring x);


// predefined type _dcps_max_marshaled_size methods
extern OpenDDS_Dcps_Export 
size_t _dcps_max_marshaled_size (const ACE_CDR::Short& x);
extern OpenDDS_Dcps_Export 
size_t _dcps_max_marshaled_size (const ACE_CDR::UShort& x);
extern OpenDDS_Dcps_Export 
size_t _dcps_max_marshaled_size (const ACE_CDR::Long& x);
extern OpenDDS_Dcps_Export 
size_t _dcps_max_marshaled_size (const ACE_CDR::ULong& x);
extern OpenDDS_Dcps_Export 
size_t _dcps_max_marshaled_size (const ACE_CDR::LongLong& x);
extern OpenDDS_Dcps_Export 
size_t _dcps_max_marshaled_size (const ACE_CDR::ULongLong& x);
extern OpenDDS_Dcps_Export 
size_t _dcps_max_marshaled_size (const ACE_CDR::LongDouble& x);
extern OpenDDS_Dcps_Export 
size_t _dcps_max_marshaled_size (const ACE_CDR::Float& x);
extern OpenDDS_Dcps_Export 
size_t _dcps_max_marshaled_size (const ACE_CDR::Double& x);

// predefined type _dcps_max_marshaled_size method disambiguators.
extern OpenDDS_Dcps_Export 
size_t _dcps_max_marshaled_size (const ACE_OutputCDR::from_boolean x);
extern OpenDDS_Dcps_Export 
size_t _dcps_max_marshaled_size (const ACE_OutputCDR::from_char x);
extern OpenDDS_Dcps_Export 
size_t _dcps_max_marshaled_size (const ACE_OutputCDR::from_wchar x);
extern OpenDDS_Dcps_Export 
size_t _dcps_max_marshaled_size (const ACE_OutputCDR::from_octet x);

/// predefined type _dcps_max_marshaled_size method explicit disambiguators.
extern OpenDDS_Dcps_Export 
size_t _dcps_max_marshaled_size_boolean ();
extern OpenDDS_Dcps_Export 
size_t _dcps_max_marshaled_size_char ();
extern OpenDDS_Dcps_Export 
size_t _dcps_max_marshaled_size_wchar ();
extern OpenDDS_Dcps_Export 
size_t _dcps_max_marshaled_size_octet ();

/// lengths of strings and sequences are ulong 
extern OpenDDS_Dcps_Export 
size_t _dcps_max_marshaled_size_ulong ();

#endif  /* __ACE_INLINE__ */

#endif /* TAO_DDS_DCPS_SERIALIZER_H */

