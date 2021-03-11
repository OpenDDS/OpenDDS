
#include "TestMonitor.h"
#include "DataWriterListenerImpl.h"
#include "TestException.h"
#include "tests/DCPS/FooType5/FooDefTypeSupportC.h"
#include "tests/DCPS/FooType5/FooDefTypeSupportImpl.h"
#include "dds/DCPS/SubscriberImpl.h"
#include "dds/DCPS/PublisherImpl.h"
#include "dds/DCPS/Marked_Default_Qos.h"
#include "dds/DCPS/transport/framework/TransportImpl.h"
#include "ace/SString.h"

#include <set>
#include <sstream>

/**
 * @brief Construct a test system from the command line.
 */
TestMonitor::TestMonitor( int argc, ACE_TCHAR** argv, char** envp)
 : config_( argc, argv, envp)
{
/// DDS_RUN_DEBUG_LEVEL = 5;
/// OpenDDS::DCPS::DCPS_debug_level = 5;
  // Grab a local reference to the factory to ensure that we perform the
  // operations - they should not get optimized away!  Note that this
  // passes the arguments to the ORB initialization first, then strips
  // the DCPS arguments.
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) %T INFO: initializing the monitor.\n")));
  TheServiceParticipant->set_default_discovery(OpenDDS::DCPS::Discovery::DEFAULT_REPO);
  ::DDS::DomainParticipantFactory_var factory = TheParticipantFactoryWithArgs( argc, argv);

  this->subscriberParticipant_.resize(
    this->config_.subscriberDomainSize(), ::DDS::DomainParticipant::_nil()
  );
  this->publisherParticipant_.resize(
    this->config_.publisherDomainSize(),  ::DDS::DomainParticipant::_nil()
  );
  this->subscriber_.resize(  this->config_.subscriberDomainSize(), ::DDS::Subscriber::_nil());
  this->publisher_.resize(   this->config_.publisherDomainSize(),  ::DDS::Publisher::_nil());
  this->readerTopic_.resize( this->config_.subscriberDomainSize(), ::DDS::Topic::_nil());
  this->writerTopic_.resize( this->config_.publisherDomainSize(),  ::DDS::Topic::_nil());
  this->dataReader_.resize(  this->config_.subscriberDomainSize(), ::DDS::DataReader::_nil());
  this->dataWriter_.resize(  this->config_.publisherDomainSize(),  ::DDS::DataWriter::_nil());
  this->listener_.resize(    this->config_.subscriberDomainSize(), ::DDS::DataReaderListener::_nil());
  this->forwarder_.resize(   this->config_.subscriberDomainSize(), 0);

  if( signed(OpenDDS::DCPS::DCPS_debug_level) >= 0) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) %T COMMANDLINE:              Verbose == %C\n")
      ACE_TEXT("(%P|%t) %T COMMANDLINE:              Samples == %d\n")
      ACE_TEXT("(%P|%t) %T COMMANDLINE:                 Type == %C\n")
      ACE_TEXT("(%P|%t) %T COMMANDLINE:    Transport Address == %s\n"),
      (this->config_.verbose()? "true": "false"),
      this->config_.samples(),
      this->config_.typeName().c_str(),
      this->config_.transportAddressName().c_str()
    ));
    for( TestConfig::StringVector::size_type index = 0; index < this->config_.infoRepoIorSize(); ++index) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) %T COMMANDLINE:     InforRepoIOR[ %d] == %C\n"),
        index, this->config_.infoRepoIor(static_cast<int>(index)).c_str()
      ));
    }
    for( TestConfig::StringVector::size_type index = 0; index < this->config_.readerTopicNameSize(); ++index) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) %T COMMANDLINE:  readerTopicName[ %d] == %C\n"),
        index, this->config_.readerTopicName(static_cast<int>(index)).c_str()
      ));
    }
    for( TestConfig::StringVector::size_type index = 0; index < this->config_.writerTopicNameSize(); ++index) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) %T COMMANDLINE:  writerTopicName[ %d] == %C\n"),
        index, this->config_.writerTopicName(static_cast<int>(index)).c_str()
      ));
    }
    for( TestConfig::StringVector::size_type index = 0; index < this->config_.subscriberDomainSize(); ++index) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) %T COMMANDLINE: subscriberDomain[ %d] == %d\n")
        ACE_TEXT("(%P|%t) %T COMMANDLINE:    using repository key %d\n"),
        index, this->config_.subscriberDomain(static_cast<int>(index)),
        this->config_.domainToRepo( this->config_.subscriberDomain(static_cast<int>(index)))
      ));
    }
    for( TestConfig::DomainVector::size_type index = 0; index < this->config_.publisherDomainSize(); ++index) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) %T COMMANDLINE:  publisherDomain[ %d] == %d\n")
        ACE_TEXT("(%P|%t) %T COMMANDLINE:    using repository key %d\n"),
        index, this->config_.publisherDomain(static_cast<int>(index)),
        this->config_.domainToRepo( this->config_.publisherDomain(static_cast<int>(index)))
      ));
    }
  }

  for( TestConfig::StringVector::size_type index = 0; index < this->config_.infoRepoIorSize(); ++index) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) %T INFO: loading repository %d.\n"),
      index
    ));
    std::ostringstream oss;
    oss << index;
    OPENDDS_STRING key_string = oss.str().c_str();
    TheServiceParticipant->set_repo_ior(
      this->config_.infoRepoIor(static_cast<int>(index)).c_str(),
      key_string
    );
  }

  // We only need to do binding if we receive InfoRepo IOR values on the
  // command line - otherwise this is done during configuration file
  // processing.
  if( this->config_.infoRepoIorSize() > 0) {
    for( TestConfig::StringVector::size_type index = 0; index < this->config_.subscriberDomainSize(); ++index) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) %T INFO: binding subscriber domain %d to repository %d.\n"),
        this->config_.subscriberDomain(static_cast<int>(index)),
        this->config_.domainToRepo( this->config_.subscriberDomain(static_cast<int>(index)))
      ));
      std::ostringstream oss;
      oss << this->config_.domainToRepo( this->config_.subscriberDomain(static_cast<int>(index)));
      OPENDDS_STRING key_string = oss.str().c_str();
      TheServiceParticipant->set_repo_domain(
        this->config_.subscriberDomain(static_cast<int>(index)),
        key_string
      );
    }

    for( TestConfig::StringVector::size_type index = 0; index < this->config_.publisherDomainSize(); ++index) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) %T INFO: binding publisher domain %d to repository %d.\n"),
        this->config_.publisherDomain(static_cast<int>(index)),
        this->config_.domainToRepo( this->config_.publisherDomain(static_cast<int>(index)))
      ));
      std::ostringstream oss;
      oss << this->config_.domainToRepo( this->config_.publisherDomain(static_cast<int>(index)));
      OPENDDS_STRING key_string = oss.str().c_str();
      TheServiceParticipant->set_repo_domain(
        this->config_.publisherDomain(static_cast<int>(index)),
        key_string
      );
    }
  }

  //
  // Establish DomainParticipants for the subscription domains.
  //
  for( TestConfig::StringVector::size_type index = 0; index < this->config_.subscriberDomainSize(); ++index) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) %T INFO: creating subscriber participant in domain %d.\n"),
      this->config_.subscriberDomain(static_cast<int>(index))
    ));
    ParticipantMap::iterator where
      = this->participants_.find( this->config_.subscriberDomain(static_cast<int>(index)));
    if( where == this->participants_.end()) {
      this->participants_[ this->config_.subscriberDomain(static_cast<int>(index))] =
        factory->create_participant(
          this->config_.subscriberDomain(static_cast<int>(index)),
          PARTICIPANT_QOS_DEFAULT,
          ::DDS::DomainParticipantListener::_nil(),
          ::OpenDDS::DCPS::DEFAULT_STATUS_MASK
        );
      if (CORBA::is_nil (this->participants_[ this->config_.subscriberDomain(static_cast<int>(index))].in ()))
        {
          ACE_ERROR ((LM_ERROR,
            ACE_TEXT("(%P|%t) %T ERROR: create_participant failed for ")
            ACE_TEXT("subscriber[ %d] in domain %d.\n"),
            index, this->config_.subscriberDomain(static_cast<int>(index))
          ));
          throw BadParticipantException ();
        }
    }
    this->subscriberParticipant_[ index]
      = ::DDS::DomainParticipant::_duplicate
      (this->participants_[ this->config_.subscriberDomain(static_cast<int>(index))].in());
  }

  //
  // Establish DomainParticipants for the publication domains.
  //
  for( TestConfig::StringVector::size_type index = 0; index < this->config_.publisherDomainSize(); ++index) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) %T INFO: creating publisher participant in domain %d.\n"),
      this->config_.publisherDomain(static_cast<int>(index))
    ));
    ParticipantMap::iterator where
      = this->participants_.find( this->config_.publisherDomain(static_cast<int>(index)));
    if( where == this->participants_.end()) {
      this->participants_[ this->config_.publisherDomain(static_cast<int>(index))] =
        factory->create_participant(
          this->config_.publisherDomain(static_cast<int>(index)),
          PARTICIPANT_QOS_DEFAULT,
          ::DDS::DomainParticipantListener::_nil(),
          ::OpenDDS::DCPS::DEFAULT_STATUS_MASK
        );
      if (CORBA::is_nil (this->participants_[ this->config_.publisherDomain(static_cast<int>(index))].in ()))
        {
          ACE_ERROR ((LM_ERROR,
            ACE_TEXT("(%P|%t) %T ERROR: create_participant failed for ")
            ACE_TEXT("publisher[ %d] in domain %d.\n"),
            index, this->config_.publisherDomain(static_cast<int>(index))
          ));
          throw BadParticipantException ();
        }
    }
    this->publisherParticipant_[ index]
      = ::DDS::DomainParticipant::_duplicate
      (this->participants_[ this->config_.publisherDomain(static_cast<int>(index))].in());
  }

  //
  // Grab and install the transport implementations.
  //
  // We need to have a separate transport implementation for each
  // repository since the RepoId space will collide between them without
  // modification of the current implementation.
  //

  // Establish debug level.
  if( this->config_.verbose()) {
    TURN_ON_VERBOSE_DEBUG;
  }

  //
  // We have to extract the repository keys here since if we configured
  // the repositories and domains from a file, we do not have that
  // information available to us.  Drat.
  //
  std::set< OpenDDS::DCPS::Discovery::RepoKey> keylist;
  for( TestConfig::StringVector::size_type index = 0; index < this->config_.subscriberDomainSize(); ++index) {
    keylist.insert( TheServiceParticipant->domain_to_repo( this->config_.subscriberDomain(static_cast<int>(index))));
  }
  for( TestConfig::StringVector::size_type index = 0; index < this->config_.publisherDomainSize(); ++index) {
    keylist.insert( TheServiceParticipant->domain_to_repo( this->config_.publisherDomain(static_cast<int>(index))));
  }


  //
  // Establish the listeners.
  //

  for( TestConfig::StringVector::size_type index = 0; index < this->config_.subscriberDomainSize(); ++index) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) %T INFO: creating data reader listener for domain %d.\n"),
      this->config_.subscriberDomain(static_cast<int>(index))
    ));
    this->listener_[ index] =
        new ForwardingListenerImpl(
              TheServiceParticipant->domain_to_repo(
                this->config_.subscriberDomain(static_cast<int>(index))
            ));
    this->forwarder_[ index] =
      dynamic_cast<ForwardingListenerImpl*>(this->listener_[ index].in());

    if (CORBA::is_nil (this->listener_[ index].in ()))
      {
        ACE_ERROR((LM_ERROR,
          ACE_TEXT ("(%P|%t) %T ERROR: failed to obtain listener for domain %d.\n"),
          this->config_.subscriberDomain(static_cast<int>(index))
        ));
        throw BadReaderListenerException ();
      }
  }

  //
  // Establish the Type Support for the data in each participant. (??)
  //

  for( ParticipantMap::const_iterator current = this->participants_.begin();
       current != this->participants_.end();
       ++current) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) %T INFO: Installing type %C support into domain %d.\n"),
      this->config_.typeName().c_str(),
      current->first
    ));
    if( 0 == CORBA::is_nil( current->second.in())) {
      ::Xyz::FooNoKeyTypeSupport_var subscriber_data = new ::Xyz::FooNoKeyTypeSupportImpl();
      if(::DDS::RETCODE_OK != subscriber_data->register_type(
                                current->second.in (),
                                this->config_.typeName().c_str()
                              )
        ) {
        ACE_ERROR((LM_ERROR,
          ACE_TEXT("(%P|%t) %T ERROR: Unable to install type %C support for domain %d.\n"),
          this->config_.typeName().c_str(),
          current->first
        ));
        throw TestException ();
      }
    }
  }

  //
  // Establish the Subscriber Topics.
  //

  for( TestConfig::StringVector::size_type index = 0; index < this->config_.readerTopicNameSize(); ++index) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) %T INFO: creating subscription[ %d] topic: %C.\n"),
      index,
      this->config_.readerTopicName(static_cast<int>(index)).c_str()
    ));
    this->readerTopic_[ index]
      = this->participants_[ this->config_.subscriberDomain(static_cast<int>(index))]->create_topic(
          this->config_.readerTopicName(static_cast<int>(index)).c_str(),
          this->config_.typeName().c_str(),
          TOPIC_QOS_DEFAULT,
          ::DDS::TopicListener::_nil(),
          ::OpenDDS::DCPS::DEFAULT_STATUS_MASK
        );
    if( CORBA::is_nil( this->readerTopic_[ index].in()) )
      {
        ACE_ERROR ((LM_ERROR,
          ACE_TEXT ("(%P|%t) %T ERROR: Failed to create topic %C for subscriber[ %d].\n"),
          this->config_.readerTopicName(static_cast<int>(index)).c_str(),
          index
        ));
        throw BadTopicException ();
      }
  }

  //
  // Establish the Publisher Topics.
  //

  for( TestConfig::StringVector::size_type index = 0; index < this->config_.writerTopicNameSize(); ++index) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) %T INFO: creating publication[ %d] topic: %C.\n"),
      index,
      this->config_.writerTopicName(static_cast<int>(index)).c_str()
    ));
    this->writerTopic_[ index]
      = this->participants_[ this->config_.publisherDomain(static_cast<int>(index))]->create_topic(
          this->config_.writerTopicName(static_cast<int>(index)).c_str(),
          this->config_.typeName().c_str(),
          TOPIC_QOS_DEFAULT,
          ::DDS::TopicListener::_nil(),
          ::OpenDDS::DCPS::DEFAULT_STATUS_MASK
        );
    if( CORBA::is_nil( this->writerTopic_[ index].in()) )
      {
        ACE_ERROR ((LM_ERROR,
          ACE_TEXT ("(%P|%t) %T ERROR: Failed to create topic %C for publisher[ %d].\n"),
          this->config_.writerTopicName(static_cast<int>(index)).c_str(),
          index
        ));
        throw BadTopicException ();
      }
  }

  //
  // Establish the Subscribers.
  //

  for( TestConfig::StringVector::size_type index = 0; index < this->config_.subscriberDomainSize(); ++index) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) %T INFO: creating subscriber[ %d].\n"),
      index
    ));
    this->subscriber_[ index]
      = this->participants_[ this->config_.subscriberDomain(static_cast<int>(index))]->create_subscriber(
          SUBSCRIBER_QOS_DEFAULT, ::DDS::SubscriberListener::_nil(),
          ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (CORBA::is_nil (this->subscriber_[ index].in ()))
      {
        ACE_ERROR ((LM_ERROR,
          ACE_TEXT ("(%P|%t) %T ERROR: Failed to create_subscriber[ %d].\n"),
          index
        ));
        throw BadSubscriberException ();
      }

  }

  //
  // Establish the Publishers.
  //

  for( TestConfig::StringVector::size_type index = 0; index < this->config_.publisherDomainSize(); ++index) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) %T INFO: creating publisher[ %d].\n"),
      index
    ));
    this->publisher_[ index]
      = this->participants_[ this->config_.publisherDomain(static_cast<int>(index))]->create_publisher(
          PUBLISHER_QOS_DEFAULT, ::DDS::PublisherListener::_nil(),
          ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (CORBA::is_nil (this->publisher_[ index].in ()))
      {
        ACE_ERROR ((LM_ERROR,
          ACE_TEXT ("(%P|%t) %T ERROR: Failed to create_publisher[ %d].\n"),
          index
        ));
        throw BadSubscriberException ();
      }

  }

  //
  // Establish and install the DataWriter.
  //

  for( TestConfig::StringVector::size_type index = 0; index < this->config_.publisherDomainSize(); ++index) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) %T INFO: creating datawriter[ %d].\n"),
      index
    ));
    ::DDS::DataWriterListener_var listener (
        new DataWriterListenerImpl(
              TheServiceParticipant->domain_to_repo(
                this->config_.publisherDomain(static_cast<int>(index))
            )));

    //
    // Keep all data samples to allow us to establish connections in an
    // arbitrary order, with samples being buffered at the first writer
    // that has not yet had a subscription match.
    //
    ::DDS::DataWriterQos writerQos;
    this->publisher_[ index]->get_default_datawriter_qos( writerQos);

    writerQos.durability.kind                          = ::DDS::TRANSIENT_LOCAL_DURABILITY_QOS;
    writerQos.history.kind                             = ::DDS::KEEP_ALL_HISTORY_QOS;
    writerQos.resource_limits.max_samples_per_instance = ::DDS::LENGTH_UNLIMITED;
    writerQos.reliability.kind                         = ::DDS::RELIABLE_RELIABILITY_QOS;
    writerQos.reliability.max_blocking_time.sec        = 0;
    writerQos.reliability.max_blocking_time.nanosec    = 0;

    this->dataWriter_[ index]
      = this->publisher_[ index]->create_datawriter(this->writerTopic_[ index].in(),
                                                    writerQos, listener.in(),
                                                    ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if( CORBA::is_nil( this->dataWriter_[ index].in()) )
      {
        ACE_ERROR((LM_ERROR,
          ACE_TEXT("(%P|%t) %T ERROR: create datawriter[ %d] failed.\n"),
          index
        ));
        throw BadWriterException ();
      }
  }

  for( unsigned int index = 1; index < this->forwarder_.size(); ++index) {
    // Pass the writer into the (previous) forwarder.
    this->forwarder_[ index - 1]->dataWriter( this->dataWriter_[ index].in());
  }

    // The last forwarder does not.
  this->forwarder_[ this->forwarder_.size() - 1]->dataWriter(
    ::DDS::DataWriter::_nil()
  );

  //
  // Establish and install the DataReader.  This needs to be done after
  // the writer has been created and attached since it is possible (I
  // know, I've seen it happen!) that messages can be received and
  // forwarded between the time the reader is created and the writer is
  // created and attached to the forwarder if we do it the other way
  // round.
  //

  for( TestConfig::DomainVector::size_type index = 0; index < this->config_.subscriberDomainSize(); ++index) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) %T INFO: creating data reader[ %d].\n"),
      index
    ));
    ::DDS::TopicDescription_var description
      = this->participants_[ this->config_.subscriberDomain(static_cast<int>(index))]->lookup_topicdescription(
          this->config_.readerTopicName(static_cast<int>(index)).c_str()
        );

    this->dataReader_[ index]
      = this->subscriber_[ index]->create_datareader(
          description.in(),
          DATAREADER_QOS_DEFAULT,
          this->listener_[ index].in (),
          ::OpenDDS::DCPS::DEFAULT_STATUS_MASK
        );
    if( CORBA::is_nil( this->dataReader_[ index].in()) )
      {
        ACE_ERROR((LM_ERROR,
          ACE_TEXT("(%P|%t) %T ERROR: create datareader[ %d] failed.\n"),
          index
        ));
        throw BadReaderException ();
      }
  }

}

TestMonitor::~TestMonitor()
{
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) %T INFO: finalizing the test monitor.\n")));

  // Release the participants.
  for( ParticipantMap::const_iterator current = this->participants_.begin();
       current != this->participants_.end();
       ++current) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) %T INFO: releasing resources for domain %d.\n"),
      current->first
    ));
    if( 0 == CORBA::is_nil( current->second.in())) {
      DDS::DomainParticipantFactory_var dpf = TheParticipantFactory;
      if( ::DDS::RETCODE_PRECONDITION_NOT_MET
           == current->second->delete_contained_entities()
        ) {
        ACE_ERROR ((LM_ERROR,
          ACE_TEXT("(%P|%t) %T ERROR: Unable to release resources for domain %d.\n"),
          current->first
        ));

      } else if( ::DDS::RETCODE_PRECONDITION_NOT_MET
                 == dpf->delete_participant( current->second)
               ) {
        ACE_ERROR ((LM_ERROR,
          ACE_TEXT("(%P|%t) %T ERROR: Unable to release the participant for domain %d.\n"),
          current->first));
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
TestMonitor::run()
{
  // Write Foo samples.
  ::Xyz::FooNoKey foo;
  foo.data_source = 22;

  // Write into the first datawriter
  ::Xyz::FooNoKeyDataWriter_var fooWriter
    = ::Xyz::FooNoKeyDataWriter::_narrow( this->dataWriter_[ 0].in());
  if( CORBA::is_nil( fooWriter.in())) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) %T TestMonitor failed to narrow writer.\n")
    ));
    throw BadWriterException();
  }

  // DURABILITY == TRANSIENT_LOCAL locks up occasionally.
  // NOTE: This is a kluge to avoid a race condition - it is still
  //       possible, though unlikely, to lock up due to the race.
  ACE_OS::sleep(5);

  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT("(%P|%t) %T TestMonitor::run starting to write.\n")
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

    // Go ahead and forward the data.
    if( ::DDS::RETCODE_OK != fooWriter->write( foo, ::DDS::HANDLE_NIL)) {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) %T TestMonitor::run ")
        ACE_TEXT("failed to forward sample %d.\n"),
        sample
      ));
    }

    ACE_DEBUG ((LM_DEBUG, "(%P|%t) got sample %d interval %d\n", this->config_.samples(), this->config_.sample_interval ()));
    if (this->config_.sample_interval () > 0)
    {
      ACE_OS::sleep (this->config_.sample_interval ());
    }
  }

  //
  // Wait until we receive the final message at the last receiver, then
  // we are done.
  //
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) %T TestMonitor::run about to wait.\n")));
  this->forwarder_[ this->forwarder_.size() - 1]->waitForCompletion();

  // This is now a termination message.
  foo.data_source = 30;

  // The first data writer is already narrowed, terminate it.
  foo.x = 0.0;
  foo.y = 355.0f / 113.0f;
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) %T TestMonitor::run terminating %d.\n"),0));
  if( ::DDS::RETCODE_OK != fooWriter->write( foo, ::DDS::HANDLE_NIL)) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) %T TestMonitor::run ")
      ACE_TEXT("failed to send termination message %d.\n"),
      (this->dataWriter_.size() - 1)
    ));
  }

  // Narrow the reminaing writers and send a termination message.
  for( int index = 1; index < signed(this->dataWriter_.size()); ++index) {
    // Write into the first datawriter
    foo.x = float(index);
    foo.y = float(index);
    ::Xyz::FooNoKeyDataWriter_var writer
      = ::Xyz::FooNoKeyDataWriter::_narrow( this->dataWriter_[ index].in());
    if( CORBA::is_nil( writer.in())) {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) %T TestMonitor failed to narrow writer.\n")
      ));
      throw BadWriterException();
    }
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) %T TestMonitor::run terminating %d.\n"),index));
    if( ::DDS::RETCODE_OK != writer->write( foo, ::DDS::HANDLE_NIL)) {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) %T TestMonitor::run ")
        ACE_TEXT("failed to send termination message %d.\n"),
        index
      ));
    }
  }

  // Until we implement v1.2 Publisher::wait_for_acknowledgments()
  // NOTE: This is a kluge to avoid a race condition - it is still
  //       possible, though unlikely, to lock up due to the race.
  ACE_OS::sleep(5);

}
