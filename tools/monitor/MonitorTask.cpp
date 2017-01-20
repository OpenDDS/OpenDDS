// -*- C++ -*-
//

/*
*
*
* Distributed under the OpenDDS License.
* See: http://www.opendds.org/license.html
*/

#include "MonitorTask.h"
#include "MonitorDataStorage.h"
#include "Options.h"

#include "dds/DdsDcpsInfrastructureTypeSupportImpl.h"
#include "dds/monitor/monitorTypeSupportImpl.h"
#include "dds/DCPS/BuiltInTopicUtils.h"
#include "dds/DCPS/DataWriterImpl.h"
#include "dds/DCPS/GuidConverter.h"
#include "dds/DCPS/Qos_Helper.h"
#include "dds/DCPS/InfoRepoDiscovery/InfoRepoDiscovery.h"
#include "dds/DCPS/Marked_Default_Qos.h"
#include "dds/DCPS/transport/framework/TransportImpl_rch.h"
#include "dds/DdsDcpsInfoUtilsC.h"

#include "ace/OS_NS_Thread.h"

#include <sstream>

namespace { // Anonymous namespace for file scope.

  /// Type Id values for dispatching Builtin Topic samples.
  /// @note This must not conflict with the OpenDDS::DCPS::ReportType
  ///       enumeration.
  enum BuiltinReportType {
    BUILTIN_PARTICIPANT_REPORT_TYPE = OpenDDS::DCPS::TRANSPORT_REPORT_TYPE + 1,
    BUILTIN_TOPIC_REPORT_TYPE,
    BUILTIN_PUBLICATION_REPORT_TYPE,
    BUILTIN_SUBSCRIPTION_REPORT_TYPE
  };

  template <typename T> std::string stringify(const T& val) {
    std::ostringstream oss;
    oss << val;
    return oss.str();
  }

} // End of anonymous namespace

const Monitor::MonitorTask::RepoKey Monitor::MonitorTask::DEFAULT_REPO = OpenDDS::DCPS::Discovery::DEFAULT_REPO;

Monitor::MonitorTask::MonitorTask(
  MonitorDataStorage* data,
  const Options&      options,
  bool                mapExistingIORKeys
) : opened_( false),
    done_( false),
    options_( options),
    data_( data),
    gate_( this->lock_),
    waiter_( new DDS::WaitSet),
    guardCondition_( new DDS::GuardCondition),
    activeKeyInited_(false),
    lastKey_( 0)
{
  // Find and map the current IOR strings to their IOR key values.
  if (mapExistingIORKeys) {
    for( OpenDDS::DCPS::Service_Participant::RepoKeyDiscoveryMap::const_iterator
         location = TheServiceParticipant->discoveryMap().begin();
         location != TheServiceParticipant->discoveryMap().end();
         ++location) {
      OpenDDS::DCPS::InfoRepoDiscovery_rch irDisco =
        OpenDDS::DCPS::dynamic_rchandle_cast<OpenDDS::DCPS::InfoRepoDiscovery>(location->second);
      std::string ior;
      // only InfoRepoDiscovery has an ior
      if (!irDisco.is_nil()) {
        ior = irDisco->get_stringified_dcps_info_ior();
      } else {
        ior = "";
      }

      this->iorKeyMap_[ior] = location->first;
    }
  }
}

Monitor::MonitorTask::~MonitorTask()
{
  // Terminate processing cleanly.
  this->stop();
  this->iorKeyMap_.clear();
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

  ACE_GUARD(ACE_SYNCH_MUTEX, guard, this->lock_);

  // Clean up the wait conditions.
  DDS::ConditionSeq conditions;
  this->waiter_->get_conditions( conditions);
  this->waiter_->detach_conditions( conditions);

  // Reattach the guard condition.
  this->waiter_->attach_condition( this->guardCondition_);

  // Terminate the transport resources.

  // It is possible that the repository was down at this point,
  // and we do not want the exception stop the monitor process
  // so catch exception here.
  try {
    // Destroy the instrumentation participant.
    this->participant_->delete_contained_entities();
    TheParticipantFactory->delete_participant( this->participant_.in());
  }
  catch (const CORBA::TRANSIENT&)
  {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) MonitorTask::stopInstrumentation() - ")
      ACE_TEXT("caught TRANSIENT exception.\n")
    ));
  }
  catch (const CORBA::Exception& ex) {
    ex._tao_print_exception("(%P|%t)  MonitorTask::stopInstrumentation() - ");
    throw;
  }
  catch (...) {
    throw;
  }

  this->participant_ = DDS::DomainParticipant::_nil();
}

Monitor::MonitorTask::RepoKey
Monitor::MonitorTask::setRepoIor( const std::string& ior)
{
  RepoKey key;

  if( this->options_.verbose()) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) MonitorTask::setRepoIor() - ")
      ACE_TEXT("setting active repository to: %C.\n"),
      ior.c_str()
    ));
  }

  { ACE_GUARD_RETURN(ACE_SYNCH_MUTEX, guard, this->lock_, DEFAULT_REPO);

    IorKeyMap::iterator location = this->iorKeyMap_.find( ior);
    if( location != this->iorKeyMap_.end()) {
      // We already have this IOR mapped, use the existing key.
      key = location->second;
      // In case the same repo restart again, need resolve the
      // repo object reference again.
      TheServiceParticipant->set_repo_ior( ior.c_str(), key, false);

    } else {
      // We need to find an open key to use.  Check the actual
      // Service_Participant mappings for a slot.
      OpenDDS::DCPS::Service_Participant::RepoKeyDiscoveryMap::const_iterator
        keyLocation;
      do {
        key = stringify(++this->lastKey_);
        keyLocation = TheServiceParticipant->discoveryMap().find( key);
      } while( keyLocation != TheServiceParticipant->discoveryMap().end());

      // We have a new repository to install, go ahead.
      TheServiceParticipant->set_repo_ior( ior.c_str(), key, false);

      // Check that we were able to resolve and attach to the repository.
      keyLocation = TheServiceParticipant->discoveryMap().find( key);
      if( keyLocation == TheServiceParticipant->discoveryMap().end()) {
        // We failed to install this IOR, nothing left to do.
        return DEFAULT_REPO;
      }

      // Store the reverse mapping for our use.
      this->iorKeyMap_[ ior] = key;
    }
  } // End of lock scope.

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

    // Clear previous conditions.
    conditions.length(0);
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

    ACE_GUARD_RETURN(ACE_SYNCH_MUTEX, guard, this->lock_, -1);

    for( unsigned long index = 0; index < conditions.length(); ++index) {
      // Extract the current Condition.
      DDS::StatusCondition_var condition
        = DDS::StatusCondition::_narrow( conditions[ index].in());
      if( !CORBA::is_nil( condition.in())) {
        if( this->options_.verbose()) {
          ACE_DEBUG((LM_DEBUG,
            ACE_TEXT("(%P|%t) MonitorTask::svc() - ")
            ACE_TEXT("processing condition for instance %d.\n"),
            condition->get_entity()->get_instance_handle()
          ));
        }
        // It's a CommunicationStatus, process inbound data.
        DDS::DataReader_var reader
          = DDS::DataReader::_narrow( condition->get_entity());
        if( !CORBA::is_nil( reader.in())) {
          // Process inbound data here.
          this->dispatchReader( reader.in());
        }
      }
    }
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

  ACE_GUARD_RETURN(ACE_SYNCH_MUTEX, guard, this->lock_, false);

  if( this->options_.verbose()) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) MonitorTask::setActiveRepo() - ")
      ACE_TEXT("rebinding instrumentation domain %d to repository key: %C.\n"),
      this->options_.domain(),
      key.c_str()
    ));
  }

  // Remap all domains pointing to the old repository to the new one.
  // But do not attach the participant to the repository as the repo
  // may not know the monitor domain and monitor also will request
  // create_participant.
  TheServiceParticipant->remap_domains( this->activeKey_, key, false);

  // Check that the instrumentation domain repository is the new one.
  RepoKey monitorKey = TheServiceParticipant->domain_to_repo(
                        this->options_.domain()
                      );

  if(!activeKeyInited_ || monitorKey != key) {
    if (!activeKeyInited_)
      {
        activeKeyInited_ = true;
      }

    // Otherwise map the instrumentation domain onto the new repository.
    TheServiceParticipant->set_repo_domain( this->options_.domain(), key, false);
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
      ACE_TEXT("(%P|%t) ERROR: MonitorTask::setActiveRepo() - ")
      ACE_TEXT("failed to create participant.\n")
    ));
    return false;
  }

  if( this->options_.verbose()) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) MonitorTask::setActiveRepo() - ")
      ACE_TEXT("created participant.\n")
    ));
  }

  // Fire up a subscriber.
  DDS::Subscriber_var subscriber = this->participant_->create_subscriber(
                                     SUBSCRIBER_QOS_DEFAULT,
                                     ::DDS::SubscriberListener::_nil(),
                                     ::OpenDDS::DCPS::DEFAULT_STATUS_MASK
                                   );
  if( CORBA::is_nil( subscriber.in())) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: MonitorTask::setActiveRepo() - ")
      ACE_TEXT("failed to create subscriber.\n")
    ));
    return false;
  }

  if( this->options_.verbose()) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) MonitorTask::setActiveRepo() - ")
      ACE_TEXT("created subscriber.\n")
    ));
  }

  // Create each instrumentation subscription and attach them to the waiter.

  this->createSubscription<
          OpenDDS::DCPS::ServiceParticipantReportTypeSupportImpl>(
            subscriber.in(),
            OpenDDS::DCPS::SERVICE_PARTICIPANT_MONITOR_TOPIC,
            OpenDDS::DCPS::SERVICE_PARTICIPANT_REPORT_TYPE);

  this->createSubscription<
          OpenDDS::DCPS::DomainParticipantReportTypeSupportImpl>(
            subscriber.in(),
            OpenDDS::DCPS::DOMAIN_PARTICIPANT_MONITOR_TOPIC,
            OpenDDS::DCPS::DOMAIN_PARTICIPANT_REPORT_TYPE);

  this->createSubscription<
          OpenDDS::DCPS::TopicReportTypeSupportImpl>(
            subscriber.in(),
            OpenDDS::DCPS::TOPIC_MONITOR_TOPIC,
            OpenDDS::DCPS::TOPIC_REPORT_TYPE);

  this->createSubscription<
          OpenDDS::DCPS::PublisherReportTypeSupportImpl>(
            subscriber.in(),
            OpenDDS::DCPS::PUBLISHER_MONITOR_TOPIC,
            OpenDDS::DCPS::PUBLISHER_REPORT_TYPE);

  this->createSubscription<
          OpenDDS::DCPS::SubscriberReportTypeSupportImpl>(
            subscriber.in(),
            OpenDDS::DCPS::SUBSCRIBER_MONITOR_TOPIC,
            OpenDDS::DCPS::SUBSCRIBER_REPORT_TYPE);

  this->createSubscription<
          OpenDDS::DCPS::DataWriterReportTypeSupportImpl>(
            subscriber.in(),
            OpenDDS::DCPS::DATA_WRITER_MONITOR_TOPIC,
            OpenDDS::DCPS::DATA_WRITER_REPORT_TYPE);

  this->createSubscription<
          OpenDDS::DCPS::DataWriterPeriodicReportTypeSupportImpl>(
            subscriber.in(),
            OpenDDS::DCPS::DATA_WRITER_PERIODIC_MONITOR_TOPIC,
            OpenDDS::DCPS::DATA_WRITER_PERIODIC_REPORT_TYPE);

  this->createSubscription<
          OpenDDS::DCPS::DataReaderReportTypeSupportImpl>(
            subscriber.in(),
            OpenDDS::DCPS::DATA_READER_MONITOR_TOPIC,
            OpenDDS::DCPS::DATA_READER_REPORT_TYPE);

  this->createSubscription<
          OpenDDS::DCPS::DataReaderPeriodicReportTypeSupportImpl>(
            subscriber.in(),
            OpenDDS::DCPS::DATA_READER_PERIODIC_MONITOR_TOPIC,
            OpenDDS::DCPS::DATA_READER_PERIODIC_REPORT_TYPE);

  this->createSubscription<
          OpenDDS::DCPS::TransportReportTypeSupportImpl>(
            subscriber.in(),
            OpenDDS::DCPS::TRANSPORT_MONITOR_TOPIC,
            OpenDDS::DCPS::TRANSPORT_REPORT_TYPE);

  if( this->options_.verbose()) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) MonitorTask::setActiveRepo() - ")
      ACE_TEXT("created all instrumentation subscriptions.\n")
    ));
  }

  // Create each builtin topic subscription and attach them to the waiter.

  this->createBuiltinSubscription(
          OpenDDS::DCPS::BUILT_IN_PARTICIPANT_TOPIC,
          BUILTIN_PARTICIPANT_REPORT_TYPE);

  this->createBuiltinSubscription(
          OpenDDS::DCPS::BUILT_IN_TOPIC_TOPIC,
          BUILTIN_TOPIC_REPORT_TYPE);

  this->createBuiltinSubscription(
          OpenDDS::DCPS::BUILT_IN_PUBLICATION_TOPIC,
          BUILTIN_PUBLICATION_REPORT_TYPE);

  this->createBuiltinSubscription(
          OpenDDS::DCPS::BUILT_IN_SUBSCRIPTION_TOPIC,
          BUILTIN_SUBSCRIPTION_REPORT_TYPE);

  if( this->options_.verbose()) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) MonitorTask::setActiveRepo() - ")
      ACE_TEXT("attached all builtinTopic subscriptions.\n")
    ));
  }

  return true;
}

template< class TypeSupport>
void
Monitor::MonitorTask::createSubscription(
  DDS::Subscriber_ptr subscriber,
  const char*         topicName,
  int                 type
)
{
  DDS::Topic_var           topic;
  DDS::TopicQos            topicQos;
  DDS::DataReaderQos       readerQos;
  DDS::DataReader_var      reader;
  DDS::StatusCondition_var status;

  TypeSupport* typeSupport = new TypeSupport();
  typeSupport->register_type( this->participant_.in(), 0);
  topic = this->participant_->create_topic(
            topicName,
            CORBA::String_var(typeSupport->get_type_name()),
            TOPIC_QOS_DEFAULT,
            DDS::TopicListener::_nil(),
            OpenDDS::DCPS::DEFAULT_STATUS_MASK
          );

  topic->get_qos( topicQos);
  subscriber->get_default_datareader_qos( readerQos);
  subscriber->copy_from_topic_qos( readerQos, topicQos);
  readerQos.durability.kind = DDS::TRANSIENT_LOCAL_DURABILITY_QOS;
  reader = subscriber->create_datareader(
             topic,
             readerQos,
             ::DDS::DataReaderListener::_nil(),
             ::OpenDDS::DCPS::DEFAULT_STATUS_MASK
           );
  if( CORBA::is_nil( reader.in())) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: MonitorTask::createSubscription() - ")
      ACE_TEXT("failed to create a reader for %s.\n"),
      topicName
    ));
    return;
  }

  // Track the type by handle and process any initial data.
  this->handleTypeMap_[ reader->get_instance_handle()] = type;

  status = reader->get_statuscondition();
  status->set_enabled_statuses( DDS::DATA_AVAILABLE_STATUS);
  this->waiter_->attach_condition( status.in());

  if( this->options_.verbose()) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) MonitorTask::createSubscription() - ")
      ACE_TEXT("topic %C installed and mapped instance %d to type %d.\n"),
      topicName,
      reader->get_instance_handle(),
      type
    ));
  }
}

void
Monitor::MonitorTask::createBuiltinSubscription(
  const char* topicName,
  int         type
)
{
  DDS::Subscriber_var subscriber
    = this->participant_->get_builtin_subscriber();
  DDS::DataReader_var reader
    = subscriber->lookup_datareader( topicName);

  // Track the type by handle and process any initial data.
  this->handleTypeMap_[ reader->get_instance_handle()] = type;

  DDS::StatusCondition_var status = reader->get_statuscondition();
  status->set_enabled_statuses( DDS::DATA_AVAILABLE_STATUS);
  this->waiter_->attach_condition( status.in());

  if( this->options_.verbose()) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) MonitorTask::createBuiltinSubscription() - ")
      ACE_TEXT("topic %C installed and mapped instance %d to type %d.\n"),
      topicName,
      reader->get_instance_handle(),
      type
    ));
  }
}

void
Monitor::MonitorTask::dispatchReader( DDS::DataReader_ptr reader)
{
  switch( this->handleTypeMap_[ reader->get_instance_handle()]) {
    case OpenDDS::DCPS::SERVICE_PARTICIPANT_REPORT_TYPE:
      {
        this->dataUpdate<
          OpenDDS::DCPS::ServiceParticipantReportDataReader,
          OpenDDS::DCPS::ServiceParticipantReport>(
            reader);
      }
      break;

    case OpenDDS::DCPS::DOMAIN_PARTICIPANT_REPORT_TYPE:
      {
        this->dataUpdate<
          OpenDDS::DCPS::DomainParticipantReportDataReader,
          OpenDDS::DCPS::DomainParticipantReport>(
            reader);
      }
      break;

    case OpenDDS::DCPS::TOPIC_REPORT_TYPE:
      {
        this->dataUpdate<
          OpenDDS::DCPS::TopicReportDataReader,
          OpenDDS::DCPS::TopicReport>(
            reader);
      }
      break;

    case OpenDDS::DCPS::PUBLISHER_REPORT_TYPE:
      {
        this->dataUpdate<
          OpenDDS::DCPS::PublisherReportDataReader,
          OpenDDS::DCPS::PublisherReport>(
            reader);
      }
      break;

    case OpenDDS::DCPS::SUBSCRIBER_REPORT_TYPE:
      {
        this->dataUpdate<
          OpenDDS::DCPS::SubscriberReportDataReader,
          OpenDDS::DCPS::SubscriberReport>(
            reader);
      }
      break;

    case OpenDDS::DCPS::DATA_WRITER_REPORT_TYPE:
      {
        this->dataUpdate<
          OpenDDS::DCPS::DataWriterReportDataReader,
          OpenDDS::DCPS::DataWriterReport>(
            reader);
      }
      break;

    case OpenDDS::DCPS::DATA_WRITER_PERIODIC_REPORT_TYPE:
      {
        this->dataUpdate<
          OpenDDS::DCPS::DataWriterPeriodicReportDataReader,
          OpenDDS::DCPS::DataWriterPeriodicReport>(
            reader);
      }
      break;

    case OpenDDS::DCPS::DATA_READER_REPORT_TYPE:
      {
        this->dataUpdate<
          OpenDDS::DCPS::DataReaderReportDataReader,
          OpenDDS::DCPS::DataReaderReport>(
            reader);
      }
      break;

    case OpenDDS::DCPS::DATA_READER_PERIODIC_REPORT_TYPE:
      {
        this->dataUpdate<
          OpenDDS::DCPS::DataReaderPeriodicReportDataReader,
          OpenDDS::DCPS::DataReaderPeriodicReport>(
            reader);
      }
      break;

    case OpenDDS::DCPS::TRANSPORT_REPORT_TYPE:
      {
        this->dataUpdate<
          OpenDDS::DCPS::TransportReportDataReader,
          OpenDDS::DCPS::TransportReport>(
            reader);
      }
      break;

    case BUILTIN_PARTICIPANT_REPORT_TYPE:
      {
        this->builtinTopicUpdate<
          DDS::ParticipantBuiltinTopicDataDataReader,
          DDS::ParticipantBuiltinTopicData>(
            reader);
      }
      break;

    case BUILTIN_TOPIC_REPORT_TYPE:
      {
        this->builtinTopicUpdate<
          DDS::TopicBuiltinTopicDataDataReader,
          DDS::TopicBuiltinTopicData>(
            reader);
      }
      break;

    case BUILTIN_PUBLICATION_REPORT_TYPE:
      {
        this->builtinTopicUpdate<
          DDS::PublicationBuiltinTopicDataDataReader,
          DDS::PublicationBuiltinTopicData>(
            reader);
      }
      break;

    case BUILTIN_SUBSCRIPTION_REPORT_TYPE:
      {
        this->builtinTopicUpdate<
          DDS::SubscriptionBuiltinTopicDataDataReader,
          DDS::SubscriptionBuiltinTopicData>(
            reader);
      }
      break;

    default:
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: MonitorTask::dispatchReader() - ")
        ACE_TEXT("unknown type for instance %d.\n"),
        reader->get_instance_handle()
      ));
      break;
  }
}

template< class ReaderType, class DataType>
void
Monitor::MonitorTask::dataUpdate(
  DDS::DataReader_ptr reader
)
{
  // Extract the specific reader for this data.
  typename ReaderType::_var_type typedReader
    = ReaderType::_narrow( reader);
  if( CORBA::is_nil( typedReader.in())) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: MonitorTask::dataUpdate() - ")
      ACE_TEXT("failed to narrow the reader for instance %d.\n"),
      reader->get_instance_handle()
    ));
    return;
  }

  // Diagnostic information only.
  int valid   = 0;
  int invalid = 0;

  // Read and forward all available data.
  DataType        data;
  DDS::SampleInfo info;
  while( DDS::RETCODE_OK == typedReader->take_next_sample( data, info)) {
    if( info.valid_data) {
      this->data_->update(data, this->participant_);
      ++valid;

    } else if( info.instance_state & DDS::NOT_ALIVE_INSTANCE_STATE) {
      this->data_->update(data, this->participant_, true);
      ++invalid;
    }
  }

  if( this->options_.verbose()) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) MonitorTask::dataUpdate() - ")
      ACE_TEXT("forwarded %d/%d (valid/invalid) samples from instance %d.\n"),
      valid, invalid, reader->get_instance_handle()
    ));
  }
}

template< class ReaderType, class DataType>
void
Monitor::MonitorTask::builtinTopicUpdate(
  DDS::DataReader_ptr reader
)
{
  // Get the specific reader we need.
  typename ReaderType::_var_type typedReader
    = ReaderType::_narrow(reader);
  if( CORBA::is_nil( typedReader.in())) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: MonitorTask::builtinTopicUpdate() - ")
      ACE_TEXT("failed to narrow the reader for instance %d.\n"),
      reader->get_instance_handle()
    ));
    return;
  }

  // Diagnostic information only.
  int valid   = 0;
  int invalid = 0;

  // Read and forward all available new data.
  DataType        data;
  DDS::SampleInfo info;
  while( DDS::RETCODE_OK == typedReader->read_next_sample( data, info)) {
    if( info.valid_data) {
      this->data_->update(data, this->participant_);
      ++valid;

    } else if( info.instance_state & DDS::NOT_ALIVE_INSTANCE_STATE) {
      this->data_->update(data, this->participant_, true);
      ++invalid;
    }
  }

  if( this->options_.verbose()) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) MonitorTask::builtinTopicUpdate() - ")
      ACE_TEXT("forwarded %d/%d (valid/invalid) samples from instance %d.\n"),
      valid, invalid, reader->get_instance_handle()
    ));
  }
}

template< class ReaderType, class DataType, class DataTypeSeq>
bool
Monitor::MonitorTask::readBuiltinTopicData(
  const OpenDDS::DCPS::GUID_t& id,
  DataType&                    data,
  const char*                  topicName
)
{
  // Grab the BuiltinTopic subscriber.
  DDS::Subscriber_var subscriber
    = this->participant_->get_builtin_subscriber();

  // Lookup the reader for this BuiltinTopic type.
  DDS::DataReader_var reader =
    subscriber->lookup_datareader( topicName);

  // Get the specific reader we need.
  typename ReaderType::_var_type typedReader
    = ReaderType::_narrow(reader.in());

  // Find the instance to read.
  OpenDDS::DCPS::DomainParticipantImpl* dpi =
    dynamic_cast<OpenDDS::DCPS::DomainParticipantImpl*>(this->participant_.in());
  DDS::InstanceHandle_t instance = dpi->id_to_handle(id);

  if (this->options_.verbose()) {
    OpenDDS::DCPS::GuidConverter converter(id);
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) MonitorTask::readBuiltinTopicData<%s>() - ")
      ACE_TEXT("id: %C ==> BuiltinTopic key: ")
      ACE_TEXT("[0x%x, 0x%x, 0x%x], handle %d.\n"),
      topicName,
      std::string(converter).c_str(),
      data.key.value[0], data.key.value[1], data.key.value[2],
      instance
    ));
  }

  if (instance == DDS::HANDLE_NIL) {
    OpenDDS::DCPS::GuidConverter converter(id);
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) MonitorTask::readBuiltinTopicData<%s>() - ")
      ACE_TEXT("no data for id %C at this time.\n"),
      topicName,
      std::string(converter).c_str()
    ));
    return false;
  }

  // with HISTORY.depth == 1, this should be the sample that we are
  // interested in.
  DDS::SampleInfoSeq infoSeq( 1);
  DataTypeSeq        dataSeq( 1);
  if( DDS::RETCODE_OK != typedReader->read_instance(
                           dataSeq, infoSeq, 1, instance,
                           DDS::ANY_SAMPLE_STATE,
                           DDS::ANY_VIEW_STATE,
                           DDS::ANY_INSTANCE_STATE
                         )) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: MonitorTask::readBuiltinTopicData<%s>() - ")
      ACE_TEXT("failed to read instance %d.\n"),
      topicName,
      instance
    ));
    return false;
  }

  // Copy out the data we want.
  if( dataSeq.length() > 0) {
    data = dataSeq[0];

  } else {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: MonitorTask::readBuiltinTopicData<%s>() - ")
      ACE_TEXT("failed to read a sample.\n"),
      topicName
    ));
    return false;
  }

  if( this->options_.verbose()) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) MonitorTask::readBuiltinTopicData<%s>() - ")
      ACE_TEXT("%s read BuiltinTopic data.\n"),
      topicName,
      infoSeq[0].valid_data? "successfully": "failed to"
    ));
  }

  return infoSeq[0].valid_data;
}

bool
Monitor::MonitorTask::getBuiltinTopicData(
  const OpenDDS::DCPS::GUID_t&      id,
  DDS::ParticipantBuiltinTopicData& data
)
{
  return this->readBuiltinTopicData<
           DDS::ParticipantBuiltinTopicDataDataReader,
           DDS::ParticipantBuiltinTopicData,
           DDS::ParticipantBuiltinTopicDataSeq>(
             id, data, OpenDDS::DCPS::BUILT_IN_PARTICIPANT_TOPIC
           );
}

bool
Monitor::MonitorTask::getBuiltinTopicData(
  const OpenDDS::DCPS::GUID_t& id,
  DDS::TopicBuiltinTopicData&  data
)
{
  return this->readBuiltinTopicData<
           DDS::TopicBuiltinTopicDataDataReader,
           DDS::TopicBuiltinTopicData,
           DDS::TopicBuiltinTopicDataSeq>(
             id, data, OpenDDS::DCPS::BUILT_IN_TOPIC_TOPIC
           );
}

bool
Monitor::MonitorTask::getBuiltinTopicData(
  const OpenDDS::DCPS::GUID_t&      id,
  DDS::PublicationBuiltinTopicData& data
)
{
  return this->readBuiltinTopicData<
           DDS::PublicationBuiltinTopicDataDataReader,
           DDS::PublicationBuiltinTopicData,
           DDS::PublicationBuiltinTopicDataSeq>(
             id, data, OpenDDS::DCPS::BUILT_IN_PUBLICATION_TOPIC
           );
}

bool
Monitor::MonitorTask::getBuiltinTopicData(
  const OpenDDS::DCPS::GUID_t&       id,
  DDS::SubscriptionBuiltinTopicData& data
)
{
  return this->readBuiltinTopicData<
           DDS::SubscriptionBuiltinTopicDataDataReader,
           DDS::SubscriptionBuiltinTopicData,
           DDS::SubscriptionBuiltinTopicDataSeq>(
             id, data, OpenDDS::DCPS::BUILT_IN_SUBSCRIPTION_TOPIC
           );
}

