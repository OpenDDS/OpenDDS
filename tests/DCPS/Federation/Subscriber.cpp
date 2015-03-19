
#include "Subscriber.h"
#include "DataReaderListener.h"
#include "TestException.h"
#include "tests/DCPS/FooType5/FooDefTypeSupportC.h"
#include "tests/DCPS/FooType5/FooDefTypeSupportImpl.h"
#include "dds/DCPS/Marked_Default_Qos.h"
#include "dds/DCPS/transport/framework/TransportImpl.h"
#include "ace/SString.h"

/**
 * @brief Construct a test system from the command line.
 */
Subscriber::Subscriber( int argc, ACE_TCHAR** argv, char** envp)
 : config_( argc, argv, envp),
   sync_( 0)
{
  // Grab a local reference to the factory
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) INFO: initializing the subscriber.\n")));
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
      ACE_TEXT("(%P|%t) INFO: creating subscriber participant in domain %d.\n"),
      this->config_.domain()
    ));
  }
  this->participant_ = factory->create_participant(
                         this->config_.domain(),
                         PARTICIPANT_QOS_DEFAULT,
                         ::DDS::DomainParticipantListener::_nil(),
                         ::OpenDDS::DCPS::DEFAULT_STATUS_MASK
                       );
  if( CORBA::is_nil (this->participant_.in())) {
    ACE_ERROR ((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: create_participant failed for ")
      ACE_TEXT("subscriber in domain %d.\n"),
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
  // Establish the listener
  //

  if( OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) INFO: creating data reader listener for domain %d.\n"),
      this->config_.domain()
    ));
  }

  this->sync_     = new DataReaderListenerImpl( this->config_.samples());
  this->listener_ = this->sync_;
  if( CORBA::is_nil (this->listener_.in())) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT ("(%P|%t) ERROR: failed to obtain listener for domain %d.\n"),
      this->config_.domain()
    ));
    throw BadReaderListenerException ();
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
  ::Xyz::FooNoKeyTypeSupportImpl* subscriber_data = new ::Xyz::FooNoKeyTypeSupportImpl();
  if(::DDS::RETCODE_OK != subscriber_data->register_type(
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
      ACE_TEXT ("(%P|%t) ERROR: Failed to create topic %C for subscriber.\n"),
      this->config_.topicName().c_str()
    ));
    throw BadTopicException ();
  }

  //
  // Establish the Subscriber
  //

  this->subscriber_ = this->participant_->create_subscriber(
                        SUBSCRIBER_QOS_DEFAULT,
                        ::DDS::SubscriberListener::_nil(),
                        ::OpenDDS::DCPS::DEFAULT_STATUS_MASK
                      );
  if( CORBA::is_nil (this->subscriber_.in ())) {
    ACE_ERROR ((LM_ERROR, ACE_TEXT ("(%P|%t) ERROR: Failed to create_subscriber.\n")));
    throw BadSubscriberException ();
  }


  //
  // Establish and install the DataReader
  //

  if( OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) INFO: creating data reader.\n")));
  }

  ::DDS::TopicDescription_var description
    = this->participant_->lookup_topicdescription(
        this->config_.topicName().c_str()
      );

  this->dataReader_
    = this->subscriber_->create_datareader(
        description.in(),
        DATAREADER_QOS_DEFAULT,
        this->listener_.in (),
        ::OpenDDS::DCPS::DEFAULT_STATUS_MASK
      );
  if( CORBA::is_nil( this->dataReader_.in())) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: create datareader failed.\n")
    ));
    throw BadReaderException ();
  }
}

Subscriber::~Subscriber()
{
  if( OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) INFO: finalizing the subscriber.\n")));
  }

  dataReader_ = NULL;
  subscriber_ = NULL;
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
  if( OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) INFO: finalizing DCPS service.\n")));
  }
  TheServiceParticipant->shutdown ();
}

void
Subscriber::run()
{
  //
  // Wait until we receive the final message at the last receiver, then
  // we are done.
  //
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Subscriber::run about to wait.\n")));
  this->sync_->waitForCompletion();

  ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Subscriber::run done processing.\n")));
}

