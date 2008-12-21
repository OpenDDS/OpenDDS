
#include "Publisher.h"
#include "DataWriterListenerImpl.h"
#include "TestException.h"
#include "tests/DCPS/FooType5/FooDefTypeSupportC.h"
#include "tests/DCPS/FooType5/FooDefTypeSupportImpl.h"
#include "dds/DCPS/Marked_Default_Qos.h"
#include "dds/DCPS/transport/framework/TheTransportFactory.h"
#include "dds/DCPS/transport/framework/TransportImpl.h"
#include "dds/DCPS/transport/simpleTCP/SimpleTcpConfiguration.h"
#include "ace/SString.h"

#include <set>

/**
 * @brief Construct a test system from the command line.
 */
Publisher::Publisher( int argc, char** argv, char** envp)
 : config_( argc, argv, envp)
{
  // Grab a local reference to the factory
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) INFO: initializing the publisher.\n")));
  ::DDS::DomainParticipantFactory_var factory = TheParticipantFactoryWithArgs( argc, argv);

  if( OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) COMMANDLINE: Verbose == %s\n")
      ACE_TEXT("(%P|%t) COMMANDLINE: Samples == %d\n"),
      (this->config_.verbose()? "true": "false"),
      this->config_.samples()
      ));
  }

  //
  // Establish DomainParticipant
  //
  if( OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) INFO: creating publisher participant in domain %d.\n"),
      this->config_.domain()
    ));
  }
  this->participant_ = factory->create_participant(
                         this->config_.domain(),
                         PARTICIPANT_QOS_DEFAULT,
                         ::DDS::DomainParticipantListener::_nil()
                       );
  if( CORBA::is_nil( this->participant_.in())) {
    ACE_ERROR ((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: create_participant failed for ")
      ACE_TEXT("publisher in domain %d.\n"),
      this->config_.domain()
    ));
    throw BadParticipantException ();
  }

  //
  // Grab and install the transport implementation.
  //

  // Establish debug level.
  if( this->config_.verbose()) {
    TURN_ON_VERBOSE_DEBUG;
  }

  if( OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) INFO: creating a SimpleTCP transport.\n")));
  }
  this->transport_ = TheTransportFactory->create_transport_impl(
                       0,
                       "SimpleTcp",
                       OpenDDS::DCPS::DONT_AUTO_CONFIG
                     );

  OpenDDS::DCPS::TransportConfiguration_rch transport_config
    = TheTransportFactory->create_configuration(
        0,
        ACE_TString("SimpleTcp")
      );

#if 0
  OpenDDS::DCPS::SimpleTcpConfiguration* tcp_config
    = static_cast <OpenDDS::DCPS::SimpleTcpConfiguration*>( transport_config.in() );

  std::string address;
  if( address.length() > 0) {
    ACE_INET_Addr reader_address( address.c_str());
    tcp_config->local_address_ = reader_address;
  }
#endif

  if( this->transport_->configure( transport_config.in()) != 0) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: TCP ")
      ACE_TEXT("failed to configure the transport.\n")
    ));
    throw BadTransportException ();
  }

  //
  // Establish the Type Support for the data
  //

  if( OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) INFO: Installing type %s support into domain %d.\n"),
      this->config_.typeName().c_str(),
      this->config_.domain()
    ));
  }
  ::Xyz::FooNoKeyTypeSupportImpl* publisher_data = new ::Xyz::FooNoKeyTypeSupportImpl();
  if(::DDS::RETCODE_OK != publisher_data->register_type(
                            this->participant_.in(),
                            this->config_.typeName().c_str()
                          )
    ) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: Unable to install type %s support for domain %d.\n"),
      this->config_.typeName().c_str(),
      this->config_.domain()
    ));
    throw TestException ();
  }

  //
  // Establish the Topic
  //

  this->topic_ = this->participant_->create_topic(
                   this->config_.topicName().c_str(),
                   this->config_.typeName().c_str(),
                   TOPIC_QOS_DEFAULT,
                   ::DDS::TopicListener::_nil()
                 );
  if( CORBA::is_nil( this->topic_.in()) ) {
    ACE_ERROR ((LM_ERROR,
      ACE_TEXT ("(%P|%t) ERROR: Failed to create topic %s for publisher.\n"),
      this->config_.topicName().c_str()
    ));
    throw BadTopicException ();
  }

  //
  // Establish the Publisher
  //

  this->publisher_ = this->participant_->create_publisher(
                       PUBLISHER_QOS_DEFAULT,
                       ::DDS::PublisherListener::_nil()
                     );
  if( CORBA::is_nil (this->publisher_.in ())) {
    ACE_ERROR ((LM_ERROR, ACE_TEXT ("(%P|%t) ERROR: Failed to create_publisher.\n")));
    throw BadPublisherException ();
  }

  // Attach the publisher to the transport.
  OpenDDS::DCPS::PublisherImpl* pub_impl
    = dynamic_cast<OpenDDS::DCPS::PublisherImpl*>(
        this->publisher_.in()
      );
  if (0 == pub_impl) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: Failed to obtain publisher servant\n")));
    throw BadPublisherException ();
  }

  OpenDDS::DCPS::AttachStatus attach_status;

  if( OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) INFO: attaching publisher to transport \n")));
  }
  attach_status = pub_impl->attach_transport( this->transport_.in());

  if (attach_status != OpenDDS::DCPS::ATTACH_OK) {
    // We failed to attach to the transport for some reason.
    ACE_TString status_str;

    switch (attach_status) {
      case OpenDDS::DCPS::ATTACH_BAD_TRANSPORT:
        status_str = "ATTACH_BAD_TRANSPORT";
        break;
      case OpenDDS::DCPS::ATTACH_ERROR:
        status_str = "ATTACH_ERROR";
        break;
      case OpenDDS::DCPS::ATTACH_INCOMPATIBLE_QOS:
        status_str = "ATTACH_INCOMPATIBLE_QOS";
        break;
      default:
        status_str = "Unknown Status";
        break;
    }
    ACE_ERROR ((LM_ERROR,
                ACE_TEXT("(%P|%t) ERROR: Failed to attach to the transport. ")
                ACE_TEXT("AttachStatus == %s\n"),
                status_str.c_str()));
    throw BadTransportException ();
  }

  //
  // Establish and install the DataWriter.
  //

  if( OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) INFO: creating datawriter.\n")));
  }

  //
  // Keep all data samples to allow us to establish connections in an
  // arbitrary order, with samples being buffered at the first writer
  // that has not yet had a subscription match.
  //
  ::DDS::DataWriterQos writerQos;
  this->publisher_->get_default_datawriter_qos( writerQos);

  writerQos.durability.kind                          = ::DDS::TRANSIENT_LOCAL_DURABILITY_QOS;
  writerQos.history.kind                             = ::DDS::KEEP_ALL_HISTORY_QOS;
  writerQos.resource_limits.max_samples_per_instance = ::DDS::LENGTH_UNLIMITED;
  writerQos.reliability.kind                         = ::DDS::RELIABLE_RELIABILITY_QOS;
  writerQos.reliability.max_blocking_time.sec        = 0;
  writerQos.reliability.max_blocking_time.nanosec    = 0;

  this->sync_     = new DataWriterListenerImpl;
  this->listener_ = this->sync_;
  if( CORBA::is_nil (this->listener_.in())) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT ("(%P|%t) ERROR: failed to obtain listener for domain %d.\n"),
      this->config_.domain()
    ));
    throw BadWriterListenerException ();
  }

  this->dataWriter_
    = this->publisher_->create_datawriter(
        this->topic_.in(),
        writerQos,
        this->listener_.in()
      );
  if( CORBA::is_nil( this->dataWriter_.in()) ) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: create datawriter failed.\n")));
    throw BadWriterException ();
  }
}

Publisher::~Publisher()
{
  if( OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) INFO: finalizing the publisher.\n")));
  }

  // Release the participant
  if( 0 == CORBA::is_nil( this->participant_.in())) {
    if( ::DDS::RETCODE_PRECONDITION_NOT_MET
         == this->participant_->delete_contained_entities()
      ) {
      ACE_ERROR ((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: Unable to release participant resources.\n")
      ));

    } else if( ::DDS::RETCODE_PRECONDITION_NOT_MET
               == TheParticipantFactory->delete_participant( this->participant_.in())
             ) {
      ACE_ERROR ((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: Unable to release the participant.\n")
      ));
    }
  }


  // Release all the transport resources.
  if( OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) INFO: finalizing transport.\n")));
  }
  TheTransportFactory->release();

  // Release any remaining resources held for the service.
  if( OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) INFO: finalizing DCPS service.\n")));
  }
  TheServiceParticipant->shutdown ();
}

void
Publisher::run()
{
  ::DDS::InstanceHandleSeq handles;

  while (1)
  {
    dataWriter_->get_matched_subscriptions(handles);
    if (handles.length() > 0)
      break;
    else
      ACE_OS::sleep(ACE_Time_Value(0,200000));
  }

  // Write Foo samples.
  ::Xyz::FooNoKey foo;
  foo.data_source = 22;

  // Write into the datawriter
  ::Xyz::FooNoKeyDataWriter_var fooWriter
    = ::Xyz::FooNoKeyDataWriter::_narrow( this->dataWriter_.in());
  if( CORBA::is_nil( fooWriter.in())) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) Publisher failed to narrow writer.\n")
    ));
    throw BadWriterException();
  }

  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT("(%P|%t) Publisher::run starting to write.\n")
  ));

  // Write the requested number of samples.
  for( int sample = 0; sample < this->config_.samples(); ++sample) {
    // Make the data unique for each sample.
    foo.x = 1000.0f + 100.0f * sample;
    foo.y = 2000.0f + 100.0f * sample;
    if( sample == (this->config_.samples() - 1)) {
      // Final sample is the answer that ends the universe.
      foo.data_source = 42;
    }

    // Go ahead and send the data.
    if( ::DDS::RETCODE_OK != fooWriter->write( foo, ::DDS::HANDLE_NIL)) {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) Publisher::run ")
        ACE_TEXT("failed to forward sample %d.\n"),
        sample
      ));
    }
  }

  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT("(%P|%t) Publisher::run writes complete.\n")
  ));

  // End when the subscriber disconnects.
  //this->sync_->wait_for_completion();

  while (1)
    {
      dataWriter_->get_matched_subscriptions(handles);
      if (handles.length() == 0)
        break;
      else
        ACE_OS::sleep(1);
    }


  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT("(%P|%t) Publisher::run shutting down.\n")
  ));
}

