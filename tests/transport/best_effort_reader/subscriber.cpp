#include "SimpleDataReader.h"

#include <ace/OS_main.h>
#include <ace/String_Base.h>

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
