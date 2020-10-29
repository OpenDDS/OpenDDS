#include "TestMsg.h"

TestMsg::TestMsg()
: key(0)
, value("")
{
}

TestMsg::TestMsg(ACE_CDR::ULong key, const char* value)
: key(key)
, value(value)
{
}

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL
namespace OpenDDS {
namespace DCPS {

void serialized_size(
  const Encoding& encoding, size_t& size, const TestMsg& stru)
{
  primitive_serialized_size_ulong(encoding, size);
  primitive_serialized_size_ulong(encoding, size);
  size += ACE_OS::strlen(stru.value) + 1;
}

bool operator<<(Serializer& strm, const TestMsg& stru)
{
  return (strm << stru.key)
    && (strm << stru.value);
}

bool operator>>(Serializer& strm, TestMsg& stru)
{
  return (strm >> stru.key)
    && (strm >> stru.value.out());
}

void serialized_size(
  const Encoding& encoding, size_t& size, KeyOnly<const TestMsg>)
{
  primitive_serialized_size_ulong(encoding, size);
}

bool operator<<(Serializer& strm, KeyOnly<const TestMsg> stru)
{
  return strm << stru.value.key;
}

bool operator>>(Serializer& strm, KeyOnly<TestMsg> stru)
{
  return strm >> stru.value.key;
}

}
}
OPENDDS_END_VERSIONED_NAMESPACE_DECL
