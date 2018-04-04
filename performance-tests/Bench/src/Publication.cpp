// -*- C++ -*-
//

// This include needs to appear near the top so ACE gets a chance
// to figure out the correct time_t typedef.
#include "ace/OS_NS_time.h"

#include "Test.h"
#include "Publication.h"
#include "EntityProfiles.h"

#include "dds/DCPS/DataWriterImpl.h"
#include "dds/DCPS/GuidConverter.h"
#include "dds/DCPS/Qos_Helper.h"
#include "dds/DCPS/transport/framework/TransportRegistry.h"
#include "dds/DCPS/transport/multicast/MulticastInst.h"

#include "ace/High_Res_Timer.h"
#include "ace/OS_NS_unistd.h"

#include <sstream>

/// Control the spew.
namespace { enum { BE_REALLY_VERBOSE = 0};}

namespace Test {

Publication::Publication(
  const char* name,
  PublicationProfile* profile,
  bool verbose
) : name_( name),
    profile_( profile),
    verbose_( verbose),
    done_( false),
    enabled_( false),
    messages_( 0),
    timeouts_( 0),
    duration_(0.0),
    publisher_(::DDS::Publisher::_nil())
{
}

Publication::~Publication()
{
  // Terminate cleanly.
  this->stop();
  this->wait();

  if( !CORBA::is_nil( publisher_.in())) {
      publisher_->delete_datawriter(writer_.in());
  }
}

int
Publication::open( void*)
{
  int result = activate (THR_NEW_LWP | THR_JOINABLE, 1);
  if( result != 0) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: Writer::open() - ")
      ACE_TEXT("failed to activate the %C publication thread.\n"),
      this->name_.c_str()
    ));
  }
  return result;
}

int
Publication::close( u_long /* flags */)
{
  return 0;
}

void
Publication::start()
{
  if( this->profile_->source.empty()) {
    // Run as a separate thread if we are generating data.
    this->open( 0);
  }
}

void
Publication::stop()
{
  // This is the single location where this member is written.
  this->done_ = true;
}

int
Publication::messages() const
{
  return this->messages_;
}

int
Publication::timeouts() const
{
  return this->timeouts_;
}

double
Publication::duration() const
{
  return this->duration_;
}

int
Publication::associations() const
{
  DDS::PublicationMatchedStatus publicationMatches = { 0, 0, 0, 0, 0};
  if (this->writer_->get_publication_matched_status(publicationMatches) != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: ")
               ACE_TEXT("Test::Publication::associations, ")
               ACE_TEXT("Could not get publication matched status.\n")));
  }
  return publicationMatches.current_count;
}

int
Publication::missing_associations() const
{
  return this->profile_->associations - this->associations();
}

bool
Publication::ready() const
{
  return this->missing_associations() == 0;
}

::DDS::StatusCondition_ptr
Publication::get_statuscondition()
{
  if( !this->enabled_) {
    if( this->verbose_) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Publication::get_statuscondition() - publication %C: ")
        ACE_TEXT("not enabled, declining to process.\n"),
        this->name_.c_str()
      ));
    }
    return ::DDS::StatusCondition::_nil();
  }

  ACE_GUARD_RETURN(ACE_SYNCH_MUTEX, guard, this->lock_, ::DDS::StatusCondition::_nil());

  // We do not need to hold the condition so we do not duplicate it.
  return this->writer_->get_statuscondition();
}

::DDS::DataWriterListener_ptr
Publication::get_listener()
{
  if( !this->enabled_) {
    if( this->verbose_) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Publication::get_listener() - publication %C: ")
        ACE_TEXT("not enabled, declining to process.\n"),
        this->name_.c_str()
      ));
    }
    return ::DDS::DataWriterListener::_nil();
  }

  ACE_GUARD_RETURN(ACE_SYNCH_MUTEX, guard, this->lock_, ::DDS::DataWriterListener::_nil());

  // We do not need to hold the listener so we do not duplicate it.
  return this->writer_->get_listener();
}

::DDS::ReturnCode_t
Publication::set_listener(
  ::DDS::DataWriterListener_ptr a_listener,
  ::DDS::StatusMask mask
)
{
  if( !this->enabled_) {
    if( this->verbose_) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Publication::set_listener() - publication %C: ")
        ACE_TEXT("not enabled, declining to process.\n"),
        this->name_.c_str()
      ));
    }
    return ::DDS::RETCODE_NOT_ENABLED;
  }

  ACE_GUARD_RETURN(ACE_SYNCH_MUTEX, guard, this->lock_, ::DDS::RETCODE_NOT_ENABLED);

  return this->writer_->set_listener( a_listener, mask);
}

void
Publication::write( const Test::Data& sample)
{
  // Only forward if we have not been stopped.
  if( !this->done_) {
    DDS::ReturnCode_t result;
    {
      ACE_GUARD(ACE_SYNCH_MUTEX, guard, this->lock_);
      result = this->writer_->write( sample, DDS::HANDLE_NIL);
    }
    switch( result) {
      case DDS::RETCODE_OK:
        ++this->messages_;
        if( this->verbose_ && BE_REALLY_VERBOSE) {
          ACE_DEBUG((LM_DEBUG,
            ACE_TEXT("(%P|%t) Publication::write() - publication %C: ")
            ACE_TEXT("forwarded sample %d at priority %d.\n"),
            this->name_.c_str(),
            this->messages_,
            this->profile_->writerQos.transport_priority.value
          ));
        }
        break;

      case DDS::RETCODE_TIMEOUT:
        ++this->timeouts_;
        break;

      default:
        ACE_ERROR((LM_ERROR,
          ACE_TEXT("(%P|%t) Publication::write() - publication %C: ")
          ACE_TEXT("failed to forward sample: pid: %d, seq: %d, priority: %d.\n"),
            this->name_.c_str(),
            sample.pid,
            sample.seq,
            sample.priority
        ));
        break;
    }
  }
}

void
Publication::enable(
  ::DDS::DomainParticipant_ptr participant,
  ::DDS::Topic_ptr             topic
)
{
  if( this->enabled_) {
    if( this->verbose_) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Publication::enable() - publication %C: ")
        ACE_TEXT("already enabled, declining to process.\n"),
        this->name_.c_str()
      ));
    }
    return;
  }

  // Create the publisher.
  publisher_ = participant->create_publisher(
                                     this->profile_->publisherQos,
                                     ::DDS::PublisherListener::_nil(),
                                     ::OpenDDS::DCPS::DEFAULT_STATUS_MASK
                                   );
  if( CORBA::is_nil( publisher_.in())) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) Publication::enable() - publication %C: ")
      ACE_TEXT("failed to create publisher.\n"),
        this->name_.c_str()
    ));
    throw BadPublisherException();
  }

  OpenDDS::DCPS::TransportConfig_rch transport =
    TheTransportRegistry->get_config(this->profile_->transportConfig);
  if (transport.is_nil() || transport->instances_.empty()) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) Publication::enable() - publication %C: ")
      ACE_TEXT("failed to get_config() OR got empty config with name %C.\n"),
      this->name_.c_str(),
      this->profile_->transportConfig.c_str()
    ));
    throw BadTransportException();
  }
  if (this->verbose_) {
    OpenDDS::DCPS::MulticastInst* mcconfig
      = dynamic_cast<OpenDDS::DCPS::MulticastInst*>(
          transport->instances_[0].in()
        );
    bool isMcast    = false;
    bool isReliable = false;
    if (mcconfig != 0) {
      isMcast = true;
      isReliable = mcconfig->reliable_;
    }

    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) Publication::enable() - publication %C: ")
      ACE_TEXT("%C %C transport with config %C.\n"),
      this->name_.c_str(),
      (!isMcast? "obtained":
                 (isReliable? "obtained reliable":
                              "obtained best effort"
      )),
      transport->instances_[0]->transport_type_.c_str(),
      this->profile_->transportConfig.c_str()
    ));
  }

  // Attach the transport
  TheTransportRegistry->bind_config(transport, publisher_);

  if (this->verbose_) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) Publication::enable() - publication %C: ")
      ACE_TEXT("attached transport with config %C to publisher.\n"),
      this->name_.c_str(),
      this->profile_->transportConfig.c_str()
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
      ACE_TEXT("(%P|%t) Publication::enable() - publication %C: ")
      ACE_TEXT("failed to create writer.\n"),
      this->name_.c_str()
    ));
    throw BadWriterException();

  } else if( this->verbose_) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) Publication::enable() - publication %C: ")
      ACE_TEXT("created writer.\n"),
      this->name_.c_str()
    ));
  }

  this->writer_ = Test::DataDataWriter::_narrow( writer.in());
  if( CORBA::is_nil( this->writer_.in())) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) Publication::enable() - publication %C: ")
      ACE_TEXT("failed to narrow writer for Test::Data type.\n"),
      this->name_.c_str()
    ));
    throw BadWriterException();

  } else if( this->verbose_) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) Publication::enable() - publication %C: ")
      ACE_TEXT("narrowed writer for Test::Data type.\n"),
      this->name_.c_str()
    ));
  }

  // We can finally indicate successful completion.
  this->enabled_ = true;
}

int
Publication::svc ()
{
  if( !this->enabled_) {
    if( this->verbose_) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Publication::svc() - publication %C: ")
        ACE_TEXT("not enabled, declining to process.\n"),
        this->name_.c_str()
      ));
    }
    return 0;

  } else if( this->verbose_) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) Publication::svc() - publication %C: ")
      ACE_TEXT("processing starts on thread.\n"),
      this->name_.c_str()
    ));
  }

  // Honor a request to delay the start of publication.
  if( this->profile_->delay > 0) {
    ACE_Time_Value initialDelay( this->profile_->delay, 0);
    ACE_OS::sleep( initialDelay);
    if( this->verbose_) {
    ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Publication::svc() - publication %C: ")
        ACE_TEXT("publication starting after initial delay.\n"),
        this->name_.c_str()
      ));
    }
  }

  OpenDDS::DCPS::DataWriterImpl* servant
    = dynamic_cast< OpenDDS::DCPS::DataWriterImpl*>( this->writer_.in());

  OpenDDS::DCPS::GuidConverter converter(servant->get_publication_id());
  int pid = converter.checksum();

  ACE_Time_Value startTime = ACE_High_Res_Timer::gettimeofday_hr();
  int            count = 0;
  Test::Data     sample;
  sample.priority = this->profile_->writerQos.transport_priority.value;
  sample.pid      = pid;

  // Main loop for publishing.
  while( this->done_ == false) {
    sample.buffer.length( *this->profile_->size);
    sample.key = *this->profile_->instances;
    sample.seq = ++count;

    ACE_Time_Value  start = ACE_High_Res_Timer::gettimeofday_hr();
    DDS::Duration_t stamp = ::OpenDDS::DCPS::time_value_to_duration( start);
    sample.sec     = stamp.sec;
    sample.nanosec = stamp.nanosec;

    DDS::ReturnCode_t result;
    {
      ACE_GUARD_RETURN(ACE_SYNCH_MUTEX, guard, this->lock_, 0);
      result = this->writer_->write( sample, DDS::HANDLE_NIL);
    }
    switch( result) {
      case DDS::RETCODE_OK:      ++this->messages_; break;
      case DDS::RETCODE_TIMEOUT: ++this->timeouts_; break;

      default:
        ACE_ERROR((LM_ERROR,
          ACE_TEXT("(%P|%t) Publication::svc() - publication %C: ")
          ACE_TEXT("write failed with code: %d.\n"),
          this->name_.c_str(),
          result
        ));
        break;
    }

    // Determine the interval to next message here so it can be mentioned
    // in the diagnostic messsage.
    long microseconds = static_cast<long>( 1.0e6 * *this->profile_->rate);
    ACE_Time_Value interval( 0, microseconds);

    if( this->verbose_ && BE_REALLY_VERBOSE) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Publication::svc() - publication %C: ")
        ACE_TEXT("wrote sample %d at priority %d, ")
        ACE_TEXT("waiting %d microseconds to send next one.\n"),
        this->name_.c_str(),
        count,
        this->profile_->writerQos.transport_priority.value,
        interval.usec()
      ));
    }

    // Wait the remainder of the interval before sending the next message.
    ACE_Time_Value now = ACE_High_Res_Timer::gettimeofday_hr();
    interval -= (now - start);
    if( interval > ACE_Time_Value::zero) {
      ACE_OS::sleep( interval);
    }
  }

  // Wait for data to be delivered if we have been asked to.
  if( this->profile_->ackDelay) {
    DDS::Duration_t timeout = {CORBA::Long(this->profile_->ackDelay), 0};
    DDS::ReturnCode_t result;
    result = this->writer_->wait_for_acknowledgments( timeout);
    switch( result) {
      case DDS::RETCODE_OK: break;

      case DDS::RETCODE_TIMEOUT:
        ACE_ERROR((LM_ERROR,
          ACE_TEXT("(%P|%t) Publication::svc() - publication %C: ")
          ACE_TEXT("failed waiting %d seconds for acknowledgments.\n"),
          this->name_.c_str(),
          this->profile_->ackDelay
        ));
        break;

      default:
        ACE_ERROR((LM_ERROR,
          ACE_TEXT("(%P|%t) Publication::svc() - publication %C: ")
          ACE_TEXT("write failed with code: %d.\n"),
          this->name_.c_str(),
          result
        ));
        break;
    }
  }

  ACE_Time_Value elapsedTime
    = ACE_High_Res_Timer::gettimeofday_hr() - startTime;
  this->duration_ = elapsedTime.sec()
                  + (elapsedTime.usec() * 1.0e-6);

  if( this->verbose_) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) Publication::svc() - publication %C: ")
      ACE_TEXT("honoring termination request, stopping thread.\n"),
      this->name_.c_str()
    ));
  }
  return 0;
}

} // End of namespace Test

