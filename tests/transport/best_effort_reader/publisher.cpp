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

    sw1.write(TestMsg(10, "1-1"), 1);
    sw2.write(TestMsg(10, "2-1"), 1);
    sw3.write(TestMsg(10, "3-1"), 1);

    sw1.write(TestMsg(10, "1-1 Duplicate"), 1);
    sw2.write(TestMsg(10, "2-1 Duplicate"), 1);
    sw3.write(TestMsg(10, "3-1 Duplicate"), 1);

    sw3.write(TestMsg(10, "3-2"), 2);

    sw1.write(TestMsg(99, "1-3 end"), 3);
    sw2.write(TestMsg(99, "2-3 end"), 3);
    sw3.write(TestMsg(99, "3-3 end"), 3);

    ACE_OS::sleep(2);
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
