#include "TestMsg.h"
#include <dds/DCPS/Serializer.h>
#include <dds/DCPS/TypeSupportImpl.h>

using namespace OpenDDS::DCPS;

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
  return (strm << stru.key) && (strm << stru.value);
}

bool operator>>(Serializer& strm, TestMsg& stru)
{
  return (strm >> stru.key) && (strm >> stru.value.out());
}

size_t gen_max_marshaled_size(const TestMsg&, bool)
{
  return 0;
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
