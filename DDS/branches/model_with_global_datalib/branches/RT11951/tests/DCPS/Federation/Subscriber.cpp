
#include "Subscriber.h"
#include "DataReaderListener.h"
#include "TestException.h"
#include "tests/DCPS/FooType5/FooNoKeyTypeSupportC.h"
#include "tests/DCPS/FooType5/FooNoKeyTypeSupportImpl.h"
#include "dds/DCPS/Marked_Default_Qos.h"
#include "dds/DCPS/transport/framework/TheTransportFactory.h"
#include "dds/DCPS/transport/framework/TransportImpl.h"
#include "dds/DCPS/transport/simpleTCP/SimpleTcpConfiguration.h"
#include "ace/SString.h"

/**
 * @brief Construct a test system from the command line.
 */
Subscriber::Subscriber( int argc, char** argv, char** envp)
 : config_( argc, argv, envp),
   sync_( 0)
{
  // Grab a local reference to the factory
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) INFO: initializing the subscriber.\n")));
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
  this->participant_ = factory->create_participant(
                         this->config_.domain(),
                         PARTICIPANT_QOS_DEFAULT,
                         ::DDS::DomainParticipantListener::_nil()
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

  if( OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) INFO: creating a SimpleTCP transport.\n")));
  }
  this->transport_
    = TheTransportFactory->create_transport_impl(
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
      ACE_TEXT("(%P|%t) INFO: Installing type %s support into domain %d.\n"),
      this->config_.typeName().c_str(),
      this->config_.domain()
    ));
  }
  ::Xyz::FooNoKeyTypeSupportImpl* subscriber_data = new ::Xyz::FooNoKeyTypeSupportImpl();
  if(::DDS::RETCODE_OK != subscriber_data->register_type(
                            this->participant_,
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
      ACE_TEXT ("(%P|%t) ERROR: Failed to create topic %s for subscriber.\n"),
      this->config_.topicName().c_str()
    ));
    throw BadTopicException ();
  }

  //
  // Establish the Subscriber
  //

  this->subscriber_ = this->participant_->create_subscriber(
                        SUBSCRIBER_QOS_DEFAULT,
                        ::DDS::SubscriberListener::_nil()
                      );
  if( CORBA::is_nil (this->subscriber_.in ())) {
    ACE_ERROR ((LM_ERROR, ACE_TEXT ("(%P|%t) ERROR: Failed to create_subscriber.\n")));
    throw BadSubscriberException ();
  }

  // Attach the subscriber to the transport.
  OpenDDS::DCPS::SubscriberImpl* sub_impl
    = dynamic_cast<OpenDDS::DCPS::SubscriberImpl*>( this->subscriber_.in());
  if( 0 == sub_impl) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: Failed to obtain subscriber servant\n")));
    throw BadSubscriberException ();
  }

  OpenDDS::DCPS::AttachStatus attach_status;

  if( OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) INFO: attaching subscriber to transport \n")));
  }
  attach_status = sub_impl->attach_transport( this->transport_.in());

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
        this->listener_.in ()
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

  // Release the participant
  if( 0 == CORBA::is_nil( this->participant_)) {
    if( ::DDS::RETCODE_PRECONDITION_NOT_MET
         == this->participant_->delete_contained_entities()
      ) {
      ACE_ERROR ((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: Unable to release participant resources.\n")
      ));

    } else if( ::DDS::RETCODE_PRECONDITION_NOT_MET
               == TheParticipantFactory->delete_participant( this->participant_)
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

  // Release all the transport resources.
  if( OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) INFO: finalizing transport.\n")));
  }
  TheTransportFactory->release();
}

void
Subscriber::run()
{
  // NOTE: This is a kluge to avoid a race condition - it is still
  //       possible, though unlikely, to lock up due to the race.
  ACE_OS::sleep(5);

  //
  // Wait until we receive the final message at the last receiver, then
  // we are done.
  //
  if( OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Subscriber::run about to wait.\n")));
  }
  this->sync_->waitForCompletion();

  if( OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Subscriber::run done processing.\n")));
  }
}

