// -*- C++ -*-

#include "Publisher.h"
#include "Test.h"
#include "Options.h"
#include "dds/DCPS/Service_Participant.h"
#include "dds/DCPS/transport/framework/TransportExceptions.h"

#include <sstream>

int ACE_TMAIN(int argc, ACE_TCHAR* argv[])
{
  try {
    // Initialize DDS.
    DDS::DomainParticipantFactory_var dpf =
      TheParticipantFactoryWithArgs( argc, argv);

    // Initialize the test.
    const Test::Options options( argc, argv);

    // Create the publisher thingie.
    Test::Publisher publisher( options);

    if( options.verbose()) {
      std::stringstream buffer;
      buffer << options.transportType();
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) publisher_main() - ")
        ACE_TEXT("started with transport %C(%C).\n"),
        buffer.str().c_str(),
        options.transportKey().c_str()
      ));
    }

    // Execute the test.
    publisher.run();

  } catch( CORBA::Exception& /* e */) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) publisher_main() - ")
      ACE_TEXT("CORBA exception caught during processing.\n")
    ));
    return 1;

  } catch (const Test::Exception& e)  {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) publisher_main() - ")
      ACE_TEXT("Test exception caught during processing: %C.\n"),
      e.what()
    ));
    return 1;

  } catch (const OpenDDS::DCPS::Transport::MiscProblem &) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) publisher_main() - ")
      ACE_TEXT("Transport::MiscProblem exception caught during processing.\n")
    ));
    return 1;
  }

  return 0;
}
