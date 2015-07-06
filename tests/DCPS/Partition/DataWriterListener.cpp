
#include "DataWriterListener.h"


Test::DataWriterListener::DataWriterListener (long expected_matches)
  : expected_matches_ (expected_matches)
  , publication_matches_ (0)
{
}

Test::DataWriterListener::~DataWriterListener ()
{
  long const matches = this->publication_matches_.value ();
  if (matches != this->expected_matches_)
    {
      ACE_ERROR ((LM_ERROR,
                  ACE_TEXT ("(%P|%t) ERROR: Number of publications (%d) ")
                  ACE_TEXT ("does not match expected (%d).\n"),
                  matches,
                  this->expected_matches_));
    }
}

void
Test::DataWriterListener::on_offered_deadline_missed (
    ::DDS::DataWriter_ptr /* writer */,
    ::DDS::OfferedDeadlineMissedStatus const & /* status */)
{
}

void
Test::DataWriterListener::on_offered_incompatible_qos (
    ::DDS::DataWriter_ptr writer,
    ::DDS::OfferedIncompatibleQosStatus const & status)
{
  // This test only modifies the PARTITION QoS policy.
  // By design, PARTITION incompatibilities should not be reported.
  ACE_ERROR ((LM_ERROR,
              ACE_TEXT ("(%P|%t) Incompatible offered QoS (ID %d) ")
              ACE_TEXT ("unexpected reported.\n"),
              status.last_policy_id));

  // Display offered partition.
  this->display_partitions (writer);
}

void
Test::DataWriterListener::on_liveliness_lost (
    ::DDS::DataWriter_ptr /* writer */,
    const ::DDS::LivelinessLostStatus & /* status */)
{
}

void
Test::DataWriterListener::on_publication_matched (
    ::DDS::DataWriter_ptr writer,
    ::DDS::PublicationMatchedStatus const& status)
{
  if( status.total_count_change > 0) {
    this->publication_matches_ += status.total_count_change;
  }

  ACE_DEBUG((LM_DEBUG,
             ACE_TEXT("(%P|%t) ")
             ACE_TEXT("DataWriterListener::on_publication_matched\n")));

  this->display_partitions (writer);
}

void
Test::DataWriterListener::on_publication_disconnected (
    ::DDS::DataWriter_ptr /* writer */,
    ::OpenDDS::DCPS::PublicationDisconnectedStatus const & /* status */)
{
}

void
Test::DataWriterListener::on_publication_reconnected (
    ::DDS::DataWriter_ptr /* writer */,
    ::OpenDDS::DCPS::PublicationReconnectedStatus const & /* status */)
{
}

void
Test::DataWriterListener::on_publication_lost (
    ::DDS::DataWriter_ptr /* writer */,
    ::OpenDDS::DCPS::PublicationLostStatus const & /* status */)
{
}

void
Test::DataWriterListener::on_connection_deleted (
    ::DDS::DataWriter_ptr /* writer */)
{
}

void
Test::DataWriterListener::display_partitions (
  DDS::DataWriter_ptr writer) const
{
  // Display offered partition.
  DDS::Publisher_var publisher (writer->get_publisher ());
  DDS::PublisherQos pub_qos;
  publisher->get_qos (pub_qos);

  DDS::PartitionQosPolicy const & partition = pub_qos.partition;

  ACE_DEBUG ((LM_DEBUG,
              ACE_TEXT ("(%P|%t) Offered Partition\n")
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
