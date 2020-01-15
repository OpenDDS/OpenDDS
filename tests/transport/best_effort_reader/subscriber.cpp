#include "SimpleDataReader.h"

#include <ace/OS_main.h>
#include <ace/String_Base.h>

class Subscriber {
public:
  Subscriber(int argc, ACE_TCHAR* argv[]) : config(argc, argv, true) {}
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
  AppConfig config;
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
