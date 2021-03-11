
#include "TestSystem.h"
#include "DataWriterListenerImpl.h"
#include "TestException.h"
#include "tests/DCPS/FooType5/FooDefTypeSupportC.h"
#include "tests/DCPS/FooType5/FooDefTypeSupportImpl.h"
#include "dds/DCPS/SubscriberImpl.h"
#include "dds/DCPS/PublisherImpl.h"
#include "dds/DCPS/Marked_Default_Qos.h"
#include "dds/DCPS/transport/framework/TransportImpl.h"
#include "ace/SString.h"

/**
 * @brief Construct a test system from the command line.
 */
TestSystem::TestSystem( int argc, ACE_TCHAR** argv, char** envp)
 : config_( argc, argv, envp)
 , subscriberParticipant_(0)
 , publisherParticipant_(0)
 , subscriber_(0)
 , publisher_(0)
 , readerTopic_(0)
 , writerTopic_(0)
 , dataReader_(0)
 , dataWriter_(0)
 , listener_(0)
 , transport_()
{
  // Grab a local reference to the factory to ensure that we perform the
  // operations - they should not get optimized away!  Note that this
  // passes the arguments to the ORB initialization first, then strips
  // the DCPS arguments.
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) %T INFO: initializing the service.%d\n"), this->config_.infoRepoIorSize()));
  TheServiceParticipant->set_default_discovery(OpenDDS::DCPS::Discovery::DEFAULT_REPO);
  ::DDS::DomainParticipantFactory_var factory = TheParticipantFactoryWithArgs( argc, argv);

  // We only need to do loading and binding if we receive InfoRepo IOR
  // values on the command line - otherwise this is done during
  // configuration file processing.
  if( this->config_.infoRepoIorSize() > 0) {

    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) %T INFO: loading repository %C.\n"),
      OpenDDS::DCPS::Discovery::DEFAULT_REPO
    ));
    TheServiceParticipant->set_repo_ior( this->config_.infoRepoIor().c_str());

    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) %T INFO: binding subscriber domain to repository %C.\n"),
      OpenDDS::DCPS::Discovery::DEFAULT_REPO
    ));
    TheServiceParticipant->set_repo_domain(
      this->config_.subscriberDomain(),
      OpenDDS::DCPS::Discovery::DEFAULT_REPO
    );

    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) %T INFO: binding publisher domain to repository %C.\n"),
      OpenDDS::DCPS::Discovery::DEFAULT_REPO
    ));
    TheServiceParticipant->set_repo_domain(
      this->config_.publisherDomain(),
      OpenDDS::DCPS::Discovery::DEFAULT_REPO
    );

#if 1
    TheServiceParticipant->monitor_factory_->initialize();
#endif
  }

  //
  // Establish a DomainParticipant for the subscription domain.
  //

  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT("(%P|%t) %T INFO: creating subscriber participant in domain %d.\n"),
    this->config_.subscriberDomain()
  ));
  this->subscriberParticipant_ =
    factory->create_participant(
      this->config_.subscriberDomain(),
      PARTICIPANT_QOS_DEFAULT,
      ::DDS::DomainParticipantListener::_nil(),
      ::OpenDDS::DCPS::DEFAULT_STATUS_MASK
    );
  if (CORBA::is_nil (this->subscriberParticipant_.in ()))
    {
      ACE_ERROR ((LM_ERROR,
                ACE_TEXT("(%P|%t) %T ERROR: create_participant failed for subscriber.\n")));
      throw BadParticipantException ();
    }

  //
  // Establish a DomainParticipant for the publication domain.
  //

  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT("(%P|%t) %T INFO: creating publisher participant in domain %d.\n"),
    this->config_.publisherDomain()
  ));
  if( this->config_.publisherDomain() == this->config_.subscriberDomain()) {
    //
    // Just reference the same participant for both activities if they
    // are in the same domain.
    //
    this->publisherParticipant_
      = ::DDS::DomainParticipant::_duplicate(this->subscriberParticipant_.in());

  } else {
    this->publisherParticipant_ =
      factory->create_participant(
        this->config_.publisherDomain(),
        PARTICIPANT_QOS_DEFAULT,
        ::DDS::DomainParticipantListener::_nil(),
        ::OpenDDS::DCPS::DEFAULT_STATUS_MASK
      );
    if (CORBA::is_nil (this->publisherParticipant_.in ()))
      {
        ACE_ERROR ((LM_ERROR,
                  ACE_TEXT("(%P|%t) %T ERROR: create_participant failed for publisher.\n")));
        throw BadParticipantException ();
      }

  }

  //
  // Grab and install the transport implementation.
  // NOTE: Try to use a single transport first.  This is something that
  //       needs to be verified and documented.
  //

  // Establish debug level.
  if( this->config_.verbose()) {
    TURN_ON_VERBOSE_DEBUG;
  }

  //
  // Establish a listener to install into the DataReader.
  //

  ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) %T INFO: creating data reader listener.\n")));
  this->listener_ = new ForwardingListenerImpl(OpenDDS::DCPS::Discovery::DEFAULT_REPO);
  ForwardingListenerImpl* forwarder_servant =
    dynamic_cast<ForwardingListenerImpl*>(listener_.in());

  if (CORBA::is_nil (this->listener_.in ()))
    {
      ACE_ERROR ((LM_ERROR, ACE_TEXT ("(%P|%t) %T ERROR: listener is nil.\n")));
      throw BadReaderListenerException ();
    }

  //
  // Establish the Type Support for the data.
  // MJM: Do we need to have two distinct instantiations here?  Or can we
  //      share the servant across domains?
  //

  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT("(%P|%t) %T INFO: creating subscription type support for type %C.\n"),
    this->config_.typeName().c_str()
  ));
  ::Xyz::FooNoKeyTypeSupport_var subscriber_data = new ::Xyz::FooNoKeyTypeSupportImpl();
  if(::DDS::RETCODE_OK != subscriber_data->register_type(
                            this->subscriberParticipant_.in (),
                            this->config_.typeName().c_str()
                          )
    ) {
    ACE_ERROR ((LM_ERROR,
      ACE_TEXT ("(%P|%t) %T ERROR: Failed to register the subscriber FooNoKeyTypeSupport.\n")));
    throw TestException ();
  }

  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT("(%P|%t) %T INFO: creating publication type support for type %C.\n"),
    this->config_.typeName().c_str()
  ));
  ::Xyz::FooNoKeyTypeSupport_var publisher_data = new ::Xyz::FooNoKeyTypeSupportImpl();
  if(::DDS::RETCODE_OK != publisher_data->register_type(
                            this->publisherParticipant_.in (),
                            this->config_.typeName().c_str()
                          )
    ) {
    ACE_ERROR ((LM_ERROR,
      ACE_TEXT ("(%P|%t) %T ERROR: Failed to register the publisher FooNoKeyTypeSupport.\n")));
    throw TestException ();
  }

  //
  // Establish the Subscriber and Publisher Topics.
  //

  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT("(%P|%t) %T INFO: creating subscription topic: %C.\n"),
    this->config_.readerTopicName().c_str()
  ));
  this->readerTopic_
    = this->subscriberParticipant_->create_topic(
        this->config_.readerTopicName().c_str(),
        this->config_.typeName().c_str(),
        TOPIC_QOS_DEFAULT,
        ::DDS::TopicListener::_nil(),
        ::OpenDDS::DCPS::DEFAULT_STATUS_MASK
      );
  if( CORBA::is_nil( this->readerTopic_.in()) )
    {
      ACE_ERROR ((LM_ERROR,
        ACE_TEXT ("(%P|%t) %T ERROR: Failed to create_topic for subscriber.\n")));
      throw BadTopicException ();
    }

  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT("(%P|%t) %T INFO: creating publication topic: %C.\n"),
    this->config_.writerTopicName().c_str()
  ));
  this->writerTopic_
    = this->publisherParticipant_->create_topic(
        this->config_.writerTopicName().c_str(),
        this->config_.typeName().c_str(),
        TOPIC_QOS_DEFAULT,
        ::DDS::TopicListener::_nil(),
        ::OpenDDS::DCPS::DEFAULT_STATUS_MASK
      );
  if( CORBA::is_nil( this->readerTopic_.in()) )
    {
      ACE_ERROR ((LM_ERROR,
        ACE_TEXT ("(%P|%t) %T ERROR: Failed to create_topic for publisher.\n")));
      throw BadTopicException ();
    }

  //
  // Establish the Subscriber.
  //

  ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) %T INFO: creating subscriber.\n")));
  this->subscriber_ = this->subscriberParticipant_->create_subscriber(
                        SUBSCRIBER_QOS_DEFAULT,
                        ::DDS::SubscriberListener::_nil(),
                        ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (CORBA::is_nil (this->subscriber_.in ()))
    {
      ACE_ERROR ((LM_ERROR,
        ACE_TEXT ("(%P|%t) %T ERROR: Failed to create_subscriber.\n")));
      throw BadSubscriberException ();
    }

  //
  // Establish the Publisher.
  //

  ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) %T INFO: creating publisher.\n")));
  this->publisher_ = this->publisherParticipant_->create_publisher(
                       PUBLISHER_QOS_DEFAULT,
                       ::DDS::PublisherListener::_nil(),
                       ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (CORBA::is_nil (this->publisher_.in ()))
    {
      ACE_ERROR ((LM_ERROR,
        ACE_TEXT ("(%P|%t) %T ERROR: Failed to create_publisher.\n")));
      throw BadPublisherException ();
    }

  //
  // Establish and install the DataWriter.
  //

  ::DDS::DataWriterListener_var listener (new DataWriterListenerImpl(OpenDDS::DCPS::Discovery::DEFAULT_REPO));

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

  ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) %T INFO: creating data writer.\n")));
  this->dataWriter_ = this->publisher_->create_datawriter(
                        this->writerTopic_.in(),
                        writerQos,
                        listener.in(),
                        ::OpenDDS::DCPS::DEFAULT_STATUS_MASK
                      );
  if( CORBA::is_nil( this->dataWriter_.in()) )
    {
      ACE_ERROR ((LM_ERROR,
                  ACE_TEXT("(%P|%t) %T ERROR: create_datawriter failed.\n")));
      throw BadWriterException ();
    }

  // Pass the writer into the forwarder.
  forwarder_servant->dataWriter( this->dataWriter_.in());

  //
  // Establish and install the DataReader.  This needs to be done after
  // the writer has been created and attached since it is possible (I
  // know, I've seen it happen!) that messages can be received and
  // forwarded between the time the reader is created and the writer is
  // created and attached to the forwarder if we do it the other way
  // round.
  //

  ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) %T INFO: looking up subscription topic.\n")));
  ::DDS::TopicDescription_var description
    = this->subscriberParticipant_->lookup_topicdescription(
        this->config_.readerTopicName().c_str()
      );

  ACE_DEBUG((LM_DEBUG,
             ACE_TEXT("(%P|%t) %T INFO: creating data reader for topic: %C, type: %C.\n"),
             CORBA::String_var(description->get_name()).in(),
             CORBA::String_var(description->get_type_name()).in()
  ));
  this->dataReader_ = this->subscriber_->create_datareader(
                        description.in(),
                        DATAREADER_QOS_DEFAULT,
                        this->listener_.in (),
                        ::OpenDDS::DCPS::DEFAULT_STATUS_MASK
                      );
  if( CORBA::is_nil( this->dataReader_.in()) )
    {
      ACE_ERROR ((LM_ERROR,
                  ACE_TEXT("(%P|%t) %T ERROR: create_datareader failed.\n")));
      throw BadReaderException ();
    }

}

TestSystem::~TestSystem()
{
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) %T INFO: finalizing the publication apparatus.\n")));

  // Tear down the sending apparatus.
  if( ::DDS::RETCODE_PRECONDITION_NOT_MET
      == this->publisherParticipant_->delete_contained_entities()
    ) {
    ACE_ERROR ((LM_ERROR,
      ACE_TEXT("(%P|%t) %T ERROR: Unable to release the publication.\n")));

  } else {
    DDS::DomainParticipantFactory_var dpf = TheParticipantFactory;
    // Release publisher participant.
    if( ::DDS::RETCODE_PRECONDITION_NOT_MET
        == dpf->delete_participant( this->publisherParticipant_.in())
      ) {
      ACE_ERROR ((LM_ERROR,
        ACE_TEXT("(%P|%t) %T ERROR: Unable to release the publication participant.\n")));
    }
  }

  if( this->subscriberParticipant_.in() != this->publisherParticipant_.in()) {
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) %T INFO: finalizing separate subscription apparatus.\n")));

    // Tear down the receiving apparatus.
    if( ::DDS::RETCODE_PRECONDITION_NOT_MET
        == this->subscriberParticipant_->delete_contained_entities()
      ) {
      ACE_ERROR ((LM_ERROR,
        ACE_TEXT("(%P|%t) %T ERROR: Unable to release the subscription.\n")));

    } else {
      DDS::DomainParticipantFactory_var dpf = TheParticipantFactory;
      // Release subscriber participant.
      if( ::DDS::RETCODE_PRECONDITION_NOT_MET
          == dpf->delete_participant( this->subscriberParticipant_.in())
        ) {
        ACE_ERROR ((LM_ERROR,
          ACE_TEXT("(%P|%t) %T ERROR: Unable to release the subscription participant.\n")));
      }
    }
  }

  // Release all the transport resources.
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) %T INFO: finalizing transport.\n")));

  // Release any remaining resources held for the service.
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) %T INFO: finalizing DCPS service.\n")));
  TheServiceParticipant->shutdown ();
}

void
TestSystem::run()
{
  ForwardingListenerImpl* forwarder
    = dynamic_cast< ForwardingListenerImpl*>(this->listener_.in ());
  if (forwarder) {
    forwarder->waitForCompletion();
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) %T INFO: processing complete.\n")));
  } else {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) %T ERROR: Failed to obtain ForwardingListenerImpl")
      ACE_TEXT(" cannot wait for completion.\n")));
  }
}
