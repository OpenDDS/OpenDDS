#ifndef OPENDDS_TRANSPORT_TEST_MSG_H
#define OPENDDS_TRANSPORT_TEST_MSG_H

#include "TestMsg_Export.h"

#include <dds/DCPS/Serializer.h>
#include <dds/DCPS/TypeSupportImpl.h>

struct TestMsg_Export TestMsg {
  ACE_CDR::ULong key;
  TAO::String_Manager value;

  TestMsg();
  TestMsg(ACE_CDR::ULong key, const char* value);
};

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL
namespace OpenDDS {
namespace DCPS {

TestMsg_Export
bool max_serialized_size(const Encoding&, size_t&, const TestMsg&);

TestMsg_Export
void serialized_size(
  const Encoding& encoding, size_t& size, const TestMsg& stru);

TestMsg_Export
bool operator<<(Serializer& strm, const TestMsg& stru);

TestMsg_Export
bool operator>>(Serializer& strm, TestMsg& stru);

TestMsg_Export
bool max_serialized_size(
  const Encoding& encoding, size_t& size, KeyOnly<const TestMsg> stru);

TestMsg_Export
void serialized_size(
  const Encoding& encoding, size_t& size, KeyOnly<const TestMsg> stru);

TestMsg_Export
bool operator<<(Serializer& strm, KeyOnly<const TestMsg> stru);

TestMsg_Export
bool operator>>(Serializer& strm, KeyOnly<TestMsg> stru);

template <>
struct MarshalTraits<TestMsg> {
  static bool bounded(const Encoding&)
  {
    return false;
  }

  static size_t max_serialized_size(const Encoding&)
  {
    return 0;
  }

  static bool key_only_bounded(const Encoding&)
  {
    return true;
  }

  static size_t key_only_max_serialized_size(const Encoding&)
  {
    return uint32_cdr_size;
  }
};

}
}
OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_TRANSPORT_TEST_MSG_H */
