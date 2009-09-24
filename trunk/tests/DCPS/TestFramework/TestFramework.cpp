/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/WaitSet.h>

#include "TestFramework.h"

const DDS::DomainId_t TestBase::DEFAULT_DOMAIN = 42;
const char*           TestBase::DEFAULT_TOPIC = "TestFramework";
const DDS::Duration_t TestBase::DEFAULT_TIMEOUT = { 30, 0 }; // 30 seconds
const ACE_TCHAR*      TestBase::DEFAULT_TRANSPORT = ACE_TEXT("SimpleTcp");

TestBase::TestBase()
{
}

TestBase::~TestBase()
{
}

DDS::ReturnCode_t
TestBase::init_participant(DDS::DomainId_t& /*domain*/,
                           DDS::DomainParticipantQos& /*qos*/,
                           DDS::DomainParticipantListener_ptr& /*listener*/,
                           DDS::StatusMask& /*status*/)
{
  return DDS::RETCODE_OK;
}

DDS::ReturnCode_t
TestBase::init_topic(const char*& /*name*/,
                     const char*& /*type_name*/,
                     DDS::TopicQos& /*qos*/,
                     DDS::TopicListener_ptr& /*listener*/,
                     DDS::StatusMask& /*status*/)
{
  return DDS::RETCODE_OK;
}

DDS::ReturnCode_t
TestBase::init_transport(OpenDDS::DCPS::TransportIdType& /*transport_id*/,
                         ACE_TString& /*transport_type*/)
{
  return DDS::RETCODE_OK;
}

int
TestBase::run(int& argc, ACE_TCHAR* argv[])
{
  TheParticipantFactoryWithArgs(argc, argv);

  int error(0);
  try {
    init();
    error = test();
    fini();

  } catch (const CORBA::SystemException& e) {
    e._tao_print_exception("ERROR: Caught in run() -");
    return -1;
  }

  TheTransportFactory->release();
  TheServiceParticipant->shutdown();

  return error;
}

void
TestBase::init()
ACE_THROW_SPEC((CORBA::SystemException))
{
  this->participant_ = create_participant();
  this->topic_ = create_topic();

  init_i();  // delegate to child
}

void
TestBase::fini()
ACE_THROW_SPEC((CORBA::SystemException))
{
  fini_i();  // delegate to child

  this->participant_->delete_contained_entities();
  TheParticipantFactory->delete_participant(this->participant_);
}

DDS::DomainParticipant_var
TestBase::create_participant()
ACE_THROW_SPEC((CORBA::SystemException))
{
  DDS::DomainId_t domain = DEFAULT_DOMAIN;

  DDS::DomainParticipantQos qos;
  if (TheParticipantFactory->get_default_participant_qos(qos) != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("ERROR: %N:%l: create_participant() -")
               ACE_TEXT(" get_default_participant_qos failed!\n")));
    ACE_OS::exit(-1);
  }

  DDS::DomainParticipantListener_ptr listener =
    DDS::DomainParticipantListener::_nil();

  DDS::StatusMask status = OpenDDS::DCPS::DEFAULT_STATUS_MASK;

  if (init_participant(domain, qos, listener, status) != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("ERROR: %N:%l: create_participant() -")
               ACE_TEXT(" init_participant failed!\n")));
    ACE_OS::exit(-1);
  }

  DDS::DomainParticipant_var participant =
    TheParticipantFactory->create_participant(domain, qos, listener, status);

  if (CORBA::is_nil(participant.in())) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("ERROR: %N:%l: create_participant() -")
               ACE_TEXT(" create_participant failed!\n")));
    ACE_OS::exit(-1);
  }

  return participant;
}

DDS::Topic_var
TestBase::create_topic()
ACE_THROW_SPEC((CORBA::SystemException))
{
  const char* name = DEFAULT_TOPIC;

  TestMessageTypeSupport_var ts =
    new TestMessageTypeSupportImpl();

  if (ts->register_type(this->participant_.in(), "") != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("ERROR: %N:%l: create_topic() -")
               ACE_TEXT(" register_type failed!\n")));
    ACE_OS::exit(-1);
  }

  CORBA::String_var s = ts->get_type_name();
  const char* type_name = s.in();

  DDS::TopicQos qos;
  if (this->participant_->get_default_topic_qos(qos) != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("ERROR: %N:%l: create_topic() -")
               ACE_TEXT(" get_default_topic_qos failed!\n")));
    ACE_OS::exit(-1);
  }

  DDS::TopicListener_ptr listener =
    DDS::TopicListener::_nil();

  DDS::StatusMask status = OpenDDS::DCPS::DEFAULT_STATUS_MASK;

  if (init_topic(name, type_name, qos, listener, status) != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("ERROR: %N:%l: create_topic() -")
               ACE_TEXT(" init_topic failed!\n")));
    ACE_OS::exit(-1);
  }

  DDS::Topic_var topic =
    this->participant_->create_topic(name, type_name, qos, listener, status);

  if (CORBA::is_nil(topic.in())) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("ERROR: %N:%l: create_topic() -")
               ACE_TEXT(" create_topic failed!\n")));
    ACE_OS::exit(-1);
  }

  return topic;
}

OpenDDS::DCPS::TransportImpl_rch
TestBase::create_transport()
ACE_THROW_SPEC((CORBA::SystemException))
{
  OpenDDS::DCPS::TransportIdType transport_id = next_transport_id();
  ACE_TString transport_type(DEFAULT_TRANSPORT);

  if (init_transport(transport_id, transport_type) != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("ERROR: %N:%l: create_transport() -")
               ACE_TEXT(" init_transport failed!\n")));
    ACE_OS::exit(-1);
  }

  TheTransportFactory->get_or_create_configuration(transport_id, transport_type);
  return TheTransportFactory->create_transport_impl(transport_id);
}

OpenDDS::DCPS::TransportIdType
TestBase::next_transport_id()
{
  static ACE_Atomic_Op<ACE_SYNCH_MUTEX, long> transport_ids(1);
  return transport_ids++;
}

template<typename Writer>
TestPublisher<Writer>::TestPublisher()
{
}

template<typename Writer>
TestPublisher<Writer>::~TestPublisher()
{
}

template<typename Writer>
DDS::ReturnCode_t
TestPublisher<Writer>::init_publisher(DDS::PublisherQos& /*qos*/,
                                      DDS::PublisherListener_ptr& /*listener*/,
                                      DDS::StatusMask& /*status*/)
{
  return DDS::RETCODE_OK;
}

template<typename Writer>
DDS::ReturnCode_t
TestPublisher<Writer>::init_datawriter(DDS::DataWriterQos& /*qos*/,
                                       DDS::DataWriterListener_ptr& /*listener*/,
                                       DDS::StatusMask& /*status*/)
{
  return DDS::RETCODE_OK;
}

template<typename Writer>
void
TestPublisher<Writer>::wait_for_acknowledgments(DDS::Duration_t timeout)
{
  if(this->writer_->wait_for_acknowledgments(timeout) != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("ERROR: %N:%l: wait_for_acknowledgments() -")
               ACE_TEXT(" timed out!\n")));
    ACE_OS::exit(-1);
  }
}

template<typename Writer>
void
TestPublisher<Writer>::wait_for_subscribers(size_t count,
                                            DDS::Duration_t timeout)
{
    DDS::StatusCondition_var condition = this->writer_->get_statuscondition();
    condition->set_enabled_statuses(DDS::PUBLICATION_MATCHED_STATUS);

    DDS::WaitSet_var ws = new DDS::WaitSet;
    ws->attach_condition(condition);

    DDS::ConditionSeq conditions;
    DDS::PublicationMatchedStatus matches = { 0, 0, 0, 0, 0 };
    do {
      if (ws->wait(conditions, timeout) != DDS::RETCODE_OK) {
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("ERROR: %N:%l: wait_for_subscribers() -")
                   ACE_TEXT(" wait failed!\n")));
        ACE_OS::exit(-1);
      }

      if (this->writer_->get_publication_matched_status(matches) != DDS::RETCODE_OK) {
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("ERROR: %N:%l: wait_for_subscribers() -")
                   ACE_TEXT(" get_publication_matched_status failed!\n")));
        ACE_OS::exit(-1);
      }

    } while (matches.current_count < count);

    ws->detach_condition(condition);

    if (matches.current_count != count) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("ERROR: %N:%l: wait_for_subscribers() -")
                 ACE_TEXT(" timed out!\n")));
      ACE_OS::exit(-1);
    }
}

template<typename Writer>
void
TestPublisher<Writer>::init_i()
ACE_THROW_SPEC((CORBA::SystemException))
{
  this->publisher_ = create_publisher();
  this->transport_ = create_transport();

  // Attach transport to Publisher
  if (this->transport_->attach(this->publisher_.in()) != OpenDDS::DCPS::ATTACH_OK) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("ERROR: %N:%l: init_i() -")
               ACE_TEXT(" attach failed!\n")));
    ACE_OS::exit(-1);
  }

  this->writer_ = create_datawriter();

  this->writer_i_ = Writer::_narrow(this->writer_);
  if (CORBA::is_nil(this->writer_i_)) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("ERROR: %N:%l: init_i() -")
               ACE_TEXT(" _narrow failed!\n")));
    ACE_OS::exit(-1);
  }
}

template<typename Writer>
void
TestPublisher<Writer>::fini_i()
ACE_THROW_SPEC((CORBA::SystemException))
{
}

template<typename Writer>
DDS::Publisher_var
TestPublisher<Writer>::create_publisher()
ACE_THROW_SPEC((CORBA::SystemException))
{
  DDS::PublisherQos qos;
  if (this->participant_->get_default_publisher_qos(qos) != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("ERROR: %N:%l: create_publisher() -")
               ACE_TEXT(" get_default_publisher_qos failed!\n")));
    ACE_OS::exit(-1);
  }

  DDS::PublisherListener_ptr listener =
    DDS::PublisherListener::_nil();

  DDS::StatusMask status = OpenDDS::DCPS::DEFAULT_STATUS_MASK;

  if (init_publisher(qos, listener, status) != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("ERROR: %N:%l: create_publisher() -")
               ACE_TEXT(" init_publisher failed!\n")));
    ACE_OS::exit(-1);
  }

  DDS::Publisher_var publisher =
    this->participant_->create_publisher(qos, listener, status);

  if (CORBA::is_nil(publisher.in())) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("ERROR: %N:%l: create_publisher() -")
               ACE_TEXT(" create_publisher failed!\n")));
    ACE_OS::exit(-1);
  }

  return publisher;
}

template<typename Writer>
DDS::DataWriter_var
TestPublisher<Writer>::create_datawriter()
ACE_THROW_SPEC((CORBA::SystemException))
{
  DDS::DataWriterQos qos;
  if (this->publisher_->get_default_datawriter_qos(qos) != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("ERROR: %N:%l: create_datawriter() -")
               ACE_TEXT(" get_default_datawriter_qos failed!\n")));
    ACE_OS::exit(-1);
  }

  DDS::DataWriterListener_ptr listener =
    DDS::DataWriterListener::_nil();

  DDS::StatusMask status = OpenDDS::DCPS::DEFAULT_STATUS_MASK;

  if (init_datawriter(qos, listener, status) != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("ERROR: %N:%l: create_datawriter() -")
               ACE_TEXT(" init_datawriter failed!\n")));
    ACE_OS::exit(-1);
  }

  DDS::DataWriter_var writer =
    this->publisher_->create_datawriter(this->topic_.in(), qos, listener, status);

  if (CORBA::is_nil(writer.in())) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("ERROR: %N:%l: create_datawriter() -")
               ACE_TEXT(" create_datawriter failed!\n")));
    ACE_OS::exit(-1);
  }

  return writer;
}

template<typename Reader>
TestSubscriber<Reader>::TestSubscriber()
{
}

template<typename Reader>
TestSubscriber<Reader>::~TestSubscriber()
{
}

template<typename Reader>
DDS::ReturnCode_t
TestSubscriber<Reader>::init_subscriber(DDS::SubscriberQos& /*qos*/,
                                        DDS::SubscriberListener_ptr& /*listener*/,
                                        DDS::StatusMask& /*status*/)
{
  return DDS::RETCODE_OK;
}

template<typename Reader>
DDS::ReturnCode_t
TestSubscriber<Reader>::init_datareader(DDS::DataReaderQos& /*qos*/,
                                        DDS::DataReaderListener_ptr& /*listener*/,
                                        DDS::StatusMask& /*status*/)
{
  return DDS::RETCODE_OK;
}

template<typename Reader>
void
TestSubscriber<Reader>::init_i()
ACE_THROW_SPEC((CORBA::SystemException))
{
  this->subscriber_ = create_subscriber();
  this->transport_ = create_transport();

  // Attach transport to Subscriber
  if (this->transport_->attach(this->subscriber_.in()) != OpenDDS::DCPS::ATTACH_OK) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("ERROR: %N:%l: init_i() -")
               ACE_TEXT(" attach failed!\n")));
    ACE_OS::exit(-1);
  }

  this->reader_ = create_datareader();

  this->reader_i_ = Reader::_narrow(this->reader_);
  if (CORBA::is_nil(this->reader_i_)) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("ERROR: %N:%l: init_i() -")
               ACE_TEXT(" _narrow failed!\n")));
    ACE_OS::exit(-1);
  }
}

template<typename Reader>
void
TestSubscriber<Reader>::fini_i()
ACE_THROW_SPEC((CORBA::SystemException))
{
}

template<typename Reader>
DDS::Subscriber_var
TestSubscriber<Reader>::create_subscriber()
ACE_THROW_SPEC((CORBA::SystemException))
{
  DDS::SubscriberQos qos;
  if (this->participant_->get_default_subscriber_qos(qos) != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("ERROR: %N:%l: create_subscriber() -")
               ACE_TEXT(" get_default_subscriber_qos failed!\n")));
    ACE_OS::exit(-1);
  }

  DDS::SubscriberListener_ptr listener =
    DDS::SubscriberListener::_nil();

  DDS::StatusMask status = OpenDDS::DCPS::DEFAULT_STATUS_MASK;

  if (init_subscriber(qos, listener, status) != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("ERROR: %N:%l: create_subscriber() -")
               ACE_TEXT(" init_subscriber failed!\n")));
    ACE_OS::exit(-1);
  }

  DDS::Subscriber_var subscriber =
    this->participant_->create_subscriber(qos, listener, status);

  if (CORBA::is_nil(subscriber.in())) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("ERROR: %N:%l: create_subscriber() -")
               ACE_TEXT(" create_subscriber failed!\n")));
    ACE_OS::exit(-1);
  }

  return subscriber;
}

template<typename Reader>
DDS::DataReader_var
TestSubscriber<Reader>::create_datareader()
ACE_THROW_SPEC((CORBA::SystemException))
{
  DDS::DataReaderQos qos;
  if (this->subscriber_->get_default_datareader_qos(qos) != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("ERROR: %N:%l: create_datareader() -")
               ACE_TEXT(" get_default_datareader_qos failed!\n")));
    ACE_OS::exit(-1);
  }

  DDS::DataReaderListener_ptr listener =
    DDS::DataReaderListener::_nil();

  DDS::StatusMask status = OpenDDS::DCPS::DEFAULT_STATUS_MASK;

  if (init_datareader(qos, listener, status) != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("ERROR: %N:%l: create_datareader() -")
               ACE_TEXT(" init_datareader failed!\n")));
    ACE_OS::exit(-1);
  }

  DDS::DataReader_var reader =
    this->subscriber_->create_datareader(this->topic_.in(), qos, listener, status);

  if (CORBA::is_nil(reader.in())) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("ERROR: %N:%l: create_datareader() -")
               ACE_TEXT(" create_datareader failed!\n")));
    ACE_OS::exit(-1);
  }

  return reader;
}

template<typename Reader>
void
TestSubscriber<Reader>::wait_for_publishers(size_t count,
                                            DDS::Duration_t timeout)
{
  DDS::StatusCondition_var condition = this->reader_->get_statuscondition();
  condition->set_enabled_statuses(DDS::SUBSCRIPTION_MATCHED_STATUS);

  DDS::WaitSet_var ws = new DDS::WaitSet;
  ws->attach_condition(condition);

  DDS::ConditionSeq conditions;
  DDS::SubscriptionMatchedStatus matches = { 0, 0, 0, 0, 0 };
  do {
    if (ws->wait(conditions, timeout) != DDS::RETCODE_OK) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("ERROR: %N:%l: wait_for_publishers() -")
                 ACE_TEXT(" wait failed!\n")));
      ACE_OS::exit(-1);
    }

    if (this->reader_->get_subscription_matched_status(matches) != ::DDS::RETCODE_OK) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("ERROR: %N:%l: wait_for_publishers() -")
                 ACE_TEXT(" get_subscription_matched_status failed!\n")));
      ACE_OS::exit(-1);
    }

  } while (matches.current_count > 0 || matches.total_count < count);

  ws->detach_condition(condition);

  if (matches.current_count != 0) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("ERROR: %N:%l: wait_for_publishers() -")
               ACE_TEXT(" timed out!\n")));
    ACE_OS::exit(-1);
  }
}

template<typename Reader, typename Writer>
TestPair<Reader, Writer>::TestPair()
{
}

template<typename Reader, typename Writer>
TestPair<Reader, Writer>::~TestPair()
{
}

template<typename Reader, typename Writer>
void
TestPair<Reader, Writer>::init_i()
ACE_THROW_SPEC((CORBA::SystemException))
{
  TestSubscriber<Reader>::init_i();
  TestPublisher<Writer>::init_i();
}

template<typename Reader, typename Writer>
void
TestPair<Reader, Writer>::fini_i()
ACE_THROW_SPEC((CORBA::SystemException))
{
  TestPublisher<Writer>::fini_i();
  TestSubscriber<Reader>::fini_i();
}
