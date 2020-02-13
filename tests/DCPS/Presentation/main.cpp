/**
 * Test coherent_access and ordered_access facets of
 * the PRESENTATION QoS policy.
 */

#include <ace/Log_Msg.h>

#include <tao/Basic_Types.h>

#include <dds/DdsDcpsInfrastructureC.h>
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/PublisherImpl.h>
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/SubscriberImpl.h>
#include <dds/DCPS/SubscriptionInstance.h>
#include <dds/DCPS/WaitSet.h>
#include <dds/DCPS/transport/framework/TransportDefs.h>
#include <dds/DCPS/SafetyProfileStreams.h>
#include <dds/DCPS/StaticIncludes.h>

#include "FooTypeTypeSupportImpl.h"

#include <iostream>

using OpenDDS::DCPS::retcode_to_string;

namespace {

static const size_t SAMPLES_PER_TEST = 100;

const DDS::Duration_t max_wait_time = {3, 0};
const unsigned coherent_sample_wait = 2;

class ListenerImpl
      : public virtual OpenDDS::DCPS::LocalObject<DDS::DataReaderListener> {
public:
  bool error, done;
  ListenerImpl()
    : error(false), done(false)
  {
  }

  virtual void on_requested_deadline_missed(
      DDS::DataReader_ptr /*reader*/,
      const DDS::RequestedDeadlineMissedStatus& /*status*/) {}

  virtual void on_requested_incompatible_qos(
      DDS::DataReader_ptr /*reader*/,
      const DDS::RequestedIncompatibleQosStatus& /*status*/) {}

  virtual void on_sample_rejected(
      DDS::DataReader_ptr /*reader*/,
      const DDS::SampleRejectedStatus& /*status*/) {}

  virtual void on_liveliness_changed(
      DDS::DataReader_ptr /*reader*/,
      const DDS::LivelinessChangedStatus& /*status*/) {}

  void on_data_available(DDS::DataReader_ptr reader)
  {
    FooDataReader_var reader_i = FooDataReader::_narrow(reader);
    if (!reader_i) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("%N:%l on_data_available() ERROR: ")
                 ACE_TEXT("_narrow reader failed!\n")));
      error = true;
      done = true;
      return;
    }

    FooSeq fooSeq;
    DDS::SampleInfoSeq info;
    DDS::ReturnCode_t take_result = reader_i->take(
      fooSeq, info,
      DDS::LENGTH_UNLIMITED,
      DDS::ANY_SAMPLE_STATE,
      DDS::ANY_VIEW_STATE,
      DDS::ANY_INSTANCE_STATE);
    if (take_result != DDS::RETCODE_OK) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("%N:%l on_data_available() ERROR: ")
                 ACE_TEXT("take failed: %C\n"),
                 retcode_to_string(take_result)));
      error = true;
      done = true;
      return;
    }

    if (fooSeq.length() != SAMPLES_PER_TEST) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("%N:%l on_data_available() ERROR: ")
                 ACE_TEXT("Unexpected number of samples taken: %d. ")
                 ACE_TEXT("Expected: %d. Printing samples:\n"),
                 fooSeq.length(), SAMPLES_PER_TEST));
      for (unsigned int i = 0; i < fooSeq.length(); ++i) {
        if (info[i].valid_data) {
          std::cout << "fooSeq[" << i << "].x: " << fooSeq[i].x << std::endl;
        } else {
          std::cout << "fooSeq[" << i << "] doesn't have valid data" << std::endl;
        }
      }
      error = true;
    }
    done = true;
  }

  virtual void on_subscription_matched(
      DDS::DataReader_ptr /*reader*/,
      const DDS::SubscriptionMatchedStatus& /*status*/) {}

  virtual void on_sample_lost(
      DDS::DataReader_ptr /*reader*/,
      const DDS::SampleLostStatus& /*status*/) {}
};

class Test {
public:
  DDS::DomainParticipant_var& participant_;
  DDS::Publisher_var& publisher_;
  DDS::Subscriber_var& subscriber_;
  DDS::DataReader_var& reader1_;
  DDS::DataReader_var& reader2_;
  DDS::DataReaderListener_var& listener_;
  DDS::DataWriter_var& writer_;

  Test(
    DDS::DomainParticipant_var& participant,
    DDS::Publisher_var& publisher,
    DDS::Subscriber_var& subscriber,
    DDS::DataReader_var& reader1,
    DDS::DataReader_var& reader2,
    DDS::DataReaderListener_var& listener,
    DDS::DataWriter_var& writer)
    : participant_(participant)
    , publisher_(publisher)
    , subscriber_(subscriber)
    , reader1_(reader1)
    , reader2_(reader2)
    , listener_(listener)
    , writer_(writer)
  {
  }

  bool
  setup_test(DDS::Topic_var& topic, bool coherent)
  {
    DDS::ReturnCode_t rc;

    // Create Subscriber
    DDS::SubscriberQos subscriber_qos;
    rc = participant_->get_default_subscriber_qos(subscriber_qos);
    if (rc != DDS::RETCODE_OK) {
      ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("%N:%l setup_test() ERROR: ")
                        ACE_TEXT("get_default_subscriber_qos failed: %C\n"),
                        retcode_to_string(rc)), false);
    }

    // TODO Currently only TOPIC access_scope is supported.
    subscriber_qos.presentation.access_scope = DDS::TOPIC_PRESENTATION_QOS;
    subscriber_qos.presentation.coherent_access = coherent;
    subscriber_qos.presentation.ordered_access = !coherent;

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
                        retcode_to_string(rc)), false);
    }

    // TODO Currently only TOPIC access_scope is supported.
    publisher_qos.presentation.access_scope = DDS::TOPIC_PRESENTATION_QOS;
    publisher_qos.presentation.coherent_access = coherent;
    publisher_qos.presentation.ordered_access = !coherent;

    publisher_ = participant_->create_publisher(
      publisher_qos, DDS::PublisherListener::_nil(),
      OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (!publisher_) {
      ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("%N:%l setup_test() ERROR: ")
                        ACE_TEXT("create_publisher failed!\n")), false);
    }

    // Common DataReader QOS
    DDS::DataReaderQos reader_qos;
    rc = subscriber_->get_default_datareader_qos(reader_qos);
    if (rc != DDS::RETCODE_OK) {
      ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("%N:%l setup_test() ERROR: ")
                        ACE_TEXT("get_default_datareader_qos failed!\n")), false);
    }
    reader_qos.history.kind = DDS::KEEP_ALL_HISTORY_QOS;
    reader_qos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;

    // Create First DataReader for Both Tests
    reader1_ = subscriber_->create_datareader(
      topic.in(), reader_qos, DDS::DataReaderListener::_nil(),
      OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (!reader1_) {
      ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("%N:%l setup_test() ERROR: ")
                        ACE_TEXT("first create_datareader failed!\n")), false);
    }

    // Create Second DataReader Just for Coherent
    if (coherent) {
      listener_ = new ListenerImpl();
      reader2_ = subscriber_->create_datareader(
        topic.in(), reader_qos, listener_,
        OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      if (!reader2_) {
        ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("%N:%l setup_test() ERROR: ")
                          ACE_TEXT("second create_datareader failed!\n")), false);
      }
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
                            retcode_to_string(rc)), false);
        }

        rc = writer_->get_publication_matched_status(matches);
        if (rc != ::DDS::RETCODE_OK) {
          ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("%N:%l setup_test() ERROR: ")
                            ACE_TEXT("Failed to get publication match status: %C\n"),
                            retcode_to_string(rc)), false);
        }
      } while (matches.total_count < (coherent ? 2 : 1));
      ws->detach_condition(cond);
    }

    return true;
  }

  ~Test()
  {
    DDS::ReturnCode_t rc;

    if (reader1_) {
      rc = subscriber_->delete_datareader(reader1_);
      if (rc != DDS::RETCODE_OK) {
        ACE_ERROR((LM_ERROR, ACE_TEXT("%N:%l ~Test() ERROR: ")
          ACE_TEXT("delete reader1 failed: %C\n"),
          retcode_to_string(rc)));
      }
    }

    if (reader2_) {
      rc = subscriber_->delete_datareader(reader2_);
      if (rc != DDS::RETCODE_OK) {
        ACE_ERROR((LM_ERROR, ACE_TEXT("%N:%l ~Test() ERROR: ")
          ACE_TEXT("delete reader2 failed: %C\n"),
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

bool
coherent_test(DDS::DomainParticipant_var& participant, DDS::Topic_var& topic)
{
  DDS::ReturnCode_t rc;

  ACE_DEBUG((LM_DEBUG, ACE_TEXT("Testing coherent_access...\n")));

  DDS::Publisher_var publisher;
  DDS::Subscriber_var subscriber;
  DDS::DataReader_var reader1;
  DDS::DataReader_var reader2;
  DDS::DataReaderListener_var listener;
  DDS::DataWriter_var writer;
  Test t(participant, publisher, subscriber, reader1, reader2, listener, writer);
  if (!t.setup_test(topic, true)) {
    ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("%N:%l coherent_test() ERROR: ")
                      ACE_TEXT("setup_test failed!\n")), false);
  }

  FooDataReader_var reader1_i = FooDataReader::_narrow(reader1);
  if (!reader1_i) {
    ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("%N:%l coherent_test() ERROR: ")
                      ACE_TEXT("_narrow reader1 failed!\n")), false);
  }

  FooDataWriter_var writer_i = FooDataWriter::_narrow(writer);
  if (!writer_i) {
    ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("%N:%l coherent_test() ERROR: ")
                      ACE_TEXT("_narrow writer failed!\n")), false);
  }

  ACE_DEBUG((LM_DEBUG, ACE_TEXT("Writing...\n")));
  publisher->begin_coherent_changes();
  for (size_t i = 0; i < SAMPLES_PER_TEST; ++i) {
    Foo foo = {0, 0, 0, 0};
    foo.x = static_cast<CORBA::Float>(100 + i);
    rc = writer_i->write(foo, DDS::HANDLE_NIL);
    if (rc != DDS::RETCODE_OK) {
        ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("%N:%l coherent_test() ERROR: ")
          ACE_TEXT("Unable to write sample %u: %C\n"),
          retcode_to_string(rc), i), false);
    }
  }
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("Waiting...\n")));
  ACE_OS::sleep(coherent_sample_wait); // wait for samples to arrive

  // Verify that no samples are yet available
  {
    FooSeq foo;
    DDS::SampleInfoSeq info;
    rc = reader1_i->take(
      foo, info, DDS::LENGTH_UNLIMITED,
      DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);
    if (rc == DDS::RETCODE_OK) {
      ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("%N:%l coherent_test() ERROR: ")
        ACE_TEXT("Expected RETCODE_NO_DATA, but got RETCODE_OK and %u samples\n"),
        foo.length()), false);
    } else if (rc != DDS::RETCODE_NO_DATA) {
      ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("%N:%l coherent_test() ERROR: ")
        ACE_TEXT("Expected RETCODE_NO_DATA, but error is: %C\n"),
        retcode_to_string(rc)), false);
    }
  }

  // After this, samples should available
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("Reading...\n")));
  publisher->end_coherent_changes();
  {
    DDS::ReadCondition_var read_condition = reader1_i->create_readcondition(
      DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE);
    DDS::WaitSet_var ws = new DDS::WaitSet;
    ws->attach_condition(read_condition);
    DDS::ConditionSeq active;
    rc = ws->wait(active, max_wait_time);
    ws->detach_condition(read_condition);
    reader1_i->delete_readcondition(read_condition);
    if (rc != DDS::RETCODE_OK) {
      ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("%N:%l coherent_test() ERROR: ")
                        ACE_TEXT("wait failed: %C\n"),
                        retcode_to_string(rc)), false);
    }
  }

  // Verify coherent samples are now available
  {
    FooSeq foo;
    DDS::SampleInfoSeq info;
    rc = reader1_i->take(
      foo, info, DDS::LENGTH_UNLIMITED,
      DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);
    if (rc != DDS::RETCODE_OK) {
      ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("%N:%l coherent_test() ERROR: ")
        ACE_TEXT("take error: %C\n"),
        retcode_to_string(rc)), false);
    }
    if (foo.length() != SAMPLES_PER_TEST) {
      ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("%N:%l coherent_test() ERROR: ")
                        ACE_TEXT("Got %u samples, expected %u\n"),
                        foo.length(), SAMPLES_PER_TEST), false);
    }
  }

  // Wait Until Listener is Done and Detach It
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("Waiting For Listener...\n")));
  ListenerImpl* listener_i = dynamic_cast<ListenerImpl*>(listener.in());
  if (!listener_i) {
    ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("%N:%l coherent_test() ERROR: ")
                      ACE_TEXT("failed to get listener!\n")), false);
  }
  for (int i = 0; (i < max_wait_time.sec) && !listener_i->done; i++) {
    ACE_OS::sleep(1);
  }
  if (!listener_i->done) {
    ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("%N:%l coherent_test() ERROR: ")
                      ACE_TEXT("Timed out waiting for listener\n")), false);
  }

  return !listener_i->error;
}

bool
ordered_test(DDS::DomainParticipant_var& participant, DDS::Topic_var& topic)
{
  DDS::ReturnCode_t rc;

  ACE_DEBUG((LM_DEBUG, ACE_TEXT("Testing ordered_access...\n")));

  DDS::Publisher_var publisher;
  DDS::Subscriber_var subscriber;
  DDS::DataReader_var reader1;
  DDS::DataReader_var reader2; // Unused
  DDS::DataReaderListener_var listener; // Unused
  DDS::DataWriter_var writer;
  Test t(participant, publisher, subscriber, reader1, reader2, listener, writer);
  if (!t.setup_test(topic, false)) {
    ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("%N:%l ordered_test() ERROR: ")
                      ACE_TEXT("setup_test failed!\n")), false);
  }

  FooDataReader_var reader1_i = FooDataReader::_narrow(reader1);
  if (!reader1_i) {
    ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("%N:%l ordered_test() ERROR: ")
                      ACE_TEXT("_narrow reader1 failed!\n")), false);
  }

  FooDataWriter_var writer_i = FooDataWriter::_narrow(writer);
  if (!writer_i) {
    ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("%N:%l ordered_test() ERROR: ")
                      ACE_TEXT("_narrow writer failed!\n")), false);
  }

  for (size_t i = 0; i < SAMPLES_PER_TEST / 2; ++i) {
    // Write first instance
    Foo foo1 = { 0, 0, 0, 0 };
    rc = writer_i->write(foo1, DDS::HANDLE_NIL);
    if (rc != DDS::RETCODE_OK) {
        ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("%N:%l ordered_test() ERROR: ")
          ACE_TEXT("Unable to write sample %u: %C\n"),
          i * 2, retcode_to_string(rc)), false);
    }

    // Write second instance
    Foo foo2 = { 42, 0, 0, 0 };
    rc = writer_i->write(foo2, DDS::HANDLE_NIL);
    if (rc != DDS::RETCODE_OK) {
        ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("%N:%l ordered_test() ERROR: ")
          ACE_TEXT("Unable to write sample %u: %C\n"),
          i * 2 + 1, retcode_to_string(rc)), false);
    }
  }

  // Wait for Samples
  {
    DDS::ReadCondition_var read_condition = reader1_i->create_readcondition(
      DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE);
    DDS::WaitSet_var ws = new DDS::WaitSet;
    ws->attach_condition(read_condition);
    DDS::ConditionSeq active;
    rc = ws->wait(active, max_wait_time);
    ws->detach_condition(read_condition);
    reader1_i->delete_readcondition(read_condition);
    if (rc != DDS::RETCODE_OK) {
      ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("%N:%l ordered_test() ERROR: ")
                        ACE_TEXT("wait failed: %C\n"),
                        retcode_to_string(rc)), false);
    }
  }

  // Verify samples are sorted in source order
  {
    FooSeq foo;
    DDS::SampleInfoSeq info;

    rc = reader1_i->take(
      foo, info,
      DDS::LENGTH_UNLIMITED,
      DDS::ANY_SAMPLE_STATE,
      DDS::ANY_VIEW_STATE,
      DDS::ANY_INSTANCE_STATE);
    if (rc != DDS::RETCODE_OK) {
      ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("%N:%l ordered_test() ERROR: ")
                        ACE_TEXT("Unable to take samples: %C\n"),
                        retcode_to_string(rc)), false);
    }

    DDS::Time_t last_timestamp = {0, 0};
    for (CORBA::ULong i = 0; i < info.length(); ++i) {
      using OpenDDS::DCPS::operator<;
      if (info[i].source_timestamp < last_timestamp) {
        ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("%N:%l ordered_test() ERROR: ")
                          ACE_TEXT("Samples taken out of order! (Starting at %d)\n"),
                          i), false);
      }
      last_timestamp = info[i].source_timestamp;
    }
  }

  return true;
}
} // namespace

int
ACE_TMAIN(int argc, ACE_TCHAR** argv)
{
  int test_failures = 0;
  try {
    DDS::ReturnCode_t rc;

    DDS::DomainParticipantFactory_var dpf =
      TheParticipantFactoryWithArgs(argc, argv);

    DDS::DomainParticipant_var participant =
      dpf->create_participant(42,
                              PARTICIPANT_QOS_DEFAULT,
                              DDS::DomainParticipantListener::_nil(),
                              OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (!participant) {
      ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("%N:%l main() ERROR: ")
                        ACE_TEXT("create_participant failed!\n")), 1);
    }

    // Register Type (FooType)
    FooTypeSupport_var ts = new FooTypeSupportImpl;
    rc = ts->register_type(participant.in(), "");
    if (rc != DDS::RETCODE_OK) {
      ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("%N:%l main() ERROR: ")
                        ACE_TEXT("register_type failed: %C\n"),
                        retcode_to_string(rc)), 1);
    }

    // Create Topic (FooTopic)
    DDS::Topic_var topic =
      participant->create_topic("FooTopic",
                                CORBA::String_var(ts->get_type_name()),
                                TOPIC_QOS_DEFAULT,
                                DDS::TopicListener::_nil(),
                                OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (!topic) {
      ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("%N:%l main() ERROR: ")
                        ACE_TEXT("create_topic failed!\n")), 1);
    }

    // Coherent Test
    if (!coherent_test(participant, topic)) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("Coherent Test Failed\n")));
      test_failures++;
    }

    // Ordered Test
    if (!ordered_test(participant, topic)) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("Ordered Test Failed\n")));
      test_failures++;
    }

    // Clean-up!
    participant->delete_contained_entities();
    dpf->delete_participant(participant);
    TheServiceParticipant->shutdown();
  } catch (const CORBA::Exception& e) {
    e._tao_print_exception("Caught in main()");
    return 1;
  }

  return test_failures;
}
