// -*- C++ -*-

#include "Options.h"
#include "Test.h"
#include "Subscriber.h"
#include "dds/DCPS/Service_Participant.h"

#include <sstream>

int ACE_TMAIN(int argc, ACE_TCHAR *argv[])
{
  int result = -1;

  try {
    // Initialize DDS.
    DDS::DomainParticipantFactory_var dpf =
      TheParticipantFactoryWithArgs( argc, argv);

    // Initialize the test.
    const Test::Options options( argc, argv);

    // Create the subscriber thingie.
    Test::Subscriber subscriber( options);

    if( options.verbose()) {
      std::stringstream buffer;
      buffer << options.transportType();
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) subscriber_main() - ")
        ACE_TEXT("started with transport %C(%d).\n"),
        buffer.str().c_str(),
        options.transportKey()
      ));
    }

    // Execute the test.
    subscriber.run();

    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) subscriber_main() - ")
      ACE_TEXT("test over after receiving %d samples.\n"),
      subscriber.count()
    ));

    // Test passes if all the low priority samples are recieved in order
    // and if the high priority sample is received before the low priority
    // sample that was sent immediately before it
    if( subscriber.passed()) {
      result = 0;
    }

  } catch( CORBA::Exception& /* e */) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) subscriber_main() - ")
      ACE_TEXT("CORBA exception caught during processing.\n")
    ));

  } catch (const Test::Exception& e)  {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) subscriber_main() - ")
      ACE_TEXT("Test exception caught during processing: %C.\n"),
      e.what()
    ));

  }

  return result;
}

