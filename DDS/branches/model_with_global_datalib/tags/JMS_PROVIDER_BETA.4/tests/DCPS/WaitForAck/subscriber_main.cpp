// -*- C++ -*-
// $Id$

#include "Subscriber.h"
#include "Test.h"
#include "Options.h"
#include "dds/DCPS/Service_Participant.h"

#include <iomanip>
#include <fstream>
#include <sstream>

int
main( int argc, char *argv[])
{
  int result = -1;

  try {
    // Initialize DDS.
    TheParticipantFactoryWithArgs( argc, argv);

    // Initialize the test.
    const Test::Options options( argc, argv);

    if( options) {
      // Create the subscriber thingie.
      Test::Subscriber subscriber( options);

      if( options.verbose()) {
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) subscriber_main() - started.\n")));
      }

      // Execute the test.
      subscriber.run();

      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) subscriber_main() - test complete.\n")
      ));

      // Test passes if there were no exceptions.
      result = 0;
    }

  } catch( CORBA::Exception& /* e */) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) subscriber_main() - ")
      ACE_TEXT("CORBA exception caught during processing.\n")
    ));

  } catch( Test::Exception e)  {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) subscriber_main() - ")
      ACE_TEXT("Test exception caught during processing: %s.\n"),
      e.what()
    ));

  }

  return result;
}

