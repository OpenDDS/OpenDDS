
#include "DataReaderListener.h"
#include "TestTypeSupportC.h"
#include "TestTypeSupportImpl.h"
#include "ace/OS_NS_unistd.h"

Test::DataReaderListener::DataReaderListener( const bool verbose)
 : verbose_( verbose),
   count_( 0),
   received_samples_invalid_( false),
   priority_sample_( NOT_RECEIVED)
{
}

Test::DataReaderListener::~DataReaderListener ()
{
}

unsigned int
Test::DataReaderListener::count() const
{
  return this->count_;
}

bool
Test::DataReaderListener::passed() const
{
  return !received_samples_invalid_ && priority_sample_ == RECEIVED_VALID;
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

  Test::Data      data;
  DDS::SampleInfo info;
  int             count = 0;

  while( DDS::RETCODE_OK == dr->take_next_sample( data, info)) {
    if( info.valid_data) {
      ++this->count_;
      if (count_ % 50 == 0)
        ACE_OS::sleep(2);

      if (count_ % 1000 == 0)
        ACE_DEBUG((LM_DEBUG,
          ACE_TEXT("(%P|%t) DataReaderListener::on_data_available() - ")
          ACE_TEXT("received %d samples\n"),
          count_));

      if( this->verbose_) {
        ACE_DEBUG((LM_DEBUG,
          ACE_TEXT("(%P|%t) DataReaderListener::on_data_available() - ")
          ACE_TEXT("received valid sample(%d): %03d: %d, %C priority\n"),
          count,
          data.key,
          data.value,
          (data.priority == true ? "high" : "low")
        ));
      }

      if (data.priority) {
        if (priority_sample_ != NOT_RECEIVED) {
          priority_sample_ = RECEIVED_INVALID;
          ACE_ERROR((LM_ERROR,
                     ACE_TEXT("(%P|%t) ERROR: DataReaderListener::on_data_available() - ")
                     ACE_TEXT("Received multiple high priority samples.\n")));
        }
        else if (received_samples_.count(data.before_value)) {
          priority_sample_ = RECEIVED_INVALID;
          // (there is at least one entry so rbegin is valid)
          const long highest = *received_samples_.rbegin();
          ACE_ERROR((LM_ERROR,
                     ACE_TEXT("(%P|%t) ERROR: DataReaderListener::on_data_available() - ")
                     ACE_TEXT("Did not receive high priority sample before low priority ")
                     ACE_TEXT("sample %d (highest received sample=%d.\n"),
                     data.before_value,
                     highest));
        }
        else
          priority_sample_ = RECEIVED_VALID;
      }
      else if (!received_samples_.insert(data.value).second) {
        received_samples_invalid_ = true;
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("(%P|%t) ERROR: DataReaderListener::on_data_available() - ")
                   ACE_TEXT("Received duplicate sample %d.\n"),
                   data.value));
      }
      else if (data.value > 1 && !received_samples_.count(data.value - 1)) {
        received_samples_invalid_ = true;
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("(%P|%t) ERROR: DataReaderListener::on_data_available() - ")
                   ACE_TEXT("Received the sample %d before the previous sample.\n"),
                   data.value));
      }
      else if (received_samples_.count(data.value + 1)) {
        received_samples_invalid_ = true;
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("(%P|%t) ERROR: DataReaderListener::on_data_available() - ")
                   ACE_TEXT("Received the sample %d after the next sample.\n"),
                   data.value));
      }
    }
    else if (info.instance_state == DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE)
    {
      ACE_DEBUG((LM_DEBUG,
          ACE_TEXT("(%P|%t) DataReaderListener::on_data_available() - ")
          ACE_TEXT("received dispose\n")));
    }
    else if (info.instance_state == DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE)
    {
      ACE_DEBUG((LM_DEBUG,
          ACE_TEXT("(%P|%t) DataReaderListener::on_data_available() - ")
          ACE_TEXT("received unregister\n")));
    }
    else
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: DataReaderListener::on_data_available() - ")
                 ACE_TEXT("received an INVALID sample.\n")));
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

