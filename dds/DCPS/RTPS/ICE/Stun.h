/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifdef OPENDDS_SECURITY
#ifndef OPENDDS_RTPS_STUN_H
#define OPENDDS_RTPS_STUN_H

#include "ace/INET_Addr.h"
#include "dds/DCPS/Serializer.h"

#include "dds/DCPS/RTPS/rtps_export.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include <map>
#include <string>
#include <vector>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace STUN {

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

const ACE_UINT32 MAGIC_COOKIE = 0x2112A442;
const size_t HEADER_SIZE = 20;

const ACE_UINT16 BAD_REQUEST = 400;
const ACE_UINT16 UNAUTHORIZED = 401;
const ACE_UINT16 UNKNOWN_ATTRIBUTE = 420;

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
  ICE_CONTROLLING    = 0x802A,
  LAST_ATTRIBUTE     = 0xFFFF
};

struct OpenDDS_Rtps_Export Attribute {
  Attribute() : type(LAST_ATTRIBUTE), unknown_length(0) {}

  AttributeType type;

  ACE_INET_Addr mapped_address; // MAPPED_ADDRESS, XOR_MAPPED_ADDRESS
  std::string username; // USERNAME
  union {
    unsigned char message_integrity[20]; // MESSAGE_INTEGRITY
    ACE_UINT32 fingerprint; // FINGERPRINT
    ACE_UINT32 priority; // PRIORITY
    ACE_UINT64 ice_tie_breaker; // ICE_CONTROLLED, ICE_CONTROLLING
  };
  struct {
    ACE_UINT16 code;
    std::string reason;
  } error;
  std::vector<AttributeType> unknown_attributes;

  ACE_UINT16 unknown_length;

  ACE_UINT16 length() const;
};

OpenDDS_Rtps_Export
Attribute make_mapped_address(const ACE_INET_Addr& addr);

OpenDDS_Rtps_Export
Attribute make_username(const std::string& username);

OpenDDS_Rtps_Export
Attribute make_message_integrity();

OpenDDS_Rtps_Export
Attribute make_error_code(ACE_UINT16 code, const std::string& reason);

OpenDDS_Rtps_Export
Attribute make_unknown_attributes(const std::vector<AttributeType>& unknown_attributes);

OpenDDS_Rtps_Export
Attribute make_xor_mapped_address(const ACE_INET_Addr& addr);

OpenDDS_Rtps_Export
Attribute make_unknown_attribute(ACE_UINT16 type, ACE_UINT16 length);

OpenDDS_Rtps_Export
Attribute make_priority(ACE_UINT32 priority);

OpenDDS_Rtps_Export
Attribute make_use_candidate();;

OpenDDS_Rtps_Export
Attribute make_fingerprint();

OpenDDS_Rtps_Export
Attribute make_ice_controlling(ACE_UINT64 ice_tie_breaker);

OpenDDS_Rtps_Export
Attribute make_ice_controlled(ACE_UINT64 ice_tie_breaker);

OpenDDS_Rtps_Export
bool operator>>(DCPS::Serializer& serializer, Attribute& attribute);

OpenDDS_Rtps_Export
bool operator<<(DCPS::Serializer& serializer, const Attribute& attribute);

struct OpenDDS_Rtps_Export TransactionId {
  ACE_UINT8 data[12];
  bool operator<(const TransactionId& other) const;
  bool operator==(const TransactionId& other) const;
  bool operator!=(const TransactionId& other) const;
};

struct OpenDDS_Rtps_Export Message {
  typedef std::vector<Attribute> AttributesType;
  typedef AttributesType::const_iterator const_iterator;

  Class class_;
  Method method;
  TransactionId transaction_id;

  Message()
  : class_(REQUEST), method(BINDING), block(0), length_(0), length_for_message_integrity_(0) {}

  void generate_transaction_id();

  void clear_transaction_id();

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
  ACE_UINT16 length() const
  {
    return length_;
  }
  ACE_UINT16 length_for_message_integrity() const
  {
    return length_for_message_integrity_;
  }

  std::vector<AttributeType> unknown_comprehension_required_attributes() const;
  bool get_mapped_address(ACE_INET_Addr& address) const;
  bool get_priority(ACE_UINT32& priority) const;
  bool get_username(std::string& username) const;
  bool has_message_integrity() const;
  bool verify_message_integrity(const std::string& password) const;
  void compute_message_integrity(const std::string& password, unsigned char message_integrity[20]) const;
  bool has_error_code() const;
  ACE_UINT16 get_error_code() const;
  std::string get_error_reason() const;
  bool has_unknown_attributes() const;
  std::vector<AttributeType> get_unknown_attributes() const;
  bool has_fingerprint() const;
  ACE_UINT32 compute_fingerprint() const;
  bool has_ice_controlled() const;
  bool has_ice_controlling() const;
  bool has_use_candidate() const;

  ACE_Message_Block* block;
  std::string password; // For integrity hashing.

private:
  AttributesType attributes_;
  ACE_UINT16 length_;
  ACE_UINT16 length_for_message_integrity_;
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
#endif /* OPENDDS_SECURITY */
