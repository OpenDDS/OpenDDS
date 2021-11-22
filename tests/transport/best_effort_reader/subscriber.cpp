#include "SimpleDataReader.h"

#include <dds/DCPS/AssociationData.h>
#include "dds/DCPS/RTPS/BaseMessageUtils.h"
#include <dds/OpenddsDcpsExtTypeSupportImpl.h>
#ifdef ACE_AS_STATIC_LIBS
#  include <dds/DCPS/RTPS/RtpsDiscovery.h>
#  include <dds/DCPS/transport/rtps_udp/RtpsUdp.h>
#endif
#include <ace/OS_main.h>
#include <ace/String_Base.h>
#include <iostream>
#include <fstream>

/*
The test contains 3 writers and 3 readers, associated as follows:
  writer1 <- reader1
  writer2 <- reader1, reader2
  writer3 <- reader1, reader2, reader3 (reliable)
All writers are reliable. All readers except reader3 are best-effort.
*/


class Subscriber {
public:
  Subscriber(int argc, ACE_TCHAR* argv[]) : config(argc, argv) {
    if(!config.configureTransport()) {
      throw std::string("Failed to create Subscriber.");
    }

    using OpenDDS::RTPS::message_block_to_sequence;

    ACE_INET_Addr remote_addr("127.0.0.1:12345");
    OpenDDS::DCPS::LocatorSeq locators;
    locators.length(1);
    address_to_locator(locators[0], remote_addr);

    const OpenDDS::DCPS::Encoding& locators_encoding = OpenDDS::RTPS::get_locators_encoding();
    size_t size_locator = 0;
    serialized_size(locators_encoding, size_locator, locators);
    ACE_Message_Block mb_locator(size_locator + 1);
    OpenDDS::DCPS::Serializer ser_loc(&mb_locator, locators_encoding);
    if (!(ser_loc << locators) ||
        !(ser_loc << ACE_OutputCDR::from_boolean(false))) { // requires inline QoS
      std::cerr << "subscriber serialize locators failed\n";
    }

    publication.remote_reliable_ = true;
    publication.remote_data_.length(1);
    publication.remote_data_[0].transport_type = "rtps_udp";
    message_block_to_sequence(mb_locator, publication.remote_data_[0].data);
  }

  int run() {
    SimpleDataReader reader1(config, 0, publication);
    SimpleDataReader reader2(config, 1, publication);
    SimpleDataReader reader3(config, 2, publication);
    writeSubReady();
    while (!(reader1.done() && reader2.done() && reader3.done())) {
      ACE_OS::sleep(1);
    }
    return 0;
  }

private:
  void writeSubReady() {
    std::ofstream os("subready.txt", std::ios::binary);
    os << "Ready\n";
    std::cerr << "*** Ready written to subready.txt ***\n";
  }

  AppConfig config;
  OpenDDS::DCPS::AssociationData publication;
};

int ACE_TMAIN(int argc, ACE_TCHAR* argv[])
{
  try {
    Subscriber test(argc, argv);
    return test.run();
  } catch (...) {
    return 1;
  }
}
