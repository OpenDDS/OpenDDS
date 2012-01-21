#include "dds/DCPS/Serializer.h"

using namespace OpenDDS::DCPS;

struct TestMsg {
  ACE_CDR::ULong key;
  TAO::String_Manager value;
};

void gen_find_size(const TestMsg& stru, size_t& size, size_t& padding)
{
  if ((size + padding) % 4) {
    padding += 4 - ((size + padding) % 4);
  }
  size += gen_max_marshaled_size(stru.key);
  find_size_ulong(size, padding);
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

bool gen_is_bounded_size(const TestMsg& stru)
{
  ACE_UNUSED_ARG(stru);
  return false;
}

size_t gen_max_marshaled_size(const TestMsg& stru, bool align)
{
  ACE_UNUSED_ARG(stru);
  ACE_UNUSED_ARG(align);
  return 0;
}

bool gen_is_bounded_size(KeyOnly<const TestMsg>)
{
  return true;
}

size_t gen_max_marshaled_size(KeyOnly<const TestMsg>, bool /*align*/)
{
  return 4;
}

void gen_find_size(KeyOnly<const TestMsg> stru, size_t& size, size_t& padding)
{
  if ((size + padding) % 4) {
    padding += 4 - ((size + padding) % 4);
  }
  size += gen_max_marshaled_size(stru.t.key);
}

bool operator<<(Serializer& strm, KeyOnly<const TestMsg> stru)
{
  return strm << stru.t.key;
}

bool operator>>(Serializer& strm, KeyOnly<TestMsg> stru)
{
  return (strm >> stru.t.key);
}
