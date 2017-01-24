// -*- C++ -*-

#include "Process.h"

#include "Test.h"
#include "Options.h"
#include "EntityProfiles.h"
#include "Publication.h"
#include "Subscription.h"
#include "TestTypeSupportImpl.h"
#include "ace/OS_NS_unistd.h"

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

  DDS::DomainParticipantFactory_var dpf = TheParticipantFactory;
  // Clean up the participants and their resources.
  for( ParticipantMap::iterator current = this->participants_.begin();
       current != this->participants_.end();
       ++current
     ) {
    current->second->delete_contained_entities();
    dpf->delete_participant( current->second.ptr());
  }

  // Clean up the rest of the service resources.
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
   publicationsAssociations_(0),
   subscriptionWaiter_( new DDS::WaitSet),
   guardCondition_( new DDS::GuardCondition),
   condition_( this->lock_),
   terminated_( false)
{
  DDS::DomainParticipantFactory_var dpf = TheParticipantFactory;
  for( Options::ParticipantProfileMap::const_iterator current
         = this->options_.participantProfileMap().begin();
       current != this->options_.participantProfileMap().end();
       ++current
     ) {
    // Create the current participant.
    this->participants_[ current->first]
      = dpf->create_participant(
          current->second->domainId,
          current->second->qos,
          DDS::DomainParticipantListener::_nil(),
          ::OpenDDS::DCPS::DEFAULT_STATUS_MASK
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
    CORBA::String_var type_name = testData->get_type_name();
    if( ::DDS::RETCODE_OK
     != testData->register_type( this->participants_[ current->first].in(), 0)) {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: Process::Process() - ")
        ACE_TEXT("unable to install type %C support into participant %C.\n"),
        type_name.in(),
        current->first.c_str()
      ));
      throw BadTypeSupportException ();

    } else if( this->options_.verbose()) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Process::Process() - ")
        ACE_TEXT("created type %C support in participant %C.\n"),
        type_name.in(),
        current->first.c_str()
      ));
    }

    // Save the data type name for later use in creating Topics.
    if( this->dataTypeName_.empty()) {
      this->dataTypeName_ = type_name;
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
          DDS::TopicListener::_nil(),
          ::OpenDDS::DCPS::DEFAULT_STATUS_MASK
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
    status->set_enabled_statuses( DDS::SUBSCRIPTION_MATCHED_STATUS);
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
    // Grab the topic for which this publication will be created.
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

    // Grab the participant in which this publication will be installed.
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
    status->set_enabled_statuses( DDS::PUBLICATION_MATCHED_STATUS);
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

    // Add the publications expected associations
    publicationsAssociations_ += current->second->associations;
  }

  // Allow the publication waiting to be interrupted.
  this->publicationWaiter_->attach_condition( this->guardCondition_.in());

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
  while(!this->terminated_) {
    // Check each publication for its readiness to run.
    int  missing = 0;
    for( PublicationMap::const_iterator current = this->publications_.begin();
         current != this->publications_.end();
         ++current
       ) {
      missing += current->second->missing_associations();
    }
    if( missing == 0) {
      break;
    }

    if( this->options_.verbose()) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Process::run() - ")
        ACE_TEXT("waiting for %d subscriptions to attach to publications.\n"),
        missing
      ));
    }

    // Wait for the matched status to be updated.
    DDS::Duration_t   timeout = { DDS::DURATION_INFINITE_SEC, DDS::DURATION_INFINITE_NSEC};
    DDS::ConditionSeq discard;
    if( DDS::RETCODE_OK != this->publicationWaiter_->wait( discard, timeout)) {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: Process::run() - ")
        ACE_TEXT("failed to synchronize at start of test.\n")
      ));
      throw BadSyncException();
    }
  }

  // Only proceed if we have not been instructed to terminate.
  if(!this->terminated_) {
    // Kluge to bias the race between BuiltinTopic samples and application
    // samples towards the BuiltinTopics during association establishment.
    ACE_OS::sleep( 2);

    if( this->options_.verbose() && this->publications_.size() > 0) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Process::run() - ")
        ACE_TEXT("starting to publish samples.\n")
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
      ACE_TEXT("publication %C stopping with results:\n")
      ACE_TEXT("   test elapsed time: %g,\n")
      ACE_TEXT("   messages     sent: %d,\n")
      ACE_TEXT("   messages timedout: %d.\n"),
      current->first.c_str(),
      current->second->duration(),
      current->second->messages(),
      current->second->timeouts()
    ));
    delete current->second;
  }
  this->publications_.clear();

  if( this->options_.verbose() && this->publications_.size() > 0) {
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
  do {
    // Check for active subscriptions (associated with publications).
    int associations = 0;
    for( SubscriptionMap::const_iterator current = this->subscriptions_.begin();
         current != this->subscriptions_.end();
         ++current
       ) {
      associations += current->second->associations();
    }
    if( associations == 0) {
      break;
    }
    if( this->options_.verbose()) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Process::run() - ")
        ACE_TEXT("waiting for %d remote publications to finish.\n"),
        associations
      ));
    }

    // Now wait for a change in associations.  Wait for up to 30 seconds.
    DDS::Duration_t   timeout = { 30, 0 };
    DDS::ConditionSeq discard;
    if( DDS::RETCODE_OK != this->subscriptionWaiter_->wait( discard, timeout)) {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: Process::run() - ")
        ACE_TEXT("failed to synchronize subscription conditions.\n")
      ));
      throw BadSyncException();
    }

    // If we have been terminated by signal, only wait once.  If we
    // haven't, wait until either the processes terminate nicely or the
    // user sends a signal.
  } while( !this->terminated_);

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

