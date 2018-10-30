/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "Stun.h"

#include "dds/DCPS/security/framework/SecurityRegistry.h"
#include "dds/DCPS/security/framework/SecurityConfig.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace STUN {

#if OPENDDS_SECURITY

uint16_t Attribute::length() const
{
  switch (type) {
  case MAPPED_ADDRESS:
    // TODO(jrw972):  Handle IPv6.
    return 8;

  case USERNAME:
    return username.size();

  case MESSAGE_INTEGRITY:
    return 20;

  case ERROR_CODE:
    return 4 + error.reason.size();

  case UNKNOWN_ATTRIBUTES:
    return 2 * unknown_attributes.size();

  case XOR_MAPPED_ADDRESS:
    // TODO(jrw972):  Handle IPv6.
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
  }

  return unknown_length;
}

Attribute make_mapped_address(const ACE_INET_Addr& addr)
{
  Attribute attribute;
  attribute.type = MAPPED_ADDRESS;
  // TODO(jrw972):  Handle IPv6.
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

Attribute make_error_code(uint16_t code, const std::string& reason)
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
  // TODO(jrw972):  Handle IPv6.
  attribute.mapped_address = addr;
  return attribute;
}

Attribute make_priority(uint32_t priority)
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

Attribute make_unknown_attribute(uint16_t type, uint16_t length)
{
  Attribute attribute;
  attribute.type = static_cast<AttributeType>(type);
  attribute.unknown_length = length;
  return attribute;
}

bool operator>>(DCPS::Serializer& serializer, Attribute& attribute)
{
  uint16_t attribute_type;
  uint16_t attribute_length;

  if (!(serializer >> attribute_type)) {
    return false;
  }

  if (!(serializer >> attribute_length)) {
    return false;
  }

  switch (attribute_type) {
  case MAPPED_ADDRESS: {
    ACE_CDR::Char family;
    uint16_t port;

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

      uint32_t address;

      if (!(serializer >> address)) {
        return false;
      }

      attribute = make_mapped_address(ACE_INET_Addr(port, address));
    }

    else if (family == IPv6) {
      // TODO(jrw972):  Handle IPv6.
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

    attribute = make_username(std::string(reinterpret_cast<char*>(buffer), attribute_length));
  }
  break;

  case MESSAGE_INTEGRITY: {
    attribute = make_message_integrity();

    if (!serializer.read_octet_array(attribute.message_integrity, sizeof(attribute.message_integrity))) {
      return false;
    }
  }
  break;

  case ERROR_CODE: {
    uint32_t x;

    if (!(serializer >> x)) {
      return false;
    }

    uint32_t class_ = (x & (0x7 << 8)) >> 8;

    if (class_ < 3 || class_ >= 7) {
      return false;
    }

    uint32_t num = x & 0xFF;

    if (num > 100) {
      return false;
    }

    uint16_t code = class_ * 100 + num;

    size_t reason_length = attribute_length - 4;

    if (reason_length > 763) {
      return false;
    }

    unsigned char buffer[763];

    if (!serializer.read_octet_array(buffer, reason_length)) {
      return false;
    }

    attribute = make_error_code(code, std::string(reinterpret_cast<char*>(buffer), reason_length));
  }
  break;

  case UNKNOWN_ATTRIBUTES: {
    std::vector<AttributeType> unknown_attributes;

    for (size_t count = attribute_length / 2; count != 0; --count) {
      uint16_t code;

      if (!(serializer >> code)) {
        return false;
      }

      unknown_attributes.push_back(static_cast<AttributeType>(code));
    }

    attribute = make_unknown_attributes(unknown_attributes);
  }
  break;

  case XOR_MAPPED_ADDRESS: {
    ACE_CDR::Char family;
    uint16_t port;

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

      uint32_t address;

      if (!(serializer >> address)) {
        return false;
      }

      address ^= MAGIC_COOKIE;
      attribute = make_xor_mapped_address(ACE_INET_Addr(port, address));
    }

    else if (family == IPv6) {
      // TODO(jrw972):  Handle IPv6.
    }
  }
  break;

  case PRIORITY: {
    uint32_t priority;

    if (!(serializer >> priority)) {
      return false;
    }

    attribute = make_priority(priority);
  }
  break;

  case USE_CANDIDATE:
    attribute = make_use_candidate();
    break;

  case FINGERPRINT: {
    attribute = make_fingerprint();

    if (!(serializer >> attribute.fingerprint)) {
      return false;
    }
  }
  break;

  case ICE_CONTROLLED: {
    ACE_UINT64 ice_tie_breaker;

    if (!(serializer >> ice_tie_breaker)) {
      return false;
    }

    attribute = make_ice_controlled(ice_tie_breaker);
  }
  break;

  case ICE_CONTROLLING: {
    ACE_UINT64 ice_tie_breaker;

    if (!(serializer >> ice_tie_breaker)) {
      return false;
    }

    attribute = make_ice_controlling(ice_tie_breaker);
  }
  break;

  default: {
    if (!serializer.skip(attribute_length)) {
      return false;
    }

    attribute = make_unknown_attribute(attribute_type, attribute_length);
  }
  break;
  }

  // All attributes are aligned on 32-bit boundaries.
  if (!serializer.skip((4 - (attribute_length & 0x3)) % 4)) {
    return false;
  }

  return true;
}

bool operator<<(DCPS::Serializer& serializer, const Attribute& attribute)
{
  uint16_t attribute_type = attribute.type;
  uint16_t attribute_length = attribute.length();
  serializer << attribute_type;
  serializer << attribute_length;

  switch (attribute_type) {
  case MAPPED_ADDRESS: {
    serializer << static_cast<ACE_CDR::Char>(0);
    serializer << static_cast<ACE_CDR::Char>(IPv4);
    serializer << static_cast<uint16_t>(attribute.mapped_address.get_port_number());
    serializer << attribute.mapped_address.get_ip_address();
    // TODO(jrw972):  Handle IPv6.
  }
  break;

  case USERNAME: {
    serializer.write_octet_array(reinterpret_cast<const ACE_CDR::Octet*>(attribute.username.c_str()), attribute.username.size());
  }
  break;

  case MESSAGE_INTEGRITY: {
    serializer.write_octet_array(attribute.message_integrity, sizeof(attribute.message_integrity));
  }
  break;

  case ERROR_CODE: {
    uint8_t class_ = attribute.error.code / 100;
    uint8_t num = attribute.error.code % 100;
    serializer << static_cast<ACE_CDR::Char>(0);
    serializer << static_cast<ACE_CDR::Char>(0);
    serializer << static_cast<ACE_CDR::Char>(class_);
    serializer << static_cast<ACE_CDR::Char>(num);
    serializer.write_octet_array(reinterpret_cast<const ACE_CDR::Octet*>(attribute.error.reason.c_str()), attribute.error.reason.size());
  }
  break;

  case UNKNOWN_ATTRIBUTES: {
    for (std::vector<AttributeType>::const_iterator pos = attribute.unknown_attributes.begin(),
         limit = attribute.unknown_attributes.end(); pos != limit; ++pos) {
      serializer << static_cast<uint16_t>(*pos);
    }
  }
  break;

  case XOR_MAPPED_ADDRESS: {
    serializer << static_cast<ACE_CDR::Char>(0);
    serializer << static_cast<ACE_CDR::Char>(IPv4);
    serializer << static_cast<uint16_t>(attribute.mapped_address.get_port_number() ^ (MAGIC_COOKIE >> 16));
    serializer << (attribute.mapped_address.get_ip_address() ^ MAGIC_COOKIE);
    // TODO(jrw972):  Handle IPv6.
  }
  break;

  case PRIORITY: {
    serializer << attribute.priority;
  }
  break;

  case USE_CANDIDATE:
    break;

  case FINGERPRINT: {
    serializer << attribute.fingerprint;
  }
  break;

  case ICE_CONTROLLED:
  case ICE_CONTROLLING: {
    serializer << attribute.ice_tie_breaker;
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
  TheSecurityRegistry->default_config()->get_utility()->generate_random_bytes(transaction_id.data, sizeof(transaction_id.data));
}

std::vector<AttributeType> Message::unknown_comprehension_required_attributes() const
{
  std::vector<AttributeType> retval;

  for (const AttributesType::value_type& attribute : attributes_) {
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

bool Message::get_priority(uint32_t& priority) const
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
  DCPS::Serializer serializer(block, true);

  // Write the length and resize for hashing.
  block->wr_ptr(block->base() + 2);
  uint16_t message_length = length_for_message_integrity();
  serializer << message_length;
  block->wr_ptr(block->base() + 20 + length_for_message_integrity() - 24);

  // Compute the SHA1.
  TheSecurityRegistry->default_config()->get_utility()->hmac(message_integrity, block->rd_ptr(), block->length(), password);

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

uint16_t Message::get_error_code() const
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

uint32_t Message::compute_fingerprint() const
{
  ACE_Message_Block* block = this->block->duplicate();
  block->rd_ptr(block->base());
  DCPS::Serializer serializer(block, true);

  // Resize for hashing.
  block->wr_ptr(block->base() + 20 + length() - 8);

  // Compute the CRC-32
  uint32_t crc = ACE::crc32(block->rd_ptr(), block->length());

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

bool operator>>(DCPS::Serializer& serializer, Message& message)
{
  uint16_t message_type;
  uint16_t message_length;
  uint32_t magic_cookie;

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

  while (serializer.length() != 0) {
    bool have_integrity = false;
    bool have_fingerprint = false;

    Attribute attribute;

    if (!(serializer >> attribute)) {
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
  uint16_t message_class = message.class_;
  uint16_t message_method = message.method;
  uint16_t message_type =
    ((message_method & 0xF80) << 2) |
    ((message_class & 0x2) << 7) |
    ((message_method & 0x0070) << 1) |
    ((message_class & 0x1) << 4) |
    (message_method & 0x000F);
  serializer << message_type;

  uint16_t message_length = message.length();
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

    serializer << attribute;
  }

  return true;
}

#endif /* OPENDDS_SECURITY */

} // namespace STUN
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
