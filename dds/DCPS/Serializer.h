/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

/**
 * @file Serializer.h
 *
 * The serialization interface for a C++ type called Type consists of the
 * following overloads:
 *
 *   bool max_serialized_size(
 *       const Encoding& encoding, size_t& size, const Type&);
 *     Get the maximum possible byte size of Type in serialized form of the
 *     encoding. Aligns size first if appropriate for the encoding. Returns
 *     true if the type has a bounded maximum size, else false.
 *
 *     TODO(iguessthislldo): Make this a template? I don't see how this would
 *     ever actually need a Type value. There are places like the
 *     DataWriterImpl where we create an instance just to use this function.
 *
 *   void serialized_size(
 *       const Encoding& encoding, size_t& size, const Type& value);
 *     Get the byte size of the representation of value.
 *
 *   bool operator>>(Serializer& serializer, Type& value);
 *     Tries to encode value into the stream of the serializer. Returns true if
 *     successful, else false.
 *
 *   bool operator<<(Serializer& serializer, const Type& value);
 *     Tries to decodes a representation of Type located at the current
 *     position of the stream and use that to set value. Returns true if
 *     successful, else false.
 */

#ifndef OPENDDS_DCPS_SERIALIZER_H
#define OPENDDS_DCPS_SERIALIZER_H

#include "dcps_export.h"

#ifndef ACE_LACKS_PRAGMA_ONCE
#  pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "Definitions.h"
#include "PoolAllocator.h"

#include <tao/String_Alloc.h>

#include <ace/CDR_Base.h>
#include <ace/CDR_Stream.h>

#include <limits>

#include <string>

ACE_BEGIN_VERSIONED_NAMESPACE_DECL
class ACE_Message_Block;
ACE_END_VERSIONED_NAMESPACE_DECL

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

enum Endianness {
  ENDIAN_BIG = 0,
  ENDIAN_LITTLE = 1,
#ifdef ACE_LITTLE_ENDIAN
  ENDIAN_NATIVE = ENDIAN_LITTLE,
  ENDIAN_NONNATIVE = ENDIAN_BIG
#else
  ENDIAN_NATIVE = ENDIAN_BIG,
  ENDIAN_NONNATIVE = ENDIAN_LITTLE
#endif
};

OpenDDS_Dcps_Export
OPENDDS_STRING endianness_to_string(Endianness endianness);

const size_t boolean_cdr_size = 1;
const size_t byte_cdr_size = 1;
const size_t int8_cdr_size = 1;
const size_t uint8_cdr_size = 1;
const size_t int16_cdr_size = 2;
const size_t uint16_cdr_size = 2;
const size_t int32_cdr_size = 4;
const size_t uint32_cdr_size = 4;
const size_t int64_cdr_size = 8;
const size_t uint64_cdr_size = 8;
const size_t float32_cdr_size = 4;
const size_t float64_cdr_size = 8;
const size_t float128_cdr_size = 16;
const size_t char8_cdr_size = 1;
const size_t char16_cdr_size = 2;

/**
 * Align "value" by "by" if it's not already.
 */
OpenDDS_Dcps_Export
void align(size_t& value, size_t by);

/**
 * Represents the settings of the serialized stream. Passed to things like
 * Serializer, serialized_size(), and max_serialized_size(), etc.
 */
class OpenDDS_Dcps_Export Encoding {
public:
  /**
   * Encoding Kinds.
   *
   * TODO(iguessthislldo) Separate this into real encoding and the
   * encapsulation header encoding values.
   *
   * For encapsulation header encoding kinds that can have an endianness on the
   * LSB, define them with the LSB set to 0 (even). If this assumption can't be
   * true anymore, please update this class and remove this paragraph.
   */
  enum Kind {
    KIND_CDR_PLAIN = 0x0000,
    KIND_CDR_PARAMLIST = 0x0002,
    KIND_XCDR2_PLAIN = 0x0010,
    KIND_XCDR2_PARAMLIST = 0x0012,
    KIND_XCDR2_DELIMITED = 0x0014,
    KIND_XML = 0x0100,
    // TODO(iguessthislldo): XTypes spec also mentions a XML value of 0004!?!

    // Implementation specific encoding kinds with CDR headers, if we have any,
    // go after 0xC0000 and 0xFFFF.

    // Values below are OpenDDS specific that don't don't have a encapsulation
    // header value, so they are put above the 2 byte value limit.
    KIND_CUSTOM = 0x10000,
    KIND_UNKNOWN = KIND_CUSTOM + 0,
    /// This is unaligned CDR with no XCDR behavior.
    KIND_CDR_UNALIGNED = KIND_CUSTOM + 2,
  };

  enum Alignment {
    ALIGN_NONE = 0, // No Alignment Needed
    ALIGN_CDR = 8, // Align for CDR and XCDR1
    ALIGN_XCDR2 = 4, // Align for XCDR2
    ALIGN_MAX = ALIGN_CDR // For Serializer::ALIGN_PAD
  };

  enum Extensibility {
    FINAL,
    APPENDABLE,
    MUTABLE
  };

  /**
   * XCDR version derived from the Encoding kind.
   */
  enum XcdrVersion {
    XCDR_NONE,
    XCDR1,
    XCDR2
  };

  /// Use KIND_UNKNOWN and ENDIAN_NATIVE
  /// TODO(iguessthislldo) Change to KIND_CDR_PLAIN?
  Encoding();

  explicit Encoding(Kind kind, Endianness endianness = ENDIAN_NATIVE);

  Encoding(Kind kind, bool swap_bytes);

  Kind kind() const;
  void kind(Kind value);

  Endianness endianness() const;
  void endianness(Endianness value);

  Alignment alignment() const;
  void alignment(Alignment value);

  /**
   * Should the padding bytes being inserted into the stream be zero
   * initialized?
   */
  bool zero_init_padding() const;
  void zero_init_padding(bool value);

  /// Return the maximum alignment dictated by the alignment policy.
  size_t max_align() const;

  /**
   * Align "value" to "by" and according to the stream's alignment.
   */
  void align(size_t& value, size_t by = std::numeric_limits<size_t>::max()) const;

  /// Return XCDR version based what encoding is being used.
  XcdrVersion xcdr_version() const;

  /**
   * Returns true if the encoding kind is excepted to have a header for RTPS
   * serialized data payloads.
   */
  static bool has_cdr_header(Kind kind);

  /// Returns has_cdr_header(this->kind_)
  bool has_cdr_header() const;

  /**
   * Returns true if the encoding kind is supported by OpenDDS.
   */
  static bool supported(Kind kind);

  /// Returns supported(this->kind_)
  bool supported() const;

  /**
   * Returns true if the endianness applies to this Encoding kind.
   */
  static bool has_endianness(Kind kind);

  /// Returns has_endianness(this->kind_)
  bool has_endianness() const;

  OPENDDS_STRING to_string() const;

  static OPENDDS_STRING kind_to_string(Kind value);

private:
  Kind kind_;
  Endianness endianness_;
  Alignment alignment_;
  bool zero_init_padding_;
};

class Serializer;

OpenDDS_Dcps_Export
bool operator>>(Serializer& s, Encoding& encoding);

OpenDDS_Dcps_Export
bool operator<<(Serializer& s, const Encoding& encoding);

OpenDDS_Dcps_Export
bool max_serialized_size(
  const Encoding& encoding, size_t& size, const Encoding&);

OpenDDS_Dcps_Export
void serialized_size(
  const Encoding& encoding, size_t& size, const Encoding& value);

/**
 * Convenience function for the max_serialized_size of a single value with no
 * alignment needed.
 */
template <typename T>
size_t max_serialized_size(const Encoding& encoding, const T& value)
{
  size_t size = 0;
  max_serialized_size(encoding, size, value);
  return size;
}

/**
 * Convenience function for the serialized_size of a single value with no
 * alignment needed.
 */
template <typename T>
size_t serialized_size(const Encoding& encoding, const T& value)
{
  size_t size = 0;
  serialized_size(encoding, size, value);
  return size;
}

/**
 * @class Serializer
 *
 * @brief Class to serialize and deserialize data for DDS.
 *
 * This class provides a mechanism to insert and extract data to and
 * from an ACE_Message_Block chain that represents the data which
 * can be transported on the wire to other DDS service participants.
 */
class OpenDDS_Dcps_Export Serializer {
public:

  // Alias Alignment for Backwards Compatibility
  typedef Encoding::Alignment Alignment;
  static const Alignment ALIGN_NONE = Encoding::ALIGN_NONE;
  static const Alignment ALIGN_CDR = Encoding::ALIGN_CDR;

  /**
   * Constructor with a message block chain.  This installs the
   * message block chain and sets the current block to the first in
   * the chain.  Memory management is the responsibility of the owner
   * of this object, and is not performed internally.  Ownership of
   * the message block chain is retained by the owner of this object
   * and the lifetime of the chain must be longer than the use of
   * this object.
   *
   * This constructor is meant for using a specific predefined encoding scheme.
   */
  Serializer(ACE_Message_Block* chain, const Encoding& encoding);

  /**
   * More convenient version of the constructor above if you don't need to
   * reuse the Encoding object.
   */
  Serializer(ACE_Message_Block* chain, Encoding::Kind kind,
    Endianness endianness = ENDIAN_NATIVE);

  /**
   * This constructor is for reading a stream that might or might not have a
   * RTPS CDR header. If has_cdr_header is true, the Serializer will attempt to
   * read and set itself up using the header. Use good_bit() to see if it
   * succeeded. If has_cdr_header is false it will use
   * Encoding::KIND_CDR_UNALIGNED and is_little_endian to create the Encoding
   * object. is_little_endian is made to be used with "byte_order" values
   * littered around OpenDDS, which correspond to ACE_CDR_BYTE_ORDER in
   * ace/CDR_Base.h.
   *
   * TODO(iguessthislldo): Remove this constructor? It's probably less useful
   * than I originally thought, as I think it's only useful in DataReader.
   */
  Serializer(ACE_Message_Block* chain, bool has_cdr_header,
    bool is_little_endian);

  virtual ~Serializer();

  const Encoding& encoding() const;
  void encoding(const Encoding& value);

  /// Establish byte swapping behavior.
  void swap_bytes(bool do_swap);

  /// Examine byte swapping behavior.
  bool swap_bytes() const;

  /// Get the Explicit Endianness of the Stream
  Endianness endianness() const;

  /// Set the Explicit Endianness of the Stream
  void endianness(Endianness value);

  /// Examine alignment behavior.
  Alignment alignment() const;
  void alignment(Alignment value);

  /// Reset alignment as if a new instance were created
  void reset_alignment();

  /// Examine the state of the stream abstraction.
  bool good_bit() const;

  /// Number of bytes left to read in message block chain
  size_t length() const;

  /// Read a narrow string.
  size_t read_string(ACE_CDR::Char*& dest,
    ACE_CDR::Char* str_alloc(ACE_CDR::ULong) = CORBA::string_alloc,
    void str_free(ACE_CDR::Char*) = CORBA::string_free);

  /// Read a wide string.
  size_t read_string(ACE_CDR::WChar*& dest,
    ACE_CDR::WChar* str_alloc(ACE_CDR::ULong) = CORBA::wstring_alloc,
    void str_free(ACE_CDR::WChar*) = CORBA::wstring_free);

  /// Skip the logical rd_ptr() over a given number of bytes = n * size.
  /// If alignment is enabled, skips any padding to align to 'size' before
  /// skipping the n * size bytes.
  /// This is used by the RTPS protocol to allow reading messages from
  /// future versions of the spec which may have additional optional fields.
  bool skip(ACE_CDR::UShort n, int size = 1);

  const char* pos_rd() const { return current_ ? current_->rd_ptr() : 0; }

  /**
   * The buffer @a x must be large enough to contain @a length
   * elements.
   * Return @c false on failure and @c true on success.
   */
  //@{ @name Read basic IDL types arrays
  bool read_boolean_array(ACE_CDR::Boolean* x, ACE_CDR::ULong length);
  bool read_char_array(ACE_CDR::Char* x, ACE_CDR::ULong length);
  bool read_wchar_array(ACE_CDR::WChar* x, ACE_CDR::ULong length);
  bool read_octet_array(ACE_CDR::Octet* x, ACE_CDR::ULong length);
  bool read_short_array(ACE_CDR::Short* x, ACE_CDR::ULong length);
  bool read_ushort_array(ACE_CDR::UShort* x, ACE_CDR::ULong length);
  bool read_long_array(ACE_CDR::Long* x, ACE_CDR::ULong length);
  bool read_ulong_array(ACE_CDR::ULong* x, ACE_CDR::ULong length);
  bool read_longlong_array(ACE_CDR::LongLong* x, ACE_CDR::ULong length);
  bool read_ulonglong_array(ACE_CDR::ULongLong* x, ACE_CDR::ULong length);
  bool read_float_array(ACE_CDR::Float* x, ACE_CDR::ULong length);
  bool read_double_array(ACE_CDR::Double* x, ACE_CDR::ULong length);
  bool read_longdouble_array(ACE_CDR::LongDouble* x, ACE_CDR::ULong length);
  //@}

  /// Note: the portion written starts at x and ends
  ///    at x + length.
  /// The length is *NOT* stored into the CDR stream.
  //@{ @name Array write operations
  bool write_boolean_array(const ACE_CDR::Boolean* x, ACE_CDR::ULong length);
  bool write_char_array(const ACE_CDR::Char* x, ACE_CDR::ULong length);
  bool write_wchar_array(const ACE_CDR::WChar* x, ACE_CDR::ULong length);
  bool write_octet_array(const ACE_CDR::Octet* x, ACE_CDR::ULong length);
  bool write_short_array(const ACE_CDR::Short* x, ACE_CDR::ULong length);
  bool write_ushort_array(const ACE_CDR::UShort* x, ACE_CDR::ULong length);
  bool write_long_array(const ACE_CDR::Long* x, ACE_CDR::ULong length);
  bool write_ulong_array(const ACE_CDR::ULong* x, ACE_CDR::ULong length);
  bool write_longlong_array(const ACE_CDR::LongLong* x, ACE_CDR::ULong length);
  bool write_ulonglong_array(const ACE_CDR::ULongLong* x, ACE_CDR::ULong length);
  bool write_float_array(const ACE_CDR::Float* x, ACE_CDR::ULong length);
  bool write_double_array(const ACE_CDR::Double* x, ACE_CDR::ULong length);
  bool write_longdouble_array(const ACE_CDR::LongDouble* x, ACE_CDR::ULong length);
  //@}

  // Insertion operators.
  friend OpenDDS_Dcps_Export
  bool operator<<(Serializer& s, ACE_CDR::Char x);
  friend OpenDDS_Dcps_Export
  bool operator<<(Serializer& s, ACE_CDR::Short x);
  friend OpenDDS_Dcps_Export
  bool operator<<(Serializer& s, ACE_CDR::UShort x);
  friend OpenDDS_Dcps_Export
  bool operator<<(Serializer& s, ACE_CDR::Long x);
  friend OpenDDS_Dcps_Export
  bool operator<<(Serializer& s, ACE_CDR::ULong x);
  friend OpenDDS_Dcps_Export
  bool operator<<(Serializer& s, ACE_CDR::LongLong x);
  friend OpenDDS_Dcps_Export
  bool operator<<(Serializer& s, ACE_CDR::ULongLong x);
  friend OpenDDS_Dcps_Export
  bool operator<<(Serializer& s, ACE_CDR::Float x);
  friend OpenDDS_Dcps_Export
  bool operator<<(Serializer& s, ACE_CDR::Double x);
  friend OpenDDS_Dcps_Export
  bool operator<<(Serializer& s, ACE_CDR::LongDouble x);
  friend OpenDDS_Dcps_Export
  bool operator<<(Serializer& s, const ACE_CDR::Char* x);
  friend OpenDDS_Dcps_Export
  bool operator<<(Serializer& s, const ACE_CDR::WChar* x);

#ifdef NONNATIVE_LONGDOUBLE
  friend OpenDDS_Dcps_Export
  bool operator<<(Serializer& s, long double x);
#endif

  // Using the ACE CDR Stream disambiguators.
  friend OpenDDS_Dcps_Export
  bool operator<<(Serializer& s, ACE_OutputCDR::from_boolean x);
  friend OpenDDS_Dcps_Export
  bool operator<<(Serializer& s, ACE_OutputCDR::from_char x);
  friend OpenDDS_Dcps_Export
  bool operator<<(Serializer& s, ACE_OutputCDR::from_wchar x);
  friend OpenDDS_Dcps_Export
  bool operator<<(Serializer& s, ACE_OutputCDR::from_octet x);
  friend OpenDDS_Dcps_Export
  bool operator<<(Serializer& s, ACE_OutputCDR::from_string x);
  friend OpenDDS_Dcps_Export
  bool operator<<(Serializer& s, ACE_OutputCDR::from_wstring x);

  friend OpenDDS_Dcps_Export
  bool operator<<(Serializer& s, const OPENDDS_STRING& x);

  template <typename CharT>
  struct FromBoundedString {
    typedef std::basic_string<CharT, std::char_traits<CharT>,
                              OPENDDS_ALLOCATOR(CharT) > string_t;
    FromBoundedString(const string_t& str, ACE_CDR::ULong bound)
      : str_(str), bound_(bound) {}
    const string_t& str_;
    ACE_CDR::ULong bound_;
  };

  friend OpenDDS_Dcps_Export
  bool operator<<(Serializer& s, FromBoundedString<char> x);

#ifdef DDS_HAS_WCHAR
  friend OpenDDS_Dcps_Export
  bool operator<<(Serializer& s, const OPENDDS_WSTRING& x);

  friend OpenDDS_Dcps_Export
  bool operator<<(Serializer& s, FromBoundedString<wchar_t> x);
#endif /* DDS_HAS_WCHAR */

  // Extraction operators.
  friend OpenDDS_Dcps_Export
  bool operator>>(Serializer& s, ACE_CDR::Char& x);
  friend OpenDDS_Dcps_Export
  bool operator>>(Serializer& s, ACE_CDR::Short& x);
  friend OpenDDS_Dcps_Export
  bool operator>>(Serializer& s, ACE_CDR::UShort& x);
  friend OpenDDS_Dcps_Export
  bool operator>>(Serializer& s, ACE_CDR::Long& x);
  friend OpenDDS_Dcps_Export
  bool operator>>(Serializer& s, ACE_CDR::ULong& x);
  friend OpenDDS_Dcps_Export
  bool operator>>(Serializer& s, ACE_CDR::LongLong& x);
  friend OpenDDS_Dcps_Export
  bool operator>>(Serializer& s, ACE_CDR::ULongLong& x);
  friend OpenDDS_Dcps_Export
  bool operator>>(Serializer& s, ACE_CDR::Float& x);
  friend OpenDDS_Dcps_Export
  bool operator>>(Serializer& s, ACE_CDR::Double& x);
  friend OpenDDS_Dcps_Export
  bool operator>>(Serializer& s, ACE_CDR::LongDouble& x);
  friend OpenDDS_Dcps_Export
  bool operator>>(Serializer& s, ACE_CDR::Char*& x);
  friend OpenDDS_Dcps_Export
  bool operator>>(Serializer& s, ACE_CDR::WChar*& x);

#ifdef NONNATIVE_LONGDOUBLE
  friend OpenDDS_Dcps_Export
  bool operator>>(Serializer& s, long double& x);
#endif

  // Using the ACE CDR Stream disambiguators.
  friend OpenDDS_Dcps_Export
  bool operator>>(Serializer& s, ACE_InputCDR::to_boolean x);
  friend OpenDDS_Dcps_Export
  bool operator>>(Serializer& s, ACE_InputCDR::to_char x);
  friend OpenDDS_Dcps_Export
  bool operator>>(Serializer& s, ACE_InputCDR::to_wchar x);
  friend OpenDDS_Dcps_Export
  bool operator>>(Serializer& s, ACE_InputCDR::to_octet x);
  friend OpenDDS_Dcps_Export
  bool operator>>(Serializer& s, ACE_InputCDR::to_string x);
  friend OpenDDS_Dcps_Export
  bool operator>>(Serializer& s, ACE_InputCDR::to_wstring x);

  friend OpenDDS_Dcps_Export
  bool operator>>(Serializer& s, OPENDDS_STRING& x);

  template <typename CharT>
  struct ToBoundedString {
    typedef std::basic_string<CharT, std::char_traits<CharT>,
                              OPENDDS_ALLOCATOR(CharT) > string_t;
    ToBoundedString(string_t& str, ACE_CDR::ULong bound)
      : str_(str), bound_(bound) {}
    string_t& str_;
    ACE_CDR::ULong bound_;
  };

  friend OpenDDS_Dcps_Export
  bool operator>>(Serializer& s, ToBoundedString<char> x);

#ifdef DDS_HAS_WCHAR
  friend OpenDDS_Dcps_Export
  bool operator>>(Serializer& s, OPENDDS_WSTRING& x);

  friend OpenDDS_Dcps_Export
  bool operator>>(Serializer& s, ToBoundedString<wchar_t> x);
#endif /* DDS_HAS_WCHAR */

  /// Read from the chain into a destination buffer.
  // This method doesn't respect alignment, so use with care.
  // Any of the other public methods (which know the type) are preferred.
  void buffer_read(char* dest, size_t size, bool swap);

  /// Align for reading: moves current_->rd_ptr() past the alignment padding.
  /// Alignments of 2, 4, or 8 are supported by CDR and this implementation.
  bool align_r(size_t alignment);

  /// Align for writing: moves current_->wr_ptr() past the padding, possibly
  /// zero-filling the pad bytes (based on the alignment_ setting).
  /// Alignments of 2, 4, or 8 are supported by CDR and this implementation.
  bool align_w(size_t alignment);

  /**
   * Read a XCDR parameter ID used in XCDR parameter lists.
   *
   * Returns true if successful.
   */
  bool read_parameter_id(unsigned& id, size_t& size);

  /**
   * Write a XCDR parameter ID used in XCDR parameter lists.
   *
   * Returns true if successful.
   */
  bool write_parameter_id(unsigned id, size_t size);

  /**
   * Read a delimiter used for XCDR2 delimited data.
   *
   * Returns true if successful.
   */
  bool read_delimiter(size_t& size);

  /**
   * Write a delimiter used for XCDR2 delimited data.
   *
   * Returns true if successful.
   */
  bool write_delimiter(size_t size);

private:
  /// Read an array of values from the chain.
  /// NOTE: This assumes that the buffer contains elements that are
  ///       properly aligned.  The buffer must have padding if the
  ///       elements are not naturally aligned; or this routine should
  ///       not be used.
  void read_array(char* x, size_t size, ACE_CDR::ULong length);
  void read_array(char* x, size_t size, ACE_CDR::ULong length, bool swap);

  /// Write to the chain from a source buffer.
  void buffer_write(const char* src, size_t size, bool swap);

  /// Write an array of values to the chain.
  /// NOTE: This assumes that there is _no_ padding between the array
  ///       elements.  If this is not the case, do not use this
  ///       method.  If padding exists in the array, it will be
  ///       written when _not_ swapping, and will _not_ be written
  ///       when swapping, resulting in corrupted data.
  void write_array(const char* x, size_t size, ACE_CDR::ULong length);
  void write_array(const char* x, size_t size, ACE_CDR::ULong length, bool swap);

  /// Efficient straight copy for quad words and shorter.  This is
  /// an instance method to match the swapcpy semantics.
  void smemcpy(char* to, const char* from, size_t n);

  /// Efficient swapping copy for quad words and shorter.  This is an
  /// instance method to allow clearing the good_bit_ on error.
  void swapcpy(char* to, const char* from, size_t n);

  /// Implementation of the actual read from the chain.
  size_t doread(char* dest, size_t size, bool swap, size_t offset);

  /// Implementation of the actual write to the chain.
  size_t dowrite(const char* dest, size_t size, bool swap, size_t offset);

  /// Update alignment state when a cont() chain is followed during a read.
  void align_cont_r();

  /// Update alignment state when a cont() chain is followed during a write.
  void align_cont_w();

  /// Currently active message block in chain.
  ACE_Message_Block* current_;

  /// Encoding Settings
  Encoding encoding_;

  /// Indicates whether bytes will be swapped for this stream.
  bool swap_bytes_;

  /// Indicates the current state of the stream abstraction.
  bool good_bit_;

  /**
   * Number of bytes off of max alignment that the current_ block's / rd_ptr()
   * started at.
   */
  unsigned char align_rshift_;

  /**
   * Number of bytes off of max alignment that the current_ block's wr_ptr()
   * started at.
   */
  unsigned char align_wshift_;

  /// Buffer that is copied for zero padding
  static const char ALIGN_PAD[Encoding::ALIGN_MAX];

public:
#if defined ACE_LITTLE_ENDIAN
  static const bool SWAP_BE = true;
#else
  static const bool SWAP_BE = false;
#endif

  // Flags and reserved ids used in parameter list ids.
  static const ACE_CDR::UShort pid_extended = 0x3f01;
  static const ACE_CDR::UShort pid_list_end = 0x3f02;
  static const ACE_CDR::UShort pid_impl_extension = 0x8000;
  static const ACE_CDR::UShort pid_must_understand = 0x4000;

  // EMHEADER must understand flag
  static const ACE_CDR::ULong emheader_must_understand = 1 << 31;
};

template<typename T> struct KeyOnly {
  explicit KeyOnly(T& mess) : t(mess) { }
  T& t;
};

namespace IDL {
  // Although similar to C++11 reference_wrapper, this template has the
  // additional Tag parameter to allow the IDL compiler to generate distinct
  // overloads for sequence/array typedefs that map to the same C++ types.
  template <typename T, typename /*Tag*/>
  struct DistinctType {
    typedef T value_type;
    T* val_;
    DistinctType(T& val) : val_(&val) {}
    operator T&() const { return *val_; }
  };
}

// predefined type methods
OpenDDS_Dcps_Export
bool max_serialized_size(
  const Encoding& encoding, size_t& size, const ACE_CDR::Short& value,
  size_t count = 1);
OpenDDS_Dcps_Export
bool max_serialized_size(
  const Encoding& encoding, size_t& size, const ACE_CDR::UShort& value,
  size_t count = 1);
OpenDDS_Dcps_Export
bool max_serialized_size(
  const Encoding& encoding, size_t& size, const ACE_CDR::Long& value,
  size_t count = 1);
OpenDDS_Dcps_Export
bool max_serialized_size(
  const Encoding& encoding, size_t& size, const ACE_CDR::ULong& value,
  size_t count = 1);
OpenDDS_Dcps_Export
bool max_serialized_size(
  const Encoding& encoding, size_t& size, const ACE_CDR::LongLong& value,
  size_t count = 1);
OpenDDS_Dcps_Export
bool max_serialized_size(
  const Encoding& encoding, size_t& size, const ACE_CDR::ULongLong& value,
  size_t count = 1);
OpenDDS_Dcps_Export
bool max_serialized_size(
  const Encoding& encoding, size_t& size, const ACE_CDR::Float& value,
  size_t count = 1);
OpenDDS_Dcps_Export
bool max_serialized_size(
  const Encoding& encoding, size_t& size, const ACE_CDR::Double& value,
  size_t count = 1);
OpenDDS_Dcps_Export
bool max_serialized_size(
  const Encoding& encoding, size_t& size, const ACE_CDR::LongDouble& value,
  size_t count = 1);

// predefined type method disambiguators.
OpenDDS_Dcps_Export
bool max_serialized_size(
  const Encoding& encoding, size_t& size,
  const ACE_OutputCDR::from_boolean value, size_t count = 1);
OpenDDS_Dcps_Export
bool max_serialized_size(
  const Encoding& encoding, size_t& size,
  const ACE_OutputCDR::from_char value, size_t count = 1);
OpenDDS_Dcps_Export
bool max_serialized_size(
  const Encoding& encoding, size_t& size,
  const ACE_OutputCDR::from_wchar value, size_t count = 1);
OpenDDS_Dcps_Export
bool max_serialized_size(
  const Encoding& encoding, size_t& size,
  const ACE_OutputCDR::from_octet value, size_t count = 1);

/// predefined type method explicit disambiguators.
OpenDDS_Dcps_Export
void max_serialized_size_boolean(const Encoding& encoding, size_t& size,
  size_t count = 1);
OpenDDS_Dcps_Export
void max_serialized_size_char(const Encoding& encoding, size_t& size,
  size_t count = 1);
OpenDDS_Dcps_Export
void max_serialized_size_wchar(const Encoding& encoding, size_t& size,
  size_t count = 1);
OpenDDS_Dcps_Export
void max_serialized_size_octet(const Encoding& encoding, size_t& size,
  size_t count = 1);
OpenDDS_Dcps_Export
void max_serialized_size_ulong(const Encoding& encoding, size_t& size,
  size_t count = 1);

OpenDDS_Dcps_Export
void serialized_size_ulong(const Encoding& encoding, size_t& size,
  size_t count = 1);

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#ifdef __ACE_INLINE__
#  include "Serializer.inl"
#endif

#endif /* OPENDDS_DCPS_SERIALIZER_H */
