// -*- C++ -*-
// ============================================================================
/**
 *  @file   suscriber.cpp
 *
 *
 *
 */
// ============================================================================

#include "Subscriber.h"
#include "TestException.h"
#include "tao/Exception.h"
#include "dds/DCPS/debug.h"
#include "dds/DCPS/StaticIncludes.h"

int ACE_TMAIN(int argc, ACE_TCHAR* argv[])
{
  OpenDDS::DCPS::set_DCPS_debug_level( 0);
  int status = 0;
  try {
    Subscriber subscriber( argc, argv, 0);
    subscriber.run();

  } catch (const CORBA::Exception& ex) {
    ex._tao_print_exception ("(%P|%t) %T FATAL: Subscriber - CORBA problem detected.\n");
    status = -1;

  } catch (const std::exception& ex) {
    ACE_ERROR ((LM_ERROR,
      ACE_TEXT("(%P|%t) %T FATAL: Subscriber - ")
      ACE_TEXT("%C exception caught in main().\n"),
      ex.what()
    ));
    status = -2;

  } catch(...) {
    ACE_ERROR ((LM_ERROR,
      ACE_TEXT("(%P|%t) %T FATAL: Subscriber - ")
      ACE_TEXT("Unspecified exception caught in main() - panic.\n")
    ));
    status = -3;

  }

  return status;
}

