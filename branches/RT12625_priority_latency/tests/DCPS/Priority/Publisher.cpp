// -*- C++ -*-
// $Id$

#include "TestTypeSupportImpl.h"
#include "DataWriterListener.h"
#include "Options.h"

#include "dds/DCPS/Service_Participant.h"
#include "dds/DCPS/Marked_Default_Qos.h"
#include "dds/DCPS/PublisherImpl.h"
#include "dds/DCPS/transport/framework/TheTransportFactory.h"
#include "dds/DCPS/transport/simpleTCP/SimpleTcpConfiguration.h"
#include "dds/DCPS/transport/simpleUnreliableDgram/SimpleUdpConfiguration.h"
#include "dds/DCPS/transport/simpleUnreliableDgram/SimpleMcastConfiguration.h"
#include "dds/DCPS/transport/ReliableMulticast/ReliableMulticastTransportConfiguration.h"

#ifdef ACE_AS_STATIC_LIBS
#include "dds/DCPS/transport/simpleTCP/SimpleTcp.h"
#include "dds/DCPS/transport/simpleUnreliableDgram/SimpleUnreliableDgram.h"
#include "dds/DCPS/transport/ReliableMulticast/ReliableMulticast.h"
#endif

#include <sstream>

namespace { // Anonymous namespace for file scope.

  /// Transport keys linking to ini file configurations.
  enum {
    TCP_KEY = 1,
    UDP_KEY = 2,
    MC_KEY  = 3,
    RMC_KEY = 4
  };

  /// DDS Domain for data traffic.
  enum {
    USER_DOMAIN = 21
  };

} // End of anonymous namespace.

class Publisher {
  public:
    /// Construct with option information.
    Publisher( const Options& options);

    /// Destructor.
    ~Publisher();

    /// Execute the test.
    void run();

  private:
    /// Test options.
    const Options& options_;

    /// Test transport.
    OpenDDS::DCPS::TransportImpl_rch transport_;

    /// DomainParticipant.
    DDS::DomainParticipant_var participant_;
};

Publisher::~Publisher()
{
  if( ! CORBA::is_nil( this->participant_.in())) {
    this->participant_->delete_contained_entities();
    TheParticipantFactory->delete_participant( this->participant_.in());
  }
  TheTransportFactory->release();
  TheServiceParticipant->shutdown();
}

Publisher::Publisher( const Options& options)
 : options_( options)
{
  // Tailor the transport creation.
  OpenDDS::DCPS::TransportIdType transportKey = 0;
  std::string transportLib;
  switch( this->options_.transportType()) {
    case Options::TCP: transportKey = TCP_KEY; break;
    case Options::UDP: transportKey = UDP_KEY; break;
    case Options::MC:  transportKey = MC_KEY;  break;
    case Options::RMC: transportKey = RMC_KEY; break;

    default:
    case Options::NONE:
      transportKey = TCP_KEY;
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) Publisher::Publisher() - ")
        ACE_TEXT("unrecognized transport type to create, creating TCP.\n")
      ));
      break;
  }

  // Perform the transport creation.
  this->transport_
    = TheTransportFactory->create_transport_impl( transportKey, OpenDDS::DCPS::AUTO_CONFIG);

  // Create the DomainParticipant
  this->participant_
    = TheParticipantFactory->create_participant(
        USER_DOMAIN,
        PARTICIPANT_QOS_DEFAULT,
        DDS::DomainParticipantListener::_nil()
      );
  if( CORBA::is_nil( this->participant_.in())) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: Publisher::Publisher() - ")
      ACE_TEXT("failed to create a participant.\n")
    ));
    throw /* SOMETHING */;
  }

}

void
Publisher::run()
{
  ; // Execute the test.
}

int
main( int argc, char *argv[])
{
  try {
    // Initialize DDS.
    TheParticipantFactoryWithArgs( argc, argv);

    // Initialize the test.
    const Options options( argc, argv);

    // Create the publisher thingie.
    Publisher publisher( options);

    if( OpenDDS::DCPS::DCPS_debug_level > 0) {
      std::stringstream buffer;
      buffer << options.transportType();
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) publisher main() - ")
        ACE_TEXT("started with transport %s, id %d.\n"),
        buffer.str().c_str(),
        options.publisherId()
      ));
    }

    // Execute the test.
    publisher.run();

    ACE_OS::exit( 1); // Force a failure until we finish coding.

  } catch( CORBA::Exception& e) {
    return 1;
  }

  return 0;
}
