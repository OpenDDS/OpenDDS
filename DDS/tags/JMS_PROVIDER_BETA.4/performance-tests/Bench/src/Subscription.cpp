// -*- C++ -*-
// $Id$

#include "Subscription.h"

#include "Test.h"
#include "EntityProfiles.h"
#include "TestTypeSupportImpl.h"
#include "DataReaderListener.h"

#include "dds/DCPS/RepoIdConverter.h"
#include "dds/DCPS/DataReaderImpl.h"
#include "dds/DCPS/SubscriberImpl.h"
#include "dds/DCPS/transport/framework/TheTransportFactory.h"

#include <iostream>
#include <sstream>

namespace Test {

Subscription::Subscription(
  const char* name,
  SubscriptionProfile* profile,
  bool verbose
) : name_( name),
    profile_( profile),
    verbose_( verbose),
    enabled_( false)
{
}

Subscription::~Subscription()
{
}

::DDS::StatusCondition_ptr
Subscription::get_statuscondition()
{
  if( !this->enabled_) {
    if( this->verbose_) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Subscription::get_statuscondition() - subscription %s: ")
        ACE_TEXT("not enabled, declining to process.\n"),
        this->name_.c_str()
      ));
    }
    return ::DDS::StatusCondition::_nil();
  }

  // We do not need to hold the condition so we do not duplicate it.
  return this->reader_->get_statuscondition();
}

void
Subscription::set_destination( Publication* publication)
{
  if( !this->enabled_) {
    if( this->verbose_) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Subscription::set_destination() - subscription %s: ")
        ACE_TEXT("not enabled, declining to process.\n"),
        this->name_.c_str()
      ));
    }
    return;
  }

  // Establish the writer as a forwarding destination.
  this->listener_->set_destination( publication);
}

void
Subscription::enable(
  ::DDS::DomainParticipant_ptr participant,
  ::DDS::Topic_ptr             topic
)
{
  if( this->enabled_) {
    if( this->verbose_) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Subscription::enable() - subscription %s: ")
        ACE_TEXT("already enabled, declining to process.\n"),
        this->name_.c_str()
      ));
    }
    return;
  }

  // Create the subscriber.
  ::DDS::Subscriber_var subscriber = participant->create_subscriber(
                                       this->profile_->subscriberQos,
                                       ::DDS::SubscriberListener::_nil()
                                     );
  if( CORBA::is_nil( subscriber.in())) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) Subscription::enable() - subscription %s: ")
      ACE_TEXT("failed to create subscriber.\n"),
        this->name_.c_str()
    ));
    throw BadSubscriberException();
  }

  // Configure the data gathering behaviors if there is a place to store
  // the data.  Otherwise leave it unconfigured.
  if( !this->profile_->datafile.empty()) {
    OpenDDS::DCPS::SubscriberImpl* servant
      = dynamic_cast< ::OpenDDS::DCPS::SubscriberImpl*>( subscriber.in());
    if( 0 == servant) {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: Subscription::enable() - subscription %s: ")
        ACE_TEXT("failed to narrow subscriber servant.\n"),
        this->name_.c_str()
      ));
      throw BadServantException();
    }

    // Configure the data gathering here.  Do this before creating and
    // enabling the reader to ensure that it is setup correctly as the
    // reader is associated with any publications.
    servant->raw_latency_buffer_size() = this->profile_->bound;
    servant->raw_latency_buffer_type() = this->profile_->retention;
    if( this->verbose_) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Subscription::enable() - subscription %s: ")
        ACE_TEXT("configured to capture %d latency measurements of type %d ")
        ACE_TEXT("per writer to file %s.\n"),
        this->name_.c_str(),
        this->profile_->bound,
        this->profile_->retention,
        this->profile_->datafile.c_str()
      ));
    }
  }

  // Create the transport
  OpenDDS::DCPS::TransportImpl_rch transport
    = TheTransportFactory->create_transport_impl(
        this->profile_->transport,
        ::OpenDDS::DCPS::AUTO_CONFIG
      );
  if( transport.is_nil()) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) Subscription::enable() - subscription %s: ")
      ACE_TEXT("failed to create transport with index %d.\n"),
      this->name_.c_str(),
      this->profile_->transport
    ));
    throw BadTransportException();

  } else if( this->verbose_) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) Subscription::enable() - subscription %s: ")
      ACE_TEXT("created transport with index %d.\n"),
      this->name_.c_str(),
      this->profile_->transport
    ));
  }

  // Attach the transport
  if( ::OpenDDS::DCPS::ATTACH_OK != transport->attach( subscriber)) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) Subscription::enable() - subscription %s: ")
      ACE_TEXT("failed to attach transport with index %d to subscriber.\n"),
      this->name_.c_str(),
      this->profile_->transport
    ));
    throw BadAttachException();

  } else if( this->verbose_) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) Subscription::enable() - subscription %s: ")
      ACE_TEXT("attached transport with index %d to subscriber.\n"),
      this->name_.c_str(),
      this->profile_->transport
    ));
  }

  // Derive the reader Qos values.
  ::DDS::TopicQos topicQos;
  topic->get_qos( topicQos);

  ::DDS::DataReaderQos readerQos;
  subscriber->get_default_datareader_qos( readerQos);

  subscriber->copy_from_topic_qos( readerQos, topicQos);

  this->profile_->copyToReaderQos( readerQos);

  // Create the reader.
  DDS::DataReader_var reader = subscriber->create_datareader(
                                 topic,
                                 readerQos,
                                 ::DDS::DataReaderListener::_nil()
                               );
  if( CORBA::is_nil( reader.in())) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) Subscription::enable() - subscription %s: ")
      ACE_TEXT("failed to create reader.\n"),
      this->name_.c_str()
    ));
    throw BadReaderException();

  } else if( this->verbose_) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) Subscription::enable() - subscription %s: ")
      ACE_TEXT("created reader.\n"),
      this->name_.c_str()
    ));
  }

  this->reader_ = ::OpenDDS::DCPS::DataReaderEx::_narrow( reader.in());
  if( CORBA::is_nil( this->reader_.in())) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: Subscription::enable() - subscription %s: ")
      ACE_TEXT("failed to narrow reader to extract statistics.\n"),
      this->name_.c_str()
    ));
    throw BadReaderException();
  }

  // Clear and start statistics gathering.  Ideally we would want to
  // configurably delay the start here to avoid edge effects.  Do this
  // before installing the listener to avoid missing the first samples in
  // the gathered data.  Do this only if we have configured to actually
  // gather data!
  if( !this->profile_->datafile.empty()) {
    this->reader_->reset_latency_stats();
    this->reader_->statistics_enabled( true);
  }

  // Create the listener.
  this->listener_ = new DataReaderListener(
                          !this->profile_->datafile.empty(),
                          this->profile_->bound,
                          this->profile_->retention,
                          this->verbose_
                        );
  this->safe_listener_ = this->listener_;
  if( this->verbose_) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) Subscription::enable() - subscription %s: ")
      ACE_TEXT("created reader listener.\n"),
      this->name_.c_str()
    ));
  }

  // Set the listener mask here so that we don't conflict with the
  // StatusCondition(s) that we want to wait on in the main thread.
  this->reader_->set_listener( this->listener_, DDS::DATA_AVAILABLE_STATUS);

  // We can finally indicate successful completion.
  this->enabled_ = true;
}

int
Subscription::total_messages() const
{
  return this->listener_->total_messages();
}

int
Subscription::valid_messages() const
{
  return this->listener_->valid_messages();
}

const std::map< long, long>&
Subscription::counts() const
{
  return this->listener_->counts();
}

const std::map< long, long>&
Subscription::bytes() const
{
  return this->listener_->bytes();
}

const std::map< long, long>&
Subscription::priorities() const
{
  return this->listener_->priorities();
}

std::ostream&
Subscription::summaryData( std::ostream& str) const
{
  ::OpenDDS::DCPS::LatencyStatisticsSeq statistics;
  this->reader_->get_latency_stats( statistics);
  str << " --- last hop statistical summary for subscription " << this->name_
      << " ---" << std::endl
      << statistics;

  str << " --- full path statistical summary for subscription " << this->name_
      << " ---" << std::endl;
  return this->listener_->summaryData( str);
}

std::ostream&
Subscription::rawData( std::ostream& str) const
{
  // Configure the raw data gathering and extract the raw latency data
  // container.
  OpenDDS::DCPS::DataReaderImpl* readerImpl
    = dynamic_cast< OpenDDS::DCPS::DataReaderImpl*>( this->reader_.in());
  if( readerImpl == 0) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: Subscription::enable() - subscription %s: ")
      ACE_TEXT("failed to derive reader implementation.\n"),
      this->name_.c_str()
    ));
    throw BadReaderException();
  }

  int index = 0;
  str << " --- last hop statistical data for subscription " << this->name_
      << " ---" << std::endl;
  for( OpenDDS::DCPS::DataReaderImpl::StatsMapType::const_iterator current
         = readerImpl->raw_latency_statistics().begin();
       current != readerImpl->raw_latency_statistics().end();
       ++current, ++index) {
    OpenDDS::DCPS::RepoIdConverter converter(current->first);
    str << std::endl << "  Writer[ " << converter << "]" << std::endl;
    current->second.raw_data( str);
  }

  str << " --- full path statistical data for subscription " << this->name_
      << " ---" << std::endl;
  return this->listener_->rawData( str);
}

} // End of namespace Test

std::ostream&
operator<<( std::ostream& str, const OpenDDS::DCPS::LatencyStatisticsSeq& statistics)
{
  for( unsigned long index = 0; index < statistics.length(); ++index) {
    OpenDDS::DCPS::RepoIdConverter converter(statistics[ index].publication);
    str << "  Writer[ " << converter << "]" << std::endl;
    str << "     samples: " << statistics[ index].n << std::endl;
    str << "        mean: " << statistics[ index].mean << std::endl;
    str << "     minimum: " << statistics[ index].minimum << std::endl;
    str << "     maximum: " << statistics[ index].maximum << std::endl;
    str << "    variance: " << statistics[ index].variance << std::endl;
  }
  return str;
}

