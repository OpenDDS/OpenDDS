
#include "DataReaderListener.h"
#include "Publication.h"
#include "TestTypeSupportC.h"
#include "TestTypeSupportImpl.h"

#include "dds/DCPS/Qos_Helper.h"

#include "ace/High_Res_Timer.h"

#include <iostream>
#include <iomanip>
#include <sstream>

/// Control the spew.
namespace { enum { BE_REALLY_VERBOSE = 0};}

Test::DataReaderListener::WriterStats::WriterStats(
  int bound,
  OpenDDS::DCPS::DataCollector< double>::OnFull type
) : stats_( bound, type)
{
}

void
Test::DataReaderListener::WriterStats::add_stat( DDS::Duration_t delay)
{
  double datum = static_cast<double>( delay.sec);
  datum += 1.0e-9 * static_cast<double>( delay.nanosec);
  this->stats_.add( datum);
}

std::ostream&
Test::DataReaderListener::WriterStats::summaryData( std::ostream& str) const
{
  str << std::dec;
  str << "     samples: " << this->stats_.n() << std::endl;
  str << "        mean: " << this->stats_.mean() << std::endl;
  str << "     minimum: " << this->stats_.minimum() << std::endl;
  str << "     maximum: " << this->stats_.maximum() << std::endl;
  str << "    variance: " << this->stats_.var() << std::endl;
  return str;
}

std::ostream&
Test::DataReaderListener::WriterStats::rawData( std::ostream& str) const
{
  str << std::dec << this->stats_.size()
      << " samples out of " << this->stats_.n() << std::endl;
  return str << this->stats_;
}

Test::DataReaderListener::DataReaderListener(
  bool collectData,
  int rawDataBound,
  OpenDDS::DCPS::DataCollector< double>::OnFull rawDataType,
  bool verbose
) : collectData_( collectData),
    verbose_( verbose),
    totalMessages_( 0),
    validMessages_( 0),
    rawDataBound_( rawDataBound),
    rawDataType_( rawDataType),
    destination_( 0)
{
}

Test::DataReaderListener::~DataReaderListener()
{
}

void
Test::DataReaderListener::set_destination( Publication* publication)
{
  this->destination_ = publication;
}

int
Test::DataReaderListener::total_messages() const
{
  return this->totalMessages_;
}

int
Test::DataReaderListener::valid_messages() const
{
  return this->validMessages_;
}

const std::map< long, long>&
Test::DataReaderListener::counts() const
{
  return this->counts_;
}

const std::map< long, long>&
Test::DataReaderListener::bytes() const
{
  return this->bytes_;
}

const std::map< long, long>&
Test::DataReaderListener::priorities() const
{
  return this->priorities_;
}

std::ostream&
Test::DataReaderListener::summaryData( std::ostream& str) const
{
  char fill = str.fill( '0');
  for( StatsMap::const_iterator current = this->stats_.begin();
       current != this->stats_.end();
       ++current
     ) {
    str << "  Writer[ 0x" << std::hex << std::setw(8) << current->first << "]" << std::endl;
    current->second.summaryData( str);
  }
  str.fill( fill);
  return str;
}

std::ostream&
Test::DataReaderListener::rawData( std::ostream& str) const
{
  char fill = str.fill( '0');
  for( StatsMap::const_iterator current = this->stats_.begin();
       current != this->stats_.end();
       ++current
     ) {
    str << "  Writer[ 0x" << std::hex << std::setw(8) << current->first << "]" << std::endl;
    current->second.rawData( str);
  }
  str.fill( fill);
  return str;
}

void
Test::DataReaderListener::on_data_available (DDS::DataReader_ptr reader)
{
  DataDataReader_var dr = DataDataReader::_narrow (reader);
  if (CORBA::is_nil (dr.in ())) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT ("(%P|%t) DataReaderListener::on_data_available() - ")
      ACE_TEXT ("data on unexpected reader type.\n")
    ));
    return;
  }

  // Mark the reception time of all samples read during this call as now.
  // This is the closest we can come to determining what the actual
  // reception timestamp is without hacking the service internals.
  // This actually does reflect delays experienced by applications more
  // accurately since this is where an application first observes any
  // received data.
  ACE_Time_Value now = ACE_High_Res_Timer::gettimeofday_hr();

  Data            data;
  DDS::SampleInfo info;

  while( DDS::RETCODE_OK == dr->take_next_sample( data, info)) {
    ++this->totalMessages_;
    if( info.valid_data) {
      ++this->counts_[ data.pid];
      ++this->validMessages_;
      this->bytes_[ data.pid] += data.buffer.length();
      this->priorities_[ data.pid] = data.priority; // faster than conditional.

      // Forward the current sample if there is a destination for it.
      if( this->destination_) {
        this->destination_->write( data);
      }

      // Collect the (presumably) round trip statistics at this point.
      if( this->collectData_) {
        // This does not affect any existing value and inserts only if
        // no value for this writer is currently present.
        std::pair< StatsMap::iterator, bool> result = this->stats_.insert(
          StatsMap::value_type(
            data.pid,
            WriterStats( this->rawDataBound_, this->rawDataType_)
          )
        );

        // This is a very annoying series of conversions.
        DDS::Duration_t sent_time = { data.sec, data.nanosec };
        result.first->second.add_stat(
          OpenDDS::DCPS::time_value_to_duration(
            now - OpenDDS::DCPS::duration_to_time_value( sent_time)
          )
        );
      }

      if( this->verbose_ && BE_REALLY_VERBOSE) {
        ACE_DEBUG((LM_DEBUG,
          ACE_TEXT("(%P|%t) DataReaderListener::on_data_available() - ")
          ACE_TEXT("received priority %d sample %d, length %d\n"),
          data.priority,
          data.seq,
          data.buffer.length()
        ));
      }
    } else {
      std::stringstream buffer;
      buffer.fill( '0');
      for( std::map< long, long>::const_iterator current = this->counts_.begin();
           current != this->counts_.end();
           ++current
         ) {
        buffer << "  Writer[ 0x" << std::hex << std::setw(8);
        buffer << current->first << "]: ";
        buffer << std::dec << current->second;
        buffer << " samples, " << this->bytes_[ current->first];
        buffer << " bytes." << std::endl;
      }
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) DataReaderListener::on_data_available() - ")
        ACE_TEXT("received a non-data sample.  After messages: ")
        ACE_TEXT("total: %d, valid: %d.\n%C"),
        this->totalMessages_,
        this->validMessages_,
        buffer.str().c_str()
      ));
    }
  }

  if( this->verbose_ && BE_REALLY_VERBOSE) {
    ::OpenDDS::DCPS::DataReaderEx_var readerex
      = ::OpenDDS::DCPS::DataReaderEx::_narrow( reader);
    if( !CORBA::is_nil( readerex.in())) {
      ::OpenDDS::DCPS::LatencyStatisticsSeq statistics;
      readerex->get_latency_stats( statistics);
      std::stringstream buffer;
      for( unsigned long index = 0; index < statistics.length(); ++index) {
        buffer << "Writer[ " << statistics[ index].publication << "] - ";
        buffer << "samples==" << std::dec << statistics[ index].n;
        buffer << ", mean==" << statistics[ index].mean;
        buffer << ", minimum==" << statistics[ index].minimum;
        buffer << ", maximum==" << statistics[ index].maximum;
        buffer << ", variance==" << statistics[ index].variance;
        buffer << std::endl;
      }
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) DataReaderListener::on_data_available() - ")
        ACE_TEXT("statistics for %d writers at sample %d:\n%C"),
        statistics.length(),
        this->totalMessages_,
        buffer.str().c_str()
      ));

    } else {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: DataReaderListener::on_data_available() - ")
        ACE_TEXT("failed to narrow extended reader to gather statistics.\n")
      ));
    }
  }
}

void
Test::DataReaderListener::on_requested_deadline_missed (
    DDS::DataReader_ptr,
    const DDS::RequestedDeadlineMissedStatus &)
{
}

void
Test::DataReaderListener::on_requested_incompatible_qos (
    DDS::DataReader_ptr,
    const DDS::RequestedIncompatibleQosStatus &)
{
}

void
Test::DataReaderListener::on_liveliness_changed (
    DDS::DataReader_ptr,
    const DDS::LivelinessChangedStatus &)
{
}

void
Test::DataReaderListener::on_subscription_matched (
    DDS::DataReader_ptr,
    const DDS::SubscriptionMatchedStatus&)
{
}

void
Test::DataReaderListener::on_sample_rejected (
    DDS::DataReader_ptr,
    DDS::SampleRejectedStatus const &)
{
}

void
Test::DataReaderListener::on_sample_lost (DDS::DataReader_ptr,
                                          DDS::SampleLostStatus const &)
{
}

void
Test::DataReaderListener::on_subscription_disconnected (
    DDS::DataReader_ptr,
    ::OpenDDS::DCPS::SubscriptionDisconnectedStatus const &)
{
}

void
Test::DataReaderListener::on_subscription_reconnected (
    DDS::DataReader_ptr,
    ::OpenDDS::DCPS::SubscriptionReconnectedStatus const &)
{
}

void
Test::DataReaderListener::on_subscription_lost (
    DDS::DataReader_ptr,
    ::OpenDDS::DCPS::SubscriptionLostStatus const &)
{
}

void
Test::DataReaderListener::on_budget_exceeded (
    DDS::DataReader_ptr,
    const ::OpenDDS::DCPS::BudgetExceededStatus&)
{
}

void
Test::DataReaderListener::on_connection_deleted (DDS::DataReader_ptr)
{
}

