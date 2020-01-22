#include "SimpleDataReader.h"

#include "dds/DCPS/transport/framework/TransportExceptions.h"
#include "dds/DCPS/transport/framework/ReceivedDataSample.h"
#include "dds/DCPS/transport/framework/TransportRegistry.h"
#include "dds/DCPS/transport/rtps_udp/RtpsUdpInst.h"
#ifdef ACE_AS_STATIC_LIBS
#include "dds/DCPS/transport/rtps_udp/RtpsUdp.h"
#endif

#include <ace/OS_main.h>
#include <ace/String_Base.h>

class Subscriber {
public:
  Subscriber(int argc, ACE_TCHAR* argv[]) : config(argc, argv) {
    if(!configureTransport()) {
      throw std::string("Failed to create Subscriber.");
    }
  }

  int run() {
    SimpleDataReader reader1(config, 0);
    SimpleDataReader reader2(config, 1);
    SimpleDataReader reader3(config, 2);
    while (!(reader1.done() && reader2.done() && reader3.done())) {
      ACE_OS::sleep(1);
    }
    return 0;
  }

private:
  bool configureTransport();
  AppConfig config;
};

bool Subscriber::configureTransport(){
  try {
    OpenDDS::DCPS::TransportInst_rch transportInst = TheTransportRegistry->create_inst("my_rtps", "rtps_udp");
    OpenDDS::DCPS::RtpsUdpInst* rtpsInst = dynamic_cast<OpenDDS::DCPS::RtpsUdpInst*>(transportInst.in());
    if (!rtpsInst) {
      ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: Failed to cast to RtpsUdpInst*%m\n")), false);
    }
    rtpsInst->datalink_release_delay_ = 0;
    rtpsInst->local_address(config.getPort(), ACE_TEXT_ALWAYS_CHAR(config.getHost().c_str()));

    OpenDDS::DCPS::TransportConfig_rch cfg = TheTransportRegistry->create_config("cfg");
    cfg->instances_.push_back(transportInst);
    TheTransportRegistry->global_config(cfg);
    return true;
  } catch (const OpenDDS::DCPS::Transport::Exception& e) {
    ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: %C%m\n"), typeid(e).name()), false);
  } catch (const CORBA::Exception& e) {
    ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: %C%m\n"), e._info().c_str()), false);
  } catch (...) {
    ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: in configureTransport()%m\n")), false);
  }
}

int ACE_TMAIN(int argc, ACE_TCHAR* argv[])
{
  try {
    Subscriber test(argc, argv);
    return test.run();
  } catch (...) {
    return 1;
  }
}
