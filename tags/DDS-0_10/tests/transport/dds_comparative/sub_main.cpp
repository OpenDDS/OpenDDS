// -*- C++ -*-
//
// $Id$
#include "SubDriver.h"
#include "TestException.h"
#include "ace/Log_Msg.h"


int
main(int argc, char* argv[])
{
  try
  {
    SubDriver driver;
    driver.run(argc, argv);
    return 0;
  }
  catch (const TestException&)
  {
    ACE_ERROR((LM_ERROR,
               "(%P|%t) SubDriver TestException.\n"));
  }
  catch (...)
  {
    ACE_ERROR((LM_ERROR,
               "(%P|%t) SubDriver unknown (...) exception.\n"));
  }

  return 1;
}
