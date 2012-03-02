// -*- C++ -*-
// $Id$

#include "Process.h"
#include "Test.h"
#include "Options.h"
#include "Shutdown.h"
#include "dds/DCPS/Service_Participant.h"

#ifdef ACE_AS_STATIC_LIBS
#include "dds/DCPS/transport/tcp/Tcp.h"
#include "dds/DCPS/transport/udp/Udp.h"
#include "dds/DCPS/transport/multicast/Multicast.h"
#endif

#include <iostream>

int ACE_TMAIN(int argc, ACE_TCHAR *argv[])
{
  try {
    // Initialize DDS.
    TheParticipantFactoryWithArgs( argc, argv);

    // Initialize the test.
    const Test::Options options( argc, argv);

    // Only run if we have a valid configuration.
    if( options) {
      // Create the process thingie.
      Test::Process  process( options);

      // Install a signal handler to shutdown testing gracefully.
      Test::Shutdown shutdown( process);
      Service_Shutdown service_shutdown( shutdown);


      // Execute the test.
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) testprocess() - starting.\n")));
      process.run();

      std::cout << process << std::endl;
    }

  } catch( CORBA::Exception& /* e */) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) testprocess() - ")
      ACE_TEXT("CORBA exception caught during processing.\n")
    ));
    return 1;

  } catch( const Test::Exception& e)  {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) testprocess() - ")
      ACE_TEXT("Test exception caught during processing: %C.\n"),
      e.what()
    ));
    return 1;

  }

  ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) testprocess() - terminating normally.\n")));
  return 0;
}

