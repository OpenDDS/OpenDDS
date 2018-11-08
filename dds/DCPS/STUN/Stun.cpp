/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "Stun.h"

#include <iostream>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace STUN {

  uint16_t Attribute::length() const {
    switch (type) {
    case MAPPED_ADDRESS:
      // TODO:  Handle IPv6.
      return 8;
    case XOR_MAPPED_ADDRESS:
      // TODO:  Handle IPv6.
      return 8;
    }

    return unknown_length;
  }

  Attribute make_mapped_address(const ACE_INET_Addr& addr) {
    Attribute attribute;
    attribute.type = MAPPED_ADDRESS;
    // TODO:  Handle IPv6
    attribute.mapped_address = addr;
    return attribute;
  }

  Attribute make_xor_mapped_address(const ACE_INET_Addr& addr) {
    Attribute attribute;
    attribute.type = XOR_MAPPED_ADDRESS;
    // TODO:  Handle IPv6
    attribute.mapped_address = addr;
    return attribute;
  }

  Attribute make_unknown_attribute(uint16_t type, uint16_t length) {
    Attribute attribute;
    attribute.type = static_cast<AttributeType>(type);
    attribute.unknown_length = length;
    return attribute;
  }

  bool operator>>(DCPS::Serializer& serializer, Attribute& attribute) {
    uint16_t attribute_type;
    uint16_t attribute_length;

    if (!(serializer >> attribute_type)) {
      return false;
    }

    if (!(serializer >> attribute_length)) {
      return false;
    }

    switch (attribute_type) {
    case MAPPED_ADDRESS:
      {
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
          struct sockaddr_in addr;
          addr.sin_family = AF_INET;
          addr.sin_port = htons(port);
          addr.sin_addr.s_addr = htonl(address);
          attribute = make_mapped_address(ACE_INET_Addr(&addr, sizeof(addr)));
        } else if (family == IPv6) {
          // TODO:  Implement.
          std::cerr << "TODO: Implement MAPPED_ADDRESS IPv6" << std::endl;
        }
      }
      break;
    case XOR_MAPPED_ADDRESS:
      {
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
          struct sockaddr_in addr;
          addr.sin_family = AF_INET;
          addr.sin_port = htons(port);
          addr.sin_addr.s_addr = htonl(address);
          attribute = make_xor_mapped_address(ACE_INET_Addr(&addr, sizeof(addr)));
        } else if (family == IPv6) {
          // TODO:  Implement.
          std::cerr << "TODO: Implement XOR_MAPPED_ADDRESS IPv6" << std::endl;
        }
      }
      break;
    default:
      {
        if (!serializer.skip(attribute_length)) {
          return false;
        }
        attribute = make_unknown_attribute(attribute_type, attribute_length);
      }
      break;
    }

    // All attributes are aligned on 32-bit boundaries.
    if (!serializer.skip(attribute_length % 4)) {
      return false;
    }

    return true;
  }

  bool operator<<(DCPS::Serializer& serializer, const Attribute& attribute) {
    uint16_t attribute_type = attribute.type;
    uint16_t attribute_length = attribute.length();
    serializer << attribute_type;
    serializer << attribute_length;

    switch (attribute_type) {
    case MAPPED_ADDRESS:
      {
        serializer << static_cast<ACE_CDR::Char>(0);
        serializer << static_cast<ACE_CDR::Char>(IPv4);
        serializer << static_cast<uint16_t>(attribute.mapped_address.get_port_number());
        serializer << attribute.mapped_address.get_ip_address();
        // TODO:  Handle IPv6.
      }
      break;
    case XOR_MAPPED_ADDRESS:
      {
        serializer << static_cast<ACE_CDR::Char>(0);
        serializer << static_cast<ACE_CDR::Char>(IPv4);
        serializer << static_cast<uint16_t>(attribute.mapped_address.get_port_number() ^ (MAGIC_COOKIE >> 16));
        serializer << (attribute.mapped_address.get_ip_address() ^ MAGIC_COOKIE);
        // TODO:  Handle IPv6.
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

  bool Message::contains_unknown_comprehension_required_attributes() const {
    for (const AttributesType::value_type& attribute : m_attributes) {
      switch (attribute.type) {
      case MAPPED_ADDRESS:
      case XOR_MAPPED_ADDRESS:
        break;
      default:
        if (attribute.type < 0x8000) {
          return true;
        }
      }
    }
    return false;
  }

  bool operator>>(DCPS::Serializer& serializer, Message& message) {
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

    if (!serializer.read_octet_array(message.transaction_id, 12)) {
      return false;
    }

    // At this point there should be message.length bytes remaining.
    if (serializer.length() != message_length) {
      return false;
    }

    message.valid_header = true;

    while (serializer.length() != 0) {
      Attribute attribute;
      if (!(serializer >> attribute)) {
        return false;
      }
      message.append_attribute(attribute);
      if (message.length() > message_length) {
        return false;
      }
    }

    message.valid_attributes = true;

    return true;
  }

  bool operator<<(DCPS::Serializer& serializer, const Message& message) {
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
    serializer.write_octet_array(message.transaction_id, sizeof(message.transaction_id));

    for (Message::const_iterator pos = message.begin(), limit = message.end();
         pos != limit; ++pos) {
      const Attribute& attribute = *pos;
      serializer << attribute;
    }

    return true;
  }

  void Participant::receive(const ACE_INET_Addr& address, const Message& message) {
    if (!message.valid_header) {
      std::cerr << "Not a valid header" << std::endl;
      return;
    }

    if (!message.valid_attributes) {
      std::cerr << "TODO:  Send a 400" << std::endl;
      return;
    }

    switch (message.class_) {
    case STUN::REQUEST:
      request(address, message);
      break;
    case STUN::INDICATION:
      indication(address, message);
      break;
    case STUN::SUCCESS_RESPONSE:
      success_response(address, message);
      break;
    case STUN::ERROR_RESPONSE:
      error_response(address, message);
      break;
    }
  }

  void Participant::request(const ACE_INET_Addr& address, const Message& message) {
    if (message.contains_unknown_comprehension_required_attributes()) {
      std::cerr << "TODO: Send 420 with unknown attributes" << std::endl;
      return;
    }

    switch (message.method) {
    case STUN::BINDING:
      {
        Message response;
        response.class_ = SUCCESS_RESPONSE;
        response.method = STUN::BINDING;
        memcpy(response.transaction_id, message.transaction_id, sizeof(message.transaction_id));
        response.append_attribute(make_mapped_address(address));
        response.append_attribute(make_xor_mapped_address(address));
        m_sender->send(address, response);
      }
      break;
    default:
      // Unknown method.  Stop processing.
      std::cerr << "TODO: Send error for unsupported method" << std::endl;
      break;
    }
  }

  void Participant::indication(const ACE_INET_Addr& /*address*/, const Message& message) {
    if (message.contains_unknown_comprehension_required_attributes()) {
      // No further processing according to spec.
      return;
    }

    switch (message.method) {
    case STUN::BINDING:
      // Done.
      break;
    default:
      // Unknown method.  Stop processing.
      break;
    }
  }

  void Participant::success_response(const ACE_INET_Addr& /*address*/, const Message& /*message*/) {
    std::cerr << "TODO: Implement success_response" << std::endl;
  }

  void Participant::error_response(const ACE_INET_Addr& /*address*/, const Message& /*message*/) {
    std::cerr << "TODO: Implement error_response" << std::endl;
  }

} // namespace STUN
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
