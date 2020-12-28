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

    unsigned int seq = 2;
    bool r = sw1.writeHeartbeat(seq, 1, AppConfig::readerId[0]);

    r = r && sw2.writeHeartbeat(seq, 1, AppConfig::readerId[0]);
    r = r && sw2.writeHeartbeat(seq, 1, AppConfig::readerId[1]);

    r = r && sw3.writeHeartbeat(seq, 1, AppConfig::readerId[0]);
    r = r && sw3.writeHeartbeat(seq, 1, AppConfig::readerId[1]);
    r = r && sw3.writeHeartbeat(seq, 1, AppConfig::readerId[2]);

    r = r && sw1.write(seq, TestMsg(10, "1-2"));
    r = r && sw2.write(seq, TestMsg(10, "2-2"));
    r = r && sw3.write(seq, TestMsg(10, "3-2"));

    r = r && sw1.write(seq, TestMsg(10, "1-2 Duplicate"));
    r = r && sw2.write(seq, TestMsg(10, "2-2 Duplicate"));
    r = r && sw3.write(seq, TestMsg(10, "3-2 Duplicate"));

    ++seq;
    r = r && sw3.write(seq, TestMsg(10, "3-3"));

    ++seq;
    r = r && sw1.write(seq, TestMsg(99, "1-4 end"));
    r = r && sw2.write(seq, TestMsg(99, "2-4 end"));
    r = r && sw3.write(seq, TestMsg(99, "3-4 end"));

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
