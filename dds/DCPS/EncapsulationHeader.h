/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_ENCAPSULATION_HEADER_H
#define OPENDDS_DCPS_ENCAPSULATION_HEADER_H

#include "Serializer.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

/**
 * Represents the RTPS encapsulation header for serialized data.
 *
 * This consists of 4 bytes that appear in front of the data. The first two
 * bytes represents the kind of the encoding the data uses and the last two
 * bytes (known as "options") are traditionally reserved.
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

  EncapsulationHeader(Kind kind = KIND_CDR_BE,
                      ACE_CDR::UShort options = 0)
  : kind_(kind)
  , options_(options)
  {}

  /**
   * Success can be verified using is_good()
   */
  EncapsulationHeader(const Encoding& enc,
                      Extensibility ext,
                      ACE_CDR::UShort options = 0);

  Kind kind() const
  {
    return kind_;
  }

  void kind(Kind value)
  {
    kind_ = value;
  }

  ACE_UINT16 options() const
  {
    return options_;
  }

  void options(ACE_UINT16 value)
  {
    options_ = value;
  }

  /**
   * post-initialization test for a successful call to from_encoding during
   * construction of this encapsulation header.
   */
  bool is_good() const
  {
    return kind_ != KIND_INVALID;
  }

  String to_string() const;

  static bool set_encapsulation_options(Message_Block_Ptr& mb);

private:
  /// The first two bytes as a big endian integer
  Kind kind_;
  /// The last two bytes as a big endian integer
  ACE_CDR::UShort options_;
};

OpenDDS_Dcps_Export
bool operator>>(Serializer& s, EncapsulationHeader& value);

OpenDDS_Dcps_Export
bool operator<<(Serializer& s, const EncapsulationHeader& value);

/**
 * Translate from an encoding, returns false if it failed.
 */
OpenDDS_Dcps_Export
bool from_encoding(EncapsulationHeader& header,
                   const Encoding& encoding, Extensibility extensibility);

/**
 * Translate to an encoding, returns false if it failed.
 */
OpenDDS_Dcps_Export
bool to_encoding(Encoding& encoding,
                 const EncapsulationHeader& header,
                 Extensibility expected_extensibility);

/**
 * Like to_encoding, but without an expected extensibility.
 */
OpenDDS_Dcps_Export
bool to_any_encoding(Encoding& encoding,
                     const EncapsulationHeader& header);

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_DCPS_ENCAPSULATION_HEADER_H */
