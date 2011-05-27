
#include "TestMonitor.h"
#include "DataWriterListenerImpl.h"
#include "TestException.h"
#include "tests/DCPS/FooType5/FooNoKeyTypeSupportC.h"
#include "tests/DCPS/FooType5/FooNoKeyTypeSupportImpl.h"
#include "dds/DCPS/SubscriberImpl.h"
#include "dds/DCPS/PublisherImpl.h"
#include "dds/DCPS/Marked_Default_Qos.h"
#include "dds/DCPS/transport/framework/TheTransportFactory.h"
#include "dds/DCPS/transport/framework/TransportImpl.h"
#include "dds/DCPS/transport/simpleTCP/SimpleTcpConfiguration.h"
#include "ace/SString.h"

#include <set>

namespace { // Anonymous namespace for file scope.

  /// Comparison operation for repository set.
  class RepoOrder {
    public: bool operator()(
                   const ::OpenDDS::DCPS::DCPSInfo_var& left,
                   const ::OpenDDS::DCPS::DCPSInfo_var& right
                 ) { return left.in() < right.in(); }
  };

} // End of anonymous namespace.

/**
 * @brief Construct a test system from the command line.
 */
TestMonitor::TestMonitor( int argc, char** argv, char** envp)
 : config_( argc, argv, envp)
{
/// DDS_RUN_DEBUG_LEVEL = 5;
/// OpenDDS::DCPS::DCPS_debug_level = 5;
  // Grab a local reference to the factory to ensure that we perform the
  // operations - they should not get optimized away!  Note that this
  // passes the arguments to the ORB initialization first, then strips
  // the DCPS arguments.
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("%T (%P|%t) INFO: initializing the monitor.\n")));
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
      ACE_TEXT("%T (%P|%t) COMMANDLINE:              Verbose == %s\n")
      ACE_TEXT("%T (%P|%t) COMMANDLINE:              Samples == %d\n")
      ACE_TEXT("%T (%P|%t) COMMANDLINE:                 Type == %s\n")
      ACE_TEXT("%T (%P|%t) COMMANDLINE:    Transport Address == %s\n"),
      (this->config_.verbose()? "true": "false"),
      this->config_.samples(),
      this->config_.typeName().c_str(),
      this->config_.transportAddressName().c_str()
    ));
    for( int index = 0; index < this->config_.infoRepoIorSize(); ++index) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("%T (%P|%t) COMMANDLINE:     InforRepoIOR[ %d] == %s\n"),
        index, this->config_.infoRepoIor( index).c_str()
      ));
    }
    for( int index = 0; index < this->config_.readerTopicNameSize(); ++index) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("%T (%P|%t) COMMANDLINE:  readerTopicName[ %d] == %s\n"),
        index, this->config_.readerTopicName( index).c_str()
      ));
    }
    for( int index = 0; index < this->config_.writerTopicNameSize(); ++index) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("%T (%P|%t) COMMANDLINE:  writerTopicName[ %d] == %s\n"),
        index, this->config_.writerTopicName( index).c_str()
      ));
    }
    for( int index = 0; index < this->config_.subscriberDomainSize(); ++index) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("%T (%P|%t) COMMANDLINE: subscriberDomain[ %d] == %d\n")
        ACE_TEXT("%T (%P|%t) COMMANDLINE:    using repository key %d\n"),
        index, this->config_.subscriberDomain( index),
        this->config_.domainToRepo( this->config_.subscriberDomain( index))
      ));
    }
    for( int index = 0; index < this->config_.publisherDomainSize(); ++index) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("%T (%P|%t) COMMANDLINE:  publisherDomain[ %d] == %d\n")
        ACE_TEXT("%T (%P|%t) COMMANDLINE:    using repository key %d\n"),
        index, this->config_.publisherDomain( index),
        this->config_.domainToRepo( this->config_.publisherDomain( index))
      ));
    }
  }

  for( int index = 0; index < this->config_.infoRepoIorSize(); ++index) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("%T (%P|%t) INFO: loading repository %d.\n"),
      index
    ));
    TheServiceParticipant->set_repo_ior(
      this->config_.infoRepoIor( index).c_str(),
      index
    );
  }

  // We only need to do binding if we receive InfoRepo IOR values on the
  // command line - otherwise this is done during configuration file
  // processing.
  if( this->config_.infoRepoIorSize() > 0) {
    for( int index = 0; index < this->config_.subscriberDomainSize(); ++index) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("%T (%P|%t) INFO: binding subscriber domain %d to repository %d.\n"),
        this->config_.subscriberDomain( index),
        this->config_.domainToRepo( this->config_.subscriberDomain( index))
      ));
      TheServiceParticipant->set_repo_domain(
        this->config_.subscriberDomain( index),
        this->config_.domainToRepo( this->config_.subscriberDomain( index))
      );
    }

    for( int index = 0; index < this->config_.publisherDomainSize(); ++index) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("%T (%P|%t) INFO: binding publisher domain %d to repository %d.\n"),
        this->config_.publisherDomain( index),
        this->config_.domainToRepo( this->config_.publisherDomain( index))
      ));
      TheServiceParticipant->set_repo_domain(
        this->config_.publisherDomain( index),
        this->config_.domainToRepo( this->config_.publisherDomain( index))
      );
    }
  }

  //
  // Establish DomainParticipants for the subscription domains.
  //
  for( int index = 0; index < this->config_.subscriberDomainSize(); ++index) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("%T (%P|%t) INFO: creating subscriber participant in domain %d.\n"),
      this->config_.subscriberDomain( index)
    ));
    ParticipantMap::iterator where
      = this->participants_.find( this->config_.subscriberDomain( index));
    if( where == this->participants_.end()) {
      this->participants_[ this->config_.subscriberDomain( index)] =
        factory->create_participant(
          this->config_.subscriberDomain( index),
          PARTICIPANT_QOS_DEFAULT,
          ::DDS::DomainParticipantListener::_nil()
        );
      if (CORBA::is_nil (this->participants_[ this->config_.subscriberDomain( index)].in ()))
        {
          ACE_ERROR ((LM_ERROR,
            ACE_TEXT("%T (%P|%t) ERROR: create_participant failed for ")
            ACE_TEXT("subscriber[ %d] in domain %d.\n"),
            index, this->config_.subscriberDomain( index)
          ));
          throw BadParticipantException ();
        }
    }
    this->subscriberParticipant_[ index]
      = ::DDS::DomainParticipant::_duplicate(
          this->participants_[ this->config_.subscriberDomain( index)]
        );
  }

  //
  // Establish DomainParticipants for the publication domains.
  //
  for( int index = 0; index < this->config_.publisherDomainSize(); ++index) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("%T (%P|%t) INFO: creating publisher participant in domain %d.\n"),
      this->config_.publisherDomain( index)
    ));
    ParticipantMap::iterator where
      = this->participants_.find( this->config_.publisherDomain( index));
    if( where == this->participants_.end()) {
      this->participants_[ this->config_.publisherDomain( index)] =
        factory->create_participant(
          this->config_.publisherDomain( index),
          PARTICIPANT_QOS_DEFAULT,
          ::DDS::DomainParticipantListener::_nil()
        );
      if (CORBA::is_nil (this->participants_[ this->config_.publisherDomain( index)].in ()))
        {
          ACE_ERROR ((LM_ERROR,
            ACE_TEXT("%T (%P|%t) ERROR: create_participant failed for ")
            ACE_TEXT("publisher[ %d] in domain %d.\n"),
            index, this->config_.publisherDomain( index)
          ));
          throw BadParticipantException ();
        }
    }
    this->publisherParticipant_[ index]
      = ::DDS::DomainParticipant::_duplicate(
          this->participants_[ this->config_.publisherDomain( index)]
        );
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
  std::set< OpenDDS::DCPS::Service_Participant::RepoKey> keylist;
  for( int index = 0; index < this->config_.subscriberDomainSize(); ++index) {
    keylist.insert( TheServiceParticipant->domain_to_repo( this->config_.subscriberDomain( index)));
  }
  for( int index = 0; index < this->config_.publisherDomainSize(); ++index) {
    keylist.insert( TheServiceParticipant->domain_to_repo( this->config_.publisherDomain( index)));
  }

  ACE_DEBUG((LM_DEBUG, ACE_TEXT("%T (%P|%t) INFO: creating transports for repositories.\n")));
  ACE_UINT32 transportKey = 0;
  for( std::set< OpenDDS::DCPS::Service_Participant::RepoKey>::const_iterator current
         = keylist.begin();
       current != keylist.end();
       ++current, ++transportKey) {

    // Only create transports that need to be.
    if( false == TheTransportFactory->obtain( transportKey).is_nil()) {
      continue;
    }

    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("%T (%P|%t) INFO: creating a SimpleTCP transport for repository: %d.\n"),
      *current
    ));
    this->transport_[ *current]
      = TheTransportFactory->create_transport_impl(
          transportKey,
          "SimpleTcp",
          OpenDDS::DCPS::DONT_AUTO_CONFIG
        );

    OpenDDS::DCPS::TransportConfiguration_rch reader_config
      = TheTransportFactory->create_configuration(
          transportKey,
          ACE_TString("SimpleTcp")
        );

//  OpenDDS::DCPS::SimpleTcpConfiguration* reader_tcp_config
//    = static_cast <OpenDDS::DCPS::SimpleTcpConfiguration*>( reader_config.in() );
//
//  if( this->config_.transportAddressName().length() > 0)
//  {
//    ACE_INET_Addr reader_address( this->config_.transportAddressName().c_str());
//    reader_tcp_config->local_address_ = reader_address;
//  }
    // else use default address - OS assigned.

    if( this->transport_[ *current]->configure( reader_config.in()) != 0)
    {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("%T (%P|%t) ERROR: TCP ")
        ACE_TEXT("failed to configure the transport.\n")));
      throw BadTransportException ();
    }
  }

  //
  // Establish the listeners.
  //

  for( int index = 0; index < this->config_.subscriberDomainSize(); ++index) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("%T (%P|%t) INFO: creating data reader listener for domain %d.\n"),
      this->config_.subscriberDomain( index)
    ));
    this->forwarder_[ index]
      = new ForwardingListenerImpl(
              TheServiceParticipant->domain_to_repo(
                this->config_.subscriberDomain( index)
            ));

    this->listener_[ index]
      = ::OpenDDS::DCPS::servant_to_reference( this->forwarder_[ index]);

    if (CORBA::is_nil (this->listener_[ index].in ()))
      {
        ACE_ERROR((LM_ERROR,
          ACE_TEXT ("%T (%P|%t) ERROR: failed to obtain listener for domain %d.\n"),
          this->config_.subscriberDomain( index)
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
      ACE_TEXT("%T (%P|%t) INFO: Installing type %s support into domain %d.\n"),
      this->config_.typeName().c_str(),
      current->first
    ));
    if( 0 == CORBA::is_nil( current->second)) {
      ::Xyz::FooNoKeyTypeSupportImpl* subscriber_data = new ::Xyz::FooNoKeyTypeSupportImpl();
      if(::DDS::RETCODE_OK != subscriber_data->register_type(
                                current->second.in (),
                                this->config_.typeName().c_str()
                              )
        ) {
        ACE_ERROR((LM_ERROR,
          ACE_TEXT("%T (%P|%t) ERROR: Unable to install type %s support for domain %d.\n"),
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

  for( int index = 0; index < this->config_.readerTopicNameSize(); ++index) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("%T (%P|%t) INFO: creating subscription[ %d] topic: %s.\n"),
      index,
      this->config_.readerTopicName( index).c_str()
    ));
    this->readerTopic_[ index]
      = this->participants_[ this->config_.subscriberDomain( index)]->create_topic(
          this->config_.readerTopicName( index).c_str(),
          this->config_.typeName().c_str(),
          TOPIC_QOS_DEFAULT,
          ::DDS::TopicListener::_nil()
        );
    if( CORBA::is_nil( this->readerTopic_[ index].in()) )
      {
        ACE_ERROR ((LM_ERROR,
          ACE_TEXT ("%T (%P|%t) ERROR: Failed to create topic %s for subscriber[ %d].\n"),
          this->config_.readerTopicName( index).c_str(),
          index
        ));
        throw BadTopicException ();
      }
  }

  //
  // Establish the Publisher Topics.
  //

  for( int index = 0; index < this->config_.writerTopicNameSize(); ++index) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("%T (%P|%t) INFO: creating publication[ %d] topic: %s.\n"),
      index,
      this->config_.writerTopicName( index).c_str()
    ));
    this->writerTopic_[ index]
      = this->participants_[ this->config_.publisherDomain( index)]->create_topic(
          this->config_.writerTopicName( index).c_str(),
          this->config_.typeName().c_str(),
          TOPIC_QOS_DEFAULT,
          ::DDS::TopicListener::_nil()
        );
    if( CORBA::is_nil( this->writerTopic_[ index].in()) )
      {
        ACE_ERROR ((LM_ERROR,
          ACE_TEXT ("%T (%P|%t) ERROR: Failed to create topic %s for publisher[ %d].\n"),
          this->config_.writerTopicName( index).c_str(),
          index
        ));
        throw BadTopicException ();
      }
  }

  //
  // Establish the Subscribers.
  //

  for( int index = 0; index < this->config_.subscriberDomainSize(); ++index) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("%T (%P|%t) INFO: creating subscriber[ %d].\n"),
      index
    ));
    this->subscriber_[ index]
      = this->participants_[ this->config_.subscriberDomain( index)]->create_subscriber(
          SUBSCRIBER_QOS_DEFAULT, ::DDS::SubscriberListener::_nil());
    if (CORBA::is_nil (this->subscriber_[ index].in ()))
      {
        ACE_ERROR ((LM_ERROR,
          ACE_TEXT ("%T (%P|%t) ERROR: Failed to create_subscriber[ %d].\n"),
          index
        ));
        throw BadSubscriberException ();
      }

    // Attach the subscriber to the transport.
    OpenDDS::DCPS::SubscriberImpl* sub_impl
      = OpenDDS::DCPS::reference_to_servant<OpenDDS::DCPS::SubscriberImpl>(
          this->subscriber_[ index].in ()
        );

    if (0 == sub_impl)
      {
        ACE_ERROR((LM_ERROR,
          ACE_TEXT("%T (%P|%t) ERROR: Failed to obtain subscriber servant[ %d]\n"),
          index
        ));
        throw BadSubscriberException ();
      }

    OpenDDS::DCPS::AttachStatus attach_status;

    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("%T (%P|%t) INFO: attaching subscriber[ %d] to transport \n"),
      index
    ));
    attach_status
      = sub_impl->attach_transport(
          this->transport_[
            TheServiceParticipant->domain_to_repo( this->config_.subscriberDomain( index))
          ].in()
        );

    if (attach_status != OpenDDS::DCPS::ATTACH_OK)
      {
        // We failed to attach to the transport for some reason.
        ACE_TString status_str;

        switch (attach_status)
          {
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
                    ACE_TEXT("%T (%P|%t) ERROR: Failed to attach to the transport. ")
                    ACE_TEXT("AttachStatus == %s\n"),
                    status_str.c_str()));
        throw BadTransportException ();
      }
  }

  //
  // Establish the Publishers.
  //

  for( int index = 0; index < this->config_.publisherDomainSize(); ++index) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("%T (%P|%t) INFO: creating publisher[ %d].\n"),
      index
    ));
    this->publisher_[ index]
      = this->participants_[ this->config_.publisherDomain( index)]->create_publisher(
          PUBLISHER_QOS_DEFAULT, ::DDS::PublisherListener::_nil());
    if (CORBA::is_nil (this->publisher_[ index].in ()))
      {
        ACE_ERROR ((LM_ERROR,
          ACE_TEXT ("%T (%P|%t) ERROR: Failed to create_publisher[ %d].\n"),
          index
        ));
        throw BadSubscriberException ();
      }

    // Attach the publisher to the transport.
    OpenDDS::DCPS::PublisherImpl* pub_impl
      = OpenDDS::DCPS::reference_to_servant<OpenDDS::DCPS::PublisherImpl>(
          this->publisher_[ index].in ()
        );

    if (0 == pub_impl)
      {
        ACE_ERROR((LM_ERROR,
          ACE_TEXT("%T (%P|%t) ERROR: Failed to obtain publisher servant[ %d]\n"),
          index
        ));
        throw BadSubscriberException ();
      }

    OpenDDS::DCPS::AttachStatus attach_status;

    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("%T (%P|%t) INFO: attaching publisher[ %d] to transport \n"),
      index
    ));
    attach_status
      = pub_impl->attach_transport(
          this->transport_[
            TheServiceParticipant->domain_to_repo( this->config_.publisherDomain( index))
          ].in()
        );

    if (attach_status != OpenDDS::DCPS::ATTACH_OK)
      {
        // We failed to attach to the transport for some reason.
        ACE_TString status_str;

        switch (attach_status)
          {
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
                    ACE_TEXT("%T (%P|%t) ERROR: Failed to attach to the transport. ")
                    ACE_TEXT("AttachStatus == %s\n"),
                    status_str.c_str()));
        throw BadTransportException ();
      }
  }

  //
  // Establish and install the DataWriter.
  //

  for( int index = 0; index < this->config_.publisherDomainSize(); ++index) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("%T (%P|%t) INFO: creating datawriter[ %d].\n"),
      index
    ));
    DataWriterListenerImpl* listenerServant
      = new DataWriterListenerImpl(
              TheServiceParticipant->domain_to_repo(
                this->config_.publisherDomain( index)
            ));
    ::DDS::DataWriterListener_var listener
      = ::OpenDDS::DCPS::servant_to_reference( listenerServant);

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

    this->dataWriter_[ index] = this->publisher_[ index]->create_datawriter(
                                  this->writerTopic_[ index].in(),
                                  writerQos,
                                  listener
                                );
    if( CORBA::is_nil( this->dataWriter_[ index].in()) )
      {
        ACE_ERROR((LM_ERROR,
          ACE_TEXT("%T (%P|%t) ERROR: create datawriter[ %d] failed.\n"),
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

  for( int index = 0; index < this->config_.subscriberDomainSize(); ++index) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("%T (%P|%t) INFO: creating data reader[ %d].\n"),
      index
    ));
    ::DDS::TopicDescription_var description
      = this->participants_[ this->config_.subscriberDomain( index)]->lookup_topicdescription(
          this->config_.readerTopicName( index).c_str()
        );

    this->dataReader_[ index]
      = this->subscriber_[ index]->create_datareader(
          description.in(),
          DATAREADER_QOS_DEFAULT,
          this->listener_[ index].in ()
        );
    if( CORBA::is_nil( this->dataReader_[ index].in()) )
      {
        ACE_ERROR((LM_ERROR,
          ACE_TEXT("%T (%P|%t) ERROR: create datareader[ %d] failed.\n"),
          index
        ));
        throw BadReaderException ();
      }
  }

}

TestMonitor::~TestMonitor()
{
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("%T (%P|%t) INFO: finalizing the test monitor.\n")));

  // Release the participants.
  for( ParticipantMap::const_iterator current = this->participants_.begin();
       current != this->participants_.end();
       ++current) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("%T (%P|%t) INFO: releasing resources for domain %d.\n"),
      current->first
    ));
    if( 0 == CORBA::is_nil( current->second)) {
      if( ::DDS::RETCODE_PRECONDITION_NOT_MET
           == current->second->delete_contained_entities()
        ) {
        ACE_ERROR ((LM_ERROR,
          ACE_TEXT("%T (%P|%t) ERROR: Unable to release resources for domain %d.\n"),
          current->first
        ));

      } else if( ::DDS::RETCODE_PRECONDITION_NOT_MET
                 == TheParticipantFactory->delete_participant( current->second)
               ) {
        ACE_ERROR ((LM_ERROR,
          ACE_TEXT("%T (%P|%t) ERROR: Unable to release the participant for domain %d.\n"),
          current->first));
      }
    }
  }

  // Release any remaining resources held for the service.
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("%T (%P|%t) INFO: finalizing DCPS service.\n")));
  TheServiceParticipant->shutdown ();

  // Release all the transport resources.
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("%T (%P|%t) INFO: finalizing transport.\n")));
  TheTransportFactory->release();

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
      ACE_TEXT("%T (%P|%t) TestMonitor failed to narrow writer.\n")
    ));
    throw BadWriterException();
  }

  // DURABILITY == TRANSIENT_LOCAL locks up occasionally.
  // NOTE: This is a kluge to avoid a race condition - it is still
  //       possible, though unlikely, to lock up due to the race.
  ACE_OS::sleep(5);

  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT("%T (%P|%t) TestMonitor::run starting to write.\n")
  ));

  // Write the requested number of samples.
  for( int sample = 0; sample < this->config_.samples(); ++sample) {
    // Make the data unique for each sample.
    foo.x = 1000.0 + 100.0f * sample;
    foo.y = 2000.0 + 100.0f * sample;
    if( sample == (this->config_.samples() - 1)) {
      // Final sample is the answer that ends the universe.
      foo.data_source = 42;
    }

    // Go ahead and forward the data.
    if( ::DDS::RETCODE_OK != fooWriter->write( foo, ::DDS::HANDLE_NIL)) {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("%T (%P|%t) TestMonitor::run ")
        ACE_TEXT("failed to forward sample %d.\n"),
        sample
      ));
    }
  }

  //
  // Wait until we receive the final message at the last receiver, then
  // we are done.
  //
  this->forwarder_[ this->forwarder_.size() - 1]->waitForCompletion();

  // This is now a termination message.
  foo.data_source = 30;

  // The first data writer is already narrowed, terminate it.
  foo.x = 0.0;
  foo.y = 355.0 / 113.0;
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("%T (%P|%t) TestMonitor::run terminating %d.\n"),0));
  if( ::DDS::RETCODE_OK != fooWriter->write( foo, ::DDS::HANDLE_NIL)) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("%T (%P|%t) TestMonitor::run ")
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
        ACE_TEXT("%T (%P|%t) TestMonitor failed to narrow writer.\n")
      ));
      throw BadWriterException();
    }
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("%T (%P|%t) TestMonitor::run terminating %d.\n"),index));
    if( ::DDS::RETCODE_OK != writer->write( foo, ::DDS::HANDLE_NIL)) {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("%T (%P|%t) TestMonitor::run ")
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

