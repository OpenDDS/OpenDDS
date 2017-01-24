// -*- C++ -*-

#include "Subscriber.h"

#include "Test.h"
#include "Options.h"

#include "TestTypeSupportImpl.h"
#include "DataReaderListener.h"

#include "dds/DCPS/Service_Participant.h"
#include "dds/DCPS/Marked_Default_Qos.h"
#include "dds/DCPS/SubscriberImpl.h"

#include "dds/DCPS/StaticIncludes.h"
#if defined ACE_AS_STATIC_LIBS && !defined OPENDDS_SAFETY_PROFILE
#include "dds/DCPS/transport/udp/Udp.h"
#include "dds/DCPS/transport/multicast/Multicast.h"
#endif

#include <sstream>

namespace Test {

Subscriber::~Subscriber()
{
  this->waiter_->detach_condition( this->status_.in());

  if( ! CORBA::is_nil( this->participant_.in())) {
    this->participant_->delete_contained_entities(); // Also deletes listener.
    DDS::DomainParticipantFactory_var dpf = TheParticipantFactory;
    dpf->delete_participant( this->participant_.in());
  }
  TheServiceParticipant->shutdown();
}

Subscriber::Subscriber( const Options& options)
 : options_( options),
   listener_( 0),
   waiter_( new DDS::WaitSet)
{
  DDS::DomainParticipantFactory_var dpf = TheParticipantFactory;
  // Create the DomainParticipant
  this->participant_
    = dpf->create_participant(
        this->options_.domain(),
        PARTICIPANT_QOS_DEFAULT,
        DDS::DomainParticipantListener::_nil(),
        ::OpenDDS::DCPS::DEFAULT_STATUS_MASK
      );
  if( CORBA::is_nil( this->participant_.in())) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: Subscriber::Subscriber() - ")
      ACE_TEXT("failed to create a participant.\n")
    ));
    throw BadParticipantException();

  } else if( this->options_.verbose()) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) Subscriber::Subscriber() - ")
      ACE_TEXT("created participant in domain %d.\n"),
      this->options_.domain()
    ));
  }

  // Create the listener.
  this->listener_ = new DataReaderListener( this->options_.verbose());
  this->safe_listener_ = this->listener_;
  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT("(%P|%t) Subscriber::Subscriber() - ")
    ACE_TEXT("created reader listener.\n")
  ));

  // Create and register the type support.
  DataTypeSupportImpl* testData = new DataTypeSupportImpl();
  CORBA::String_var type_name = testData->get_type_name();
  if( ::DDS::RETCODE_OK
   != testData->register_type( this->participant_.in(), 0)) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: Subscriber::Subscriber() - ")
      ACE_TEXT("unable to install type %C support.\n"),
      type_name.in()
    ));
    throw BadTypeSupportException ();

  }

  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT("(%P|%t) Subscriber::Subscriber() - ")
    ACE_TEXT("created type %C support.\n"),
    type_name.in()
  ));

  // Create the topic.
  this->topic_ = this->participant_->create_topic(
                   this->options_.topicName().c_str(),
                   type_name,
                   TOPIC_QOS_DEFAULT,
                   ::DDS::TopicListener::_nil(),
                   ::OpenDDS::DCPS::DEFAULT_STATUS_MASK
                 );
  if( CORBA::is_nil( this->topic_.in())) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: Subscriber::Subscriber() - ")
      ACE_TEXT("failed to create topic %C.\n"),
      this->options_.topicName().c_str()
    ));
    throw BadTopicException();

  }

  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT("(%P|%t) Subscriber::Subscriber() - ")
    ACE_TEXT("created topic %C.\n"),
    this->options_.topicName().c_str()
  ));

  // Create the subscriber.
  this->subscriber_ = this->participant_->create_subscriber(
                        SUBSCRIBER_QOS_DEFAULT,
                        ::DDS::SubscriberListener::_nil(),
                        ::OpenDDS::DCPS::DEFAULT_STATUS_MASK
                      );
  if( CORBA::is_nil( this->subscriber_.in())) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: Subscriber::Subscriber() - ")
      ACE_TEXT("failed to create subscriber.\n")
    ));
    throw BadSubscriberException();

  }

  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT("(%P|%t) Subscriber::Subscriber() - ")
    ACE_TEXT("created subscriber.\n")
  ));

  // Reader Qos policy values.
  ::DDS::DataReaderQos readerQos;
  this->subscriber_->get_default_datareader_qos( readerQos);

//  readerQos.durability.kind                          = ::DDS::TRANSIENT_LOCAL_DURABILITY_QOS;
  readerQos.history.kind                             = ::DDS::KEEP_ALL_HISTORY_QOS;
  readerQos.history.depth                            = 1;
  readerQos.destination_order.kind                   = ::DDS::BY_SOURCE_TIMESTAMP_DESTINATIONORDER_QOS;
//  readerQos.resource_limits.max_samples_per_instance = ::DDS::LENGTH_UNLIMITED;

  // Reliability varies with the transport implementation.
  switch( this->options_.transportType()) {
    case Options::TCP:
    case Options::MC:
      readerQos.reliability.kind = ::DDS::RELIABLE_RELIABILITY_QOS;
      break;

    case Options::UDP:
      readerQos.reliability.kind = ::DDS::BEST_EFFORT_RELIABILITY_QOS;
      break;

    case Options::TRANSPORT_NONE:
    default:
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: Subscriber::Subscriber() - ")
        ACE_TEXT("unrecognized transport when setting up Qos policies.\n")
      ));
      throw BadQosException();
  }

  // Create the reader.
  this->reader_ = this->subscriber_->create_datareader(
                    this->topic_.in(),
                    readerQos,
                    DDS::DataReaderListener::_nil(),
                    ::OpenDDS::DCPS::DEFAULT_STATUS_MASK
                  );
  if( CORBA::is_nil( this->reader_.in())) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: Subscriber::Subscriber() - ")
      ACE_TEXT("failed to create reader.\n")
    ));
    throw BadReaderException();

  }

  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT("(%P|%t) Subscriber::Subscriber() - ")
    ACE_TEXT("created reader.\n")
  ));

  // Set the listener mask here so that we don't conflict with the
  // StatusCondition(s) that we want to wait on in the main thread.
  this->reader_->set_listener( this->listener_, DDS::DATA_AVAILABLE_STATUS);

  // Grab, enable and attach the status condition for test synchronization.
  this->status_ = this->reader_->get_statuscondition();
  this->status_->set_enabled_statuses( DDS::SUBSCRIPTION_MATCHED_STATUS);
  if (this->waiter_->attach_condition( this->status_.in()) != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: Subscriber::Subscriber() - ")
      ACE_TEXT("failed to match subscription.\n")
    ));
    throw BadAttachException();
  }

  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT("(%P|%t) Subscriber::Subscriber() - ")
    ACE_TEXT("created StatusCondition and WaitSet for test synchronization.\n")
  ));

}

unsigned int
Subscriber::count() const
{
  return this->listener_->count();
}

bool
Subscriber::passed() const
{
  return this->listener_->passed();
}

void
Subscriber::run()
{
  DDS::Duration_t   timeout = { DDS::DURATION_INFINITE_SEC, DDS::DURATION_INFINITE_NSEC};
  DDS::ConditionSeq conditions;
  DDS::SubscriptionMatchedStatus matches = { 0, 0, 0, 0, 0};
  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT("(%P|%t) Subscriber::run() - ")
    ACE_TEXT("waiting for publications to attach.\n")
  ));
  do {
    if( DDS::RETCODE_OK != this->waiter_->wait( conditions, timeout)) {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: Subscriber::run() - ")
        ACE_TEXT("failed to synchronize at start of test.\n")
      ));
      throw BadSyncException();
    }

    if (this->reader_->get_subscription_matched_status (matches) != ::DDS::RETCODE_OK)
    {
      ACE_ERROR ((LM_ERROR,
        "ERROR: failed to get subscription matched status\n"));
      ACE_OS::exit (1);
    }

    if( this->options_.verbose()) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Subscriber::run() - ")
        ACE_TEXT("%d publications attached.\n"),
        matches.current_count
      ));
    }

  } while( matches.current_count > 0);

  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT("(%P|%t) Subscriber::run() - ")
    ACE_TEXT("shutting down after publications were removed.\n")
  ));

}

} // End of namespace Test

