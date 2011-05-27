// -*- C++ -*-
// $Id$

#include "Test.h"
#include "Options.h"
#include "Publisher.h"
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

    // Create the publisher thingie.
    Test::Publisher publisher( options);

    if( OpenDDS::DCPS::DCPS_debug_level > 0) {
      std::stringstream buffer;
      buffer << options.transportType();
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) publisher_main() - ")
        ACE_TEXT("started with transport %s(%d).\n"),
        buffer.str().c_str(),
        options.transportKey()
      ));
    }

    // Execute the test.
    publisher.run();

  } catch( CORBA::Exception& e) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) publisher_main() - ")
      ACE_TEXT("CORBA exception caught during processing.\n")
    ));
    return 1;

  } catch( Test::Exception e)  {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) publisher_main() - ")
      ACE_TEXT("Test exception caught during processing: %s.\n"),
      e.what()
    ));
    return 1;

  }

  return 0;
}

