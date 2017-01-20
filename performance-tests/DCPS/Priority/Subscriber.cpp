// -*- C++ -*-

#include "Subscriber.h"

#include "Test.h"
#include "Options.h"

#include "TestTypeSupportImpl.h"
#include "DataReaderListener.h"

#include "dds/DCPS/Service_Participant.h"
#include "dds/DCPS/Marked_Default_Qos.h"
#include "dds/DCPS/DataReaderImpl.h"
#include "dds/DCPS/SubscriberImpl.h"
#include "dds/DCPS/GuidConverter.h"
#include "dds/DCPS/transport/framework/TransportRegistry.h"

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
    this->participant_->delete_contained_entities();
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

  // Create the transport.
  OpenDDS::DCPS::TransportConfig_rch transport =
    TheTransportRegistry->get_config(this->options_.transportKey());

  if (transport.is_nil()) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: Subscriber::Subscriber() - ")
      ACE_TEXT("failed to get %C transport.\n"),
      this->options_.transportKey().c_str()
    ));
    throw BadTransportException();

  } else if( this->options_.verbose()) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) Subscriber::Subscriber() - ")
      ACE_TEXT("created %C transport.\n"),
      this->options_.transportKey().c_str()
    ));
  }

  // Create the listener.
  this->listener_ = new DataReaderListener( this->options_.verbose());
  this->safe_listener_ = this->listener_;
  if( this->options_.verbose()) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) Subscriber::Subscriber() - ")
      ACE_TEXT("created reader listener.\n")
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

  } else if( this->options_.verbose()) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) Subscriber::Subscriber() - ")
      ACE_TEXT("created topic %C.\n"),
      this->options_.topicName().c_str()
    ));
  }

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

  } else if( this->options_.verbose()) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) Subscriber::Subscriber() - ")
      ACE_TEXT("created subscriber.\n")
    ));
  }

  // Attach the transport to the subscriber.
  ::OpenDDS::DCPS::SubscriberImpl* servant
    = dynamic_cast< ::OpenDDS::DCPS::SubscriberImpl*>( this->subscriber_.in());
  if( 0 == servant) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: Subscriber::Subscriber() - ")
      ACE_TEXT("failed to narrow subscriber servant.\n")
    ));
    throw BadServantException();
  }

  // Configure the raw data gathering.
  servant->raw_latency_buffer_size() = this->options_.raw_buffer_size();
  servant->raw_latency_buffer_type() = this->options_.raw_buffer_type();
  if( this->options_.verbose()) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) Subscriber::Subscriber() - ")
      ACE_TEXT("configured to capture %d latency measurements of type %d ")
      ACE_TEXT("per writer to file %C.\n"),
      this->options_.raw_buffer_size(),
      this->options_.raw_buffer_type(),
      this->options_.rawOutputFilename().c_str()
    ));
  }

  TheTransportRegistry->bind_config(transport, servant);

  if( this->options_.verbose()) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) Subscriber::Subscriber() - ")
      ACE_TEXT("attached transport to subscriber.\n")
    ));
  }

  // Reader Qos policy values.
  ::DDS::DataReaderQos readerQos;
  this->subscriber_->get_default_datareader_qos( readerQos);

  readerQos.durability.kind                          = ::DDS::TRANSIENT_LOCAL_DURABILITY_QOS;
  readerQos.history.kind                             = ::DDS::KEEP_ALL_HISTORY_QOS;
  readerQos.resource_limits.max_samples_per_instance = ::DDS::LENGTH_UNLIMITED;

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
  ::DDS::DataReader_var reader
    = this->subscriber_->create_datareader(
        this->topic_.in(),
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

  this->reader_ = ::OpenDDS::DCPS::DataReaderEx::_narrow( reader.in());
  if( CORBA::is_nil( this->reader_.in())) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: Subscriber::Subscriber() - ")
      ACE_TEXT("failed to narrow reader to extract statistics.\n")
    ));
    throw BadReaderException();
  }

  // Clear and start statistics gathering.  Ideally we would want to
  // configurably delay the start here to avoid edge effects.
  this->reader_->reset_latency_stats();
  this->reader_->statistics_enabled( true);

  // Set the listener mask here so that we don't conflict with the
  // StatusCondition(s) that we want to wait on in the main thread.
  this->reader_->set_listener( this->listener_, DDS::DATA_AVAILABLE_STATUS);

  // Grab, enable and attach the status condition for test synchronization.
  this->status_ = this->reader_->get_statuscondition();
  this->status_->set_enabled_statuses( DDS::SUBSCRIPTION_MATCHED_STATUS);
  this->waiter_->attach_condition( this->status_.in());

  if( this->options_.verbose()) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) Subscriber::Subscriber() - ")
      ACE_TEXT("created StatusCondition and WaitSet for test synchronization.\n")
    ));
  }

}

int
Subscriber::total_messages() const
{
  return this->listener_->total_messages();
}

int
Subscriber::valid_messages() const
{
  return this->listener_->valid_messages();
}

const std::map< long, long>&
Subscriber::counts() const
{
  return this->listener_->counts();
}

const std::map< long, long>&
Subscriber::bytes() const
{
  return this->listener_->bytes();
}

const std::map< long, long>&
Subscriber::priorities() const
{
  return this->listener_->priorities();
}

void
Subscriber::run()
{
  DDS::Duration_t   timeout = { DDS::DURATION_INFINITE_SEC, DDS::DURATION_INFINITE_NSEC};
  DDS::ConditionSeq conditions;
  DDS::SubscriptionMatchedStatus matches = { 0, 0, 0, 0, 0};
  do {
    if( this->options_.verbose()) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Subscriber::run() - ")
        ACE_TEXT("%d publications attached.\n"),
        matches.current_count
      ));
    }
    if( DDS::RETCODE_OK != this->waiter_->wait( conditions, timeout)) {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: Subscriber::run() - ")
        ACE_TEXT("failed to synchronize at start of test.\n")
      ));
      throw BadSyncException();
    }
    if (this->reader_->get_subscription_matched_status(matches) != ::DDS::RETCODE_OK)
    {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: Subscriber::run() - ")
        ACE_TEXT("failed to get subscription matched status.\n")));
      ACE_OS::exit (1);
    }
  } while( matches.current_count > 0);

  if( this->options_.verbose()) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) Subscriber::run() - ")
      ACE_TEXT("shutting down after all publications were removed.\n")
    ));
  }
}

}

std::ostream&
operator<<( std::ostream& str, const Test::Subscriber& value)
{
  ::OpenDDS::DCPS::LatencyStatisticsSeq statistics;
  value.reader_->get_latency_stats( statistics);
  str << " --- statistical summary ---" << std::endl;
  for( unsigned long index = 0; index < statistics.length(); ++index) {
    OpenDDS::DCPS::GuidConverter converter(statistics[ index].publication);
    str << "  Writer[ " << OPENDDS_STRING(converter) << "]" << std::endl;
    str << "     samples: " << statistics[ index].n << std::endl;
    str << "        mean: " << statistics[ index].mean << std::endl;
    str << "     minimum: " << statistics[ index].minimum << std::endl;
    str << "     maximum: " << statistics[ index].maximum << std::endl;
    str << "    variance: " << statistics[ index].variance << std::endl;
  }

  return str;
}

namespace Test
{

std::ostream&
Subscriber::rawData( std::ostream& str) const
{
  // Configure the raw data gathering and extract the raw latency data
  // container.
  OpenDDS::DCPS::DataReaderImpl* readerImpl
    = dynamic_cast< OpenDDS::DCPS::DataReaderImpl*>( this->reader_.in());
  if( readerImpl == 0) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: Subscriber::Subscriber() - ")
      ACE_TEXT("failed to derive reader implementation.\n")
    ));
    throw BadReaderException();
  }

  int index = 0;
  for( OpenDDS::DCPS::DataReaderImpl::StatsMapType::const_iterator current
         = readerImpl->raw_latency_statistics().begin();
       current != readerImpl->raw_latency_statistics().end();
       ++current, ++index) {
    OpenDDS::DCPS::GuidConverter converter(current->first);
    str << std::endl << "  Writer[ " << OPENDDS_STRING(converter) << "]" << std::endl;
#ifndef OPENDDS_SAFETY_PROFILE
    current->second.raw_data( str);
#endif //OPENDDS_SAFETY_PROFILE
  }
  return str;
}

} // End of namespace Test

