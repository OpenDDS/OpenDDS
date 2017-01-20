// -*- C++ -*-

#include "Publisher.h"

#include "Test.h"
#include "Options.h"
#include "Writer.h"

#include "TestTypeSupportImpl.h"

#include "dds/DCPS/Service_Participant.h"
#include "dds/DCPS/Marked_Default_Qos.h"
#include "dds/DCPS/PublisherImpl.h"
#include "dds/DCPS/StaticIncludes.h"
#ifdef ACE_AS_STATIC_LIBS
#include <dds/DCPS/transport/rtps_udp/RtpsUdp.h>
#endif

#include "ace/Condition_T.h"
#include "ace/OS_NS_unistd.h"

#include <sstream>

namespace Test {

int
Publisher::status() const
{
  return this->status_;
}

Publisher::~Publisher()
{
  DDS::ConditionSeq conditions;
  this->waiter_->get_conditions( conditions);
  this->waiter_->detach_conditions( conditions);

  if( ! CORBA::is_nil( this->participant_.in())) {
    this->participant_->delete_contained_entities();
    DDS::DomainParticipantFactory_var dpf = TheParticipantFactory;
    dpf->delete_participant( this->participant_.in());
  }
  TheServiceParticipant->shutdown();
}

Publisher::Publisher( const Options& options)
 : status_( 0),
   options_( options),
   participant_(0),
   publisher_(0),
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
      ACE_TEXT("(%P|%t) ERROR: Publisher::Publisher() - ")
      ACE_TEXT("failed to create a participant.\n")
    ));
    throw BadParticipantException();

  } else if( this->options_.verbose()) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) Publisher::Publisher() - ")
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
      ACE_TEXT("(%P|%t) ERROR: Publisher::Publisher() - ")
      ACE_TEXT("unable to install type %C support.\n"),
      type_name.in()
    ));
    throw BadTypeSupportException ();

  } else if( this->options_.verbose()) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) Publisher::Publisher() - ")
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
      ACE_TEXT("(%P|%t) ERROR: Publisher::Publisher() - ")
      ACE_TEXT("failed to create topic %C.\n"),
      this->options_.topicName().c_str()
    ));
    throw BadTopicException();

  } else if( this->options_.verbose()) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) Publisher::Publisher() - ")
      ACE_TEXT("created topic %C.\n"),
      this->options_.topicName().c_str()
    ));
  }

  // Create the publisher.
  this->publisher_ = this->participant_->create_publisher(
      PUBLISHER_QOS_DEFAULT,
      ::DDS::PublisherListener::_nil(),
      ::OpenDDS::DCPS::DEFAULT_STATUS_MASK
  );
  if( CORBA::is_nil(this->publisher_.in())) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: Publisher::Publisher() - ")
      ACE_TEXT("failed to create publisher.\n")
    ));
    throw BadPublisherException();

  } else if( this->options_.verbose()) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) Publisher::Publisher() - ")
      ACE_TEXT("created publisher.\n")
    ));
  }

  // Writer Qos policy values.
  ::DDS::DataWriterQos writerQos;
  this->publisher_->get_default_datawriter_qos( writerQos);

  writerQos.durability.kind                          = ::DDS::TRANSIENT_LOCAL_DURABILITY_QOS;
  writerQos.history.kind                             = ::DDS::KEEP_ALL_HISTORY_QOS;
  writerQos.resource_limits.max_samples_per_instance = ::DDS::LENGTH_UNLIMITED;
  writerQos.reliability.kind                         = ::DDS::RELIABLE_RELIABILITY_QOS;

  if( this->options_.verbose()) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) Publisher::Publisher() - ")
      ACE_TEXT("starting to create %d publications.\n"),
      this->options_.publications()
    ));
  }

  // Build as many publications as are specified.
  for( int index = 0; index < this->options_.publications(); ++index) {
    // Create the writer.
    DDS::DataWriter_var writer
      = this->publisher_->create_datawriter(
          topic.in(),
          writerQos,
          DDS::DataWriterListener::_nil(),
          ::OpenDDS::DCPS::DEFAULT_STATUS_MASK
        );
    if( CORBA::is_nil( writer.in())) {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: Publisher::Publisher() - ")
        ACE_TEXT("failed to create writer.\n")
      ));
      throw BadWriterException();

    } else if( this->options_.verbose()) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Publisher::Publisher() - ")
        ACE_TEXT("created publication %d.\n"),
        (1+index)
      ));
    }

    // Create a publication and store it.
    this->publications_.push_back(
      new Writer( writer.in(), index, this->options_.verbose())
    );

    //
    // Grab, enable and attach the status condition for test
    // synchronization of the current publication.
    //
    DDS::StatusCondition_var status = writer->get_statuscondition();
    status->set_enabled_statuses( DDS::PUBLICATION_MATCHED_STATUS);
    this->waiter_->attach_condition( status.in());

    if( this->options_.verbose()) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Publisher::Publisher() - ")
        ACE_TEXT("created StatusCondition for publication %d.\n"),
        (1+index)
      ));
    }
  }
}

void
Publisher::run()
{
  DDS::Duration_t   timeout = { DDS::DURATION_INFINITE_SEC, DDS::DURATION_INFINITE_NSEC};
  DDS::ConditionSeq conditions;
  DDS::PublicationMatchedStatus matches = { 0, 0, 0, 0, 0};
  const int readers_per_publication = 2;
  unsigned int cummulative_count = 0;
  do {
    if( this->options_.verbose()) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Publisher::run() - ")
        ACE_TEXT("%d of %d subscriptions attached, waiting for more.\n"),
        cummulative_count,
        this->publications_.size()*readers_per_publication
      ));
    }
    if( DDS::RETCODE_OK != this->waiter_->wait( conditions, timeout)) {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: Publisher::run() - ")
        ACE_TEXT("failed to synchronize at start of test.\n")
      ));
      throw BadSyncException();
    }
    for( unsigned long index = 0; index < conditions.length(); ++index) {
      DDS::StatusCondition_var condition
        = DDS::StatusCondition::_narrow( conditions[ index].in());

      DDS::Entity_var writer_entity = condition->get_entity();
      DDS::DataWriter_var writer = DDS::DataWriter::_narrow( writer_entity);
      if( !CORBA::is_nil( writer.in())) {
        DDS::StatusMask changes = writer->get_status_changes();
        if( changes & DDS::PUBLICATION_MATCHED_STATUS) {
          if (writer->get_publication_matched_status(matches) != ::DDS::RETCODE_OK)
          {
            ACE_ERROR ((LM_ERROR,
              "ERROR: failed to get publication matched status\n"));
            ACE_OS::exit (1);
          }
          cummulative_count += matches.current_count_change;
        }
      }
    }

  // We know that there are 2 subscriptions matched with each publication.
  } while( cummulative_count < (readers_per_publication*this->publications_.size()));

  // Kluge to bias the race between BuiltinTopic samples and application
  // samples towards the BuiltinTopics during association establishment.
  // ACE_OS::sleep( 2);

  if( this->options_.verbose()) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) Publisher::run() - ")
      ACE_TEXT("starting to publish samples with %d matched subscriptions.\n"),
      cummulative_count
    ));
  }

  for( unsigned int index = 0; index < this->publications_.size(); ++index) {
    this->publications_[ index]->start();
  }

  // Allow some traffic to occur before making any wait() calls.
  ACE_OS::sleep( 2);

  ::DDS::Duration_t delay = { 5, 0 }; // Wait for up to 5 seconds.
  if (this->options_.publisher())
  {
    DDS::ReturnCode_t error =
      this->publisher_->wait_for_acknowledgments(delay);
    if (error != DDS::RETCODE_OK)
    {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) ERROR: Publisher::run() - ")
        ACE_TEXT("publisher wait failed with code: %d.\n"),
        error));
      ++this->status_;
    }
  }
  else
  {
    for( unsigned int index = 0; index < this->publications_.size(); ++index) {
      // First wait on this writer.
      ::DDS::ReturnCode_t result
        = this->publications_[ index]->wait_for_acks( delay);
      if( result != ::DDS::RETCODE_OK) {
        ACE_DEBUG((LM_DEBUG,
          ACE_TEXT("(%P|%t) ERROR: Publisher::run() - ")
          ACE_TEXT("publication %d wait failed with code: %d.\n"),
          index,
          result
        ));
        ++this->status_;
      }
    }
  }

  // Signal the writers to terminate.
  for( unsigned int index = 0; index < this->publications_.size(); ++index) {
    this->publications_[ index]->stop();
  }

  // Additional wait() calls will be made by each thread during shutdown.

  // Separate loop so the termination messages can be handled concurrently.
  for( unsigned int index = 0; index < this->publications_.size(); ++index) {
    // Join and clean up.
    this->publications_[ index]->wait();
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) Publisher::run() - ")
      ACE_TEXT("publication %d stopping after sending %d messages.\n"),
      index,
      this->publications_[ index]->messages()
    ));
    this->status_ += this->publications_[ index]->status();
    delete this->publications_[ index];
  }
  this->publications_.clear();

  if( this->options_.verbose()) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) Publisher::run() - ")
      ACE_TEXT("finished publishing samples.\n")
    ));
  }
}

} // End of namespace Test

