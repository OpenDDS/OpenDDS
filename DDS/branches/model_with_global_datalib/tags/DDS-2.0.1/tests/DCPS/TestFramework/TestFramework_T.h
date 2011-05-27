/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef DCPS_TESTFRAMEWORK_T_H
#define DCPS_TESTFRAMEWORK_T_H

#include "TestFramework.h"

template<typename Writer>
class TestPublisher : public virtual TestBase {
public:
  typedef typename Writer::_ptr_type Writer_ptr;
  typedef typename Writer::_var_type Writer_var;

  TestPublisher();
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
    DDS::Duration_t timeout = DEFAULT_TIMEOUT);

  void wait_for_subscribers(
    CORBA::Long count = 1,
    DDS::Duration_t timeout = DEFAULT_TIMEOUT);

protected:
  DDS::Publisher_var publisher_;
  OpenDDS::DCPS::TransportImpl_rch transport_;

  DDS::DataWriter_var writer_;
  Writer_ptr writer_i_;

  virtual void init_i()
  ACE_THROW_SPEC((CORBA::SystemException));

  virtual void fini_i()
  ACE_THROW_SPEC((CORBA::SystemException));

  DDS::Publisher_var create_publisher()
  ACE_THROW_SPEC((CORBA::SystemException));

  DDS::DataWriter_var create_datawriter()
  ACE_THROW_SPEC((CORBA::SystemException));
};

template<typename Reader>
class TestSubscriber : public virtual TestBase {
public:
  typedef typename Reader::_ptr_type Reader_ptr;
  typedef typename Reader::_var_type Reader_var;

  TestSubscriber();
  virtual ~TestSubscriber();

  virtual DDS::ReturnCode_t init_subscriber(
    DDS::SubscriberQos& qos,
    DDS::SubscriberListener_ptr& listener,
    DDS::StatusMask& status);

  virtual DDS::ReturnCode_t init_datareader(
    DDS::DataReaderQos& qos,
    DDS::DataReaderListener_ptr& listener,
    DDS::StatusMask& status);

  void wait_for_publishers(
    CORBA::Long count = 1,
    DDS::Duration_t timeout = DEFAULT_TIMEOUT);

protected:
  DDS::Subscriber_var subscriber_;
  OpenDDS::DCPS::TransportImpl_rch transport_;

  DDS::DataReader_var reader_;
  Reader_ptr reader_i_;

  virtual void init_i()
  ACE_THROW_SPEC((CORBA::SystemException));

  virtual void fini_i()
  ACE_THROW_SPEC((CORBA::SystemException));

  DDS::Subscriber_var create_subscriber()
  ACE_THROW_SPEC((CORBA::SystemException));

  DDS::DataReader_var create_datareader()
  ACE_THROW_SPEC((CORBA::SystemException));
};

template<typename Reader, typename Writer>
class TestPair : public virtual TestSubscriber<Reader>,
                 public virtual TestPublisher<Writer> {
public:
  TestPair();
  virtual ~TestPair();

protected:
  virtual void init_i()
  ACE_THROW_SPEC((CORBA::SystemException));

  virtual void fini_i()
  ACE_THROW_SPEC((CORBA::SystemException));
};

#if defined (ACE_TEMPLATES_REQUIRE_SOURCE)
#include "TestFramework_T.cpp"
#endif

#if defined (ACE_TEMPLATES_REQUIRE_PRAGMA)
#pragma message("TestFramework_T.cpp template inst")
#pragma implementation("TestFramework_T.cpp")
#endif

#endif  /* DCPS_TESTFRAMEWORK_T_H */
