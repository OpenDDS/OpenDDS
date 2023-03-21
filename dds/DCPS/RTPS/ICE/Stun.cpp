/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifdef OPENDDS_SECURITY

#include "Stun.h"

#include "dds/DCPS/security/framework/SecurityRegistry.h"
#include "dds/DCPS/security/framework/SecurityConfig.h"
#include "dds/DCPS/RTPS/RtpsCoreC.h"
#include "dds/DCPS/NetworkResource.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace STUN {

ACE_UINT16 Attribute::length() const
{
  switch (type) {
  case MAPPED_ADDRESS:
#if ACE_HAS_IPV6
    if (mapped_address.get_type() == AF_INET6) {
      return 20;
    }
#endif
    return 8;

  case USERNAME:
    return static_cast<ACE_UINT16>(username.size());

  case MESSAGE_INTEGRITY:
    return 20;

  case ERROR_CODE:
    return static_cast<ACE_UINT16>(4 + error.reason.size());

  case UNKNOWN_ATTRIBUTES:
    return static_cast<ACE_UINT16>(2 * unknown_attributes.size());

  case XOR_MAPPED_ADDRESS:
#if ACE_HAS_IPV6
    if (mapped_address.get_type() == AF_INET6) {
      return 20;
    }
#endif
    return 8;

  case PRIORITY:
    return 4;

  case USE_CANDIDATE:
    return 0;

  case FINGERPRINT:
    return 4;

  case ICE_CONTROLLED:
  case ICE_CONTROLLING:
    return 8;

  case GUID_PREFIX:
    return sizeof(DCPS::GuidPrefix_t);

  default:
    break;
  }

  return unknown_length;
}

Attribute make_mapped_address(const ACE_INET_Addr& addr)
{
  Attribute attribute;
  attribute.type = MAPPED_ADDRESS;
  attribute.mapped_address = addr;
  return attribute;
}

Attribute make_username(const std::string& username)
{
  Attribute attribute;
  attribute.type = USERNAME;
  attribute.username = username;
  return attribute;
}

Attribute make_message_integrity()
{
  Attribute attribute;
  attribute.type = MESSAGE_INTEGRITY;
  return attribute;
}

Attribute make_error_code(ACE_UINT16 code, const std::string& reason)
{
  Attribute attribute;
  attribute.type = ERROR_CODE;
  attribute.error.code = code;
  attribute.error.reason = reason;
  return attribute;
}

Attribute make_unknown_attributes(const std::vector<AttributeType>& unknown_attributes)
{
  Attribute attribute;
  attribute.type = UNKNOWN_ATTRIBUTES;
  attribute.unknown_attributes = unknown_attributes;
  return attribute;
}

Attribute make_xor_mapped_address(const ACE_INET_Addr& addr)
{
  Attribute attribute;
  attribute.type = XOR_MAPPED_ADDRESS;
  attribute.mapped_address = addr;
  return attribute;
}

Attribute make_priority(ACE_UINT32 priority)
{
  Attribute attribute;
  attribute.type = PRIORITY;
  attribute.priority = priority;
  return attribute;
}

Attribute make_use_candidate()
{
  Attribute attribute;
  attribute.type = USE_CANDIDATE;
  return attribute;
}

Attribute make_fingerprint()
{
  Attribute attribute;
  attribute.type = FINGERPRINT;
  return attribute;
}

Attribute make_ice_controlling(ACE_UINT64 ice_tie_breaker)
{
  Attribute attribute;
  attribute.type = ICE_CONTROLLING;
  attribute.ice_tie_breaker = ice_tie_breaker;
  return attribute;
}

Attribute make_ice_controlled(ACE_UINT64 ice_tie_breaker)
{
  Attribute attribute;
  attribute.type = ICE_CONTROLLED;
  attribute.ice_tie_breaker = ice_tie_breaker;
  return attribute;
}

OpenDDS_Rtps_Export
Attribute make_guid_prefix(const DCPS::GuidPrefix_t& guid_prefix)
{
  Attribute attribute;
  attribute.type = GUID_PREFIX;
  std::memcpy(attribute.guid_prefix, guid_prefix, sizeof(guid_prefix));
  return attribute;
}

Attribute make_unknown_attribute(ACE_UINT16 type, ACE_UINT16 length)
{
  Attribute attribute;
  attribute.type = static_cast<AttributeType>(type);
  attribute.unknown_length = length;
  return attribute;
}

bool operator>>(DCPS::Serializer& serializer, AttributeHolder& holder)
{
  ACE_UINT16 attribute_type;
  ACE_UINT16 attribute_length;

  if (!(serializer >> attribute_type)) {
    return false;
  }

  if (!(serializer >> attribute_length)) {
    return false;
  }

  switch (attribute_type) {
  case MAPPED_ADDRESS: {
    ACE_CDR::Char family;
    ACE_UINT16 port;

    if (!serializer.skip(1)) {
      return false;
    }

    if (!(serializer >> family)) {
      return false;
    }

    if (!(serializer >> port)) {
      return false;
    }

    if (family == IPv4) {
      if (attribute_length != 8) {
        return false;
      }

      ACE_UINT32 address;

      if (!(serializer >> address)) {
        return false;
      }

      holder.attribute = make_mapped_address(ACE_INET_Addr(port, address));
    } else if (family == IPv6) {
      if (attribute_length != 20) {
        return false;
      }

      ACE_CDR::Octet address[16];

      if (!(serializer.read_octet_array(address, 16))) {
        return false;
      }
      ACE_INET_Addr addr;
      addr.set_type(AF_INET6);
      addr.set_address(reinterpret_cast<const char*>(address), 16, 0);
      addr.set_port_number(port);

      holder.attribute = make_mapped_address(addr);
    }
  }
  break;

  case USERNAME: {
    if (attribute_length > 512) {
      return false;
    }

    unsigned char buffer[512];

    if (!serializer.read_octet_array(buffer, attribute_length)) {
      return false;
    }

    holder.attribute = make_username(std::string(reinterpret_cast<char*>(buffer), attribute_length));
  }
  break;

  case MESSAGE_INTEGRITY: {
    holder.attribute = make_message_integrity();

    if (!serializer.read_octet_array(holder.attribute.message_integrity, sizeof(holder.attribute.message_integrity))) {
      return false;
    }
  }
  break;

  case ERROR_CODE: {
    ACE_UINT32 x;

    if (!(serializer >> x)) {
      return false;
    }

    ACE_UINT32 class_ = (x & (0x7 << 8)) >> 8;

    if (class_ < 3 || class_ >= 7) {
      return false;
    }

    ACE_UINT32 num = x & 0xFF;

    if (num > 100) {
      return false;
    }

    ACE_UINT16 code = class_ * 100 + num;

    const ACE_CDR::ULong reason_length = attribute_length - 4;

    if (reason_length > 763) {
      return false;
    }

    unsigned char buffer[763];

    if (!serializer.read_octet_array(buffer, reason_length)) {
      return false;
    }

    holder.attribute = make_error_code(code, std::string(reinterpret_cast<char*>(buffer), reason_length));
  }
  break;

  case UNKNOWN_ATTRIBUTES: {
    std::vector<AttributeType> unknown_attributes;

    for (size_t count = attribute_length / 2; count != 0; --count) {
      ACE_UINT16 code;

      if (!(serializer >> code)) {
        return false;
      }

      unknown_attributes.push_back(static_cast<AttributeType>(code));
    }

    holder.attribute = make_unknown_attributes(unknown_attributes);
  }
  break;

  case XOR_MAPPED_ADDRESS: {
    ACE_CDR::Char family;
    ACE_UINT16 port;

    if (!serializer.skip(1)) {
      return false;
    }

    if (!(serializer >> family)) {
      return false;
    }

    if (!(serializer >> port)) {
      return false;
    }

    port ^= MAGIC_COOKIE >> 16;

    if (family == IPv4) {
      if (attribute_length != 8) {
        return false;
      }

      ACE_UINT32 address;

      if (!(serializer >> address)) {
        return false;
      }

      address ^= MAGIC_COOKIE;
      holder.attribute = make_xor_mapped_address(ACE_INET_Addr(port, address));
    } else if (family == IPv6) {
      if (attribute_length != 20) {
        return false;
      }

      ACE_CDR::Octet address[16];

      if (!(serializer.read_octet_array(address, 16))) {
        return false;
      }

      address[0] ^= ((MAGIC_COOKIE & 0xFF000000) >> 24);
      address[1] ^= ((MAGIC_COOKIE & 0x00FF0000) >> 16);
      address[2] ^= ((MAGIC_COOKIE & 0x0000FF00) >> 8);
      address[3] ^= ((MAGIC_COOKIE & 0x000000FF) >> 0);
      address[4] ^= holder.tid.data[0];
      address[5] ^= holder.tid.data[1];
      address[6] ^= holder.tid.data[2];
      address[7] ^= holder.tid.data[3];
      address[8] ^= holder.tid.data[4];
      address[9] ^= holder.tid.data[5];
      address[10] ^= holder.tid.data[6];
      address[11] ^= holder.tid.data[7];
      address[12] ^= holder.tid.data[8];
      address[13] ^= holder.tid.data[9];
      address[14] ^= holder.tid.data[10];
      address[15] ^= holder.tid.data[11];

      ACE_INET_Addr addr;
      addr.set_type(AF_INET6);
      addr.set_address(reinterpret_cast<const char*>(address), 16, 0);
      addr.set_port_number(port);

      holder.attribute = make_xor_mapped_address(addr);
    }
  }
  break;

  case PRIORITY: {
    ACE_UINT32 priority;

    if (!(serializer >> priority)) {
      return false;
    }

    holder.attribute = make_priority(priority);
  }
  break;

  case USE_CANDIDATE:
    holder.attribute = make_use_candidate();
    break;

  case FINGERPRINT: {
    holder.attribute = make_fingerprint();

    if (!(serializer >> holder.attribute.fingerprint)) {
      return false;
    }
  }
  break;

  case ICE_CONTROLLED: {
    ACE_UINT64 ice_tie_breaker;

    if (!(serializer >> ice_tie_breaker)) {
      return false;
    }

    holder.attribute = make_ice_controlled(ice_tie_breaker);
  }
  break;

  case ICE_CONTROLLING: {
    ACE_UINT64 ice_tie_breaker;

    if (!(serializer >> ice_tie_breaker)) {
      return false;
    }

    holder.attribute = make_ice_controlling(ice_tie_breaker);
  }
  break;

  case GUID_PREFIX: {
    DCPS::GuidPrefix_t guid_prefix;
    if (!(serializer.read_octet_array(guid_prefix, sizeof(guid_prefix)))) {
      return false;
    }

    holder.attribute = make_guid_prefix(guid_prefix);
  }
  break;

  default: {
    if (!serializer.skip(attribute_length)) {
      return false;
    }

    holder.attribute = make_unknown_attribute(attribute_type, attribute_length);
  }
  break;
  }

  // All attributes are aligned on 32-bit boundaries.
  if (!serializer.skip((4 - (attribute_length & 0x3)) % 4)) {
    return false;
  }

  return true;
}

bool operator<<(DCPS::Serializer& serializer, ConstAttributeHolder& holder)
{
  ACE_UINT16 attribute_type = holder.attribute.type;
  ACE_UINT16 attribute_length = holder.attribute.length();
  serializer << attribute_type;
  serializer << attribute_length;

  switch (attribute_type) {
  case MAPPED_ADDRESS: {
    serializer << static_cast<ACE_CDR::Char>(0);
    if (holder.attribute.mapped_address.get_type() == AF_INET) {
      serializer << static_cast<ACE_CDR::Char>(IPv4);
      serializer << static_cast<ACE_UINT16>(holder.attribute.mapped_address.get_port_number());
      serializer << holder.attribute.mapped_address.get_ip_address();
    } else {
      serializer << static_cast<ACE_CDR::Char>(IPv6);
      serializer << static_cast<ACE_UINT16>(holder.attribute.mapped_address.get_port_number());
      DDS::OctetArray16 a;
      DCPS::address_to_bytes(a, holder.attribute.mapped_address);
      serializer.write_octet_array(a, 16);
    }
  }
  break;

  case USERNAME: {
    serializer.write_octet_array(reinterpret_cast<const ACE_CDR::Octet*>(holder.attribute.username.c_str()),
                                 static_cast<ACE_CDR::ULong>(holder.attribute.username.size()));
  }
  break;

  case MESSAGE_INTEGRITY: {
    serializer.write_octet_array(holder.attribute.message_integrity, sizeof(holder.attribute.message_integrity));
  }
  break;

  case ERROR_CODE: {
    ACE_UINT8 class_ = holder.attribute.error.code / 100;
    ACE_UINT8 num = holder.attribute.error.code % 100;
    serializer << static_cast<ACE_CDR::Char>(0);
    serializer << static_cast<ACE_CDR::Char>(0);
    serializer << static_cast<ACE_CDR::Char>(class_);
    serializer << static_cast<ACE_CDR::Char>(num);
    serializer.write_octet_array(reinterpret_cast<const ACE_CDR::Octet*>(holder.attribute.error.reason.c_str()),
                                 static_cast<ACE_CDR::ULong>(holder.attribute.error.reason.size()));
  }
  break;

  case UNKNOWN_ATTRIBUTES: {
    for (std::vector<AttributeType>::const_iterator pos = holder.attribute.unknown_attributes.begin(),
         limit = holder.attribute.unknown_attributes.end(); pos != limit; ++pos) {
      serializer << static_cast<ACE_UINT16>(*pos);
    }
  }
  break;

  case XOR_MAPPED_ADDRESS: {
    serializer << static_cast<ACE_CDR::Char>(0);
    if (holder.attribute.mapped_address.get_type() == AF_INET) {
      serializer << static_cast<ACE_CDR::Char>(IPv4);
      serializer << static_cast<ACE_UINT16>(holder.attribute.mapped_address.get_port_number() ^ (MAGIC_COOKIE >> 16));
      serializer << (holder.attribute.mapped_address.get_ip_address() ^ MAGIC_COOKIE);
    } else {
      serializer << static_cast<ACE_CDR::Char>(IPv6);
      serializer << static_cast<ACE_UINT16>(holder.attribute.mapped_address.get_port_number() ^ (MAGIC_COOKIE >> 16));

      DDS::OctetArray16 a;
      DCPS::address_to_bytes(a, holder.attribute.mapped_address);

      a[0] ^= ((MAGIC_COOKIE & 0xFF000000) >> 24);
      a[1] ^= ((MAGIC_COOKIE & 0x00FF0000) >> 16);
      a[2] ^= ((MAGIC_COOKIE & 0x0000FF00) >> 8);
      a[3] ^= ((MAGIC_COOKIE & 0x000000FF) >> 0);
      a[4] ^= holder.tid.data[0];
      a[5] ^= holder.tid.data[1];
      a[6] ^= holder.tid.data[2];
      a[7] ^= holder.tid.data[3];
      a[8] ^= holder.tid.data[4];
      a[9] ^= holder.tid.data[5];
      a[10] ^= holder.tid.data[6];
      a[11] ^= holder.tid.data[7];
      a[12] ^= holder.tid.data[8];
      a[13] ^= holder.tid.data[9];
      a[14] ^= holder.tid.data[10];
      a[15] ^= holder.tid.data[11];

      serializer.write_octet_array(a, 16);
    }
  }
  break;

  case PRIORITY: {
    serializer << holder.attribute.priority;
  }
  break;

  case USE_CANDIDATE:
    break;

  case FINGERPRINT: {
    serializer << holder.attribute.fingerprint;
  }
  break;

  case ICE_CONTROLLED:
  case ICE_CONTROLLING: {
    serializer << holder.attribute.ice_tie_breaker;
  }
  break;

  case GUID_PREFIX: {
    serializer.write_octet_array(holder.attribute.guid_prefix, sizeof(holder.attribute.guid_prefix));
  }
  break;

  default:
    // Don't serialize attributes that are not understood.
    return false;
    break;
  }

  // Align to 32 bits.
  while (attribute_length % 4 != 0) {
    serializer << static_cast<ACE_CDR::Char>(0);
    attribute_length += 1;
  }

  return true;
}

bool TransactionId::operator<(const TransactionId& other) const
{
  return (memcmp(this->data, other.data, sizeof(data)) < 0);
}

bool TransactionId::operator==(const TransactionId& other) const
{
  return (memcmp(this->data, other.data, sizeof(data)) == 0);
}

bool TransactionId::operator!=(const TransactionId& other) const
{
  return (memcmp(this->data, other.data, sizeof(data)) != 0);
}

void Message::generate_transaction_id()
{
  TheSecurityRegistry->builtin_config()->get_utility()->generate_random_bytes(transaction_id.data, sizeof(transaction_id.data));
}

void Message::clear_transaction_id()
{
  ACE_OS::memset(transaction_id.data, 0, sizeof(transaction_id.data));
}

std::vector<AttributeType> Message::unknown_comprehension_required_attributes() const
{
  std::vector<AttributeType> retval;

  for (AttributesType::const_iterator pos = attributes_.begin(), limit = attributes_.end(); pos != limit; ++pos) {
    const AttributesType::value_type& attribute = *pos;
    switch (attribute.type) {
    case MAPPED_ADDRESS:
    case USERNAME:
    case MESSAGE_INTEGRITY:
    case ERROR_CODE:
    case UNKNOWN_ATTRIBUTES:
    case XOR_MAPPED_ADDRESS:
    case PRIORITY:
    case USE_CANDIDATE:
      break;

    default:
      if (attribute.type < 0x8000) {
        retval.push_back(attribute.type);
      }
    }
  }

  return retval;
}

bool Message::get_mapped_address(ACE_INET_Addr& address) const
{
  for (STUN::Message::const_iterator pos = begin(), limit = end(); pos != limit; ++pos) {
    if (pos->type == STUN::XOR_MAPPED_ADDRESS) {
      address = pos->mapped_address;
      return true;
    }
  }

  for (STUN::Message::const_iterator pos = begin(), limit = end(); pos != limit; ++pos) {
    if (pos->type == STUN::MAPPED_ADDRESS) {
      address = pos->mapped_address;
      return true;
    }
  }

  return false;
}

bool Message::get_priority(ACE_UINT32& priority) const
{
  bool flag = false;

  for (STUN::Message::const_iterator pos = begin(), limit = end(); pos != limit; ++pos) {
    if (pos->type == STUN::PRIORITY) {
      flag = true;
      priority = pos->priority;
      // Use the last.
    }
  }

  return flag;
}

bool Message::get_username(std::string& username) const
{
  bool flag = false;

  for (STUN::Message::const_iterator pos = begin(), limit = end(); pos != limit; ++pos) {
    if (pos->type == STUN::USERNAME) {
      flag = true;
      username = pos->username;
      // Use the last.
    }
  }

  return flag;
}

bool Message::has_message_integrity() const
{
  for (STUN::Message::const_iterator pos = begin(), limit = end(); pos != limit; ++pos) {
    if (pos->type == STUN::MESSAGE_INTEGRITY) {
      return true;
    }
  }

  return false;
}

bool Message::verify_message_integrity(const std::string& password) const
{
  bool verified = false;

  for (STUN::Message::const_iterator pos = begin(), limit = end(); pos != limit; ++pos) {
    if (pos->type == STUN::MESSAGE_INTEGRITY) {
      unsigned char computed_message_integrity[20];
      compute_message_integrity(password, computed_message_integrity);
      verified = memcmp(computed_message_integrity, pos->message_integrity, 20) == 0;
      // Use the last.
    }
  }

  return verified;
}

void Message::compute_message_integrity(const std::string& password, unsigned char message_integrity[20]) const
{
  ACE_Message_Block* block = this->block->duplicate();
  block->rd_ptr(block->base());
  DCPS::Serializer serializer(block, encoding);

  // Write the length and resize for hashing.
  block->wr_ptr(block->base() + 2);
  ACE_UINT16 message_length = length_for_message_integrity();
  serializer << message_length;
  block->wr_ptr(block->base() + HEADER_SIZE + length_for_message_integrity() - 24);

  // Compute the SHA1.
  TheSecurityRegistry->builtin_config()->get_utility()->hmac(message_integrity, block->rd_ptr(), block->length(), password);

  // Write the correct length.
  block->wr_ptr(block->base() + 2);
  message_length = length();
  serializer << message_length;

  block->release();
}

bool Message::has_error_code() const
{
  for (STUN::Message::const_iterator pos = begin(), limit = end(); pos != limit; ++pos) {
    if (pos->type == STUN::ERROR_CODE) {
      return true;
    }
  }

  return false;
}

ACE_UINT16 Message::get_error_code() const
{
  for (STUN::Message::const_iterator pos = begin(), limit = end(); pos != limit; ++pos) {
    if (pos->type == STUN::ERROR_CODE) {
      return pos->error.code;
    }
  }

  return 0;
}

std::string Message::get_error_reason() const
{
  for (STUN::Message::const_iterator pos = begin(), limit = end(); pos != limit; ++pos) {
    if (pos->type == STUN::ERROR_CODE) {
      return pos->error.reason;
    }
  }

  return std::string();
}

bool Message::has_unknown_attributes() const
{
  for (STUN::Message::const_iterator pos = begin(), limit = end(); pos != limit; ++pos) {
    if (pos->type == STUN::UNKNOWN_ATTRIBUTES) {
      return true;
    }
  }

  return false;
}

std::vector<AttributeType> Message::get_unknown_attributes() const
{
  for (STUN::Message::const_iterator pos = begin(), limit = end(); pos != limit; ++pos) {
    if (pos->type == STUN::UNKNOWN_ATTRIBUTES) {
      return pos->unknown_attributes;
    }
  }

  return std::vector<AttributeType>();
}

bool Message::has_fingerprint() const
{
  for (STUN::Message::const_iterator pos = begin(), limit = end(); pos != limit; ++pos) {
    if (pos->type == STUN::FINGERPRINT) {
      return true;
    }
  }

  return false;
}

ACE_UINT32 Message::compute_fingerprint() const
{
  ACE_Message_Block* block = this->block->duplicate();
  block->rd_ptr(block->base());
  DCPS::Serializer serializer(block, encoding);

  // Resize for hashing.
  block->wr_ptr(block->base() + HEADER_SIZE + length() - 8);

  // Compute the CRC-32
  ACE_UINT32 crc = ACE::crc32(block->rd_ptr(), block->length());

  block->release();

  return crc ^ 0x5354554E;
}

bool Message::has_ice_controlled() const
{
  for (STUN::Message::const_iterator pos = begin(), limit = end(); pos != limit; ++pos) {
    if (pos->type == STUN::ICE_CONTROLLED) {
      return true;
    }
  }

  return false;
}

bool Message::has_ice_controlling() const
{
  for (STUN::Message::const_iterator pos = begin(), limit = end(); pos != limit; ++pos) {
    if (pos->type == STUN::ICE_CONTROLLING) {
      return true;
    }
  }

  return false;
}

bool Message::has_use_candidate() const
{
  for (STUN::Message::const_iterator pos = begin(), limit = end(); pos != limit; ++pos) {
    if (pos->type == STUN::USE_CANDIDATE) {
      return true;
    }
  }

  return false;
}

bool Message::get_guid_prefix(DCPS::GuidPrefix_t& guid_prefix) const
{
  for (STUN::Message::const_iterator pos = begin(), limit = end(); pos != limit; ++pos) {
    if (pos->type == STUN::GUID_PREFIX) {
      std::memcpy(guid_prefix, pos->guid_prefix, sizeof(guid_prefix));
      return true;
    }
  }

  return false;
}

bool operator>>(DCPS::Serializer& serializer, Message& message)
{
  ACE_UINT16 message_type;
  ACE_UINT16 message_length;
  ACE_UINT32 magic_cookie;

  if (!(serializer >> message_type)) {
    return false;
  }

  if (!(serializer >> message_length)) {
    return false;
  }

  if ((message_type & 0xC000) != 0) {
    return false;
  }

  if (message_length % 4 != 0) {
    return false;
  }

  message.class_ = static_cast<Class>(((message_type & (1 << 8)) >> 7) | ((message_type & (1 << 4)) >> 4));
  message.method = static_cast<Method>(((message_type & 0x3E00) >> 2) | ((message_type & 0xE0) >> 1) | (message_type & 0xF));

  if (!(serializer >> magic_cookie)) {
    return false;
  }

  if (magic_cookie != MAGIC_COOKIE) {
    return false;
  }

  if (!serializer.read_octet_array(message.transaction_id.data, 12)) {
    return false;
  }

  // At this point there should be message.length bytes remaining.
  if (serializer.length() != message_length) {
    return false;
  }

  bool have_integrity = false;
  bool have_fingerprint = false;

  while (serializer.length() != 0) {
    Attribute attribute;
    AttributeHolder holder(attribute, message.transaction_id);

    if (!(serializer >> holder)) {
      return false;
    }

    message.append_attribute(attribute);

    if ((have_integrity && attribute.type != FINGERPRINT) || have_fingerprint) {
      return false;
    }

    if (attribute.type == FINGERPRINT && attribute.fingerprint != message.compute_fingerprint()) {
      return false;
    }

    if (message.length() > message_length) {
      return false;
    }

    have_integrity = have_integrity || attribute.type == MESSAGE_INTEGRITY;
    have_fingerprint = have_fingerprint || attribute.type == FINGERPRINT;
  }

  return true;
}

bool operator<<(DCPS::Serializer& serializer, const Message& message)
{
  ACE_UINT16 message_class = message.class_;
  ACE_UINT16 message_method = message.method;
  ACE_UINT16 message_type =
    ((message_method & 0xF80) << 2) |
    ((message_class & 0x2) << 7) |
    ((message_method & 0x0070) << 1) |
    ((message_class & 0x1) << 4) |
    (message_method & 0x000F);
  serializer << message_type;

  ACE_UINT16 message_length = message.length();
  serializer << message_length;
  serializer << MAGIC_COOKIE;
  serializer.write_octet_array(message.transaction_id.data, sizeof(message.transaction_id.data));

  for (Message::const_iterator pos = message.begin(), limit = message.end();
       pos != limit; ++pos) {
    const Attribute& attribute = *pos;

    if (attribute.type == MESSAGE_INTEGRITY) {
      // Compute the hash.
      message.compute_message_integrity(message.password, const_cast<Attribute&>(attribute).message_integrity);
    }

    else if (attribute.type == FINGERPRINT) {
      // Compute the hash.
      const_cast<Attribute&>(attribute).fingerprint = message.compute_fingerprint();
    }

    ConstAttributeHolder holder(attribute, message.transaction_id);
    serializer << holder;
  }

  return true;
}

} // namespace STUN
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
#endif /* OPENDDS_SECURITY */
