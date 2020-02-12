#include "AppConfig.h"
#include "SocketWriter.h"
#include "TestMsg.h"

class Publisher {
public:
  Publisher(int argc, ACE_TCHAR* argv[]) : config(argc, argv) {}

  int run()
  {
    SocketWriter sw1(AppConfig::writerId[0], config.getHostAddress());
    SocketWriter sw2(AppConfig::writerId[1], config.getHostAddress());
    SocketWriter sw3(AppConfig::writerId[2], config.getHostAddress());

    sw1.writeHeartbeat(1, 1);
    sw2.writeHeartbeat(1, 1);
    sw3.writeHeartbeat(1, 1);

    sw1.write(TestMsg(10, "1-2"), 2);
    sw2.write(TestMsg(10, "2-2"), 2);
    sw3.write(TestMsg(10, "3-2"), 2);

    sw1.write(TestMsg(10, "1-2 Duplicate"), 2);
    sw2.write(TestMsg(10, "2-2 Duplicate"), 2);
    sw3.write(TestMsg(10, "3-2 Duplicate"), 2);

    sw3.write(TestMsg(10, "3-3"), 3);

    sw1.write(TestMsg(99, "1-4 end"), 4);
    sw2.write(TestMsg(99, "2-4 end"), 4);
    sw3.write(TestMsg(99, "3-4 end"), 4);

    return 0;
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
