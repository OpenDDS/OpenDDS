// -*- C++ -*-

#include "Publisher.h"
#include "Test.h"
#include "Options.h"
#include "dds/DCPS/Service_Participant.h"

#include <sstream>

int ACE_TMAIN( int argc, ACE_TCHAR *argv[])
{
  int result = 1;
  try {
    // Initialize DDS.
    DDS::DomainParticipantFactory_var dpf =
      TheParticipantFactoryWithArgs( argc, argv);

    // Initialize the test.
    const Test::Options options( argc, argv);

    if( options) {
      // Create the publisher thingie.
      Test::Publisher publisher( options);

      if( options.verbose()) {
        ACE_DEBUG((LM_DEBUG,
          ACE_TEXT("(%P|%t) publisher_main() - started.\n")
        ));
      }

      // Execute the test.
      publisher.run();

      // Gather the results.
      result = publisher.status();
    }

  } catch( CORBA::Exception& /* e */) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) publisher_main() - ")
      ACE_TEXT("CORBA exception caught during processing.\n")
    ));
    return 2;

  } catch(const Test::Exception& e)  {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) publisher_main() - ")
      ACE_TEXT("Test exception caught during processing: %C.\n"),
      e.what()
    ));
    return 3;

  }

  return result;
}

