/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef DCPS_TESTFRAMEWORK_T_H
#define DCPS_TESTFRAMEWORK_T_H

#include "TestFramework.h"

template<typename Writer>
class TestPublisher {
public:
  typedef typename Writer::_ptr_type Writer_ptr;
  typedef typename Writer::_var_type Writer_var;

  TestPublisher(TestBase& test);
  virtual ~TestPublisher();

  virtual DDS::ReturnCode_t init_publisher(
    DDS::PublisherQos& qos,
    DDS::PublisherListener_ptr& listener,
    DDS::StatusMask& status);

  virtual DDS::ReturnCode_t init_datawriter(
    DDS::DataWriterQos& qos,
    DDS::DataWriterListener_ptr& listener,
    DDS::StatusMask& status);

  void wait_for_acknowledgments(
    DDS::Duration_t timeout = TestBase::DEFAULT_TIMEOUT);

  void wait_for_subscribers(
    CORBA::Long count = 1,
    DDS::Duration_t timeout = TestBase::DEFAULT_TIMEOUT);

  int write_message(TestMessage& message);
  int write_w_timestamp(
    TestMessage& message,
    DDS::InstanceHandle_t& instance,
    DDS::Time_t& timestamp);

  DDS::InstanceHandle_t register_instance(TestMessage& message);

  virtual void init_i();
  virtual void fini_i();

protected:
  TestBase& test_;
  DDS::Publisher_var publisher_;

  DDS::DataWriter_var writer_;
  Writer_ptr writer_i_;

  DDS::Publisher_var create_publisher();
  DDS::DataWriter_var create_datawriter();

  DDS::DomainParticipant_var&
  get_participant() { return test_.get_participant(); }

  DDS::Topic_var&
  get_topic() { return test_.get_topic(); }
};

template<typename Reader>
class TestSubscriber {
public:
  typedef typename Reader::_ptr_type Reader_ptr;
  typedef typename Reader::_var_type Reader_var;

  TestSubscriber(TestBase& test);
  virtual ~TestSubscriber();

  virtual DDS::ReturnCode_t init_subscriber(
    DDS::SubscriberQos& qos,
    DDS::SubscriberListener_ptr& listener,
    DDS::StatusMask& status);

  virtual DDS::ReturnCode_t init_datareader(
    DDS::DataReaderQos& qos,
    DDS::DataReaderListener_ptr& listener,
    DDS::StatusMask& status);

  DDS::ReadCondition_ptr create_readcondition(
    DDS::SampleStateMask sample_states,
    DDS::ViewStateMask view_states,
    DDS::InstanceStateMask instance_states);

  void wait_for_publishers(
    CORBA::Long count = 1,
    DDS::Duration_t timeout = TestBase::DEFAULT_TIMEOUT);

  DDS::ReturnCode_t read_w_condition(
    TestMessageSeq& data_values,
    DDS::SampleInfoSeq& sample_infos,
    CORBA::Long max_samples,
    DDS::ReadCondition_ptr a_condition);

  DDS::ReturnCode_t take_w_condition (
    TestMessageSeq & data_values,
    DDS::SampleInfoSeq & sample_infos,
    CORBA::Long max_samples,
    DDS::ReadCondition_ptr a_condition);

  DDS::ReturnCode_t take_next_sample(
    TestMessage& message,
    DDS::SampleInfo& si);

  DDS::ReturnCode_t take_instance(
    TestMessageSeq& messages,
    DDS::SampleInfoSeq& si,
    long max_samples,
    DDS::InstanceHandle_t& handle,
    DDS::SampleStateMask sample_states,
    DDS::ViewStateMask view_states,
    DDS::InstanceStateMask instance_states);

  DDS::InstanceHandle_t lookup_instance(
    TestMessage& message);

  virtual void init_i();
  virtual void fini_i();

protected:
  TestBase& test_;
  DDS::Subscriber_var subscriber_;

  DDS::DataReader_var reader_;
  Reader_ptr reader_i_;

  DDS::Subscriber_var create_subscriber();
  DDS::DataReader_var create_datareader();

  DDS::DomainParticipant_var&
  get_participant() { return test_.get_participant(); }

  DDS::Topic_var&
  get_topic() { return test_.get_topic(); }
};

template<typename Reader, typename Writer>
class TestPair : public virtual TestSubscriber<Reader>,
                 public virtual TestPublisher<Writer> {
public:
  TestPair();
  virtual ~TestPair();

protected:
  virtual void init_i();

  virtual void fini_i();
};

#if defined (ACE_TEMPLATES_REQUIRE_SOURCE)
#include "TestFramework_T.cpp"
#endif

#if defined (ACE_TEMPLATES_REQUIRE_PRAGMA)
#pragma message("TestFramework_T.cpp template inst")
#pragma implementation("TestFramework_T.cpp")
#endif

#endif  /* DCPS_TESTFRAMEWORK_T_H */
