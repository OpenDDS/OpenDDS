// -*- C++ -*-
// $Id$

#include "Process.h"
#include "Test.h"
#include "Options.h"
#include "dds/DCPS/Service_Participant.h"

#include <sstream>

int
main( int argc, char *argv[])
{
  try {
    // Initialize DDS.
    TheParticipantFactoryWithArgs( argc, argv);

    // Initialize the test.
    const Test::Options options( argc, argv);

    // Create the process thingie.
    Test::Process process( options);

    // Execute the test.
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) testprocess() - starting.\n")));
    process.run();

  } catch( CORBA::Exception& /* e */) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) testprocess() - ")
      ACE_TEXT("CORBA exception caught during processing.\n")
    ));
    return 1;

  } catch( Test::Exception e)  {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) testprocess() - ")
      ACE_TEXT("Test exception caught during processing: %s.\n"),
      e.what()
    ));
    return 1;

  }

  ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) testprocess() - terminating normally.\n")));
  return 0;
}

