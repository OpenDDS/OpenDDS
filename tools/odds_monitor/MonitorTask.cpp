// -*- C++ -*-
//
// $Id$

#include "MonitorTask.h"
#include "MonitorData.h"
#include "Options.h"

#include "dds/monitor/monitorTypeSupportImpl.h"
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
    inUse_( false),
    options_( options),
    data_( data),
    gate_( this->lock_),
    waiter_( new DDS::WaitSet),
    guardCondition_( new DDS::GuardCondition),
    lastKey_( 0)
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
  this->stopInstrumentation();

  // This is the single location where this member is written, don't
  // bother to lock access.
  this->done_ = true;
  {
    ACE_GUARD_RETURN(ACE_SYNCH_MUTEX, guard, this->lock_, -1);
    this->inUse_ = false;
  }
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
Monitor::MonitorTask::stopInstrumentation()
{
  // Only shutdown if we are currently actively processing data from a
  // repository.
  if( CORBA::is_nil( this->participant_.in())) {
    return;
  }

  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT("(%P|%t) MonitorTask::stopInstrumentation() - ")
    ACE_TEXT("removing participant for domain %d.\n"),
    this->participant_->get_domain_id()
  ));

  { // Wait to gain control of processing.
    ACE_GUARD(ACE_SYNCH_MUTEX, guard, this->lock_);
    while( this->inUse_) {
      this->gate_.wait();
    }
    this->inUse_ = true;
  }

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
  this->participant_ = DDS::DomainParticipant::_nil();

  /// Yield control.
  {
    ACE_GUARD(ACE_SYNCH_MUTEX, guard, this->lock_);
    this->inUse_ = false;
  }
  this->gate_.broadcast();
}

Monitor::MonitorTask::RepoKey
Monitor::MonitorTask::setRepoIor( const std::string& ior)
{
  if( this->options_.verbose()) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) MonitorTask::setRepoIor() - ")
      ACE_TEXT("setting active repository to: %C.\n"),
      ior.c_str()
    ));
  }

  { // Wait to gain control of processing.
    ACE_GUARD_RETURN(ACE_SYNCH_MUTEX, guard, this->lock_, DEFAULT_REPO);
    while( this->inUse_) {
      this->gate_.wait();
    }
    this->inUse_ = true;
  }

  RepoKey key;
  IorKeyMap::iterator location = this->iorKeyMap_.find( ior);
  if( location != this->iorKeyMap_.end()) {
    // We already have this IOR mapped, use the existing key.
    key = location->second;

  } else {
    // We need to find an open key to use.  Check the actual
    // Service_Participant mappings for a slot.
    OpenDDS::DCPS::Service_Participant::KeyIorMap::const_iterator
      keyLocation;
    key = this->lastKey_;
    do {
      keyLocation = TheServiceParticipant->keyIorMap().find( ++key);
    } while( keyLocation != TheServiceParticipant->keyIorMap().end());
    this->lastKey_ = key;

    // We have a new repository to install, go ahead.
    TheServiceParticipant->set_repo_ior( ior.c_str(), key);

    // Check that we were able to resolve and attach to the repository.
    keyLocation = TheServiceParticipant->keyIorMap().find( key);
    if( keyLocation == TheServiceParticipant->keyIorMap().end()) {
      // We failed to install this IOR, nothing left to do.
      {
        ACE_GUARD_RETURN(ACE_SYNCH_MUTEX, guard, this->lock_, DEFAULT_REPO);
        this->inUse_ = false;
      }
      return DEFAULT_REPO;
    }

    // Store the reverse mapping for our use.
    this->iorKeyMap_[ ior] = key;
  }

  /// Yield control.
  {
    ACE_GUARD_RETURN(ACE_SYNCH_MUTEX, guard, this->lock_, DEFAULT_REPO);
    this->inUse_ = false;
  }

  // Shutdown current processing.  This has the effect of removing the
  // existing participant.
  // N.B. The current participant needs to be removed before remapping
  //      the domain since it would be attached to the new repository,
  //      which does not work unless federated.  This needs to work in
  //      the case of non-federated repositories.
  this->stopInstrumentation();

  return key;
}

int
Monitor::MonitorTask::svc()
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
    if( this->options_.verbose()) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) MonitorTask::svc() - ")
        ACE_TEXT("waiting for work.\n")
      ));
    }

    if( DDS::RETCODE_OK != this->waiter_->wait( conditions, timeout)) {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: MonitorTask::svc() - ")
        ACE_TEXT("failed to synchronize DDS conditions.\n")
      ));
      return -1;
    }

    if( this->options_.verbose()) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) MonitorTask::svc() - ")
        ACE_TEXT("processing conditions.\n")
      ));
    }

    { // Wait to gain control of processing.
      ACE_GUARD_RETURN(ACE_SYNCH_MUTEX, guard, this->lock_, -1);
      while( this->inUse_) {
        this->gate_.wait();
      }
      this->inUse_ = true;
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
          // Process inbound data here.
          this->dispatchReader( reader.in());
        }
      }
    }

    // Yield before we start another pass.
    {
      ACE_GUARD_RETURN(ACE_SYNCH_MUTEX, guard, this->lock_, -1);
      this->inUse_ = false;
    }
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

bool
Monitor::MonitorTask::setActiveRepo( RepoKey key)
{
  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT("(%P|%t) MonitorTask::setActiveRepo() - ")
    ACE_TEXT("previous instrumentation has been torn down, ")
    ACE_TEXT("establishing new monitoring.\n")
  ));

  { // Wait to gain control of processing.
    ACE_GUARD_RETURN(ACE_SYNCH_MUTEX, guard, this->lock_, false);
    while( this->inUse_) {
      this->gate_.wait();
    }
    this->inUse_ = true;
  }

  if( this->options_.verbose()) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) MonitorTask::setActiveRepo() - ")
      ACE_TEXT("rebinding instrumentation domain %d to repository key: %d.\n"),
      this->options_.domain(),
      key
    ));
  }

  // Remap all domains pointing to the old repository to the new one.
  TheServiceParticipant->remap_domains( this->activeKey_, key);

  // Check that the instrumentation domain repository is the new one.
  RepoKey monitorKey = TheServiceParticipant->domain_to_repo(
                         this->options_.domain()
                       );
  if( monitorKey != key) {
    // Otherwise map the instrumentation domain onto the new repository.
    TheServiceParticipant->set_repo_domain( this->options_.domain(), key);
  }

  // Save the newly active repository key.
  this->activeKey_ = key;

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
      ACE_TEXT("(%P|%t) ERROR: MonitorTask::startInstrumentation() - ")
      ACE_TEXT("failed to create participant.\n")
    ));
    {
      ACE_GUARD_RETURN(ACE_SYNCH_MUTEX, guard, this->lock_, false);
      this->inUse_ = false;
    }
    return false;
  }

  if( this->options_.verbose()) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) MonitorTask::startInstrumentation() - ")
      ACE_TEXT("created participant.\n")
    ));
  }

  // Fire up a subscriber.
  ::DDS::Subscriber_var subscriber = this->participant_->create_subscriber(
                                       SUBSCRIBER_QOS_DEFAULT,
                                       ::DDS::SubscriberListener::_nil(),
                                       ::OpenDDS::DCPS::DEFAULT_STATUS_MASK
                                     );
  if( CORBA::is_nil( subscriber.in())) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: MonitorTask::startInstrumentation() - ")
      ACE_TEXT("failed to create subscriber.\n")
    ));
    {
      ACE_GUARD_RETURN(ACE_SYNCH_MUTEX, guard, this->lock_, false);
      this->inUse_ = false;
    }
    return false;
  }

  if( this->options_.verbose()) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) MonitorTask::startInstrumentation() - ")
      ACE_TEXT("created subscriber.\n")
    ));
  }

  // Extract a transport index for this subscription.
  OpenDDS::DCPS::TransportIdType transportKey
    = static_cast<OpenDDS::DCPS::TransportIdType>( this->activeKey_);

  // Grab the transport itself.
  OpenDDS::DCPS::TransportImpl_rch transport
    = TheTransportFactory->obtain( transportKey);
  if( transport.is_nil()) {
    transport = TheTransportFactory->create_transport_impl(
                  transportKey,
                  "SimpleTcp",
                  OpenDDS::DCPS::AUTO_CONFIG
                );
    if( transport.is_nil()) {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: MonitorTask::startInstrumentation() - ")
        ACE_TEXT("failed to create transport.\n")
      ));
      {
        ACE_GUARD_RETURN(ACE_SYNCH_MUTEX, guard, this->lock_, false);
        this->inUse_ = false;
      }
      return false;
    }
  }

  if( ::OpenDDS::DCPS::ATTACH_OK != transport->attach( subscriber.in())) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: MonitorTask::startInstrumentation() - ")
      ACE_TEXT("failed to attach transport to subscriber.\n")
    ));
    {
      ACE_GUARD_RETURN(ACE_SYNCH_MUTEX, guard, this->lock_, false);
      this->inUse_ = false;
    }
    return false;
  }

  if( this->options_.verbose()) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) MonitorTask::startInstrumentation() - ")
      ACE_TEXT("attached to transport with index %d.\n"),
      transportKey
    ));
  }

  DDS::Topic_var           topic;
  DDS::TopicQos            topicQos;
  DDS::DataReaderQos       readerQos;
  DDS::DataReader_var      reader;
  DDS::StatusCondition_var status;

  // Create each topic and subscription together so we can keep things on the stack.

  OpenDDS::DCPS::ServiceParticipantReportTypeSupportImpl* serviceParticipantReportTypeSupport
    = new OpenDDS::DCPS::ServiceParticipantReportTypeSupportImpl();
  serviceParticipantReportTypeSupport->register_type( this->participant_.in(), 0);
  topic = this->participant_->create_topic(
            OpenDDS::DCPS::SERVICE_PARTICIPANT_MONITOR_TOPIC,
            serviceParticipantReportTypeSupport->get_type_name(),
            TOPIC_QOS_DEFAULT,
            DDS::TopicListener::_nil(),
            OpenDDS::DCPS::DEFAULT_STATUS_MASK
          );

  topic->get_qos( topicQos);
  subscriber->get_default_datareader_qos( readerQos);
  subscriber->copy_from_topic_qos( readerQos, topicQos);
  reader = subscriber->create_datareader(
             topic,
             readerQos,
             ::DDS::DataReaderListener::_nil(),
             ::OpenDDS::DCPS::DEFAULT_STATUS_MASK
           );
  if( CORBA::is_nil( reader.in())) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: MonitorTask::startInstrumentation() - ")
      ACE_TEXT("failed to create a reader for SERVICE_PARTICIPANT_MONITOR_TOPIC.\n")
    ));
    {
      ACE_GUARD_RETURN(ACE_SYNCH_MUTEX, guard, this->lock_, false);
      this->inUse_ = false;
    }
    return false;
  }
  status = reader->get_statuscondition();
  status->set_enabled_statuses( DDS::DATA_AVAILABLE_STATUS);
  this->waiter_->attach_condition( status.in());
  this->handleTypeMap_[ reader->get_instance_handle()]
    = OpenDDS::DCPS::SERVICE_PARTICIPANT_REPORT_TYPE;

  OpenDDS::DCPS::DomainParticipantReportTypeSupportImpl* domainParticipantReportTypeSupport
    = new OpenDDS::DCPS::DomainParticipantReportTypeSupportImpl();
  domainParticipantReportTypeSupport->register_type( this->participant_.in(), 0);
  topic = this->participant_->create_topic(
            OpenDDS::DCPS::DOMAIN_PARTICIPANT_MONITOR_TOPIC,
            domainParticipantReportTypeSupport->get_type_name(),
            TOPIC_QOS_DEFAULT,
            DDS::TopicListener::_nil(),
            OpenDDS::DCPS::DEFAULT_STATUS_MASK
          );

  topic->get_qos( topicQos);
  subscriber->get_default_datareader_qos( readerQos);
  subscriber->copy_from_topic_qos( readerQos, topicQos);
  reader = subscriber->create_datareader(
             topic,
             readerQos,
             ::DDS::DataReaderListener::_nil(),
             ::OpenDDS::DCPS::DEFAULT_STATUS_MASK
           );
  if( CORBA::is_nil( reader.in())) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: MonitorTask::startInstrumentation() - ")
      ACE_TEXT("failed to create a reader for DOMAIN_PARTICIPANT_MONITOR_TOPIC.\n")
    ));
    {
      ACE_GUARD_RETURN(ACE_SYNCH_MUTEX, guard, this->lock_, false);
      this->inUse_ = false;
    }
    return false;
  }
  status = reader->get_statuscondition();
  status->set_enabled_statuses( DDS::DATA_AVAILABLE_STATUS);
  this->waiter_->attach_condition( status.in());
  this->handleTypeMap_[ reader->get_instance_handle()]
    = OpenDDS::DCPS::DOMAIN_PARTICIPANT_REPORT_TYPE;

  OpenDDS::DCPS::TopicReportTypeSupportImpl* topicReportTypeSupport
    = new OpenDDS::DCPS::TopicReportTypeSupportImpl();
  topicReportTypeSupport->register_type( this->participant_.in(), 0);
  topic = this->participant_->create_topic(
            OpenDDS::DCPS::TOPIC_MONITOR_TOPIC,
            topicReportTypeSupport->get_type_name(),
            TOPIC_QOS_DEFAULT,
            DDS::TopicListener::_nil(),
            OpenDDS::DCPS::DEFAULT_STATUS_MASK
          );

  topic->get_qos( topicQos);
  subscriber->get_default_datareader_qos( readerQos);
  subscriber->copy_from_topic_qos( readerQos, topicQos);
  reader = subscriber->create_datareader(
             topic,
             readerQos,
             ::DDS::DataReaderListener::_nil(),
             ::OpenDDS::DCPS::DEFAULT_STATUS_MASK
           );
  if( CORBA::is_nil( reader.in())) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: MonitorTask::startInstrumentation() - ")
      ACE_TEXT("failed to create a reader for TOPIC_MONITOR_TOPIC.\n")
    ));
    {
      ACE_GUARD_RETURN(ACE_SYNCH_MUTEX, guard, this->lock_, false);
      this->inUse_ = false;
    }
    return false;
  }
  status = reader->get_statuscondition();
  status->set_enabled_statuses( DDS::DATA_AVAILABLE_STATUS);
  this->waiter_->attach_condition( status.in());
  this->handleTypeMap_[ reader->get_instance_handle()]
    = OpenDDS::DCPS::TOPIC_REPORT_TYPE;

  OpenDDS::DCPS::PublisherReportTypeSupportImpl* publisherReportTypeSupport
    = new OpenDDS::DCPS::PublisherReportTypeSupportImpl();
  publisherReportTypeSupport->register_type( this->participant_.in(), 0);
  topic = this->participant_->create_topic(
            OpenDDS::DCPS::PUBLISHER_MONITOR_TOPIC,
            publisherReportTypeSupport->get_type_name(),
            TOPIC_QOS_DEFAULT,
            DDS::TopicListener::_nil(),
            OpenDDS::DCPS::DEFAULT_STATUS_MASK
          );

  topic->get_qos( topicQos);
  subscriber->get_default_datareader_qos( readerQos);
  subscriber->copy_from_topic_qos( readerQos, topicQos);
  reader = subscriber->create_datareader(
             topic,
             readerQos,
             ::DDS::DataReaderListener::_nil(),
             ::OpenDDS::DCPS::DEFAULT_STATUS_MASK
           );
  if( CORBA::is_nil( reader.in())) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: MonitorTask::startInstrumentation() - ")
      ACE_TEXT("failed to create a reader for PUBLISHER_MONITOR_TOPIC.\n")
    ));
    {
      ACE_GUARD_RETURN(ACE_SYNCH_MUTEX, guard, this->lock_, false);
      this->inUse_ = false;
    }
    return false;
  }
  status = reader->get_statuscondition();
  status->set_enabled_statuses( DDS::DATA_AVAILABLE_STATUS);
  this->waiter_->attach_condition( status.in());
  this->handleTypeMap_[ reader->get_instance_handle()]
    = OpenDDS::DCPS::PUBLISHER_REPORT_TYPE;

  OpenDDS::DCPS::SubscriberReportTypeSupportImpl* subscriberReportTypeSupport
    = new OpenDDS::DCPS::SubscriberReportTypeSupportImpl();
  subscriberReportTypeSupport->register_type( this->participant_.in(), 0);
  topic = this->participant_->create_topic(
            OpenDDS::DCPS::SUBSCRIBER_MONITOR_TOPIC,
            subscriberReportTypeSupport->get_type_name(),
            TOPIC_QOS_DEFAULT,
            DDS::TopicListener::_nil(),
            OpenDDS::DCPS::DEFAULT_STATUS_MASK
          );

  topic->get_qos( topicQos);
  subscriber->get_default_datareader_qos( readerQos);
  subscriber->copy_from_topic_qos( readerQos, topicQos);
  reader = subscriber->create_datareader(
             topic,
             readerQos,
             ::DDS::DataReaderListener::_nil(),
             ::OpenDDS::DCPS::DEFAULT_STATUS_MASK
           );
  if( CORBA::is_nil( reader.in())) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: MonitorTask::startInstrumentation() - ")
      ACE_TEXT("failed to create a reader for SUBSCRIBER_MONITOR_TOPIC.\n")
    ));
    {
      ACE_GUARD_RETURN(ACE_SYNCH_MUTEX, guard, this->lock_, false);
      this->inUse_ = false;
    }
    return false;
  }
  status = reader->get_statuscondition();
  status->set_enabled_statuses( DDS::DATA_AVAILABLE_STATUS);
  this->waiter_->attach_condition( status.in());
  this->handleTypeMap_[ reader->get_instance_handle()]
    = OpenDDS::DCPS::SUBSCRIBER_REPORT_TYPE;

  OpenDDS::DCPS::DataWriterReportTypeSupportImpl* dataWriterReportTypeSupport
    = new OpenDDS::DCPS::DataWriterReportTypeSupportImpl();
  dataWriterReportTypeSupport->register_type( this->participant_.in(), 0);
  topic = this->participant_->create_topic(
            OpenDDS::DCPS::DATA_WRITER_MONITOR_TOPIC,
            dataWriterReportTypeSupport->get_type_name(),
            TOPIC_QOS_DEFAULT,
            DDS::TopicListener::_nil(),
            OpenDDS::DCPS::DEFAULT_STATUS_MASK
          );

  topic->get_qos( topicQos);
  subscriber->get_default_datareader_qos( readerQos);
  subscriber->copy_from_topic_qos( readerQos, topicQos);
  reader = subscriber->create_datareader(
             topic,
             readerQos,
             ::DDS::DataReaderListener::_nil(),
             ::OpenDDS::DCPS::DEFAULT_STATUS_MASK
           );
  if( CORBA::is_nil( reader.in())) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: MonitorTask::startInstrumentation() - ")
      ACE_TEXT("failed to create a reader for DATA_WRITER_MONITOR_TOPIC.\n")
    ));
    {
      ACE_GUARD_RETURN(ACE_SYNCH_MUTEX, guard, this->lock_, false);
      this->inUse_ = false;
    }
    return false;
  }
  status = reader->get_statuscondition();
  status->set_enabled_statuses( DDS::DATA_AVAILABLE_STATUS);
  this->waiter_->attach_condition( status.in());
  this->handleTypeMap_[ reader->get_instance_handle()]
    = OpenDDS::DCPS::DATA_WRITER_REPORT_TYPE;

  OpenDDS::DCPS::DataWriterPeriodicReportTypeSupportImpl* dataWriterPeriodicReportTypeSupport
    = new OpenDDS::DCPS::DataWriterPeriodicReportTypeSupportImpl();
  dataWriterPeriodicReportTypeSupport->register_type( this->participant_.in(), 0);
  topic = this->participant_->create_topic(
            OpenDDS::DCPS::DATA_WRITER_PERIODIC_MONITOR_TOPIC,
            dataWriterPeriodicReportTypeSupport->get_type_name(),
            TOPIC_QOS_DEFAULT,
            DDS::TopicListener::_nil(),
            OpenDDS::DCPS::DEFAULT_STATUS_MASK
          );

  topic->get_qos( topicQos);
  subscriber->get_default_datareader_qos( readerQos);
  subscriber->copy_from_topic_qos( readerQos, topicQos);
  reader = subscriber->create_datareader(
             topic,
             readerQos,
             ::DDS::DataReaderListener::_nil(),
             ::OpenDDS::DCPS::DEFAULT_STATUS_MASK
           );
  if( CORBA::is_nil( reader.in())) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: MonitorTask::startInstrumentation() - ")
      ACE_TEXT("failed to create a reader for DATA_WRITER_PERIODIC_MONITOR_TOPIC.\n")
    ));
    {
      ACE_GUARD_RETURN(ACE_SYNCH_MUTEX, guard, this->lock_, false);
      this->inUse_ = false;
    }
    return false;
  }
  status = reader->get_statuscondition();
  status->set_enabled_statuses( DDS::DATA_AVAILABLE_STATUS);
  this->waiter_->attach_condition( status.in());
  this->handleTypeMap_[ reader->get_instance_handle()]
    = OpenDDS::DCPS::DATA_WRITER_PERIODIC_REPORT_TYPE;

  OpenDDS::DCPS::DataReaderReportTypeSupportImpl* dataReaderReportTypeSupport
    = new OpenDDS::DCPS::DataReaderReportTypeSupportImpl();
  dataReaderReportTypeSupport->register_type( this->participant_.in(), 0);
  topic = this->participant_->create_topic(
            OpenDDS::DCPS::DATA_READER_MONITOR_TOPIC,
            dataReaderReportTypeSupport->get_type_name(),
            TOPIC_QOS_DEFAULT,
            DDS::TopicListener::_nil(),
            OpenDDS::DCPS::DEFAULT_STATUS_MASK
          );

  topic->get_qos( topicQos);
  subscriber->get_default_datareader_qos( readerQos);
  subscriber->copy_from_topic_qos( readerQos, topicQos);
  reader = subscriber->create_datareader(
             topic,
             readerQos,
             ::DDS::DataReaderListener::_nil(),
             ::OpenDDS::DCPS::DEFAULT_STATUS_MASK
           );
  if( CORBA::is_nil( reader.in())) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: MonitorTask::startInstrumentation() - ")
      ACE_TEXT("failed to create a reader for DATA_READER_MONITOR_TOPIC.\n")
    ));
    {
      ACE_GUARD_RETURN(ACE_SYNCH_MUTEX, guard, this->lock_, false);
      this->inUse_ = false;
    }
    return false;
  }
  status = reader->get_statuscondition();
  status->set_enabled_statuses( DDS::DATA_AVAILABLE_STATUS);
  this->waiter_->attach_condition( status.in());
  this->handleTypeMap_[ reader->get_instance_handle()]
    = OpenDDS::DCPS::DATA_READER_REPORT_TYPE;

  OpenDDS::DCPS::DataReaderPeriodicReportTypeSupportImpl* dataReaderPeriodicReportTypeSupport
    = new OpenDDS::DCPS::DataReaderPeriodicReportTypeSupportImpl();
  dataReaderPeriodicReportTypeSupport->register_type( this->participant_.in(), 0);
  topic = this->participant_->create_topic(
            OpenDDS::DCPS::DATA_READER_PERIODIC_MONITOR_TOPIC,
            dataReaderPeriodicReportTypeSupport->get_type_name(),
            TOPIC_QOS_DEFAULT,
            DDS::TopicListener::_nil(),
            OpenDDS::DCPS::DEFAULT_STATUS_MASK
          );

  topic->get_qos( topicQos);
  subscriber->get_default_datareader_qos( readerQos);
  subscriber->copy_from_topic_qos( readerQos, topicQos);
  reader = subscriber->create_datareader(
             topic,
             readerQos,
             ::DDS::DataReaderListener::_nil(),
             ::OpenDDS::DCPS::DEFAULT_STATUS_MASK
           );
  if( CORBA::is_nil( reader.in())) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: MonitorTask::startInstrumentation() - ")
      ACE_TEXT("failed to create a reader for DATA_READER_PERIODIC_MONITOR_TOPIC.\n")
    ));
    {
      ACE_GUARD_RETURN(ACE_SYNCH_MUTEX, guard, this->lock_, false);
      this->inUse_ = false;
    }
    return false;
  }
  status = reader->get_statuscondition();
  status->set_enabled_statuses( DDS::DATA_AVAILABLE_STATUS);
  this->waiter_->attach_condition( status.in());
  this->handleTypeMap_[ reader->get_instance_handle()]
    = OpenDDS::DCPS::DATA_READER_PERIODIC_REPORT_TYPE;

  OpenDDS::DCPS::TransportReportTypeSupportImpl* transportReportTypeSupport
    = new OpenDDS::DCPS::TransportReportTypeSupportImpl();
  transportReportTypeSupport->register_type( this->participant_.in(), 0);
  topic = this->participant_->create_topic(
            OpenDDS::DCPS::TRANSPORT_MONITOR_TOPIC,
            transportReportTypeSupport->get_type_name(),
            TOPIC_QOS_DEFAULT,
            DDS::TopicListener::_nil(),
            OpenDDS::DCPS::DEFAULT_STATUS_MASK
          );

  topic->get_qos( topicQos);
  subscriber->get_default_datareader_qos( readerQos);
  subscriber->copy_from_topic_qos( readerQos, topicQos);
  reader = subscriber->create_datareader(
             topic,
             readerQos,
             ::DDS::DataReaderListener::_nil(),
             ::OpenDDS::DCPS::DEFAULT_STATUS_MASK
           );
  if( CORBA::is_nil( reader.in())) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: MonitorTask::startInstrumentation() - ")
      ACE_TEXT("failed to create a reader for TRANSPORT_MONITOR_TOPIC.\n")
    ));
    {
      ACE_GUARD_RETURN(ACE_SYNCH_MUTEX, guard, this->lock_, false);
      this->inUse_ = false;
    }
    return false;
  }
  status = reader->get_statuscondition();
  status->set_enabled_statuses( DDS::DATA_AVAILABLE_STATUS);
  this->waiter_->attach_condition( status.in());
  this->handleTypeMap_[ reader->get_instance_handle()]
    = OpenDDS::DCPS::TRANSPORT_REPORT_TYPE;

  // Yield control.
  {
    ACE_GUARD_RETURN(ACE_SYNCH_MUTEX, guard, this->lock_, false);
    this->inUse_ = false;
  }
  this->gate_.broadcast();

  return true;
}

void
Monitor::MonitorTask::dispatchReader( DDS::DataReader_ptr reader)
{
  switch( this->handleTypeMap_[ reader->get_instance_handle()]) {
    case OpenDDS::DCPS::SERVICE_PARTICIPANT_REPORT_TYPE:
      {
        InboundData<
          OpenDDS::DCPS::ServiceParticipantReportDataReader,
          OpenDDS::DCPS::ServiceParticipantReport
        > handler;
        handler.process( reader, this->data_);
      }
      break;

    case OpenDDS::DCPS::DOMAIN_PARTICIPANT_REPORT_TYPE:
      {
        InboundData<
          OpenDDS::DCPS::DomainParticipantReportDataReader,
          OpenDDS::DCPS::DomainParticipantReport
        > handler;
        handler.process( reader, this->data_);
      }
      break;

    case OpenDDS::DCPS::TOPIC_REPORT_TYPE:
      {
        InboundData<
          OpenDDS::DCPS::TopicReportDataReader,
          OpenDDS::DCPS::TopicReport
        > handler;
        handler.process( reader, this->data_);
      }
      break;

    case OpenDDS::DCPS::PUBLISHER_REPORT_TYPE:
      {
        InboundData<
          OpenDDS::DCPS::PublisherReportDataReader,
          OpenDDS::DCPS::PublisherReport
        > handler;
        handler.process( reader, this->data_);
      }
      break;

    case OpenDDS::DCPS::SUBSCRIBER_REPORT_TYPE:
      {
        InboundData<
          OpenDDS::DCPS::SubscriberReportDataReader,
          OpenDDS::DCPS::SubscriberReport
        > handler;
        handler.process( reader, this->data_);
      }
      break;

    case OpenDDS::DCPS::DATA_WRITER_REPORT_TYPE:
      {
        InboundData<
          OpenDDS::DCPS::DataWriterReportDataReader,
          OpenDDS::DCPS::DataWriterReport
        > handler;
        handler.process( reader, this->data_);
      }
      break;

    case OpenDDS::DCPS::DATA_WRITER_PERIODIC_REPORT_TYPE:
      {
        InboundData<
          OpenDDS::DCPS::DataWriterPeriodicReportDataReader,
          OpenDDS::DCPS::DataWriterPeriodicReport
        > handler;
        handler.process( reader, this->data_);
      }
      break;

    case OpenDDS::DCPS::DATA_READER_REPORT_TYPE:
      {
        InboundData<
          OpenDDS::DCPS::DataReaderReportDataReader,
          OpenDDS::DCPS::DataReaderReport
        > handler;
        handler.process( reader, this->data_);
      }
      break;

    case OpenDDS::DCPS::DATA_READER_PERIODIC_REPORT_TYPE:
      {
        InboundData<
          OpenDDS::DCPS::DataReaderPeriodicReportDataReader,
          OpenDDS::DCPS::DataReaderPeriodicReport
        > handler;
        handler.process( reader, this->data_);
      }
      break;

    case OpenDDS::DCPS::TRANSPORT_REPORT_TYPE:
      {
        InboundData<
          OpenDDS::DCPS::TransportReportDataReader,
          OpenDDS::DCPS::TransportReport
        > handler;
        handler.process( reader, this->data_);
      }
      break;
  }
}

template< class ReaderType, class DataType>
void
Monitor::MonitorTask::InboundData< ReaderType, DataType>::process(
  DDS::DataReader_ptr reader,
  MonitorData*        model
)
{
  // Extract the specific reader for this data.
  typename ReaderType::_var_type typedReader
    = ReaderType::_narrow( reader);
  if( CORBA::is_nil( typedReader.in())) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: InboundData::process() - ")
      ACE_TEXT("failed to narrow the reader.\n")
    ));
    return;
  }

  // Read and forward all available data.
  DataType        data;
  DDS::SampleInfo info;
  while( DDS::RETCODE_OK == typedReader->take_next_sample( data, info)) {
    if( info.valid_data) {
      model->update( data);

    } else if( info.instance_state & DDS::NOT_ALIVE_INSTANCE_STATE) {
      model->update( data, true);
    }
  }
}

