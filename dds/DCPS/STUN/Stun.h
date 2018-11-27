/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_RTPS_STUN_H
#define OPENDDS_RTPS_STUN_H

#include "ace/INET_Addr.h"
#include "dds/DCPS/Serializer.h"

#include "stun_export.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace STUN {
  namespace DCPS = OpenDDS::DCPS;

  enum Class {
    REQUEST          = 0,
    INDICATION       = 1,
    SUCCESS_RESPONSE = 2 ,
    ERROR_RESPONSE   = 3
  };

  enum Method {
    BINDING = 0x001
  };

  enum Family {
    IPv4 = 0x01,
    IPv6 = 0x02
  };

  const uint32_t MAGIC_COOKIE = 0x2112A442;

  enum AttributeType {
    MAPPED_ADDRESS     = 0x0001,
    // USERNAME           = 0x0006,
    // MESSAGE_INTEGRITY  = 0x0008,
    // ERROR_CODE         = 0x0009,
    // UNKNOWN_ATTRIBUTES = 0x000A,
    // REALM              = 0x0014,
    // NONCE              = 0x0015,
    XOR_MAPPED_ADDRESS = 0x0020,

    // SOFTWARE           = 0x8022,
    // ALTERNATE_SERVER   = 0x8023,
    // FINGERPRINT        = 0x8028
  };

  struct Attribute {
    AttributeType type;

    ACE_INET_Addr mapped_address; // shared by MAPPED_ADDRESS and XOR_MAPPED_ADDRESS
    std::string username;
    std::string message_integrity;
    uint32_t fingerprint;
    struct {
      uint16_t code;
      std::string reason;
    } error;
    std::string realm;
    std::string nonce;
    std::vector<uint16_t> unknown_attributes;
    std::string software;
    ACE_INET_Addr alternate_server;

    uint16_t unknown_length;

    uint16_t length() const;
  };

  Attribute make_mapped_address(const ACE_INET_Addr& addr);
  Attribute make_xor_mapped_address(const ACE_INET_Addr& addr);
  Attribute make_unknown_attribute(uint16_t type, uint16_t length);

  bool operator>>(DCPS::Serializer& serializer, Attribute& attribute);
  bool operator<<(DCPS::Serializer& serializer, const Attribute& attribute);

  struct Message {
    typedef std::vector<Attribute> AttributesType;
    typedef AttributesType::const_iterator const_iterator;

    Class class_;
    Method method;

    uint8_t transaction_id[12];
    bool valid_header;
    bool valid_attributes;

    Message() : valid_header(false), valid_attributes(false), m_length(0) {}

    void append_attribute(const Attribute& attribute) {
      m_attributes.push_back(attribute);
      m_length += (4 + attribute.length() + 3) & ~3;
    }

    const_iterator begin() const { return m_attributes.begin(); }
    const_iterator end() const { return m_attributes.end(); }
    uint16_t length() const { return m_length; }

    bool contains_unknown_comprehension_required_attributes() const;

  private:
    AttributesType m_attributes;
    uint16_t m_length;
  };

  OpenDDS_Stun_Export bool operator>>(DCPS::Serializer& serializer, Message& message);
  OpenDDS_Stun_Export bool operator<<(DCPS::Serializer& serializer, const Message& message);

  class Sender {
  public:
    virtual void send(const ACE_INET_Addr& address, const Message& message) = 0;
    virtual ~Sender() = default;
  };

  class OpenDDS_Stun_Export Participant {
  public:
    Participant(Sender* a_sender) : m_sender(a_sender) {}

    void receive(const ACE_INET_Addr& address, const Message& message);

  private:
    Sender* m_sender;

    void request(const ACE_INET_Addr& address, const Message& message);
    void indication(const ACE_INET_Addr& /*address*/, const Message& message);
    void success_response(const ACE_INET_Addr& /*address*/, const Message& /*message*/);
    void error_response(const ACE_INET_Addr& /*address*/, const Message& /*message*/);
  };

} // namespace OpenDDS
} // namespace STUN

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_RTPS_STUN_H */
