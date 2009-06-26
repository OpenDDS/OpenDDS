// -*- C++ -*-
//
// $Id$
#include "dds/DCPS/transport/framework/TransportImpl.h"
#include "SubDriver.h"
#include "TestException.h"
#include "ace/Log_Msg.h"


int
ACE_TMAIN (int argc, ACE_TCHAR *argv[])
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
