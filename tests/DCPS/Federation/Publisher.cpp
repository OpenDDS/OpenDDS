
#include "Publisher.h"
#include "DataWriterListenerImpl.h"
#include "TestException.h"
#include "tests/DCPS/FooType5/FooDefTypeSupportC.h"
#include "tests/DCPS/FooType5/FooDefTypeSupportImpl.h"
#include "dds/DCPS/Marked_Default_Qos.h"
#include "dds/DCPS/transport/framework/TransportImpl.h"
#include "ace/SString.h"
#include "ace/OS_NS_unistd.h"
#include "tests/Utils/ExceptionStreams.h"

#include <set>

/**
 * @brief Construct a test system from the command line.
 */
Publisher::Publisher( int argc, ACE_TCHAR** argv, char** envp)
 : config_( argc, argv, envp)
{
  // Grab a local reference to the factory
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) INFO: initializing the publisher.\n")));
  ::DDS::DomainParticipantFactory_var factory = TheParticipantFactoryWithArgs( argc, argv);

  if( OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) COMMANDLINE: Verbose == %C\n")
      ACE_TEXT("(%P|%t) COMMANDLINE: Samples == %d\n"),
      (this->config_.verbose()? "true": "false"),
      this->config_.samples()
      ));
  }

#if 1
    TheServiceParticipant->monitor_factory_->initialize();
#endif


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
                         ::DDS::DomainParticipantListener::_nil(),
                         ::OpenDDS::DCPS::DEFAULT_STATUS_MASK
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

  //
  // Establish the Type Support for the data
  //

  if( OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) INFO: Installing type %C support into domain %d.\n"),
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
      ACE_TEXT("(%P|%t) ERROR: Unable to install type %C support for domain %d.\n"),
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
                   ::DDS::TopicListener::_nil(),
                   ::OpenDDS::DCPS::DEFAULT_STATUS_MASK
                 );
  if( CORBA::is_nil( this->topic_.in()) ) {
    ACE_ERROR ((LM_ERROR,
      ACE_TEXT ("(%P|%t) ERROR: Failed to create topic %C for publisher.\n"),
      this->config_.topicName().c_str()
    ));
    throw BadTopicException ();
  }

  //
  // Establish the Publisher
  //

  this->publisher_ = this->participant_->create_publisher(
                       PUBLISHER_QOS_DEFAULT,
                       ::DDS::PublisherListener::_nil(),
                       ::OpenDDS::DCPS::DEFAULT_STATUS_MASK
                     );
  if( CORBA::is_nil (this->publisher_.in ())) {
    ACE_ERROR ((LM_ERROR, ACE_TEXT ("(%P|%t) ERROR: Failed to create_publisher.\n")));
    throw BadPublisherException ();
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
        this->listener_.in(),
        ::OpenDDS::DCPS::DEFAULT_STATUS_MASK
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

  dataWriter_ = NULL;
  publisher_ = NULL;
  topic_ = NULL;

  // Release the participant
  if( 0 == CORBA::is_nil( this->participant_.in())) {
    DDS::DomainParticipantFactory_var dpf = TheParticipantFactory;
    if( ::DDS::RETCODE_PRECONDITION_NOT_MET
         == this->participant_->delete_contained_entities()
      ) {
      ACE_ERROR ((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: Unable to release participant resources.\n")
      ));

    } else if( ::DDS::RETCODE_PRECONDITION_NOT_MET
               == dpf->delete_participant( this->participant_.in())
             ) {
      ACE_ERROR ((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: Unable to release the participant.\n")
      ));
    }
  }


  // Release any remaining resources held for the service.
  if (OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) INFO: finalizing DCPS service.\n")));
  }
  TheServiceParticipant->shutdown();
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

    if (this->config_.sample_interval() > 0)
    {
      ACE_OS::sleep (this->config_.sample_interval());
    }
  }

  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT("(%P|%t) Publisher::run writes complete.\n")
  ));

  // End when the subscriber disconnects.
  //this->sync_->wait_for_completion();

  std::cout << "Publisher waiting for subscriber to exit" << std::endl;
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
  std::cout << "Publisher exiting" << std::endl;
}

