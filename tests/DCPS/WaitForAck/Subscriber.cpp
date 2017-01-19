// -*- C++ -*-

#include "Subscriber.h"

#include "Test.h"
#include "Options.h"

#include "TestTypeSupportImpl.h"

#include "dds/DCPS/Service_Participant.h"
#include "dds/DCPS/Marked_Default_Qos.h"
#include "dds/DCPS/DataReaderImpl.h"
#include "dds/DCPS/SubscriberImpl.h"

#include "dds/DCPS/StaticIncludes.h"
#ifdef ACE_AS_STATIC_LIBS
#include <dds/DCPS/transport/rtps_udp/RtpsUdp.h>
#endif

#include <sstream>

namespace Test {

Subscriber::~Subscriber()
{
  this->waiter_->detach_condition( this->status_.in());

  if( ! CORBA::is_nil( this->participant_.in())) {
    this->participant_->delete_contained_entities();
    DDS::DomainParticipantFactory_var dpf = TheParticipantFactory;
    dpf->delete_participant( this->participant_.in());
  }
  TheServiceParticipant->shutdown();
}

Subscriber::Subscriber( const Options& options)
 : options_( options),
   participant_(0),
   waiter_( new DDS::WaitSet),
   status_(0)
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

  } else if( this->options_.verbose()) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) Subscriber::Subscriber() - ")
      ACE_TEXT("created type %C support.\n"),
      type_name.in()
    ));
  }

  // Create the topic.
  DDS::Topic_var topic = this->participant_->create_topic(
                           this->options_.topicName().c_str(),
                           type_name,
                           TOPIC_QOS_DEFAULT,
                           ::DDS::TopicListener::_nil(),
                           ::OpenDDS::DCPS::DEFAULT_STATUS_MASK
                         );
  if( CORBA::is_nil( topic.in())) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: Subscriber::Subscriber() - ")
      ACE_TEXT("failed to create topic %C.\n"),
      this->options_.topicName().c_str()
    ));
    throw BadTopicException();

  } else if( this->options_.verbose()) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) Subscriber::Subscriber() - ")
      ACE_TEXT("created topic %C.\n"),
      this->options_.topicName().c_str()
    ));
  }

  // Create the subscriber.
  DDS::Subscriber_var subscriber
    = this->participant_->create_subscriber(
        SUBSCRIBER_QOS_DEFAULT,
        ::DDS::SubscriberListener::_nil(),
        ::OpenDDS::DCPS::DEFAULT_STATUS_MASK
      );
  if( CORBA::is_nil( subscriber.in())) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: Subscriber::Subscriber() - ")
      ACE_TEXT("failed to create subscriber.\n")
    ));
    throw BadSubscriberException();

  } else if( this->options_.verbose()) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) Subscriber::Subscriber() - ")
      ACE_TEXT("created subscriber.\n")
    ));
  }

  // Reader Qos policy values.
  ::DDS::DataReaderQos readerQos;
  subscriber->get_default_datareader_qos( readerQos);
  readerQos.reliability.kind = ::DDS::RELIABLE_RELIABILITY_QOS;

  // Create the readers.
  for( int index = 0; index < 2; ++index) {
    ::DDS::DataReader_var reader
      = subscriber->create_datareader(
          topic.in(),
          readerQos,
          DDS::DataReaderListener::_nil(),
          ::OpenDDS::DCPS::DEFAULT_STATUS_MASK
        );
    if( CORBA::is_nil( reader.in())) {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: Subscriber::Subscriber() - ")
        ACE_TEXT("failed to create reader.\n")
      ));
      throw BadReaderException();

    } else if( this->options_.verbose()) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Subscriber::Subscriber() - ")
        ACE_TEXT("created reader.\n")
      ));
    }

    this->reader_[ index] = ::OpenDDS::DCPS::DataReaderEx::_narrow( reader.in());
    if( CORBA::is_nil( this->reader_[ index].in())) {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: Subscriber::Subscriber() - ")
        ACE_TEXT("failed to narrow reader to extract statistics.\n")
      ));
      throw BadReaderException();
    }

    // Grab, enable and attach the status condition for test synchronization.
    this->status_ = this->reader_[ index]->get_statuscondition();
    this->status_->set_enabled_statuses( DDS::SUBSCRIPTION_MATCHED_STATUS);
    this->waiter_->attach_condition( this->status_.in());

    if( this->options_.verbose()) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Subscriber::Subscriber() - ")
        ACE_TEXT("created StatusCondition and WaitSet for test synchronization.\n")
      ));
    }
  }
}

void
Subscriber::run()
{
  DDS::Duration_t   timeout = { DDS::DURATION_INFINITE_SEC, DDS::DURATION_INFINITE_NSEC};
  DDS::ConditionSeq conditions;
  DDS::SubscriptionMatchedStatus matches = { 0, 0, 0, 0, 0};
  int current_count = 0;
  int total_count = 0;
  do {
    if( this->options_.verbose()) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Subscriber::run() - ")
        ACE_TEXT("%d publications attached.\n"),
        current_count
      ));
    }
    if( DDS::RETCODE_OK != this->waiter_->wait( conditions, timeout)) {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: Subscriber::run() - ")
        ACE_TEXT("failed to synchronize at start of test.\n")
      ));
      throw BadSyncException();
    }
    if (this->reader_[ 0]->get_subscription_matched_status(matches) != ::DDS::RETCODE_OK)
    {
      ACE_ERROR ((LM_ERROR,
        ACE_TEXT ("ERROR: failed to get subscription matched status\n")));
      ACE_OS::exit (1);
    }

    current_count = matches.current_count;
    total_count = matches.total_count;

    if (this->reader_[ 1]->get_subscription_matched_status(matches) != ::DDS::RETCODE_OK)
    {
      ACE_ERROR ((LM_ERROR,
        ACE_TEXT("ERROR: failed to get subscription matched status\n")));
      ACE_OS::exit (1);
    }

    current_count += matches.current_count;
    total_count += matches.total_count;

  } while(total_count == 0 || current_count > 0);

  if( this->options_.verbose()) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) Subscriber::run() - ")
      ACE_TEXT("shutting down after all publications were removed.\n")
    ));
  }
}

} // End of namespace Test

