// -*- C++ -*-

#include "Subscriber.h"
#include "Test.h"
#include "Options.h"
#include "Commas.h"
#include "dds/DCPS/Service_Participant.h"
#include "dds/DCPS/transport/framework/TransportExceptions.h"

#include <iomanip>
#include <fstream>
#include <sstream>

int ACE_TMAIN(int argc, ACE_TCHAR* argv[])
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
        ACE_TEXT("started with transport %C(%C).\n"),
        buffer.str().c_str(),
        options.transportKey().c_str()
      ));
    }

    // Execute the test.
    subscriber.run();

    std::stringstream buffer;
    buffer.fill( '0');
    for( std::map< long, long>::const_iterator current = subscriber.counts().begin();
         current != subscriber.counts().end();
         ++current
       ) {
      // Utility iterator into data maps.
      std::map< long, long>::const_iterator where;

      // Find the priority for this writer.
      long priority = -1;
      where = subscriber.priorities().find( current->first);
      if( where != subscriber.priorities().end()) {
        priority = where->second;
      }

      // Find the byte count for this writer.
      long bytes = -1;
      where = subscriber.bytes().find( current->first);
      if( where != subscriber.bytes().end()) {
        bytes = where->second;
      }

      // Form the message for this writer.
      buffer << "   writer[ 0x" << std::hex << current->first << "] == "
             << Commas( current->second) << " samples / "
             << Commas( bytes) << " bytes recieved at priority "
             << std::dec << priority << "." << std::endl;

    }
    buffer << "Total messages received: " << std::dec << subscriber.total_messages() << std::endl;
    buffer << "Valid messages received: " << std::dec << subscriber.valid_messages() << std::endl;
    buffer << subscriber << std::endl;

    // Put any raw data out if indicated.
    if( !options.rawOutputFilename().empty()) {
      std::ofstream rawOutput( options.rawOutputFilename().c_str());
      subscriber.rawData( rawOutput);
    }

    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) subscriber_main() - ")
      ACE_TEXT("test over:\n%C"),
      buffer.str().c_str()
    ));

    // Test passes if there were no exceptions.
    result = 0;

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

  } catch (const OpenDDS::DCPS::Transport::MiscProblem &) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) subscriber_main() - ")
      ACE_TEXT("Transport::MiscProblem exception caught during processing.\n")
    ));
    return 1;
  }

  return result;
}

