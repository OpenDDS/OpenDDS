
#include "DataReaderListener.h"
#include "TestTypeSupportC.h"
#include "TestTypeSupportImpl.h"

#include <sstream>

/// Control the spew.
namespace { enum { BE_REALLY_VERBOSE = 0};}

Test::DataReaderListener::DataReaderListener( const bool verbose)
 : verbose_( verbose),
   total_messages_( 0),
   valid_messages_( 0)
{
}

Test::DataReaderListener::~DataReaderListener ()
{
}

int
Test::DataReaderListener::total_messages() const
{
  return this->total_messages_;
}

int
Test::DataReaderListener::valid_messages() const
{
  return this->valid_messages_;
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

void
Test::DataReaderListener::on_data_available (DDS::DataReader_ptr reader)
{
  Test::DataDataReader_var dr = Test::DataDataReader::_narrow (reader);
  if (CORBA::is_nil (dr.in ())) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT ("(%P|%t) Test::DataReaderListener::on_data_available() - ")
      ACE_TEXT ("data on unexpected reader type.\n")
    ));
    return;
  }

#ifdef ZERO_COPY_TAKE
  enum { SAMPLES_PER_TAKE = 10};
  Test::DataSeq      data( 0);
  DDS::SampleInfoSeq info( 0);

  while( DDS::RETCODE_OK == dr->take(
                              data,
                              info,
                              SAMPLES_PER_TAKE,
                              DDS::ANY_SAMPLE_STATE,
                              DDS::ANY_VIEW_STATE,
                              DDS::ANY_INSTANCE_STATE
                            )
       ) {
    this->total_messages_ += data.length();
    for( unsigned long index = 0; index < data.length(); ++index) {
      if( info[ index].valid_data) {
        ++this->valid_messages_;
      } else {
        ACE_ERROR((LM_ERROR,
          ACE_TEXT("(%P|%t) ERROR: DataReaderListener::on_data_available() - ")
          ACE_TEXT("received an INVALID sample.\n")
        ));
      }
      this->bytes_[ data[ index].pid] += data[ index].buffer.length();
      this->priorities_[ data[ index].pid] = data[ index].priority; // Faster than a conditional.
      if( this->verbose_ && BE_REALLY_VERBOSE) {
        ACE_DEBUG((LM_DEBUG,
          ACE_TEXT("(%P|%t) DataReaderListener::on_data_available() - ")
          ACE_TEXT("received %d samples.\n"),
          data.length()
        ));
      }
    }
    // Do this here since we are not going out of scope before we might
    // call the take() again.  This resets the length to 0 for us as well
    // as managing memory.
    dr->return_loan( data, info);
  }

#else /* ZERO_COPY_TAKE */
  Test::Data      data;
  DDS::SampleInfo info;

  while( DDS::RETCODE_OK == dr->take_next_sample( data, info)) {
    ++this->total_messages_;
    if( info.valid_data) {
      ++this->counts_[ data.pid];
      ++this->valid_messages_;
      this->bytes_[ data.pid] += data.buffer.length();
      this->priorities_[ data.pid] = data.priority; // faster than conditional.
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
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: DataReaderListener::on_data_available() - ")
        ACE_TEXT("received an INVALID sample.\n")
      ));
    }
  }
#endif /* ZERO_COPY_TAKE */

  if( this->verbose_ && BE_REALLY_VERBOSE) {
    ::OpenDDS::DCPS::DataReaderEx_var readerex
      = ::OpenDDS::DCPS::DataReaderEx::_narrow( reader);
    if( !CORBA::is_nil( readerex.in())) {
      ::OpenDDS::DCPS::LatencyStatisticsSeq statistics;
      readerex->get_latency_stats( statistics);
      std::stringstream buffer;
      for( unsigned long index = 0; index < statistics.length(); ++index) {
        buffer << "Writer[ " << to_string(statistics[ index].publication) << "] - ";
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
        this->total_messages_,
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
  if( this->verbose_) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) DataReaderListener::on_liveliness_changed()")
    ));
  }
}

void
Test::DataReaderListener::on_subscription_matched (
    DDS::DataReader_ptr,
    const DDS::SubscriptionMatchedStatus &)
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

