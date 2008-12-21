#include "dds/DCPS/transport/framework/TransportImpl.h"
#include "PubDriver.h"
#include "TestException.h"
#include "ace/Log_Msg.h"


int
main(int argc, char* argv[])
{
  try
  {
    PubDriver driver;
    driver.run(argc, argv);
    return 0;
  }
  catch (const TestException&)
  {
    ACE_ERROR((LM_ERROR,
               "(%P|%t) PubDriver TestException.\n"));
  }
  catch (...)
  {
    ACE_ERROR((LM_ERROR,
               "(%P|%t) PubDriver unknown (...) exception.\n"));
  }

  return 1;
}
