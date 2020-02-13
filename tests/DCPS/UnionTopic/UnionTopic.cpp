#include <dds/DdsDcpsInfrastructureC.h>
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/WaitSet.h>
#include <dds/DCPS/SafetyProfileStreams.h>
#ifdef ACE_AS_STATIC_LIBS
#  include "dds/DCPS/RTPS/RtpsDiscovery.h"
#  include "dds/DCPS/transport/rtps_udp/RtpsUdp.h"
#endif

using OpenDDS::DCPS::retcode_to_string;

#include "UnionTopicTypeSupportImpl.h"

using namespace UnionTopic;

#include <map>
#include <string>

const DDS::Duration_t max_wait_time = {10, 0};
const int domain = 142;

class Test {
public:
  DDS::DomainParticipant_var& participant_;
  DDS::Publisher_var& publisher_;
  DDS::Subscriber_var& subscriber_;
  DDS::DataReader_var& reader_;
  DDS::DataWriter_var& writer_;

  Test(
    DDS::DomainParticipant_var& participant,
    DDS::Publisher_var& publisher,
    DDS::Subscriber_var& subscriber,
    DDS::DataReader_var& reader,
    DDS::DataWriter_var& writer)
    : participant_(participant)
    , publisher_(publisher)
    , subscriber_(subscriber)
    , reader_(reader)
    , writer_(writer)
  {
  }

  bool
  setup_test(DDS::Topic_var& topic)
  {
    DDS::ReturnCode_t rc;

    // Create Subscriber
    subscriber_ = participant_->create_subscriber(
      SUBSCRIBER_QOS_DEFAULT, DDS::SubscriberListener::_nil(),
      OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (!subscriber_) {
      ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("%N:%l setup_test() ERROR: ")
        ACE_TEXT("create_subscriber failed!\n")), false);
    }

    // Create Publisher
    publisher_ = participant_->create_publisher(
      PUBLISHER_QOS_DEFAULT, DDS::PublisherListener::_nil(),
      OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (!publisher_) {
      ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("%N:%l setup_test() ERROR: ")
        ACE_TEXT("create_publisher failed!\n")), false);
    }

    // Create DataReader
    DDS::DataReaderQos reader_qos;
    rc = subscriber_->get_default_datareader_qos(reader_qos);
    if (rc != DDS::RETCODE_OK) {
      ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("%N:%l setup_test() ERROR: ")
        ACE_TEXT("get_default_datareader_qos failed!\n")), false);
    }
    reader_qos.history.kind = DDS::KEEP_ALL_HISTORY_QOS;
    reader_qos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;
    reader_ = subscriber_->create_datareader(
      topic.in(), reader_qos, DDS::DataReaderListener::_nil(),
      OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (!reader_) {
      ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("%N:%l setup_test() ERROR: ")
        ACE_TEXT("first create_datareader failed!\n")), false);
    }

    // Create DataWriter
    DDS::DataWriterQos writer_qos;
    rc = publisher_->get_default_datawriter_qos(writer_qos);
    if (rc != DDS::RETCODE_OK) {
      ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("%N:%l setup_test() ERROR: ")
        ACE_TEXT("get_default_datawriter_qos failed: %C\n"),
        retcode_to_string(rc)), false);
    }
    writer_qos.history.kind = DDS::KEEP_ALL_HISTORY_QOS;
    writer_qos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;
    writer_ = publisher_->create_datawriter(
      topic.in(), writer_qos,
      DDS::DataWriterListener::_nil(),
      OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (!writer_) {
      ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("%N:%l setup_test() ERROR: ")
        ACE_TEXT("create_datawriter failed!\n")), true);
    }

    {
      DDS::StatusCondition_var cond = writer_->get_statuscondition();
      cond->set_enabled_statuses(DDS::PUBLICATION_MATCHED_STATUS);
      DDS::WaitSet_var ws = new DDS::WaitSet;
      ws->attach_condition(cond);
      DDS::ConditionSeq conditions;
      DDS::PublicationMatchedStatus matches = { 0, 0, 0, 0, 0 };
      do {
        rc = ws->wait(conditions, max_wait_time);
        if (rc != DDS::RETCODE_OK) {
          ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("%N:%l setup_test() ERROR: ")
            ACE_TEXT("wait failed: %C\n"),
            retcode_to_string(rc)), false);
        }

        rc = writer_->get_publication_matched_status(matches);
        if (rc != ::DDS::RETCODE_OK) {
          ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("%N:%l setup_test() ERROR: ")
            ACE_TEXT("Failed to get publication match status: %C\n"),
            retcode_to_string(rc)), false);
        }
      } while (matches.total_count < 1);
      ws->detach_condition(cond);
    }

    return true;
  }

  ~Test()
  {
    DDS::ReturnCode_t rc;

    if (reader_) {
      rc = subscriber_->delete_datareader(reader_);
      if (rc != DDS::RETCODE_OK) {
        ACE_ERROR((LM_ERROR, ACE_TEXT("%N:%l ~Test() ERROR: ")
          ACE_TEXT("delete reader failed: %C\n"),
          retcode_to_string(rc)));
      }
    }

    if (writer_) {
      rc = publisher_->delete_datawriter(writer_);
      if (rc != DDS::RETCODE_OK) {
        ACE_ERROR((LM_ERROR, ACE_TEXT("%N:%l ~Test() ERROR: ")
          ACE_TEXT("delete writer failed: %C\n"),
          retcode_to_string(rc)));
      }
    }

    if (subscriber_) {
      rc = participant_->delete_subscriber(subscriber_);
      if (rc != DDS::RETCODE_OK) {
        ACE_ERROR((LM_ERROR, ACE_TEXT("%N:%l ~Test() ERROR: ")
          ACE_TEXT("delete subscriber failed: %C\n"),
          retcode_to_string(rc)));
      }
    }

    if (publisher_) {
      rc = participant_->delete_publisher(publisher_);
      if (rc != DDS::RETCODE_OK) {
        ACE_ERROR((LM_ERROR, ACE_TEXT("%N:%l ~Test() ERROR: ")
          ACE_TEXT("delete publisher failed: %C\n"),
          retcode_to_string(rc)));
      }
    }
  }
};

bool wait_for_dispose(ElectionNews_tDataReader_var& reader_i) {
  DDS::ReturnCode_t rc;
  DDS::ReadCondition_var read_condition = reader_i->create_readcondition(
    DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE,
    DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE);
  DDS::WaitSet_var ws = new DDS::WaitSet;
  ws->attach_condition(read_condition);
  DDS::ConditionSeq active;
  rc = ws->wait(active, max_wait_time);
  ws->detach_condition(read_condition);
  reader_i->delete_readcondition(read_condition);
  if (rc != DDS::RETCODE_OK) {
    ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("%N:%l wait_for_dispose() ERROR: ")
      ACE_TEXT("wait failed: %C\n"),
      retcode_to_string(rc)), false);
  }
  return true;
}

bool
basic_test(DDS::DomainParticipant_var& participant, DDS::Topic_var& topic)
{
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("basic_test\n")));
  DDS::ReturnCode_t rc;

  // The Candidates and their final tallies
  typedef std::map<std::string, Vote_t> Canonical;
  Canonical canonical;
  canonical["Monkey"] = 2;
  canonical["Owl"] = 10;
  canonical["Snake"] = 3;
  canonical["Tiger"] = 25;
  canonical["Turtle"] = 5;

  // Calculate the canonical expected winner
  ElectionResult_t canonical_result;
  canonical_result.total_votes = 0;
  Vote_t max = 0;
  Canonical::iterator i, finished = canonical.end();
  for (i = canonical.begin(); i != finished; ++i) {
    Vote_t votes = i->second;
    if (votes > max) {
      Candidate_t c;
      c.name = i->first.c_str();
      c.votes = max = votes;
      canonical_result.winner = c;
    }
    canonical_result.total_votes += votes;
  }

  // Setup Our DDS Entities
  DDS::Publisher_var publisher;
  DDS::Subscriber_var subscriber;
  DDS::DataReader_var reader;
  DDS::DataWriter_var writer;
  Test t(participant, publisher, subscriber, reader, writer);
  if (!t.setup_test(topic)) {
    ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("%N:%l basic_test() ERROR: ")
      ACE_TEXT("setup_test failed!\n")), false);
  }
  ElectionNews_tDataReader_var reader_i = ElectionNews_tDataReader::_narrow(reader);
  if (!reader_i) {
    ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("%N:%l basic_test() ERROR: ")
      ACE_TEXT("_narrow reader failed!\n")), false);
  }
  ElectionNews_tDataWriter_var writer_i = ElectionNews_tDataWriter::_narrow(writer);
  if (!writer_i) {
    ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("%N:%l basic_test() ERROR: ")
      ACE_TEXT("_narrow writer failed!\n")), false);
  }

  // Write and Dispose an Instance
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("Writing Election Statuses...\n")));
  ElectionNews_t news;
  Candidate_t c;
  for (i = canonical.begin(); i != finished; ++i) {
    c.name = i->first.c_str();
    c.votes = i->second;
    news.status(c);
    rc = writer_i->write(news, DDS::HANDLE_NIL);
    if (rc != DDS::RETCODE_OK) {
      ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("%N:%l basic_test() ERROR: ")
        ACE_TEXT("Unable to write sample %C: %C\n"),
        i->first.c_str(), retcode_to_string(rc)), false);
    }
  }
  rc = writer_i->dispose(news, DDS::HANDLE_NIL);
  if (rc != DDS::RETCODE_OK) {
    ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("%N:%l basic_test() ERROR: ")
      ACE_TEXT("Unable to dispose: %C\n"),
      retcode_to_string(rc)), false);
  }

  // Wait for the Dispose and Read Them
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("Reading Election Statuses...\n")));
  if (!wait_for_dispose(reader_i)) {
    return false;
  }
  max = 0;
  ElectionNews_tSeq newsSeq;
  ElectionResult_t result_from_statuses;
  result_from_statuses.total_votes = 0;
  DDS::SampleInfoSeq info;
  rc = reader_i->take(
    newsSeq, info,
    DDS::LENGTH_UNLIMITED,
    DDS::ANY_SAMPLE_STATE,
    DDS::ANY_VIEW_STATE,
    DDS::ANY_INSTANCE_STATE);
  if (rc != DDS::RETCODE_OK) {
    ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("%N:%l basic_test() ERROR: ")
      ACE_TEXT("Unable to read: %C\n"),
      retcode_to_string(rc)), false);
  }
  size_t count = 0;
  bool got_dispose = false;
  for (CORBA::ULong i = 0; i < newsSeq.length(); i++) {
    const CORBA::ULong disc = newsSeq[i]._d();
    if (disc != ELECTION_STATUS) {
      ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("%N:%l basic_test() ERROR: ")
        ACE_TEXT("Discriminator should be ELECTION_STATUS (%d) but it's %d\n"),
        ELECTION_STATUS, disc), false);
    }
    if (info[i].valid_data) {
      const Vote_t votes = newsSeq[i].status().votes;
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("  - %C: %u\n"),
        newsSeq[i].status().name.in(), votes));
      count++;
      result_from_statuses.total_votes += votes;
      if (votes > max) {
        result_from_statuses.winner = newsSeq[i].status();
        max = votes;
      }
    } else {
      got_dispose = true;
    }
  }
  if (!got_dispose) {
    ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("%N:%l basic_test() ERROR: ")
      ACE_TEXT("No Dispose in Taken Samples\n")), false);
  }
  if (count != canonical.size()) {
    ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("%N:%l basic_test() ERROR: ")
      ACE_TEXT("Unexpected number of samples!\n")), false);
  }

  // Write and Dispose Another Instance
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("Writing Election Result...\n")));
  news.result(result_from_statuses);
  rc = writer_i->write(news, DDS::HANDLE_NIL);
  if (rc != DDS::RETCODE_OK) {
    ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("%N:%l basic_test() ERROR: ")
      ACE_TEXT("Unable to write result sample: %C\n"),
      retcode_to_string(rc)), false);
  }
  rc = writer_i->dispose(news, DDS::HANDLE_NIL);
  if (rc != DDS::RETCODE_OK) {
    ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("%N:%l basic_test() ERROR: ")
      ACE_TEXT("Unable to dispose: %C\n"),
      retcode_to_string(rc)), false);
  }

  // Wait for the Dispose and Read
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("Reading Election Result...\n")));
  if (!wait_for_dispose(reader_i)) {
    return false;
  }
  rc = reader_i->take(
    newsSeq, info,
    DDS::LENGTH_UNLIMITED,
    DDS::ANY_SAMPLE_STATE,
    DDS::ANY_VIEW_STATE,
    DDS::ANY_INSTANCE_STATE);
  if (rc != DDS::RETCODE_OK) {
    ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("%N:%l basic_test() ERROR: ")
      ACE_TEXT("Unable to read: %C\n"),
      retcode_to_string(rc)), false);
  }
  if (newsSeq.length() != 2) {
    ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("%N:%l basic_test() ERROR: ")
      ACE_TEXT("Expected 2 samples, got %u\n"),
      newsSeq.length()), false);
  }
  if (!info[0].valid_data || info[1].valid_data) {
    ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("%N:%l basic_test() ERROR: ")
      ACE_TEXT("Invalid valid_data values\n")), false);
  }
  for (CORBA::ULong i = 0; i < 2; i++) {
    const CORBA::ULong disc = newsSeq[i]._d();
    if (disc != ELECTION_RESULT) {
      ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("%N:%l basic_test() ERROR: ")
        ACE_TEXT("Discriminator should be ELECTION_RESULT (%d) but it's %d\n"),
        ELECTION_STATUS, disc), false);
    }
  }
  ElectionResult_t result_from_read = newsSeq[0].result();
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("New Animal Kingdom PM is %C with %u out of %u votes\n"),
    result_from_read.winner.name.in(), result_from_read.winner.votes,
    result_from_read.total_votes));
  if (result_from_read.total_votes != canonical_result.total_votes ||
    result_from_read.winner.votes != canonical_result.winner.votes ||
    ACE_OS::strcmp(
      result_from_read.winner.name.in(),
      canonical_result.winner.name.in())) {
    ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("%N:%l basic_test() ERROR: ")
      ACE_TEXT("That's not the expected result!\n")), false);
  }

  return true;
}

int
ACE_TMAIN(int argc, ACE_TCHAR** argv)
{
  int status = 0;
  try {
    DDS::ReturnCode_t rc;

    DDS::DomainParticipantFactory_var dpf =
      TheParticipantFactoryWithArgs(argc, argv);
    if (!dpf) {
      ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("%N:%l main() ERROR: ")
        ACE_TEXT("Participant Factory failed!\n")), 1);
    }

    DDS::DomainParticipant_var participant =
      dpf->create_participant(domain,
                              PARTICIPANT_QOS_DEFAULT,
                              DDS::DomainParticipantListener::_nil(),
                              OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (!participant) {
      ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("%N:%l main() ERROR: ")
        ACE_TEXT("create_participant failed!\n")), 1);
    }

    // Register Type
    ElectionNews_tTypeSupport_var ts = new ElectionNews_tTypeSupportImpl;
    rc = ts->register_type(participant.in(), "");
    if (rc != DDS::RETCODE_OK) {
      ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("%N:%l main() ERROR: ")
        ACE_TEXT("register_type failed: %C\n"),
        retcode_to_string(rc)), 1);
    }

    // Create Topic
    DDS::Topic_var topic =
      participant->create_topic(
        "Animal Kingdom Elections",
        CORBA::String_var(ts->get_type_name()),
        TOPIC_QOS_DEFAULT,
        DDS::TopicListener::_nil(),
        OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (!topic) {
      ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("%N:%l main() ERROR: ")
        ACE_TEXT("create_topic failed!\n")), 1);
    }

    // Basic Test
    if (!basic_test(participant, topic)) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("Basic Test Failed\n")));
      status = 1;
    }

    // Clean-up
    rc = participant->delete_contained_entities();
    if (rc != DDS::RETCODE_OK) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("%N:%l: main() ERROR: ")
        ACE_TEXT("delete_contained_entities failed: %C\n"),
        retcode_to_string(rc)));
      status = 1;
    }
    rc = dpf->delete_participant(participant.in());
    if (rc != DDS::RETCODE_OK) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("%N:%l: main() ERROR: ")
        ACE_TEXT("delete_participant failed: %C\n"),
        retcode_to_string(rc)));
      status = 1;
    }
    TheServiceParticipant->shutdown();
  } catch (const CORBA::Exception& e) {
    e._tao_print_exception("Caught in main()");
    status = 1;
  }

  return status;
}
