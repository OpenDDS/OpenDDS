// -*- C++ -*-
//
// $Id$

#include "MonitorTask.h"
#include "MonitorData.h"
#include "Options.h"

#include "dds/DCPS/DataWriterImpl.h"
#include "dds/DCPS/RepoIdConverter.h"
#include "dds/DCPS/Qos_Helper.h"
#include "dds/DCPS/Marked_Default_Qos.h"
#include "dds/DCPS/transport/framework/TheTransportFactory.h"
#include "dds/DCPS/transport/framework/TransportImpl_rch.h"

#include <sstream>

Monitor::MonitorTask::MonitorTask(
  MonitorData* data,
  const Options& options
) : opened_( false),
    done_( false),
    options_( options),
    data_( data),
    gate_( this->lock_),
    controlContext_( ExternalControl),
    waiter_( new DDS::WaitSet),
    guardCondition_( new DDS::GuardCondition),
    currentKey_( 0)
{
  // Find and map the current IOR strings to their IOR key values.
  for( OpenDDS::DCPS::Service_Participant::KeyIorMap::const_iterator
       location = TheServiceParticipant->keyIorMap().begin();
       location != TheServiceParticipant->keyIorMap().end();
       ++location) {
    this->iorKeyMap_[ location->second] = location->first;
  }
}

Monitor::MonitorTask::~MonitorTask()
{
  // Terminate processing cleanly.
  this->stop();
  this->iorKeyMap_.clear();

  // Clean up the service resources.
  TheTransportFactory->release();
  TheServiceParticipant->shutdown();
}

const Monitor::MonitorTask::IorKeyMap&
Monitor::MonitorTask::iorKeyMap() const
{
  return this->iorKeyMap_;
}

int
Monitor::MonitorTask::open( void*)
{
  if( this->opened_) {
    return -1;
  }

  int result = activate (THR_NEW_LWP | THR_JOINABLE, 1);
  if( result != 0) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: MonitorTask::open() - ")
      ACE_TEXT("failed to activate the thread.\n")
    ));
  }

  this->opened_ = true;
  return result;
}

int
Monitor::MonitorTask::close( u_long flags)
{
  if( flags == 0) {
    return 0;
  }

  // Shutdown current processing.
  this->shutdownRepo();

  // This is the single location where this member is written, don't
  // bother to lock access.
  this->done_ = true;
  this->controlContext_ = InternalControl;
  this->gate_.broadcast();

  this->guardCondition_->set_trigger_value( true);

  if( this->opened_ && (this->thread_ != ACE_OS::thr_self())) {
    this->wait();
  }
  return 0;
}

void
Monitor::MonitorTask::start()
{
  // Add the guard condition to the wait set.
  this->waiter_->attach_condition( this->guardCondition_);

  this->open( 0);
}

void
Monitor::MonitorTask::stop()
{
  this->close( 1);
}

void
Monitor::MonitorTask::shutdownRepo()
{
  // Only shutdown if we are currently actively processing data from a
  // repository.
  if( CORBA::is_nil( this->participant_.in())) {
    return;
  }

  // Wait to gain control of processing.
  if( this->controlContext_ == InternalControl) {
    ACE_GUARD(ACE_SYNCH_MUTEX, guard, this->lock_);
    while( this->controlContext_ == InternalControl) {
      this->guardCondition_->set_trigger_value( true);
      this->gate_.wait();
    }
  }
  // At this point, the context is ExternalControl.

  // Clean up the wait conditions.
  DDS::ConditionSeq conditions;
  this->waiter_->get_conditions( conditions);
  this->waiter_->detach_conditions( conditions);

  // Reattach the guard condition.
  this->waiter_->attach_condition( this->guardCondition_);

  // Terminate the transport resources.

  // Destroy the instrumentation participant.
  this->participant_->delete_contained_entities();
  TheParticipantFactory->delete_participant( this->participant_.in());

  // Yield control.
  this->controlContext_ = InternalControl;
  this->gate_.broadcast();
}

void
Monitor::MonitorTask::setRepoIor( const std::string& ior)
{
  RepoKey key;
  IorKeyMap::iterator location = this->iorKeyMap_.find( ior);
  if( location != this->iorKeyMap_.end()) {
    // We already have this IOR mapped, use the existing key.
    key = location->second;

  } else {
    // We need to find an open key to use.  Check the actual
    // Service_Participant mappings for a slot.
    OpenDDS::DCPS::Service_Participant::KeyIorMap::const_iterator
      keylocation;
    key = this->currentKey_;
    do {
      keylocation = TheServiceParticipant->keyIorMap().find( ++key);
    } while( keylocation != TheServiceParticipant->keyIorMap().end());
    this->currentKey_ = key;

    // We have a new repository to install, go ahead.
    TheServiceParticipant->set_repo_ior( ior.c_str(), key);
    this->iorKeyMap_[ ior] = key;
  }

  // Map the instrumentation domain onto the indicated repository.
  TheServiceParticipant->set_repo_domain( this->options_.domain(), key);
}

void
Monitor::MonitorTask::someMethod()
{
  // Shutdown current processing.
  this->shutdownRepo();

  // Wait to gain control of processing.
  if( this->controlContext_ == InternalControl) {
    ACE_GUARD(ACE_SYNCH_MUTEX, guard, this->lock_);
    while( this->controlContext_ == InternalControl) {
      this->guardCondition_->set_trigger_value( true);
      this->gate_.wait();
    }
  }
  // At this point, the context is ExternalControl.

  // Clear any existing data and create a new tree.

  // Establish connection to the new repository.

  // Fire up a new set of processing with the new repository.
  this->participant_
    = TheParticipantFactory->create_participant(
        this->options_.domain(),
        PARTICIPANT_QOS_DEFAULT,
        DDS::DomainParticipantListener::_nil(),
        OpenDDS::DCPS::DEFAULT_STATUS_MASK
      );
  if( CORBA::is_nil( this->participant_.in())) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: MonitorTask::setRepoIor() - ")
      ACE_TEXT("failed to create participant.\n")
    ));
    return;
  }

#if 0
  // Create the publisher.
  this->publisher_
    = participant->create_publisher(
        this->profile_->publisherQos,
        ::DDS::PublisherListener::_nil(),
        ::OpenDDS::DCPS::DEFAULT_STATUS_MASK
      );
  if( CORBA::is_nil( publisher_.in())) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) MonitorTask::enable() - publication %C: ")
      ACE_TEXT("failed to create publisher.\n"),
        this->name_.c_str()
    ));
    throw BadPublisherException();
  }

  // Try to obtain the transport first
  OpenDDS::DCPS::TransportImpl_rch transport
    = TheTransportFactory->obtain(
        this->profile_->transport
      );
  if( transport.is_nil()) {
    // Create the transport
    transport = TheTransportFactory->create_transport_impl(
        this->profile_->transport,
        ::OpenDDS::DCPS::AUTO_CONFIG
      );
    if( transport.is_nil()) {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) MonitorTask::enable() - publication %C: ")
        ACE_TEXT("failed to create transport with index %d.\n"),
        this->name_.c_str(),
        this->profile_->transport
      ));
      throw BadTransportException();
    } else if( this->verbose_) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) MonitorTask::enable() - publication %C: ")
        ACE_TEXT("created transport with index %d.\n"),
        this->name_.c_str(),
        this->profile_->transport
      ));
    }
  } else if( this->verbose_) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) MonitorTask::enable() - publication %C: ")
      ACE_TEXT("obtained transport with index %d.\n"),
      this->name_.c_str(),
      this->profile_->transport
    ));
  }

  // Attach the transport
  if( ::OpenDDS::DCPS::ATTACH_OK != transport->attach( publisher_.in())) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) MonitorTask::enable() - publication %C: ")
      ACE_TEXT("failed to attach transport with index %d to publisher.\n"),
      this->name_.c_str(),
      this->profile_->transport
    ));
    throw BadAttachException();

  } else if( this->verbose_) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) MonitorTask::enable() - publication %C: ")
      ACE_TEXT("attached transport with index %d to publisher.\n"),
      this->name_.c_str(),
      this->profile_->transport
    ));
  }

  // Derive the writer Qos values.
  ::DDS::TopicQos topicQos;
  topic->get_qos( topicQos);

  ::DDS::DataWriterQos writerQos;
  publisher_->get_default_datawriter_qos( writerQos);

  publisher_->copy_from_topic_qos( writerQos, topicQos);

  this->profile_->copyToWriterQos( writerQos);

  // Create the writer.
  DDS::DataWriter_var writer
    = publisher_->create_datawriter(
        topic,
        writerQos,
        ::DDS::DataWriterListener::_nil(),
        ::OpenDDS::DCPS::DEFAULT_STATUS_MASK
      );
  if( CORBA::is_nil( writer.in())) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) MonitorTask::enable() - publication %C: ")
      ACE_TEXT("failed to create writer.\n"),
      this->name_.c_str()
    ));
    throw BadWriterException();

  } else if( this->verbose_) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) MonitorTask::enable() - publication %C: ")
      ACE_TEXT("created writer.\n"),
      this->name_.c_str()
    ));
  }

  this->writer_ = Test::DataDataWriter::_narrow( writer.in());
  if( CORBA::is_nil( this->writer_.in())) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) MonitorTask::enable() - publication %C: ")
      ACE_TEXT("failed to narrow writer for Test::Data type.\n"),
      this->name_.c_str()
    ));
    throw BadWriterException();

  } else if( this->verbose_) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) MonitorTask::enable() - publication %C: ")
      ACE_TEXT("narrowed writer for Test::Data type.\n"),
      this->name_.c_str()
    ));
  }
#endif

  // Yield control.
  this->controlContext_ = InternalControl;
  this->gate_.broadcast();
}

int
Monitor::MonitorTask::svc ()
{
  this->thread_ = ACE_OS::thr_self();

  if( this->options_.verbose()) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) MonitorTask::svc() - ")
      ACE_TEXT("processing starts on thread.\n")
    ));
  }

  DDS::Duration_t   timeout = { DDS::DURATION_INFINITE_SEC, DDS::DURATION_INFINITE_NSEC};
  DDS::ConditionSeq conditions;

  while( !this->done_) {
    if( this->controlContext_ == ExternalControl) {
      ACE_GUARD_RETURN(ACE_SYNCH_MUTEX, guard, this->lock_, -1);
      while( this->controlContext_ == ExternalControl) {
        this->gate_.wait();
      }
    }

    if( DDS::RETCODE_OK != this->waiter_->wait( conditions, timeout)) {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: MonitorTask::svc() - ")
        ACE_TEXT("failed to synchronize DDS conditions.\n")
      ));
      return -1;
    }

    for( unsigned long index = 0; index < conditions.length(); ++index) {
      // Extract the current Condition.
      DDS::StatusCondition_var condition
        = DDS::StatusCondition::_narrow( conditions[ index].in());
      if( !CORBA::is_nil( condition.in())) {
        // Its a CommunicationStatus, process inbound data.
        DDS::DataReader_var reader
          = DDS::DataReader::_narrow( condition->get_entity());
        if( !CORBA::is_nil( reader.in())) {
          /// @TODO: Process inbound data here.
        }
      }

      // Reset the GuardCondition regardless of its trigger state.
      this->guardCondition_->set_trigger_value( false);
    }

    // Yield before we start another pass.
    this->controlContext_ = ExternalControl;
    this->gate_.broadcast();
  }

  if( this->options_.verbose()) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) MonitorTask::svc() - ")
      ACE_TEXT("honoring termination request, stopping thread.\n")
    ));
  }
  return 0;
}

