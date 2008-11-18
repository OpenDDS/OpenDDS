// -*- C++ -*-
// $Id$

#include "Publisher.h"

#include "Test.h"
#include "Options.h"

#include "TestTypeSupportImpl.h"
#include "DataWriterListener.h"

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

namespace Test {

Publisher::~Publisher()
{
  if( ! CORBA::is_nil( this->participant_.in())) {
    this->participant_->delete_contained_entities();
    TheParticipantFactory->delete_participant( this->participant_.in());
  }
  TheTransportFactory->release();
  TheServiceParticipant->shutdown();

  // delete this->listener_;
}

Publisher::Publisher( const Options& options)
 : options_( options),
   listener_( 0)
{
  // Create the DomainParticipant
  this->participant_
    = TheParticipantFactory->create_participant(
        this->options_.domain(),
        PARTICIPANT_QOS_DEFAULT,
        DDS::DomainParticipantListener::_nil()
      );
  if( CORBA::is_nil( this->participant_.in())) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: Publisher::Publisher() - ")
      ACE_TEXT("failed to create a participant.\n")
    ));
    throw BadParticipantException();

  } else if( ::OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) Publisher::Publisher() - ")
      ACE_TEXT("created participant in domain %d.\n"),
      this->options_.domain()
    ));
  }

  // Create the transport.
  this->transport_
    = TheTransportFactory->create_transport_impl(
        this->options_.transportKey(),
        OpenDDS::DCPS::AUTO_CONFIG
      );
  if( this->transport_.is_nil()) {
    std::stringstream buffer;
    buffer << this->options_.transportType();
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: Publisher::Publisher() - ")
      ACE_TEXT("failed to create %s transport.\n"),
      buffer.str().c_str()
    ));
    throw BadTransportException();

  } else if( ::OpenDDS::DCPS::DCPS_debug_level > 0) {
    std::stringstream buffer;
    buffer << this->options_.transportType();
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) Publisher::Publisher() - ")
      ACE_TEXT("created %s transport.\n"),
      buffer.str().c_str()
    ));
  }

  // Create the listener.
  this->listener_ = new DataWriterListener();
  if( ::OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) Publisher::Publisher() - ")
      ACE_TEXT("created writer listener.\n")
    ));
  }

  // Create and register the type support.
  DataTypeSupportImpl* testData = new DataTypeSupportImpl();
  if( ::DDS::RETCODE_OK
   != testData->register_type( this->participant_.in(), 0)) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: Publisher::Publisher() - ")
      ACE_TEXT("unable to install type %s support.\n"),
      testData->get_type_name()
    ));
    throw BadTypeSupportException ();

  } else if( ::OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) Publisher::Publisher() - ")
      ACE_TEXT("created type %s support.\n"),
      testData->get_type_name()
    ));
  }

  // Create the topic.
  this->topic_ = this->participant_->create_topic(
                   this->options_.topicName().c_str(),
                   testData->get_type_name(),
                   TOPIC_QOS_DEFAULT,
                   ::DDS::TopicListener::_nil()
                 );
  if( CORBA::is_nil( this->topic_.in())) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: Publisher::Publisher() - ")
      ACE_TEXT("failed to create topic %s.\n"),
      this->options_.topicName().c_str()
    ));
    throw BadTopicException();

  } else if( ::OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) Publisher::Publisher() - ")
      ACE_TEXT("created topic %s.\n"),
      this->options_.topicName().c_str()
    ));
  }

  // Create the publisher.
  this->publisher_ = this->participant_->create_publisher(
                       PUBLISHER_QOS_DEFAULT,
                       ::DDS::PublisherListener::_nil()
                     );
  if( CORBA::is_nil( this->publisher_.in())) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: Publisher::Publisher() - ")
      ACE_TEXT("failed to create publisher.\n")
    ));
    throw BadPublisherException();

  } else if( ::OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) Publisher::Publisher() - ")
      ACE_TEXT("created publisher.\n")
    ));
  }

  // Attach the transport to the publisher.
  ::OpenDDS::DCPS::PublisherImpl* servant
    = dynamic_cast< ::OpenDDS::DCPS::PublisherImpl*>( this->publisher_.in());
  if( 0 == servant) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: Publisher::Publisher() - ")
      ACE_TEXT("failed to narrow publisher servant.\n")
    ));
    throw BadServantException();
  }

  if( ::OpenDDS::DCPS::ATTACH_OK
   != servant->attach_transport( this->transport_.in())) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: Publisher::Publisher() - ")
      ACE_TEXT("failed to attach transport to publisher.\n")
    ));
    throw BadAttachException();

  } else if( ::OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) Publisher::Publisher() - ")
      ACE_TEXT("attached transport to publisher.\n")
    ));
  }

  // Writer Qos policy values.
  ::DDS::DataWriterQos writerQos;
  this->publisher_->get_default_datawriter_qos( writerQos);

  writerQos.durability.kind                          = ::DDS::TRANSIENT_LOCAL_DURABILITY_QOS;
  writerQos.history.kind                             = ::DDS::KEEP_ALL_HISTORY_QOS;
  writerQos.resource_limits.max_samples_per_instance = ::DDS::LENGTH_UNLIMITED;
  writerQos.reliability.max_blocking_time.sec        = 0;
  writerQos.reliability.max_blocking_time.nanosec    = 0;

  // Reliability varies with the transport implementation.
  switch( this->options_.transportType()) {
    case Options::TCP:
    case Options::RMC:
      writerQos.reliability.kind = ::DDS::RELIABLE_RELIABILITY_QOS;
      break;

    case Options::UDP:
    case Options::MC:
      writerQos.reliability.kind = ::DDS::BEST_EFFORT_RELIABILITY_QOS;
      break;

    case Options::NONE:
    default:
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: Publisher::Publisher() - ")
        ACE_TEXT("unrecognized transport when setting up Qos policies.\n")
      ));
      throw BadQosException();
  }

  // Actually set the priority finally.
  writerQos.transport_priority.value = this->options_.priority();

  // Create the writer.
  this->writer_ = this->publisher_->create_datawriter(
                    this->topic_.in(),
                    writerQos,
                    this->listener_
                  );
  if( CORBA::is_nil( this->writer_.in())) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: Publisher::Publisher() - ")
      ACE_TEXT("failed to create writer.\n")
    ));
    throw BadWriterException();

  } else if( ::OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) Publisher::Publisher() - ")
      ACE_TEXT("created writer.\n")
    ));
  }

}

void
Publisher::run()
{
  throw Test::Exception(); // Execute the test.
}

} // End of namespace Test

