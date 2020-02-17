#include "AppConfig.h"
#include "SocketWriter.h"
#include "TestMsg.h"
#ifdef ACE_AS_STATIC_LIBS
#  include <dds/DCPS/RTPS/RtpsDiscovery.h>
#  include <dds/DCPS/transport/rtps_udp/RtpsUdp.h>
#endif

class Publisher {
public:
  Publisher(int argc, ACE_TCHAR* argv[]) : config(argc, argv) {}

  int run()
  {
    SocketWriter sw1(AppConfig::writerId[0], config.getHostAddress());
    SocketWriter sw2(AppConfig::writerId[1], config.getHostAddress());
    SocketWriter sw3(AppConfig::writerId[2], config.getHostAddress());

    bool r = sw1.writeHeartbeat(1, 1);
    r = r && sw2.writeHeartbeat(1, 1);
    r = r && sw3.writeHeartbeat(1, 1);

    r = r && sw1.write(2, TestMsg(10, "1-2"));
    r = r && sw2.write(2, TestMsg(10, "2-2"));
    r = r && sw3.write(2, TestMsg(10, "3-2"));

    r = r && sw1.write(2, TestMsg(10, "1-2 Duplicate"));
    r = r && sw2.write(2, TestMsg(10, "2-2 Duplicate"));
    r = r && sw3.write(2, TestMsg(10, "3-2 Duplicate"));

    r = r && sw3.write(3, TestMsg(10, "3-3"));

    r = r && sw1.write(4, TestMsg(99, "1-4 end"));
    r = r && sw2.write(4, TestMsg(99, "2-4 end"));
    r = r && sw3.write(4, TestMsg(99, "3-4 end"));

    return (r ? 0 : 1);
  }

private:
  AppConfig config;
};

int ACE_TMAIN(int argc, ACE_TCHAR* argv[])
{
  try {
    Publisher test(argc, argv);
    return test.run();
  } catch (...) {
    return 1;
  }
}
