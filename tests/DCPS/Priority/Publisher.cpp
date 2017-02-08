// -*- C++ -*-

#include "Publisher.h"

#include "Test.h"
#include "Options.h"

#include "TestTypeSupportImpl.h"

#include "dds/DCPS/Service_Participant.h"
#include "dds/DCPS/Marked_Default_Qos.h"
#include "dds/DCPS/PublisherImpl.h"

#include "dds/DCPS/StaticIncludes.h"
#if defined ACE_AS_STATIC_LIBS && !defined OPENDDS_SAFETY_PROFILE
#include "dds/DCPS/transport/udp/Udp.h"
#include "dds/DCPS/transport/multicast/Multicast.h"
#endif

#include <sstream>

namespace Test {

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
 : options_( options),
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
  this->topic_ = this->participant_->create_topic(
                   this->options_.topicName().c_str(),
                   type_name,
                   TOPIC_QOS_DEFAULT,
                   ::DDS::TopicListener::_nil(),
                   ::OpenDDS::DCPS::DEFAULT_STATUS_MASK
                 );
  if( CORBA::is_nil( this->topic_.in())) {
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
  if( CORBA::is_nil( this->publisher_.in())) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: Publisher::Publisher() - ")
      ACE_TEXT("failed to create publisher.\n")
    ));
    throw BadPublisherException();

  }

  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT("(%P|%t) Publisher::Publisher() - ")
    ACE_TEXT("created publisher.\n")
  ));

  // Writer Qos policy values.
  ::DDS::DataWriterQos writerQos;
  this->publisher_->get_default_datawriter_qos( writerQos);

//  writerQos.durability.kind                          = ::DDS::TRANSIENT_LOCAL_DURABILITY_QOS;
  writerQos.history.kind                             = ::DDS::KEEP_ALL_HISTORY_QOS;
  // this number is set to have a big enough queue of data to see that the higher priority
  // message is on its own queue, if tests start failing, then this number should be increased
  // or possibly sample0.baggage length increased
  writerQos.resource_limits.max_samples_per_instance = 100;
  writerQos.resource_limits.max_samples = 100;

  // Reliability varies with the transport implementation.
  writerQos.reliability.max_blocking_time.sec = 1;
  writerQos.reliability.max_blocking_time.nanosec = 0;
  switch( this->options_.transportType()) {
    case Options::TCP:
    case Options::MC:
      writerQos.reliability.kind = ::DDS::RELIABLE_RELIABILITY_QOS;
      break;

    case Options::UDP:
      writerQos.reliability.kind = ::DDS::BEST_EFFORT_RELIABILITY_QOS;
      break;

    case Options::TRANSPORT_NONE:
    default:
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: Publisher::Publisher() - ")
        ACE_TEXT("unrecognized transport when setting up Qos policies.\n")
      ));
      throw BadQosException();
  }

  // Create the writer.
  this->writer_[0] = this->publisher_->create_datawriter(
                    this->topic_.in(),
                    writerQos,
                    DDS::DataWriterListener::_nil(),
                    ::OpenDDS::DCPS::DEFAULT_STATUS_MASK
                  );
  if( CORBA::is_nil( this->writer_[0].in())) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: Publisher::Publisher() - ")
      ACE_TEXT("failed to create writer[0].\n")
    ));
    throw BadWriterException();

  }

  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT("(%P|%t) Publisher::Publisher() - ")
    ACE_TEXT("created writer[0].\n")
  ));

  // Grab, enable and attach the status condition for test synchronization.
  this->status_[0] = this->writer_[0]->get_statuscondition();
  this->status_[0]->set_enabled_statuses( DDS::PUBLICATION_MATCHED_STATUS);
  if (this->waiter_->attach_condition( this->status_[0].in()) != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: Publisher::Publisher() - ")
      ACE_TEXT("failed to match publication.\n")
    ));
    throw BadAttachException();
  }

  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT("(%P|%t) Publisher::Publisher() - ")
    ACE_TEXT("created StatusCondition[0] for test synchronization.\n")
  ));

  // Actually set the priority finally.
  writerQos.transport_priority.value = this->options_.priority();

  // Create the writer.
  this->writer_[1] = this->publisher_->create_datawriter(
                    this->topic_.in(),
                    writerQos,
                    DDS::DataWriterListener::_nil(),
                    ::OpenDDS::DCPS::DEFAULT_STATUS_MASK
                  );
  if( CORBA::is_nil( this->writer_[1].in())) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: Publisher::Publisher() - ")
      ACE_TEXT("failed to create writer[1].\n")
    ));
    throw BadWriterException();

  }

  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT("(%P|%t) Publisher::Publisher() - ")
    ACE_TEXT("created writer[1].\n")
  ));

  // Grab, enable and attach the status condition for test synchronization.
  this->status_[1] = this->writer_[1]->get_statuscondition();
  this->status_[1]->set_enabled_statuses( DDS::PUBLICATION_MATCHED_STATUS);
  if (this->waiter_->attach_condition( this->status_[1].in()) != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: Publisher::Publisher() - ")
      ACE_TEXT("failed to match publication.\n")
    ));
    throw BadAttachException();
  }

  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT("(%P|%t) Publisher::Publisher() - ")
    ACE_TEXT("created StatusCondition[1] for test synchronization.\n")
  ));

}

void
Publisher::run()
{
  DDS::Duration_t   timeout = { DDS::DURATION_INFINITE_SEC, DDS::DURATION_INFINITE_NSEC};
  DDS::ConditionSeq conditions;
  DDS::PublicationMatchedStatus matches = { 0, 0, 0, 0, 0};
  unsigned int cummulative_count = 0;
  do {
    if( this->options_.verbose()) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Publisher::run() - ")
        ACE_TEXT("%d of 2 subscriptions attached, waiting for more.\n"),
        cummulative_count
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

  } while( cummulative_count < 2);

  /// Kluge to ensure that the remote/subscriber side endpoints have
  /// been fully associated before starting to send.  This appears to be
  /// a race between the association creation and use and the BuiltIn
  /// Topic data becoming available.  There is no existing mechanism (nor
  /// should there be) to prevent an association from exchanging data
  /// prior to the remote endpoint information becoming available via the
  /// BuiltIn Topic publications.
  ACE_OS::sleep( 2);

  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT("(%P|%t) Publisher::run() - ")
    ACE_TEXT("starting to publish samples.\n")
  ));

  Test::DataDataWriter_var writer0
    = Test::DataDataWriter::_narrow( this->writer_[0].in());
  Test::DataDataWriter_var writer1
    = Test::DataDataWriter::_narrow( this->writer_[1].in());
  Test::Data sample0;
  Test::Data sample1;
  sample0.key = 1;
  sample0.value = 0;
  // before_value is just for the high priority sample, low priority samples are in order
  sample0.before_value = 0;
  sample0.priority = false;
  // add some extra baggage to ensure
  sample0.baggage.length(9999);

  if (options_.multipleInstances())
    sample1.key = 2;
  else
    sample1.key = 1;
  sample1.value = 0;
  // will determine later which value this sample should be seen before
  sample1.before_value = 0;
  sample1.priority = true;
  bool sent = false;
  for (unsigned long num_samples = 1; num_samples < (unsigned long)-1 && !sent; ++num_samples) {
    ++sample0.value;
    if (writer0->write( sample0, DDS::HANDLE_NIL) == DDS::RETCODE_TIMEOUT) {
      // indicate the high priority sample should arrive before the indicated low priority sample
      sample1.before_value = sample0.value - 1;
      while (writer1->write( sample1, DDS::HANDLE_NIL) == DDS::RETCODE_TIMEOUT) {
        ACE_ERROR((LM_ERROR,
          ACE_TEXT("(%P|%t) ERROR: Publisher::run() - ")
          ACE_TEXT("should not have backpressure for the second writer.\n")
        ));
      }
      sent = true;
    }
  }

  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT("(%P|%t) Publisher::run() - ")
    ACE_TEXT("finished publishing %d samples.\n"),
    sample0.value
  ));

  // Make sure that the data has arriven.
  ::DDS::Duration_t shutdownDelay = {15, 0}; // Wait up to a total of 15
                                             // seconds to finish the test.
  if (this->options_.transportType() != Options::UDP && this->options_.transportType() != Options::TCP) {
    writer0->wait_for_acknowledgments(shutdownDelay);
    writer1->wait_for_acknowledgments(shutdownDelay);
  } else {
    // Wait for acks won't work with UDP...
    ACE_OS::sleep(15);
  }
}

} // End of namespace Test

