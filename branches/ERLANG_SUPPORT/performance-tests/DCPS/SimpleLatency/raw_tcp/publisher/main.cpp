#include "TestDriver.h"
#include "TestException.h"


int
main(int argc, char* argv[])
{
  TestDriver driver;

  try
  {
    driver.run(argc, argv);
    return 0;
  }
  catch (const TestException&)
  {
    ACE_ERROR((LM_ERROR,
               "(%P|%t) Publisher TestException.\n"));
  }
  catch (...)
  {
    ACE_ERROR((LM_ERROR,
               "(%P|%t) Publisher unknown (...) exception.\n"));
  }

  return 1;
}
