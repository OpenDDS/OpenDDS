/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

/**
 * @file Serializer.h
 *
 * The serialization interface for a C++ type called Type consists of the
 * following overloads:
 *
 *   void serialized_size(
 *       const Encoding& encoding, size_t& size, const Type& value);
 *     Get the byte size of the representation of value.
 *
 *   bool operator<<(Serializer& serializer, Type& value);
 *     Tries to encode value into the stream of the serializer. Returns true if
 *     successful, else false.
 *
 *   bool operator>>(Serializer& serializer, const Type& value);
 *     Tries to decodes a representation of Type located at the current
 *     position of the stream and use that to set value. Returns true if
 *     successful, else false.
 */

#ifndef OPENDDS_DCPS_SERIALIZER_H
#define OPENDDS_DCPS_SERIALIZER_H

#include <ace/config-macros.h>
#ifndef ACE_LACKS_PRAGMA_ONCE
#  pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "Definitions.h"
#include "PoolAllocator.h"
#include "Message_Block_Ptr.h"
#include "dcps_export.h"

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
String endianness_to_string(Endianness endianness);

enum Extensibility {
  FINAL,
  APPENDABLE,
  MUTABLE
};

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
const size_t xcdr1_pid_alignment = 4;

/// Align "value" by "by" if it's not already
OpenDDS_Dcps_Export
void align(size_t& value, size_t by);

/**
 * Represents the settings of the serialized stream. Passed to things like
 * Serializer, serialized_size(), and max_serialized_size(), etc.
 */
class OpenDDS_Dcps_Export Encoding {
public:
  /**
   * The encoding kinds represent the possible compatible encoding sets used
   * for serialization.
   */
  enum Kind {
    /**
     * Extensible CDR version 1 from XTypes.
     * This represents standard non-XTypes CDR if the type is final.
     */
    KIND_XCDR1,
    /**
     * Extensible CDR version 2 from XTypes.
     */
    KIND_XCDR2,
    /**
     * This is the classic encoding of OpenDDS used when there is no RTPS
     * transport being used. It has no padding bytes and no XCDR behavior.
     */
    KIND_UNALIGNED_CDR,
  };

  enum Alignment {
    ALIGN_NONE = 0, ///< No Alignment Needed
    ALIGN_CDR = 8, ///< Align for CDR and XCDR1
    ALIGN_XCDR2 = 4, ///< Align for XCDR2
    ALIGN_MAX = ALIGN_CDR ///< Maximum alignment that could be used
  };

  /**
   * XCDR version derived from the Encoding kind.
   */
  enum XcdrVersion {
    XCDR_VERSION_NONE,
    XCDR_VERSION_1,
    XCDR_VERSION_2
  };

  /// Encoding with KIND_XCDR1 and ENDIAN_NATIVE
  Encoding();

  explicit Encoding(Kind kind, Endianness endianness = ENDIAN_NATIVE);

  Encoding(Kind kind, bool swap_bytes);

  Kind kind() const;
  void kind(Kind value);

  Endianness endianness() const;
  void endianness(Endianness value);

  Alignment alignment() const;
  void alignment(Alignment value);

  ///@{
  /**
   * Should the padding bytes being inserted into the stream be zero
   * initialized?
   */
  bool zero_init_padding() const;
  void zero_init_padding(bool value);
  ///@}

  ///@{
  /**
   * Should the XCDR2 sequence DHEADER be skipped?
   * This is not spec compliant -- used for compatibility with earlier
   * OpenDDS versions that had a bug.
   * Only used for XTypes::TypeObject and related structs.
   */
  bool skip_sequence_dheader() const;
  void skip_sequence_dheader(bool value);
  ///@}

  /// Return the maximum alignment dictated by the alignment policy.
  size_t max_align() const;

  /// Align "value" to "by" and according to the stream's alignment.
  void align(size_t& value, size_t by = (std::numeric_limits<size_t>::max)()) const;

  XcdrVersion xcdr_version() const;
  void xcdr_version(XcdrVersion value);

  ///@{
  /**
   * Returns true if the encoding kind is excepted to have a header for RTPS
   * serialized data payloads.
   */
  static bool is_encapsulated(Kind kind);
  bool is_encapsulated() const;
  ///@}

  String to_string() const;
  static String kind_to_string(Kind value);

private:
  Kind kind_;
  Endianness endianness_;
  Alignment alignment_;
  bool zero_init_padding_;
  bool skip_sequence_dheader_;
  XcdrVersion xcdr_version_;
};

/**
 * Represents the RTPS encapsulation header for serialized data.
 *
 * This consists of 4 bytes that appear in front of the data. The first two
 * bytes represents the kind of the encoding the data uses and the last two
 * bytes are traditionally reserved.
 *
 * See XTypes 1.3 7.6.3.1.2
 */
class OpenDDS_Dcps_Export EncapsulationHeader {
public:
  /**
   * The known possible values of the first 2 bytes represented as big endian
   * integers.
   */
  enum Kind {
    KIND_CDR_BE = 0x0000,
    KIND_CDR_LE = 0x0001,
    KIND_PL_CDR_BE = 0x0002,
    KIND_PL_CDR_LE = 0x0003,
    KIND_CDR2_BE = 0x0006,
    KIND_CDR2_LE = 0x0007,
    KIND_D_CDR2_BE = 0x0008,
    KIND_D_CDR2_LE = 0x0009,
    KIND_PL_CDR2_BE = 0x000a,
    KIND_PL_CDR2_LE = 0x000b,
    KIND_XML = 0x0004,
    KIND_INVALID = 0xFFFF
  };

  const static size_t serialized_size = 4;
  const static size_t padding_marker_byte_index = 3;
  const static size_t padding_marker_alignment = 4;

  EncapsulationHeader(Kind k = KIND_CDR_BE, ACE_CDR::UShort options = 0);

  /**
   * Success can be verified using is_good()
   */
  EncapsulationHeader(const Encoding& enc, Extensibility ext, ACE_CDR::UShort options = 0);

  Kind kind() const;
  void kind(Kind value);

  ACE_UINT16 options() const;
  void options(ACE_UINT16 value);

  /**
   * post-initialization test for a successful call to from_encoding during
   * construction of this encapsulation header.
   */
  bool is_good() const;

  /**
   * Translate from an encoding, returns false if it failed.
   */
  bool from_encoding(const Encoding& encoding, Extensibility extensibility);

  /**
   * Translate to an encoding, returns false if it failed.
   */
  bool to_encoding(Encoding& encoding, Extensibility expected_extensibility);

  String to_string() const;

  static bool set_encapsulation_options(Message_Block_Ptr& mb);

private:
  /// The first two bytes as a big endian integer
  Kind kind_;
  /// The last two bytes as a big endian integer
  ACE_CDR::UShort options_;
};

class Serializer;

OpenDDS_Dcps_Export
bool operator>>(Serializer& s, EncapsulationHeader& value);

OpenDDS_Dcps_Export
bool operator<<(Serializer& s, const EncapsulationHeader& value);

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
 * This helper class can be used to construct ACE message blocks from
 * IDL sequences of octet (or compatible substitutes) and be used with
 * the Serializer to serialize/deserialize directly into the byte
 * buffer.  The sequence must have its length set before constructing
 * this object.  T should provide a length() method which is the size
 * of the buffer and get_buffer() which returns a pointer to the
 * underlying byte sequence.
 */
template <typename T>
class MessageBlockHelper {
public:
  /**
   * This constructor receives an already populated OctetSeq so the write pointer is advanced
   */
  explicit MessageBlockHelper(const T& seq)
    : db_(seq.length(), ACE_Message_Block::MB_DATA,
          reinterpret_cast<const char*>(seq.get_buffer()),
          0 /*alloc*/, 0 /*lock*/, ACE_Message_Block::DONT_DELETE, 0 /*db_alloc*/)
    , mb_(&db_, ACE_Message_Block::DONT_DELETE, 0 /*mb_alloc*/)
  {
    mb_.wr_ptr(mb_.space());
  }

  explicit MessageBlockHelper(T& seq)
    : db_(seq.length(), ACE_Message_Block::MB_DATA,
          reinterpret_cast<const char*>(seq.get_buffer()),
          0 /*alloc*/, 0 /*lock*/, ACE_Message_Block::DONT_DELETE, 0 /*db_alloc*/)
    , mb_(&db_, ACE_Message_Block::DONT_DELETE, 0 /*mb_alloc*/)
  {}

  operator ACE_Message_Block*() { return &mb_; }

private:
  ACE_Data_Block db_;
  ACE_Message_Block mb_;
};

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

  /// Flags and reserved ids used in parameter list ids.
  ///@{
  static const ACE_CDR::UShort pid_extended = 0x3f01;
  /**
   * Note that this is different than OpenDDS::RTPS::PID_SENTINEL(0x0001). See
   * XTypes 1.3 Table 34 for details.
   */
  static const ACE_CDR::UShort pid_list_end = 0x3f02;
  static const ACE_CDR::UShort pid_impl_extension = 0x8000;
  static const ACE_CDR::UShort pid_must_understand = 0x4000;
  ///@}

  // EMHEADER must understand flag
  static const ACE_CDR::ULong emheader_must_understand = 1U << 31U;

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
   * Equivalent to: Serializer(chain, kind, swap_bytes ? ENDIAN_NONNATIVE : ENDIAN_NATIVE)
   */
  Serializer(ACE_Message_Block* chain, Encoding::Kind kind, bool swap_bytes);

  virtual ~Serializer();

  const Encoding& encoding() const;
  void encoding(const Encoding& value);

  void swap_bytes(bool do_swap);
  bool swap_bytes() const;

  Endianness endianness() const;
  void endianness(Endianness value);

  Encoding::Alignment alignment() const;
  void alignment(Encoding::Alignment value);

  /// Reset alignment as if a new instance were created
  void reset_alignment();

  bool good_bit() const;

  /// Number of bytes left to read in message block chain
  size_t length() const;

  typedef ACE_CDR::Char* (*StrAllocate)(ACE_CDR::ULong);
  typedef void (*StrFree)(ACE_CDR::Char*);
  typedef ACE_CDR::WChar* (*WStrAllocate)(ACE_CDR::ULong);
  typedef void (*WStrFree)(ACE_CDR::WChar*);

  size_t read_string(ACE_CDR::Char*& dest,
                     StrAllocate str_alloc = 0,
                     StrFree str_free = 0);

  void free_string(ACE_CDR::Char* str,
                   StrFree str_free = 0);

  size_t read_string(ACE_CDR::WChar*& dest,
                     WStrAllocate str_alloc = 0,
                     WStrFree str_free = 0);

  void free_string(ACE_CDR::WChar* str,
                   WStrFree str_free = 0);

  /// Skip the logical rd_ptr() over a given number of bytes = n * size.
  /// If alignment is enabled, skips any padding to align to 'size' before
  /// skipping the n * size bytes.
  /// This is used by the RTPS protocol to allow reading messages from
  /// future versions of the spec which may have additional optional fields.
  bool skip(size_t n, int size = 1);

  const char* pos_rd() const { return current_ ? current_->rd_ptr() : 0; }
  const char* pos_wr() const { return current_ ? current_->wr_ptr() : 0; }

  /// Examine the logical reading position of the stream.
  size_t rpos() const { return rpos_; }

  /// Examine the logical writing position of the stream.
  size_t wpos() const { return wpos_; }

  /**
   * Read basic IDL types arrays
   * The buffer @a x must be large enough to contain @a length
   * elements.
   * Return @c false on failure and @c true on success.
   */
  ///@{
  bool read_boolean_array(ACE_CDR::Boolean* x, ACE_CDR::ULong length);
  bool read_char_array(ACE_CDR::Char* x, ACE_CDR::ULong length);
  bool read_wchar_array(ACE_CDR::WChar* x, ACE_CDR::ULong length);
  bool read_octet_array(ACE_CDR::Octet* x, ACE_CDR::ULong length);
#if OPENDDS_HAS_EXPLICIT_INTS
  bool read_int8_array(ACE_CDR::Int8* x, ACE_CDR::ULong length);
  bool read_uint8_array(ACE_CDR::UInt8* x, ACE_CDR::ULong length);
#endif
  bool read_short_array(ACE_CDR::Short* x, ACE_CDR::ULong length);
  bool read_ushort_array(ACE_CDR::UShort* x, ACE_CDR::ULong length);
  bool read_long_array(ACE_CDR::Long* x, ACE_CDR::ULong length);
  bool read_ulong_array(ACE_CDR::ULong* x, ACE_CDR::ULong length);
  bool read_longlong_array(ACE_CDR::LongLong* x, ACE_CDR::ULong length);
  bool read_ulonglong_array(ACE_CDR::ULongLong* x, ACE_CDR::ULong length);
  bool read_float_array(ACE_CDR::Float* x, ACE_CDR::ULong length);
  bool read_double_array(ACE_CDR::Double* x, ACE_CDR::ULong length);
  bool read_longdouble_array(ACE_CDR::LongDouble* x, ACE_CDR::ULong length);
  ///@}

  /// Array write operations
  /// Note: the portion written starts at x and ends
  ///    at x + length.
  /// The length is *NOT* stored into the CDR stream.
  ///@{
  bool write_boolean_array(const ACE_CDR::Boolean* x, ACE_CDR::ULong length);
  bool write_char_array(const ACE_CDR::Char* x, ACE_CDR::ULong length);
  bool write_wchar_array(const ACE_CDR::WChar* x, ACE_CDR::ULong length);
  bool write_octet_array(const ACE_CDR::Octet* x, ACE_CDR::ULong length);
#if OPENDDS_HAS_EXPLICIT_INTS
  bool write_int8_array(const ACE_CDR::Int8* x, ACE_CDR::ULong length);
  bool write_uint8_array(const ACE_CDR::UInt8* x, ACE_CDR::ULong length);
#endif
  bool write_short_array(const ACE_CDR::Short* x, ACE_CDR::ULong length);
  bool write_ushort_array(const ACE_CDR::UShort* x, ACE_CDR::ULong length);
  bool write_long_array(const ACE_CDR::Long* x, ACE_CDR::ULong length);
  bool write_ulong_array(const ACE_CDR::ULong* x, ACE_CDR::ULong length);
  bool write_longlong_array(const ACE_CDR::LongLong* x, ACE_CDR::ULong length);
  bool write_ulonglong_array(const ACE_CDR::ULongLong* x, ACE_CDR::ULong length);
  bool write_float_array(const ACE_CDR::Float* x, ACE_CDR::ULong length);
  bool write_double_array(const ACE_CDR::Double* x, ACE_CDR::ULong length);
  bool write_longdouble_array(const ACE_CDR::LongDouble* x, ACE_CDR::ULong length);
  ///@}

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
#if OPENDDS_HAS_EXPLICIT_INTS
  friend OpenDDS_Dcps_Export
  bool operator<<(Serializer& s, ACE_OutputCDR::from_uint8 x);
  friend OpenDDS_Dcps_Export
  bool operator<<(Serializer& s, ACE_OutputCDR::from_int8 x);
#endif

  friend OpenDDS_Dcps_Export
  bool operator<<(Serializer& s, const String& x);

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
  bool operator<<(Serializer& s, const WString& x);

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
#if OPENDDS_HAS_EXPLICIT_INTS
  friend OpenDDS_Dcps_Export
  bool operator>>(Serializer& s, ACE_InputCDR::to_uint8 x);
  friend OpenDDS_Dcps_Export
  bool operator>>(Serializer& s, ACE_InputCDR::to_int8 x);
#endif

  friend OpenDDS_Dcps_Export
  bool operator>>(Serializer& s, String& x);

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
  bool operator>>(Serializer& s, WString& x);

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
  bool read_parameter_id(unsigned& id, size_t& size, bool& must_understand);

  /**
   * Write a XCDR parameter ID used in XCDR parameter lists.
   *
   * Returns true if successful.
   */
  bool write_parameter_id(const unsigned id, size_t size, bool must_understand = false);

  /**
   * Write the parameter ID that marks the end of XCDR1 parameter lists.
   *
   * Returns true if successful.
   */
  bool write_list_end_parameter_id();

  /**
   * Read a delimiter used for XCDR2 delimited data.
   *
   * Returns true if successful and size will be set to the size of the CDR
   * value excluding the delimiter.
   */
  bool read_delimiter(size_t& size);

  /**
   * Write a delimiter used for XCDR2 delimited data.
   *
   * Size is assumed to include the delimiter as serialized_size would return.
   * Returns true if successful.
   */
  bool write_delimiter(size_t size);

  enum ConstructionStatus {
    ConstructionSuccessful,
    ElementConstructionFailure,
    BoundConstructionFailure
  };

  ConstructionStatus get_construction_status() const;

  void set_construction_status(ConstructionStatus cs);

  struct OpenDDS_Dcps_Export ScopedAlignmentContext {
    explicit ScopedAlignmentContext(Serializer& ser, size_t min_read = 0);
    virtual ~ScopedAlignmentContext() { restore(ser_); }

    void restore(Serializer& ser) const;

    Serializer& ser_;
    const size_t max_align_;
    const size_t start_rpos_;
    const size_t rblock_;
    const size_t min_read_;
    const size_t start_wpos_;
    const size_t wblock_;
  };

  template <typename T>
  bool peek_helper(ACE_Message_Block* const block, size_t bytes, T& t)
  {
    bool result = false;
    char* const rd_ptr = block->rd_ptr();
    const size_t length = block->length();
    if (!block->cont() || length == 0 || (bytes != 0 && bytes <= length)) {
      result = *this >> t;
    } else {
      result = peek_helper(block->cont(), bytes - length, t);
    }
    block->rd_ptr(rd_ptr);
    return result;
  }

  template <typename T>
  bool peek(T& t)
  {
    // save
    const size_t rpos = rpos_;
    const unsigned char align_rshift = align_rshift_;
    ACE_Message_Block* const current = current_;

    // read
    if (!peek_helper(current_, 0, t)) {
      return false;
    }

    // reset
    current_ = current;
    align_rshift_ = align_rshift;
    rpos_ = rpos;
    return true;
  }

  bool peek(ACE_CDR::ULong& t);

private:
  ///@{
  /// Read an array of values from the chain.
  /// NOTE: This assumes that the buffer contains elements that are
  ///       properly aligned.  The buffer must have padding if the
  ///       elements are not naturally aligned; or this routine should
  ///       not be used.
  void read_array(char* x, size_t size, ACE_CDR::ULong length);
  void read_array(char* x, size_t size, ACE_CDR::ULong length, bool swap);
  ///@}

  /// Write to the chain from a source buffer.
  void buffer_write(const char* src, size_t size, bool swap);

  ///@{
  /// Write an array of values to the chain.
  /// NOTE: This assumes that there is _no_ padding between the array
  ///       elements.  If this is not the case, do not use this
  ///       method.  If padding exists in the array, it will be
  ///       written when _not_ swapping, and will _not_ be written
  ///       when swapping, resulting in corrupted data.
  void write_array(const char* x, size_t size, ACE_CDR::ULong length);
  void write_array(const char* x, size_t size, ACE_CDR::ULong length, bool swap);
  ///@}

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

  static unsigned char offset(char* index, size_t start, size_t align);

  /// Currently active message block in chain.
  ACE_Message_Block* current_;

  /// Encoding Settings
  Encoding encoding_;

  /// Indicates whether bytes will be swapped for this stream.
  bool swap_bytes_;

  /// Indicates the current state of the stream abstraction.
  bool good_bit_;

  /// The way to judge whether tryconstruct trim is able to be properly done
  ConstructionStatus construction_status_;

  /**
   * Number of bytes off of max alignment that the current_ block's rd_ptr()
   * started at.
   */
  unsigned char align_rshift_;

  /**
   * Number of bytes off of max alignment that the current_ block's wr_ptr()
   * started at.
   */
  unsigned char align_wshift_;

  /// Logical reading position of the stream.
  size_t rpos_;

  /// Logical writing position of the stream.
  size_t wpos_;

  /// Buffer that is copied for zero padding
  static const char ALIGN_PAD[Encoding::ALIGN_MAX];
};

template<typename Type>
struct KeyOnly {
  explicit KeyOnly(Type& value)
    : value(value)
  {
  }

  Type& value;
};

template<typename Type>
struct NestedKeyOnly {
  explicit NestedKeyOnly(Type& value)
    : value(value)
  {
  }

  Type& value;
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

template<typename Type>
void set_default(Type&)
{
  OPENDDS_ASSERT(false);
}

template<typename Type, typename Tag>
void set_default(IDL::DistinctType<Type, Tag>)
{
  OPENDDS_ASSERT(false);
}

// predefined type methods
OpenDDS_Dcps_Export
bool primitive_serialized_size(
  const Encoding& encoding, size_t& size, const ACE_CDR::Short& value,
  size_t count = 1);
OpenDDS_Dcps_Export
bool primitive_serialized_size(
  const Encoding& encoding, size_t& size, const ACE_CDR::UShort& value,
  size_t count = 1);
OpenDDS_Dcps_Export
bool primitive_serialized_size(
  const Encoding& encoding, size_t& size, const ACE_CDR::Long& value,
  size_t count = 1);
OpenDDS_Dcps_Export
bool primitive_serialized_size(
  const Encoding& encoding, size_t& size, const ACE_CDR::ULong& value,
  size_t count = 1);
OpenDDS_Dcps_Export
bool primitive_serialized_size(
  const Encoding& encoding, size_t& size, const ACE_CDR::LongLong& value,
  size_t count = 1);
OpenDDS_Dcps_Export
bool primitive_serialized_size(
  const Encoding& encoding, size_t& size, const ACE_CDR::ULongLong& value,
  size_t count = 1);
OpenDDS_Dcps_Export
bool primitive_serialized_size(
  const Encoding& encoding, size_t& size, const ACE_CDR::Float& value,
  size_t count = 1);
OpenDDS_Dcps_Export
bool primitive_serialized_size(
  const Encoding& encoding, size_t& size, const ACE_CDR::Double& value,
  size_t count = 1);
OpenDDS_Dcps_Export
bool primitive_serialized_size(
  const Encoding& encoding, size_t& size, const ACE_CDR::LongDouble& value,
  size_t count = 1);

// predefined type method disambiguators.
OpenDDS_Dcps_Export
bool primitive_serialized_size(
  const Encoding& encoding, size_t& size,
  const ACE_OutputCDR::from_boolean value, size_t count = 1);
OpenDDS_Dcps_Export
bool primitive_serialized_size(
  const Encoding& encoding, size_t& size,
  const ACE_OutputCDR::from_char value, size_t count = 1);
OpenDDS_Dcps_Export
bool primitive_serialized_size(
  const Encoding& encoding, size_t& size,
  const ACE_OutputCDR::from_wchar value, size_t count = 1);
OpenDDS_Dcps_Export
bool primitive_serialized_size(
  const Encoding& encoding, size_t& size,
  const ACE_OutputCDR::from_octet value, size_t count = 1);
#if OPENDDS_HAS_EXPLICIT_INTS
OpenDDS_Dcps_Export
bool primitive_serialized_size(
  const Encoding& encoding, size_t& size,
  const ACE_OutputCDR::from_uint8 value, size_t count = 1);
OpenDDS_Dcps_Export
bool primitive_serialized_size(
  const Encoding& encoding, size_t& size,
  const ACE_OutputCDR::from_int8 value, size_t count = 1);
#endif

// predefined type method explicit disambiguators.
OpenDDS_Dcps_Export
void primitive_serialized_size_boolean(const Encoding& encoding, size_t& size,
  size_t count = 1);
OpenDDS_Dcps_Export
void primitive_serialized_size_char(const Encoding& encoding, size_t& size,
  size_t count = 1);
OpenDDS_Dcps_Export
void primitive_serialized_size_wchar(const Encoding& encoding, size_t& size,
  size_t count = 1);
OpenDDS_Dcps_Export
void primitive_serialized_size_octet(const Encoding& encoding, size_t& size,
  size_t count = 1);
OpenDDS_Dcps_Export
void primitive_serialized_size_ulong(const Encoding& encoding, size_t& size,
  size_t count = 1);
#if OPENDDS_HAS_EXPLICIT_INTS
OpenDDS_Dcps_Export
void primitive_serialized_size_uint8(const Encoding& encoding, size_t& size,
  size_t count = 1);
OpenDDS_Dcps_Export
void primitive_serialized_size_int8(const Encoding& encoding, size_t& size,
  size_t count = 1);
#endif

/// Add delimiter to the size of a serialized size if the encoding has them.
OpenDDS_Dcps_Export
void serialized_size_delimiter(const Encoding& encoding, size_t& size);

OpenDDS_Dcps_Export
void serialized_size_parameter_id(
  const Encoding& encoding, size_t& size, size_t& running_size);

OpenDDS_Dcps_Export
void serialized_size_list_end_parameter_id(
  const Encoding& encoding, size_t& size, size_t& running_size);

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#ifdef __ACE_INLINE__
#  include "Serializer.inl"
#endif

#endif /* OPENDDS_DCPS_SERIALIZER_H */
