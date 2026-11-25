#include "QOS_Common.h"
#include <charconv>
#include <limits>
#include <stdexcept>
#include <vector>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace {
template <typename Integer>
Integer parse_integer (std::string_view text)
{
  Integer value {};
  const std::from_chars_result result =
    std::from_chars (text.data (), text.data () + text.size (), value);
  if (result.ec != std::errc () || result.ptr != text.data () + text.size ())
    throw std::invalid_argument ("Invalid numeric QoS value");
  return value;
}
}

void
QosCommon::get_durability_kind(const ::dds::durabilityKind& kind,
                               ::DDS::DurabilityQosPolicyKind& dds_kind)
{
  switch (kind.integral())
    {
    case ::dds::durabilityKind::VOLATILE_DURABILITY_QOS:
      dds_kind = ::DDS::VOLATILE_DURABILITY_QOS;
      break;
    case ::dds::durabilityKind::TRANSIENT_LOCAL_DURABILITY_QOS:
      dds_kind = ::DDS::TRANSIENT_LOCAL_DURABILITY_QOS;
      break;
    case ::dds::durabilityKind::TRANSIENT_DURABILITY_QOS:
      dds_kind = ::DDS::TRANSIENT_DURABILITY_QOS;
      break;
    case ::dds::durabilityKind::PERSISTENT_DURABILITY_QOS:
      dds_kind = ::DDS::PERSISTENT_DURABILITY_QOS;
      break;
    default:
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: QosCommon::get_durability_kind - ")
        ACE_TEXT("Unknown durability kind found <%d>; returning VOLATILE_DURABILITY_QOS\n"),
        kind.integral()));
      dds_kind = ::DDS::VOLATILE_DURABILITY_QOS;
      break;
    }
}

void
QosCommon::get_history_kind(const ::dds::historyKind& kind,
                            ::DDS::HistoryQosPolicyKind& dds_kind)
{
  switch (kind.integral())
    {
    case ::dds::historyKind::KEEP_ALL_HISTORY_QOS:
      dds_kind = ::DDS::KEEP_ALL_HISTORY_QOS;
      break;
    case ::dds::historyKind::KEEP_LAST_HISTORY_QOS:
      dds_kind = ::DDS::KEEP_LAST_HISTORY_QOS;
      break;
    default:
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: QosCommon::get_history_kind - ")
        ACE_TEXT("Unknown history kind found <%d>; returning KEEP_ALL_HISTORY_QOS\n"),
        kind.integral()));
      dds_kind = ::DDS::KEEP_ALL_HISTORY_QOS;
      break;
    }
}

void
QosCommon::get_duration(::DDS::Duration_t& duration, std::string_view sec, std::string_view nsec)
{
  if (!sec.empty()) {
    if (sec == "DURATION_INFINITY" || sec == "DURATION_INFINITE_SEC") {
      duration.sec = ::DDS::DURATION_INFINITE_SEC;
    } else {
      duration.sec = parse_integer<::CORBA::Long> (sec);
    }
  }

  if (!nsec.empty()) {
    if (nsec == "DURATION_INFINITY" || nsec == "DURATION_INFINITE_NSEC") {
      duration.nanosec =  ::DDS::DURATION_INFINITE_NSEC;
    } else {
      const std::uint64_t value = parse_integer<std::uint64_t> (nsec);
      if (value >= 1000000000) {
        throw std::out_of_range ("QoS duration nanoseconds must be less than one second");
      }
      duration.nanosec = static_cast<::DDS::UInt32> (value);
    }
  }
}

void
QosCommon::get_binary_data(::DDS::OctetSeq& data, std::string_view value)
{
  const auto whitespace = [] (char c) {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
  };
  if (value.find_first_not_of (" \t\r\n") == std::string_view::npos) {
    data.length (0);
    return;
  }

  std::vector<::CORBA::Octet> bytes;
  std::size_t pos = 0;
  while (pos < value.size()) {
    while (pos < value.size() && whitespace (value[pos])) {
      ++pos;
    }
    const std::size_t start = pos;
    while (pos < value.size() && value[pos] != ',') {
      ++pos;
    }
    std::size_t end = pos;
    while (end > start && whitespace (value[end - 1])) {
      --end;
    }
    if (start == end) {
      throw std::invalid_argument ("Invalid empty byte in binary QoS value");
    }
    std::string_view token = value.substr (start, end - start);
    int base = 10;
    if (token.size() > 2 && token[0] == '0' && (token[1] == 'x' || token[1] == 'X')) {
      token.remove_prefix (2);
      base = 16;
    }
    unsigned int byte {};
    const std::from_chars_result parsed = std::from_chars (token.data (), token.data () + token.size (), byte, base);
    if (parsed.ec != std::errc () || parsed.ptr != token.data () + token.size () || byte > 255) {
      throw std::invalid_argument ("Invalid byte in binary QoS value");
    }
    bytes.push_back (static_cast<::CORBA::Octet> (byte));
    if (pos < value.size()) {
      ++pos;
      if (pos == value.size()) {
        throw std::invalid_argument ("Invalid trailing comma in binary QoS value");
      }
    }
  }

  if (bytes.size() > std::numeric_limits<::CORBA::ULong>::max()) {
    throw std::length_error ("Binary QoS value exceeds DDS sequence capacity");
  }
  const ::CORBA::ULong length = static_cast<::CORBA::ULong> (bytes.size());
  data.length (length);
  for (::CORBA::ULong i = 0; i < length; ++i) {
    data[i] = bytes[i];
  }
}

::CORBA::Long
QosCommon::get_corba_long(std::int64_t value)
{
  if (value < 0 || value > std::numeric_limits<::CORBA::Long>::max()) {
    throw std::out_of_range ("QoS integer is outside the CORBA::Long range");
  }
  return static_cast<::CORBA::Long> (value);
}

::CORBA::Long
QosCommon::get_qos_long(std::string_view value)
{
  if (value.empty()) {
    return 0;
  } else if (value == "LENGTH_UNLIMITED") {
    return -1;
  } else {
    return parse_integer<::CORBA::Long> (value);
  }
}

void
QosCommon::get_liveliness_kind(const ::dds::livelinessKind& kind,
                               ::DDS::LivelinessQosPolicyKind& dds_kind)
{
  switch (kind.integral())
    {
    case ::dds::livelinessKind::AUTOMATIC_LIVELINESS_QOS:
      dds_kind = ::DDS::AUTOMATIC_LIVELINESS_QOS;
      break;
    case ::dds::livelinessKind::MANUAL_BY_PARTICIPANT_LIVELINESS_QOS:
      dds_kind = ::DDS::MANUAL_BY_PARTICIPANT_LIVELINESS_QOS;
      break;
    case ::dds::livelinessKind::MANUAL_BY_TOPIC_LIVELINESS_QOS:
      dds_kind = ::DDS::MANUAL_BY_TOPIC_LIVELINESS_QOS;
      break;
    default:
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: QosCommon::get_liveliness_kind - ")
        ACE_TEXT("Unknown liveliness kind found <%d>; returning AUTOMATIC_LIVELINESS_QOS\n"),
        kind.integral()));
      dds_kind = ::DDS::AUTOMATIC_LIVELINESS_QOS;
      break;
    }
}

void
QosCommon::get_realiability_kind(const ::dds::reliabilityKind& kind,
                                 ::DDS::ReliabilityQosPolicyKind& dds_kind)
{
  switch (kind.integral())
    {
    case ::dds::reliabilityKind::BEST_EFFORT_RELIABILITY_QOS:
      dds_kind = ::DDS::BEST_EFFORT_RELIABILITY_QOS;
      break;
    case ::dds::reliabilityKind::RELIABLE_RELIABILITY_QOS:
      dds_kind = ::DDS::RELIABLE_RELIABILITY_QOS;
      break;
    default:
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: QosCommon::get_liveliness_kind - ")
        ACE_TEXT("Unknown reliability kind found <%d>; returning BEST_EFFORT_RELIABILITY_QOS\n"),
        kind.integral()));
      dds_kind = ::DDS::BEST_EFFORT_RELIABILITY_QOS;
      break;
    }
}

void
QosCommon::get_destination_order_kind(const ::dds::destinationOrderKind& kind,
                                      ::DDS::DestinationOrderQosPolicyKind& dds_kind)
{
  switch (kind.integral())
    {
    case ::dds::destinationOrderKind::BY_RECEPTION_TIMESTAMP_DESTINATIONORDER_QOS:
      dds_kind = ::DDS::BY_RECEPTION_TIMESTAMP_DESTINATIONORDER_QOS;
      break;
    case ::dds::destinationOrderKind::BY_SOURCE_TIMESTAMP_DESTINATIONORDER_QOS:
      dds_kind = ::DDS::BY_SOURCE_TIMESTAMP_DESTINATIONORDER_QOS;
      break;
    default:
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: QosCommon::get_destination_order_kind - ")
        ACE_TEXT("Unknown destination order kind found <%d>; returning BY_RECEPTION_TIMESTAMP_DESTINATIONORDER_QOS\n"),
        kind.integral()));
      dds_kind = ::DDS::BY_RECEPTION_TIMESTAMP_DESTINATIONORDER_QOS;
      break;
    }
}

void
QosCommon::get_ownership_kind(const ::dds::ownershipKind& kind,
                              ::DDS::OwnershipQosPolicyKind& dds_kind)
{
  switch (kind.integral())
    {
    case ::dds::ownershipKind::SHARED_OWNERSHIP_QOS:
      dds_kind = ::DDS::SHARED_OWNERSHIP_QOS;
      break;
    case ::dds::ownershipKind::EXCLUSIVE_OWNERSHIP_QOS:
      dds_kind = ::DDS::EXCLUSIVE_OWNERSHIP_QOS;
      break;
    default:
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: QosCommon::get_ownership_kind - ")
        ACE_TEXT("Unknown ownership kind found <%d>; returning SHARED_OWNERSHIP_QOS\n"),
        kind.integral()));
      dds_kind = ::DDS::SHARED_OWNERSHIP_QOS;
      break;
    }
}

void
QosCommon::get_type_consistency_kind(const ::dds::typeConsistencyKind& kind,
                                    ::DDS::TypeConsistencyEnforcementQosPolicyKind_t& dds_kind)
{
  switch (kind.integral())
    {
    case ::dds::typeConsistencyKind::DISALLOW_TYPE_COERCION:
      dds_kind = ::DDS::DISALLOW_TYPE_COERCION;
      break;
    case ::dds::typeConsistencyKind::ALLOW_TYPE_COERCION:
      dds_kind = ::DDS::ALLOW_TYPE_COERCION;
      break;
    default:
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: QosCommon::get_type_consistency_kind - ")
        ACE_TEXT("Unknown type consistency kind found <%d>; returning DISALLOW_TYPE_COERCION\n"),
        kind.integral()));
      dds_kind = ::DDS::DISALLOW_TYPE_COERCION;
      break;
    }
}

void
QosCommon::get_data_presentation_id_kind(const ::dds::dataRepresentationIdKind& kind,
                                         ::DDS::DataRepresentationId_t& dds_kind)
{
  switch (kind.integral())
    {
    case ::dds::dataRepresentationIdKind::XCDR_DATA_REPRESENTATION:
      dds_kind = ::DDS::XCDR_DATA_REPRESENTATION;
      break;
    case ::dds::dataRepresentationIdKind::XML_DATA_REPRESENTATION:
      dds_kind = ::DDS::XML_DATA_REPRESENTATION;
      break;
    case ::dds::dataRepresentationIdKind::XCDR2_DATA_REPRESENTATION:
      dds_kind = ::DDS::XCDR2_DATA_REPRESENTATION;
      break;
    case ::dds::dataRepresentationIdKind::UNALIGNED_CDR_DATA_REPRESENTATION:
      dds_kind = ::OpenDDS::DCPS::UNALIGNED_CDR_DATA_REPRESENTATION;
      break;
    default:
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: QosCommon::get_data_presentation_id_kind - ")
        ACE_TEXT("Unknown data presentation kind found <%d>; returning XCDR2_DATA_REPRESENTATION\n"),
        kind.integral()));
      dds_kind = ::DDS::XCDR2_DATA_REPRESENTATION;
      break;
    }
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
