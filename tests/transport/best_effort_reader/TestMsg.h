#ifndef test_transport_TestMsg_H
#define test_transport_TestMsg_H

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

void gen_find_size(const TestMsg& stru, size_t& size, size_t& padding);

bool operator<<(Serializer& strm, const TestMsg& stru);

bool operator>>(Serializer& strm, TestMsg& stru);

size_t gen_max_marshaled_size(const TestMsg&, bool);

size_t gen_max_marshaled_size(KeyOnly<const TestMsg>, bool /*align*/);

void gen_find_size(KeyOnly<const TestMsg> stru, size_t& size, size_t& padding);

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

#endif /* test_transport_TestMsg_H */
