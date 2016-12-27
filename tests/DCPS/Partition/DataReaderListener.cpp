
#include "DataReaderListener.h"
#include "TestTypeSupportC.h"
#include "TestTypeSupportImpl.h"

#include <ace/streams.h>
#include "tests/Utils/ExceptionStreams.h"

using std::cerr;
using std::endl;



Test::DataReaderListener::DataReaderListener (long expected_matches)
  : expected_matches_ (expected_matches)
  , subscription_matches_ (0)
{
}

Test::DataReaderListener::~DataReaderListener ()
{
  long const matches = this->subscription_matches_.value ();
  if (matches != this->expected_matches_)
    {
      ACE_ERROR ((LM_ERROR,
                  ACE_TEXT ("(%P|%t) ERROR: Number of subscriptions (%d) ")
                  ACE_TEXT ("does not match expected (%d).\n"),
                  matches,
                  this->expected_matches_));

      exit (1);
    }
}

void
Test::DataReaderListener::on_data_available (DDS::DataReader_ptr reader)
{
  Test::DataDataReader_var dr = Test::DataDataReader::_narrow (reader);
  if (CORBA::is_nil (dr.in ())) {
    cerr << "read: _narrow failed." << endl;
    exit (1);
  }

  Test::Data the_data;
  DDS::SampleInfo si;
  (void) dr->take_next_sample (the_data, si);
}

void
Test::DataReaderListener::on_requested_deadline_missed (
    DDS::DataReader_ptr,
    const DDS::RequestedDeadlineMissedStatus &)
{
}

void
Test::DataReaderListener::on_requested_incompatible_qos (
    DDS::DataReader_ptr reader,
    const DDS::RequestedIncompatibleQosStatus & status)
{
  // This test only modifies the PARTITION QoS policy.
  // By design, PARTITION incompatibilities should not be reported.
  ACE_ERROR ((LM_ERROR,
              ACE_TEXT ("(%P|%t) Incompatible requested QoS (ID %d) ")
              ACE_TEXT ("unexpected reported.\n"),
              status.last_policy_id));

  // Display requested partition.
  this->display_partitions (reader);
}

void
Test::DataReaderListener::on_liveliness_changed (
    DDS::DataReader_ptr,
    const DDS::LivelinessChangedStatus &)
{
}

void
Test::DataReaderListener::on_subscription_matched (
    DDS::DataReader_ptr reader,
    const DDS::SubscriptionMatchedStatus& status)
{
  if( status.total_count_change > 0) {
    this->subscription_matches_ += status.total_count_change;
  }

  ACE_DEBUG((LM_DEBUG,
             ACE_TEXT("(%P|%t) ")
             ACE_TEXT("DataReaderListener::on_subscription_matched\n")));

  this->display_partitions (reader);
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

void
Test::DataReaderListener::display_partitions (
  DDS::DataReader_ptr reader) const
{
  // Display requested partition.
  DDS::Subscriber_var subscriber (reader->get_subscriber ());
  DDS::SubscriberQos sub_qos;
  subscriber->get_qos (sub_qos);

  DDS::PartitionQosPolicy const & partition = sub_qos.partition;

  ACE_DEBUG ((LM_DEBUG,
              ACE_TEXT ("(%P|%t) Requested Partition\n")
              ACE_TEXT ("Partition\tName\n")
              ACE_TEXT ("=========\t====\n")));

  CORBA::ULong const len = partition.name.length ();

  if (len == 0)
    ACE_DEBUG ((LM_DEBUG,
                ACE_TEXT ("** Zero length partition name ")
                ACE_TEXT ("sequence (default) **\n")));

  for (CORBA::ULong i = 0; i != len; ++i)
    {
      char const * s = partition.name[i];

      ACE_DEBUG ((LM_DEBUG,
                  ACE_TEXT (" %u\t\t%C\n"),
                  i,
                  *s == 0 ? "\"\"" : s));
    }
}
