#include "Thrasher.h"

#include <ace/Log_Msg.h>
#include <ace/OS_main.h>
#include <ace/OS_NS_stdlib.h>

int ACE_TMAIN(int argc, ACE_TCHAR** argv)
{
  try {
    Thrasher thrasher(argc, argv);
    return thrasher.run();
  } catch (const std::exception& e) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: %C\n", e.what()));
  } catch (...) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: exception\n"));
  }
  return 101;
}
