// -*- C++ -*-
// $Id$

#include "Options.h"
#include "Test.h"
#include "Publisher.h"
#include "dds/DCPS/Service_Participant.h"

#include <sstream>

int ACE_TMAIN(int argc, ACE_TCHAR* argv[])
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
        ACE_TEXT("started with transport %C(%d), id %d.\n"),
        buffer.str().c_str(),
        options.transportKey(),
        options.publisherId()
      ));
    }

    // Execute the test.
    publisher.run();

    // this can only fail if ACE_HAS_PTHREADS is defined
    if (TheServiceParticipant->scheduler() != THR_SCHED_RR) {
      std::string scheduler;
      if (TheServiceParticipant->scheduler() == THR_SCHED_FIFO)
        scheduler = "THR_SCHED_FIFO";
      else if (TheServiceParticipant->scheduler() == THR_SCHED_DEFAULT)
        scheduler = "THR_SCHED_DEFAULT";
      else {
        std::stringstream buffer;
        buffer << TheServiceParticipant->scheduler();
        scheduler = "<Unkown scheduler val - " + buffer.str() + ">";
      }
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) publisher_main() - ")
        ACE_TEXT("scheduler should be THR_SCHDE_RR, but .\n")
        ACE_TEXT("instead it is %C.\n"),
        scheduler.c_str()
      ));
      return 1;
    }

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

  }

  return 0;
}
