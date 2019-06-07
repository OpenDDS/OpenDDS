
/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_RTPS_STUN_H
#define OPENDDS_RTPS_STUN_H

#include <map>
#include <vector>

#include "ace/INET_Addr.h"
#include "dds/DCPS/Serializer.h"

#include "dds/DCPS/RTPS/rtps_export.h"

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
  SUCCESS_RESPONSE = 2,
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
  USERNAME           = 0x0006,
  MESSAGE_INTEGRITY  = 0x0008,
  ERROR_CODE         = 0x0009,
  UNKNOWN_ATTRIBUTES = 0x000A,
  // REALM              = 0x0014,
  // NONCE              = 0x0015,
  XOR_MAPPED_ADDRESS = 0x0020,
  PRIORITY           = 0x0024,
  USE_CANDIDATE      = 0x0025,

  // SOFTWARE           = 0x8022,
  // ALTERNATE_SERVER   = 0x8023,
  FINGERPRINT        = 0x8028,
  ICE_CONTROLLED     = 0x8029,
  ICE_CONTROLLING    = 0x802A
};

struct Attribute {
  AttributeType type;

  ACE_INET_Addr mapped_address; // MAPPED_ADDRESS, XOR_MAPPED_ADDRESS
  std::string username; // USERNAME
  union {
    unsigned char message_integrity[20]; // MESSAGE_INTEGRITY
    uint32_t fingerprint; // FINGERPRINT
    uint32_t priority; // PRIORITY
    ACE_UINT64 ice_tie_breaker; // ICE_CONTROLLED, ICE_CONTROLLING
  };
  struct {
    uint16_t code;
    std::string reason;
  } error;
  std::vector<AttributeType> unknown_attributes;

  uint16_t unknown_length;

  uint16_t length() const;
};

Attribute make_mapped_address(const ACE_INET_Addr& addr);
Attribute make_username(const std::string& username);
Attribute make_message_integrity();
Attribute make_error_code(uint16_t code, const std::string& reason);
Attribute make_unknown_attributes(const std::vector<AttributeType>& unknown_attributes);
Attribute make_xor_mapped_address(const ACE_INET_Addr& addr);
Attribute make_unknown_attribute(uint16_t type, uint16_t length);
Attribute make_priority(uint32_t priority);
Attribute make_use_candidate();;

Attribute make_fingerprint();
Attribute make_ice_controlling(ACE_UINT64 ice_tie_breaker);
Attribute make_ice_controlled(ACE_UINT64 ice_tie_breaker);

bool operator>>(DCPS::Serializer& serializer, Attribute& attribute);
bool operator<<(DCPS::Serializer& serializer, const Attribute& attribute);

struct TransactionId {
  uint8_t data[12];
  bool operator<(const TransactionId& other) const;
  bool operator==(const TransactionId& other) const;
  bool operator!=(const TransactionId& other) const;
};

struct Message {
  typedef std::vector<Attribute> AttributesType;
  typedef AttributesType::const_iterator const_iterator;

  Class class_;
  Method method;
  TransactionId transaction_id;

  Message()
  : block(0), length_(0), length_for_message_integrity_(0) {}

  void generate_transaction_id();

  void append_attribute(const Attribute& attribute)
  {
    attributes_.push_back(attribute);
    length_ += (4 + attribute.length() + 3) & ~3;

    if (attribute.type == MESSAGE_INTEGRITY) {
      length_for_message_integrity_ = length_;
    }
  }

  const_iterator begin() const
  {
    return attributes_.begin();
  }
  const_iterator end() const
  {
    return attributes_.end();
  }
  uint16_t length() const
  {
    return length_;
  }
  uint16_t length_for_message_integrity() const
  {
    return length_for_message_integrity_;
  }

  std::vector<AttributeType> unknown_comprehension_required_attributes() const;
  bool get_mapped_address(ACE_INET_Addr& address) const;
  bool get_priority(uint32_t& priority) const;
  bool get_username(std::string& username) const;
  bool has_message_integrity() const;
  bool verify_message_integrity(const std::string& password) const;
  void compute_message_integrity(const std::string& password, unsigned char message_integrity[20]) const;
  bool has_error_code() const;
  uint16_t get_error_code() const;
  std::string get_error_reason() const;
  bool has_unknown_attributes() const;
  std::vector<AttributeType> get_unknown_attributes() const;
  bool has_fingerprint() const;
  uint32_t compute_fingerprint() const;
  bool has_ice_controlled() const;
  bool has_ice_controlling() const;
  bool has_use_candidate() const;

  ACE_Message_Block* block;
  std::string password; // For integrity hashing.

private:
  AttributesType attributes_;
  uint16_t length_;
  uint16_t length_for_message_integrity_;
};

OpenDDS_Rtps_Export bool operator>>(DCPS::Serializer& serializer, Message& message);
OpenDDS_Rtps_Export bool operator<<(DCPS::Serializer& serializer, const Message& message);

class Sender {
public:
  virtual void send(const ACE_INET_Addr& address, const Message& message) = 0;
  virtual ~Sender() {}
};

class OpenDDS_Rtps_Export Participant {
public:
  Participant(Sender* a_sender) : sender_(a_sender) {}

  void receive(const ACE_INET_Addr& address, const Message& message);

private:
  Sender* sender_;

  void request(const ACE_INET_Addr& address, const Message& message);
  void indication(const ACE_INET_Addr& /*address*/, const Message& message);
  void success_response(const ACE_INET_Addr& /*address*/, const Message& /*message*/);
  void error_response(const ACE_INET_Addr& /*address*/, const Message& /*message*/);
};

} // namespace STUN
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_RTPS_STUN_H */
