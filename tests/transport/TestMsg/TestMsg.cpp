#include "TestMsg.h"

TestMsg::TestMsg()
: key(0)
, value("")
{
}

TestMsg::TestMsg(ACE_CDR::ULong msgKey, TAO::String_Manager msgValue)
: key(msgKey)
, value(msgValue)
{
}

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL
namespace OpenDDS {
namespace DCPS {

bool max_serialized_size(const Encoding&, size_t&, const TestMsg&)
{
  return false;
}

void serialized_size(
  const Encoding& encoding, size_t& size, const TestMsg& stru)
{
  max_serialized_size(encoding, size, stru.key);
  serialized_size_ulong(encoding, size);
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

bool max_serialized_size(
  const Encoding& encoding, size_t& size, KeyOnly<const TestMsg> stru)
{
  max_serialized_size(encoding, size, stru.t.key);
  return true;
}

void serialized_size(
  const Encoding& encoding, size_t& size, KeyOnly<const TestMsg> stru)
{
  max_serialized_size(encoding, size, stru.t.key);
}

bool operator<<(Serializer& strm, KeyOnly<const TestMsg> stru)
{
  return strm << stru.t.key;
}

bool operator>>(Serializer& strm, KeyOnly<TestMsg> stru)
{
  return strm >> stru.t.key;
}

}
}
OPENDDS_END_VERSIONED_NAMESPACE_DECL
