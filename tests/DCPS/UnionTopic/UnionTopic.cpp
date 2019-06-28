#include <dds/DdsDcpsInfrastructureC.h>
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/WaitSet.h>
#include <dds/DCPS/SafetyProfileStreams.h>

using OpenDDS::DCPS::retcode_to_string;

#include "UnionTopicTypeSupportImpl.h"

using namespace UnionTopic;

#include <vector>
#include <string>

std::vector<UnionTopic::Candidate_t> candidates = {
  {"Turtle", 20},
  {"Monkey", 43},
  {"Owl", 27},
  {"Snake", 13},
  {"Tiger", 67}
};

const DDS::Duration_t max_wait_time = {3, 0};
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
    DDS::SubscriberQos subscriber_qos;
    rc = participant_->get_default_subscriber_qos(subscriber_qos);
    if (rc != DDS::RETCODE_OK) {
      ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("%N:%l setup_test() ERROR: ")
                        ACE_TEXT("get_default_subscriber_qos failed: %C\n"),
                        retcode_to_string(rc).c_str()), false);
    }
    subscriber_ = participant_->create_subscriber(
      subscriber_qos, DDS::SubscriberListener::_nil(),
      OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (!subscriber_) {
      ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("%N:%l setup_test() ERROR: ")
                        ACE_TEXT("create_subscriber failed!\n")), false);
    }

    // Create Publisher
    DDS::PublisherQos publisher_qos;
    rc = participant_->get_default_publisher_qos(publisher_qos);
    if (rc != DDS::RETCODE_OK) {
      ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("%N:%l setup_test() ERROR: ")
                        ACE_TEXT("get_default_publisher_qos failed: %C\n"),
                        retcode_to_string(rc).c_str()), false);
    }
    publisher_ = participant_->create_publisher(
      publisher_qos, DDS::PublisherListener::_nil(),
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
    reader_ = subscriber_->create_datareader(
      topic.in(), reader_qos, DDS::DataReaderListener::_nil(),
      OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (!reader_) {
      ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("%N:%l setup_test() ERROR: ")
                        ACE_TEXT("first create_datareader failed!\n")), false);
    }

    // Create DataWriter
    writer_ = publisher_->create_datawriter(
      topic.in(), DATAWRITER_QOS_DEFAULT,
      DDS::DataWriterListener::_nil(),
      OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (!writer_) {
      ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("%N:%l setup_test() ERROR: ")
                        ACE_TEXT("create_datawriter failed!\n")), true);
    }

    // Block until Subscriber is associated
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
                            retcode_to_string(rc).c_str()), false);
        }

        rc = writer_->get_publication_matched_status(matches);
        if (rc != ::DDS::RETCODE_OK) {
          ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("%N:%l setup_test() ERROR: ")
                            ACE_TEXT("Failed to get publication match status: %C\n"),
                            retcode_to_string(rc).c_str()), false);
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
          retcode_to_string(rc).c_str()));
      }
    }

    if (writer_) {
      rc = publisher_->delete_datawriter(writer_);
      if (rc != DDS::RETCODE_OK) {
        ACE_ERROR((LM_ERROR, ACE_TEXT("%N:%l ~Test() ERROR: ")
          ACE_TEXT("delete writer failed: %C\n"),
          retcode_to_string(rc).c_str()));
      }
    }

    if (subscriber_) {
      rc = participant_->delete_subscriber(subscriber_);
      if (rc != DDS::RETCODE_OK) {
        ACE_ERROR((LM_ERROR, ACE_TEXT("%N:%l ~Test() ERROR: ")
          ACE_TEXT("delete subscriber failed: %C\n"),
          retcode_to_string(rc).c_str()));
      }
    }

    if (publisher_) {
      rc = participant_->delete_publisher(publisher_);
      if (rc != DDS::RETCODE_OK) {
        ACE_ERROR((LM_ERROR, ACE_TEXT("%N:%l ~Test() ERROR: ")
          ACE_TEXT("delete publisher failed: %C\n"),
          retcode_to_string(rc).c_str()));
      }
    }
  }
};

bool
basic_test(DDS::DomainParticipant_var& participant, DDS::Topic_var& topic)
{
  DDS::ReturnCode_t rc;

  ACE_DEBUG((LM_DEBUG, ACE_TEXT("basic_test\n")));

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

  // Write and Read one Instance
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("Writing New Candidates...\n")));
  ElectionNews_t news;
  for (size_t i = 0; i < candidates.size(); ++i) {
    Candidate_t c;
    c.name = candidates[i].name;
    c.votes = 0;
    news.new_candidate(c);
    rc = writer_i->write(news, DDS::HANDLE_NIL);
    if (rc != DDS::RETCODE_OK) {
        ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("%N:%l basic_test() ERROR: ")
          ACE_TEXT("Unable to write sample %u: %C\n"),
          retcode_to_string(rc).c_str(), i), false);
    }
  }

  // TODO Confirm this is working
  // Dispose of the NEW_CANDIDATE Instance
  rc = writer_i->dispose(news, DDS::HANDLE_NIL);
  if (rc != DDS::RETCODE_OK) {
      ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("%N:%l basic_test() ERROR: ")
        ACE_TEXT("Unable to dispose: %C\n"),
        retcode_to_string(rc).c_str()), false);
  }

  ACE_DEBUG((LM_DEBUG, ACE_TEXT("Reading New Candidates...\n")));
  size_t count = 0;
  while (true) {
    {
      DDS::ReadCondition_var read_condition = reader_i->create_readcondition(
        DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE,
        DDS::ANY_INSTANCE_STATE);
      DDS::WaitSet_var ws = new DDS::WaitSet;
      ws->attach_condition(read_condition);
      DDS::ConditionSeq active;
      rc = ws->wait(active, max_wait_time);
      ws->detach_condition(read_condition);
      reader_i->delete_readcondition(read_condition);
      if (rc != DDS::RETCODE_OK) {
        ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("%N:%l basic_test() ERROR: ")
                          ACE_TEXT("wait failed: %C\n"),
                          retcode_to_string(rc).c_str()), false);
      }
    }

    ElectionNews_tSeq newsSeq;
    DDS::SampleInfoSeq info;
    rc = reader_i->take(
      newsSeq, info,
      DDS::LENGTH_UNLIMITED,
      DDS::ANY_SAMPLE_STATE,
      DDS::ANY_VIEW_STATE,
      DDS::ANY_INSTANCE_STATE);
    if (rc == DDS::RETCODE_OK) {
      for (size_t i = 0; i < newsSeq.length(); i++) {
        if (info[i].valid_data) {
          ACE_DEBUG((LM_DEBUG, ACE_TEXT("  - %C\n"), newsSeq[i].new_candidate().name.in()));
          count++;
        }
      }
      if (count == candidates.size()) {
        break;
      } else if (count > candidates.size()) {
        ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("%N:%l basic_test() ERROR: ")
                          ACE_TEXT("Unexpected extra samples!\n")), false);
      }
    } else {
      ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("%N:%l basic_test() ERROR: ")
                        ACE_TEXT("take_next_sample: %C\n"),
                        retcode_to_string(rc).c_str()), false);
    }
  }

  // Write and Read Another Instance (TODO)

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
                        retcode_to_string(rc).c_str()), 1);
    }

    // Create Topic
    DDS::Topic_var topic =
      participant->create_topic("Animal Kingdom Elections",
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
                 retcode_to_string(rc).c_str()));
      status = 1;
    }
    rc = dpf->delete_participant(participant.in());
    if (rc != DDS::RETCODE_OK) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("%N:%l: main() ERROR: ")
                 ACE_TEXT("delete_participant failed: %C\n"),
                 retcode_to_string(rc).c_str()));
      status = 1;
    }
    TheServiceParticipant->shutdown();
  } catch (const CORBA::Exception& e) {
    e._tao_print_exception("Caught in main()");
    status = 1;
  }

  return status;
}
