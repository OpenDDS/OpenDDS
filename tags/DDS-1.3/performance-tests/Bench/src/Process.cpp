// -*- C++ -*-
// $Id$

#include "Process.h"

#include "Test.h"
#include "Options.h"
#include "EntityProfiles.h"
#include "Publication.h"
#include "Subscription.h"
#include "TestTypeSupportImpl.h"

#include "dds/DCPS/transport/framework/TheTransportFactory.h"

#include <iostream>
#include <fstream>

namespace Test {

Process::~Process()
{
  // Clean up the wait conditions.
  DDS::ConditionSeq conditions;
  this->publicationWaiter_->get_conditions( conditions);
  this->publicationWaiter_->detach_conditions( conditions);

  this->subscriptionWaiter_->get_conditions( conditions);
  this->subscriptionWaiter_->detach_conditions( conditions);

  // Clean up the participants and their resources.
  for( ParticipantMap::iterator current = this->participants_.begin();
       current != this->participants_.end();
       ++current
     ) {
    current->second->delete_contained_entities();
    TheParticipantFactory->delete_participant( current->second.ptr());
  }

  // Clean up the rest of the service resources.
  TheTransportFactory->release();
  TheServiceParticipant->shutdown();

  // Close the output files.
  for( OutputFileMap::iterator current = this->outputFiles_.begin();
       current != this->outputFiles_.end();
       ++current
     ) {
    current->second->rdbuf()->close();
  }
}

Process::Process( const Options& options)
 : options_( options),
   publicationWaiter_( new DDS::WaitSet),
   subscriptionWaiter_( new DDS::WaitSet),
   guardCondition_( new DDS::GuardCondition),
   condition_( this->lock_),
   terminated_( false)
{
  for( Options::ParticipantProfileMap::const_iterator current
         = this->options_.participantProfileMap().begin();
       current != this->options_.participantProfileMap().end();
       ++current
     ) {
    // Create the current participant.
    this->participants_[ current->first]
      = TheParticipantFactory->create_participant(
          current->second->domainId,
          current->second->qos,
          DDS::DomainParticipantListener::_nil()
        );
    if( CORBA::is_nil( this->participants_[ current->first].in())) {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: Process::Process() - ")
        ACE_TEXT("failed to create participant %C.\n"),
        current->first.c_str()
      ));
      throw BadParticipantException();

    } else if( this->options_.verbose()) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Process::Process() - ")
        ACE_TEXT("created participant %C in domain %d.\n"),
        current->first.c_str(),
        current->second->domainId
      ));
    }

    // Create and register the type support in this participant.  We do
    // this here since there is only a single data type defined for the
    // verification test framework at this time.
    DataTypeSupportImpl* testData = new DataTypeSupportImpl();
    if( ::DDS::RETCODE_OK
     != testData->register_type( this->participants_[ current->first].in(), 0)) {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: Process::Process() - ")
        ACE_TEXT("unable to install type %C support into participant %C.\n"),
        testData->get_type_name(),
        current->first.c_str()
      ));
      throw BadTypeSupportException ();

    } else if( this->options_.verbose()) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Process::Process() - ")
        ACE_TEXT("created type %C support in participant %C.\n"),
        testData->get_type_name(),
        current->first.c_str()
      ));
    }

    // Save the data type name for later use in creating Topics.
    if( this->dataTypeName_.empty()) {
      this->dataTypeName_ = testData->get_type_name();
    }
  }

  for( Options::TopicProfileMap::const_iterator current
         = this->options_.topicProfileMap().begin();
       current != this->options_.topicProfileMap().end();
       ++current
     ) {
    // Grab the participant in which this topic will be installed.
    ParticipantMap::iterator participantLocation
      = this->participants_.find( current->second->participant);
    if( participantLocation == this->participants_.end()) {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: Process::Process() - ")
        ACE_TEXT("failed to obtain participant %C for topic %C.\n"),
        current->second->participant.c_str(),
        current->first.c_str()
      ));
      throw BadParticipantException();
    }
    DDS::DomainParticipant_var participant = participantLocation->second;

    // Create the topic.
    this->topics_[ current->first]
      = participant->create_topic(
          current->first.c_str(),
          this->dataTypeName_.c_str(),
          current->second->qos,
          DDS::TopicListener::_nil()
        );
    if( CORBA::is_nil( this->topics_[ current->first].in())) {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: Process::Process() - ")
        ACE_TEXT("failed to create topic %C in participant %C.\n"),
        current->first.c_str(),
        current->second->participant.c_str()
      ));
      throw BadTopicException();

    } else if( this->options_.verbose()) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Process::Process() - ")
        ACE_TEXT("created topic %C in participant %C.\n"),
        current->first.c_str(),
        current->second->participant.c_str()
      ));
    }
  }

  for( Options::SubscriptionProfileMap::const_iterator current
         = this->options_.subscriptionProfileMap().begin();
       current != this->options_.subscriptionProfileMap().end();
       ++current
     ) {
    // Grab the topic for which this subscription will be created.
    TopicMap::iterator topicLocation
      = this->topics_.find( current->second->topic);
    if( topicLocation == this->topics_.end()) {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: Process::Process() - ")
        ACE_TEXT("failed to obtain topic %C for subscription %C.\n"),
        current->second->topic.c_str(),
        current->first.c_str()
      ));
      throw BadTopicException();
    }
    DDS::Topic_var topic = topicLocation->second;

    // Grab the participant in which this subscription will be installed.
    Options::TopicProfileMap::const_iterator where
      = this->options_.topicProfileMap().find( current->second->topic);
    if( where == this->options_.topicProfileMap().end()) {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: Process::Process() - ")
        ACE_TEXT("failed to find topic profile %C for subscription %C.\n"),
        current->second->topic.c_str(),
        current->first.c_str()
      ));
      throw BadTopicException();
    }
    ParticipantMap::iterator participantLocation
      = this->participants_.find( where->second->participant);
    if( participantLocation == this->participants_.end()) {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: Process::Process() - ")
        ACE_TEXT("failed to obtain participant %C for subscription %C.\n"),
        where->second->participant.c_str(),
        current->first.c_str()
      ));
      throw BadParticipantException();
    }
    DDS::DomainParticipant_var participant = participantLocation->second;

    // Extract the output filename, if any.
    if( !current->second->datafile.empty()) {
      OutputFileMap::iterator where
        = this->outputFiles_.find( current->second->datafile);
      if( where == this->outputFiles_.end()) {
        this->outputFiles_.insert(
          OutputFileMap::value_type(
            current->second->datafile,
            new std::ofstream()
          )
        );
      }
    }

    // Create the subscription.
    this->subscriptions_[ current->first]
      = new Subscription(
              current->first.c_str(),
              current->second,
              this->options_.verbose()
            );
    if( this->subscriptions_[ current->first] == 0) {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: Process::Process() - ")
        ACE_TEXT("failed to create subscription %C in participant %C.\n"),
        current->first.c_str(),
        where->second->participant.c_str()
      ));
      throw BadTopicException();

    } else if( this->options_.verbose()) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Process::Process() - ")
        ACE_TEXT("created subscription %C in participant %C.\n"),
        current->first.c_str(),
        where->second->participant.c_str()
      ));
    }

    // Enable the subscription.
    this->subscriptions_[ current->first]->enable( participant, topic);

    // Extract the subscription status condition.
    DDS::StatusCondition_var status
      = this->subscriptions_[ current->first]->get_statuscondition();
    status->set_enabled_statuses( DDS::SUBSCRIPTION_MATCH_STATUS);
    this->subscriptionWaiter_->attach_condition( status.in());

    if( this->options_.verbose()) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Process::Process() - ")
        ACE_TEXT("extracted StatusCondition for ")
        ACE_TEXT("subscription %C in participant %C.\n"),
        current->first.c_str(),
        where->second->participant.c_str()
      ));
    }
  }

  // Add our GuardCondition to the subscription WaitSet so that we can
  // command it to stop waiting.
  this->subscriptionWaiter_->attach_condition( this->guardCondition_.in());

  for( Options::PublicationProfileMap::const_iterator current
         = this->options_.publicationProfileMap().begin();
       current != this->options_.publicationProfileMap().end();
       ++current
     ) {
    // Grab the topic for which this subscription will be created.
    TopicMap::iterator topicLocation
      = this->topics_.find( current->second->topic);
    if( topicLocation == this->topics_.end()) {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: Process::Process() - ")
        ACE_TEXT("failed to obtain topic %C for publication %C.\n"),
        current->second->topic.c_str(),
        current->first.c_str()
      ));
      throw BadTopicException();
    }
    DDS::Topic_var topic = topicLocation->second;

    // Grab the participant in which this subscription will be installed.
    Options::TopicProfileMap::const_iterator where
      = this->options_.topicProfileMap().find( current->second->topic);
    if( where == this->options_.topicProfileMap().end()) {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: Process::Process() - ")
        ACE_TEXT("failed to find topic profile %C for publication %C.\n"),
        current->second->topic.c_str(),
        current->first.c_str()
      ));
      throw BadTopicException();
    }
    ParticipantMap::iterator participantLocation
      = this->participants_.find( where->second->participant);
    if( participantLocation == this->participants_.end()) {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: Process::Process() - ")
        ACE_TEXT("failed to obtain participant %C for publication %C.\n"),
        where->second->participant.c_str(),
        current->first.c_str()
      ));
      throw BadParticipantException();
    }
    DDS::DomainParticipant_var participant = participantLocation->second;

    // Create the publication.
    this->publications_[ current->first]
      = new Publication(
              current->first.c_str(),
              current->second,
              this->options_.verbose()
            );
    if( this->publications_[ current->first] == 0) {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: Process::Process() - ")
        ACE_TEXT("failed to create publication %C in participant %C.\n"),
        current->first.c_str(),
        where->second->participant.c_str()
      ));
      throw BadPublisherException();

    } else if( this->options_.verbose()) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Process::Process() - ")
        ACE_TEXT("created publication %C in participant %C.\n"),
        current->first.c_str(),
        where->second->participant.c_str()
      ));
    }

    // Enable the publication.
    this->publications_[ current->first]->enable( participant, topic);

    // Extract the publication status condition.
    DDS::StatusCondition_var status
      = this->publications_[ current->first]->get_statuscondition();
    status->set_enabled_statuses( DDS::PUBLICATION_MATCH_STATUS);
    this->publicationWaiter_->attach_condition( status.in());

    if( this->options_.verbose()) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Process::Process() - ")
        ACE_TEXT("extracted StatusCondition for ")
        ACE_TEXT("publication %C in participant %C.\n"),
        current->first.c_str(),
        where->second->participant.c_str()
      ));
    }

    // Now attach any wrap arounds.
    if( !current->second->source.empty()) {
      Subscription* subscription
        = this->subscriptions_[ current->second->source];
      if( subscription == 0) {
        ACE_ERROR((LM_ERROR,
          ACE_TEXT("(%P|%t) ERROR: Process::Process() - ")
          ACE_TEXT("failed to obtain subscription %C ")
          ACE_TEXT("to forward for publication %C.\n"),
          current->second->source.c_str(),
          current->first.c_str()
        ));
        throw BadSubscriberException();
      }
      subscription->set_destination( this->publications_[ current->first]);
    }
  }

  // Go ahead and get the output files opened and ready for use.
  for( OutputFileMap::iterator current = this->outputFiles_.begin();
       current != this->outputFiles_.end();
       ++current
     ) {
    current->second->open( current->first.c_str());
  }
}

void
Process::unblock()
{
  if( this->options_.verbose()) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) Process::unblock() - ")
      ACE_TEXT("unblocking main thread and unsticking termination conditions.\n")
    ));
  }
  this->terminated_ = true;
  this->guardCondition_->set_trigger_value( true);
  this->condition_.signal();
}

void
Process::run()
{
  DDS::Duration_t   timeout = { DDS::DURATION_INFINITY_SEC, DDS::DURATION_INFINITY_NSEC};
  DDS::ConditionSeq conditions;
  DDS::PublicationMatchStatus publicationMatches = { 0, 0, 0, 0, 0};
  unsigned int cummulative_count = 0;
  do {
    if( this->options_.verbose()) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Process::run() - ")
        ACE_TEXT("%d of %d subscriptions attached to publications, waiting for more.\n"),
        cummulative_count,
        this->publications_.size()
      ));
    }
    if( DDS::RETCODE_OK != this->publicationWaiter_->wait( conditions, timeout)) {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: Process::run() - ")
        ACE_TEXT("failed to synchronize at start of test.\n")
      ));
      throw BadSyncException();
    }
    for( unsigned long index = 0; index < conditions.length(); ++index) {
      DDS::StatusCondition_var condition
        = DDS::StatusCondition::_narrow( conditions[ index].in());

      DDS::DataWriter_var writer = DDS::DataWriter::_narrow( condition->get_entity());
      if( !CORBA::is_nil( writer.in())) {
        DDS::StatusKindMask changes = writer->get_status_changes();
        if( changes & DDS::PUBLICATION_MATCH_STATUS) {
          publicationMatches = writer->get_publication_match_status();
          cummulative_count += publicationMatches.current_count_change;
        }
      }
    }

  // @NOTE: This currently makes the simplifying assumption that there
  // is a single subscription for each publication.
  } while( cummulative_count < this->publications_.size());

  // Kluge to bias the race between BuiltinTopic samples and application
  // samples towards the BuiltinTopics during association establishment.
  ACE_OS::sleep( 2);

  if( this->options_.verbose()) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) Process::run() - ")
      ACE_TEXT("starting to publish samples with %d matched subscriptions.\n"),
      cummulative_count
    ));
  }

  for( PublicationMap::const_iterator current = this->publications_.begin();
       current != this->publications_.end();
       ++current
     ) {
    current->second->start();
  }

  // Execute test for specified duration, or block until terminated externally.
  if( this->options_.duration() > 0) {
    ACE_Time_Value now = ACE_OS::gettimeofday();
    ACE_Time_Value when = now + ACE_Time_Value( this->options_.duration(), 0);

    if( this->options_.verbose()) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Process::run() - ")
        ACE_TEXT("blocking main thread for %d seconds, ")
        ACE_TEXT("from %d until %d.\n"),
        this->options_.duration(),
        now.sec(),
        when.sec()
      ));
    }
    int value = this->condition_.wait( &when);
    if( value != 0) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Process::run() - ")
        ACE_TEXT("unblocked main thread: %p.\n"),
        ACE_TEXT("wait")
      ));
    }

  } else {
    // Block the main thread, leaving the others working.
    if( this->options_.verbose()) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Process::run() - ")
        ACE_TEXT("blocking main thread until signaled by user.\n")
      ));
    }
    // Only unblock and continue on a commanded termination.
    while( !this->terminated_) {
      int value = this->condition_.wait();
      if( value != 0) {
        ACE_DEBUG((LM_DEBUG,
          ACE_TEXT("(%P|%t) Process::run() - ")
          ACE_TEXT("unblocked main thread: %p.\n"),
          ACE_TEXT("wait")
        ));
      }
    }
  }
  if( this->options_.verbose()) {
    ACE_Time_Value now = ACE_OS::gettimeofday();
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) Process::run() - ")
      ACE_TEXT("continuing main thread at %d, starting to terminate.\n"),
      now.sec()
    ));
  }

  // Signal the writers to terminate.
  for( PublicationMap::const_iterator current = this->publications_.begin();
       current != this->publications_.end();
       ++current
     ) {
    current->second->stop();
  }

  // Separate loop so the termination messages can be handled concurrently.
  for( PublicationMap::const_iterator current = this->publications_.begin();
       current != this->publications_.end();
       ++current
     ) {
    // Join and clean up.
    current->second->wait();
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) Process::run() - ")
      ACE_TEXT("publication %C stopping after sending %d messages.\n"),
      current->first.c_str(),
      current->second->messages()
    ));
    delete current->second;
  }
  this->publications_.clear();

  if( this->options_.verbose()) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) Process::run() - ")
      ACE_TEXT("finished publishing samples.\n")
    ));
  }

  //
  // At this point the process has completed sending all data and
  // stopped any forwarding.  We wait for the subscriptions to
  // become unassociated before we shut down entirely.
  //

  DDS::SubscriptionMatchStatus subscriptionMatches = { 0, 0, 0, 0, 0};
  unsigned int current_count;
  do {
    if( DDS::RETCODE_OK != this->subscriptionWaiter_->wait( conditions, timeout)) {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: Process::run() - ")
        ACE_TEXT("failed to synchronize subscription conditions.\n")
      ));
      throw BadSyncException();
    }

    // At this point, we do not actually care what *caused* the thread to
    // unblock, we just want to determine if *any* subscription still has
    // an actively associated writer.
    current_count = 0;
    this->subscriptionWaiter_->get_conditions( conditions);
    for( unsigned long index = 0; index < conditions.length(); ++index) {
      // Extract the current Condition.
      DDS::StatusCondition_var condition
        = DDS::StatusCondition::_narrow( conditions[ index].in());
      if( !CORBA::is_nil( condition.in())) {
        // Its not our GuardCondition, go ahead and extract the reader.
        DDS::DataReader_var reader = DDS::DataReader::_narrow( condition->get_entity());
        if( !CORBA::is_nil( reader.in())) {
          // Again, we do not actually care what the event or change was,
          // we just take this opportunity to determine the current status
          // of associations.
          subscriptionMatches = reader->get_subscription_match_status();
          current_count += subscriptionMatches.current_count;
        }
      }
    }

    if( this->options_.verbose()) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Process::run() - ")
        ACE_TEXT("waiting for %d remote publications to finish.\n"),
        current_count
      ));
    }

  } while( !this->terminated_ && current_count > 0);

  // At this point, we are done sending and receiving data, so we can go
  // ahead and write any data that was requested.

  // Summary data.
  for( Options::SubscriptionProfileMap::const_iterator current
         = this->options_.subscriptionProfileMap().begin();
       current != this->options_.subscriptionProfileMap().end();
       ++current
     ) {
    if( !current->second->datafile.empty()) {
      OutputFileMap::iterator where
        = this->outputFiles_.find( current->second->datafile);
      if( where != this->outputFiles_.end()) {
        if( where->second) {
          *where->second << std::endl << "SUBSCRIPTION " << current->first
                         << " SUMMARY DATA" << std::endl;
          this->subscriptions_[ current->first]->summaryData( *where->second);

        } else {
          ACE_ERROR((LM_ERROR,
            ACE_TEXT("(%P|%t) ERROR: Process::run() - ")
            ACE_TEXT("output file not active for subscription: %C.\n"),
            current->first.c_str()
          ));

        }
      } else {
        ACE_ERROR((LM_ERROR,
          ACE_TEXT("(%P|%t) ERROR: Process::run() - ")
          ACE_TEXT("output file not found for subscription: %C.\n"),
          current->first.c_str()
        ));
      }
    }
  }

  // Detailed data.
  for( Options::SubscriptionProfileMap::const_iterator current
         = this->options_.subscriptionProfileMap().begin();
       current != this->options_.subscriptionProfileMap().end();
       ++current
     ) {
    if( !current->second->datafile.empty()) {
      OutputFileMap::iterator where
        = this->outputFiles_.find( current->second->datafile);
      if( where != this->outputFiles_.end()) {
        if( where->second) {
          *where->second << std::endl << "SUBSCRIPTION " << current->first
                         << " DETAILED DATA" << std::endl;
          this->subscriptions_[ current->first]->rawData( *where->second);

        } else {
          ACE_ERROR((LM_ERROR,
            ACE_TEXT("(%P|%t) ERROR: Process::run() - ")
            ACE_TEXT("output file not active for subscription: %C.\n"),
            current->first.c_str()
          ));

        }
      } else {
        ACE_ERROR((LM_ERROR,
          ACE_TEXT("(%P|%t) ERROR: Process::run() - ")
          ACE_TEXT("output file not found for subscription: %C.\n"),
          current->first.c_str()
        ));
      }
    }
  }

  if( this->options_.verbose()) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) Process::run() - ")
      ACE_TEXT("shutting down after all subscriptions have no writers.\n")
    ));
  }
}

std::ostream&
Process::summaryData( std::ostream& str) const
{
  for( SubscriptionMap::const_iterator current = this->subscriptions_.begin();
       current != this->subscriptions_.end();
       ++current
     ) {
    str << std::endl << "SUBSCRIPTION " << current->first
        << " SUMMARY DATA" << std::endl;
    current->second->summaryData( str);
  }
  return str;
}

std::ostream&
Process::rawData( std::ostream& str) const
{
  for( SubscriptionMap::const_iterator current = this->subscriptions_.begin();
       current != this->subscriptions_.end();
       ++current
     ) {
    str << std::endl << "SUBSCRIPTION " << current->first
        << " DETAILED DATA" << std::endl;
    current->second->rawData( str);
  }
  return str;
}

} // End of namespace Test

