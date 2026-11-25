#include <testTypeSupportImpl.h>

#include <dds/DCPS/Message_Block_Ptr.h>
#include <dds/DCPS/Serializer.h>

#include <ace/OS_main.h>

#include <cstring>
#include <iostream>

using namespace OpenDDS::DCPS;

int ACE_TMAIN(int, ACE_TCHAR*[])
{
  test::ClassicMapStringKey value;
  value.map_probe["alpha"] = 42;

  const Encoding encoding(Encoding::KIND_XCDR2);
  Message_Block_Ptr buffer(new ACE_Message_Block(serialized_size(encoding, value)));
  Serializer writer(buffer.get(), encoding);
  if (!(writer << value)) {
    std::cerr << "serialize failed\n";
    return 1;
  }

  test::ClassicMapStringKey roundtrip;
  Serializer reader(buffer.get(), encoding);
  if (!(reader >> roundtrip)) {
    std::cerr << "deserialize failed\n";
    return 1;
  }

  if (roundtrip.map_probe.size() != 1) {
    std::cerr << "unexpected map size\n";
    return 1;
  }

  const test::ClassicMapStringKey::_map_probe_map::const_iterator it = roundtrip.map_probe.find("alpha");
  if (it == roundtrip.map_probe.end() || std::strcmp(it->first.in(), "alpha") != 0 || it->second != 42) {
    std::cerr << "roundtrip mismatch\n";
    return 1;
  }

  return 0;
}
