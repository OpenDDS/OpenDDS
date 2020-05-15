#ifndef OPENDDS_TRANSPORT_TEST_MSG_H
#define OPENDDS_TRANSPORT_TEST_MSG_H

#include <dds/DCPS/Serializer.h>
#include <dds/DCPS/TypeSupportImpl.h>

using namespace OpenDDS::DCPS;

struct TestMsg {
  ACE_CDR::ULong key;
  TAO::String_Manager value;
  TestMsg() : key(0), value("") {}
  TestMsg(ACE_CDR::ULong msgKey, TAO::String_Manager msgValue)
    : key(msgKey), value(msgValue) {}
};

bool max_serialize_size(const Encoding&, size_t&, const TestMsg&);

void serialized_size(
  const Encoding& encoding, size_t& size, const TestMsg& stru);

bool operator<<(Serializer& strm, const TestMsg& stru);

bool operator>>(Serializer& strm, TestMsg& stru);

bool max_serialize_size(
  const Encoding& encoding, size_t& size, KeyOnly<const TestMsg> stru);

void serialized_size(
  const Encoding& encoding, size_t& size, KeyOnly<const TestMsg> stru);

bool operator<<(Serializer& strm, KeyOnly<const TestMsg> stru);

bool operator>>(Serializer& strm, KeyOnly<TestMsg> stru);

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL
namespace OpenDDS { namespace DCPS {
template <>
struct MarshalTraits<TestMsg> {
static bool gen_is_bounded_size() { return false; }
static bool gen_is_bounded_key_size() { return true; }
};
} }
OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_TRANSPORT_TEST_MSG_H */
