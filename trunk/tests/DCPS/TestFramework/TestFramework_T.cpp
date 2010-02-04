/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef DCPS_TESTFRAMEWORK_T_CPP
#define DCPS_TESTFRAMEWORK_T_CPP

#include <dds/DCPS/WaitSet.h>

#include "TestFramework_T.h"

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
TestPublisher<Writer>::wait_for_subscribers(CORBA::Long count,
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
  this->transport_ = find_or_create_transport();

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
  this->transport_ = find_or_create_transport();

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
TestSubscriber<Reader>::wait_for_publishers(CORBA::Long count,
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

#endif  /* DCPS_TESTFRAMEWORK_T_CPP */
