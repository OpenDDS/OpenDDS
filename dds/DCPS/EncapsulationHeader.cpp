/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "EncapsulationHeader.h"

#include "SafetyProfileStreams.h"
#include "debug.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

EncapsulationHeader::EncapsulationHeader(const Encoding& enc,
                                         Extensibility ext,
                                         ACE_CDR::UShort options)
  : kind_(KIND_INVALID)
  , options_(options)
{
  if (!from_encoding(*this, enc, ext)) {
    kind_ = KIND_INVALID;
  }
}

String
EncapsulationHeader::to_string() const
{
  switch (kind_) {
  case KIND_CDR_BE:
    return "CDR/XCDR1 Big Endian Plain";
  case KIND_CDR_LE:
    return "CDR/XCDR1 Little Endian Plain";
  case KIND_PL_CDR_BE:
    return "CDR/XCDR1 Big Endian Parameter List";
  case KIND_PL_CDR_LE:
    return "CDR/XCDR1 Little Endian Parameter List";
  case KIND_CDR2_BE:
    return "XCDR2 Big Endian Plain";
  case KIND_CDR2_LE:
    return "XCDR2 Little Endian Plain";
  case KIND_D_CDR2_BE:
    return "XCDR2 Big Endian Delimited";
  case KIND_D_CDR2_LE:
    return "XCDR2 Little Endian Delimited";
  case KIND_PL_CDR2_BE:
    return "XCDR2 Big Endian Parameter List";
  case KIND_PL_CDR2_LE:
    return "XCDR2 Little Endian Parameter List";
  case KIND_XML:
    return "XML";
  case KIND_INVALID:
    return "Invalid";
  default:
    return "Unknown: 0x" + to_dds_string(static_cast<unsigned>(kind_), true);
  }
}

bool
EncapsulationHeader::set_encapsulation_options(Message_Block_Ptr& mb)
{
  if (mb->length() < padding_marker_byte_index + 1) {
    if (log_level >= LogLevel::Error) {
      ACE_ERROR((LM_ERROR,
                 "(%P|%t) ERROR: EncapsulationHeader::set_encapsulation_options: "
                 "insufficient buffer size %B\n",
                 mb->length()));
    }
    return false;
  }

  mb->rd_ptr()[padding_marker_byte_index] |= ((padding_marker_alignment - mb->length() % padding_marker_alignment) & 0x03);
  return true;
}

bool
operator>>(Serializer& s,
           EncapsulationHeader& value)
{
  ACE_CDR::Octet data[EncapsulationHeader::serialized_size];
  if (!s.read_octet_array(&data[0], EncapsulationHeader::serialized_size)) {
    return false;
  }
  value.kind(static_cast<EncapsulationHeader::Kind>(
    (static_cast<ACE_UINT16>(data[0]) << 8) | data[1]));
  value.options((static_cast<ACE_UINT16>(data[2]) << 8) | data[3]);
  s.reset_alignment();
  return true;
}

bool
operator<<(Serializer& s,
           const EncapsulationHeader& value)
{
  if (!value.is_good()) {
    return false;
  }
  ACE_CDR::Octet data[EncapsulationHeader::serialized_size];
  data[0] = (value.kind() >> 8) & 0xff;
  data[1] = value.kind() & 0xff;
  data[2] = (value.options() >> 8) & 0xff;
  data[3] = value.options() & 0xff;
  const bool ok = s.write_octet_array(&data[0],
    EncapsulationHeader::serialized_size);
  s.reset_alignment();
  return ok;
}

bool
from_encoding(EncapsulationHeader& header,
              const Encoding& encoding,
              Extensibility extensibility)
{
  const bool big = encoding.endianness() == ENDIAN_BIG;
  switch (encoding.kind()) {
  case Encoding::KIND_XCDR1:
    switch (extensibility) {
    case FINAL:
    case APPENDABLE:
      header.kind(big ? EncapsulationHeader::KIND_CDR_BE : EncapsulationHeader::KIND_CDR_LE);
      break;
    case MUTABLE:
      header.kind(big ? EncapsulationHeader::KIND_PL_CDR_BE : EncapsulationHeader::KIND_PL_CDR_LE);
      break;
    }
    break;
  case Encoding::KIND_XCDR2:
    switch (extensibility) {
    case FINAL:
      header.kind(big ? EncapsulationHeader::KIND_CDR2_BE : EncapsulationHeader::KIND_CDR2_LE);
      break;
    case APPENDABLE:
      header.kind(big ? EncapsulationHeader::KIND_D_CDR2_BE : EncapsulationHeader::KIND_D_CDR2_LE);
      break;
    case MUTABLE:
      header.kind(big ? EncapsulationHeader::KIND_PL_CDR2_BE : EncapsulationHeader::KIND_PL_CDR2_LE);
      break;
    }
    break;
  default:
    if (log_level >= LogLevel::Error) {
      ACE_ERROR((LM_ERROR,
                 "(%P|%t) ERROR: from_encoding: "
                 "got encoding with unsupported kind: %C\n",
                 Encoding::kind_to_string(encoding.kind()).c_str()));
    }
    return false;
  }

  return true;
}

namespace {
  bool
  to_encoding_i(Encoding& encoding,
                const EncapsulationHeader& header,
                Extensibility* expected_extensibility_ptr)
  {
    Extensibility expected_extensibility = expected_extensibility_ptr ?
      *expected_extensibility_ptr : FINAL; // Placeholder, doesn't matter
    bool wrong_extensibility = true;
    switch (header.kind()) {
    case EncapsulationHeader::KIND_CDR_BE:
      encoding.kind(Encoding::KIND_XCDR1);
      encoding.endianness(ENDIAN_BIG);
      wrong_extensibility = expected_extensibility == MUTABLE;
      break;

    case EncapsulationHeader::KIND_CDR_LE:
      encoding.kind(Encoding::KIND_XCDR1);
      encoding.endianness(ENDIAN_LITTLE);
      wrong_extensibility = expected_extensibility == MUTABLE;
      break;

    case EncapsulationHeader::KIND_PL_CDR_BE:
      encoding.kind(Encoding::KIND_XCDR1);
      encoding.endianness(ENDIAN_BIG);
      wrong_extensibility = expected_extensibility != MUTABLE;
      break;

    case EncapsulationHeader::KIND_PL_CDR_LE:
      encoding.kind(Encoding::KIND_XCDR1);
      encoding.endianness(ENDIAN_LITTLE);
      wrong_extensibility = expected_extensibility != MUTABLE;
      break;

    case EncapsulationHeader::KIND_CDR2_BE:
      encoding.kind(Encoding::KIND_XCDR2);
      encoding.endianness(ENDIAN_BIG);
      wrong_extensibility = expected_extensibility != FINAL;
      break;

    case EncapsulationHeader::KIND_CDR2_LE:
      encoding.kind(Encoding::KIND_XCDR2);
      encoding.endianness(ENDIAN_LITTLE);
      wrong_extensibility = expected_extensibility != FINAL;
      break;

    case EncapsulationHeader::KIND_D_CDR2_BE:
      encoding.kind(Encoding::KIND_XCDR2);
      encoding.endianness(ENDIAN_BIG);
      wrong_extensibility = expected_extensibility != APPENDABLE;
      break;

    case EncapsulationHeader::KIND_D_CDR2_LE:
      encoding.kind(Encoding::KIND_XCDR2);
      encoding.endianness(ENDIAN_LITTLE);
      wrong_extensibility = expected_extensibility != APPENDABLE;
      break;

    case EncapsulationHeader::KIND_PL_CDR2_BE:
      encoding.kind(Encoding::KIND_XCDR2);
      encoding.endianness(ENDIAN_BIG);
      wrong_extensibility = expected_extensibility != MUTABLE;
      break;

    case EncapsulationHeader::KIND_PL_CDR2_LE:
      encoding.kind(Encoding::KIND_XCDR2);
      encoding.endianness(ENDIAN_LITTLE);
      wrong_extensibility = expected_extensibility != MUTABLE;
      break;

    default:
      if (log_level >= LogLevel::Error) {
        ACE_ERROR((LM_ERROR,
                   "(%P|%t) ERROR: to_encoding_i: "
                   "unsupported encoding: %C\n",
                   header.to_string().c_str()));
      }
      return false;
    }

    if (expected_extensibility_ptr && wrong_extensibility) {
      if (log_level >= LogLevel::Error) {
        ACE_ERROR((LM_ERROR,
                   "(%P|%t) ERROR: to_encoding_i: "
                   "expected %C extensibility, but got %C\n",
                   ext_to_string(*expected_extensibility_ptr), header.to_string().c_str()));
      }
      return false;
    }

    return true;
  }
}

bool
to_encoding(Encoding& encoding,
            const EncapsulationHeader& header,
            Extensibility expected_extensibility)
{
  return to_encoding_i(encoding, header, &expected_extensibility);
}

bool
to_any_encoding(Encoding& encoding,
                const EncapsulationHeader& header)
{
  return to_encoding_i(encoding, header, 0);
}

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
