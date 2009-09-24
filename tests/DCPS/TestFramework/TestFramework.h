/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef DCPS_TESTFRAMEWORK_H
#define DCPS_TESTFRAMEWORK_H

#include <ace/Atomic_Op.h>
#include <ace/Basic_Types.h>

#include <tao/SystemException.h>

#include <dds/DdsDcpsInfrastructureC.h>
#include <dds/DdsDcpsDomainC.h>
#include <dds/DdsDcpsTopicC.h>
#include <dds/DdsDcpsSubscriptionC.h>
#include <dds/DdsDcpsPublicationC.h>

#include <dds/DCPS/transport/framework/TheTransportFactory.h>

#include "TestFrameworkC.h"
#include "TestFrameworkTypeSupportC.h"
#include "TestFrameworkTypeSupportImpl.h"

#include "TestFramework_Export.h"

class TestFramework_Export TestBase {
public:
  static const DDS::DomainId_t DEFAULT_DOMAIN;
  static const char* DEFAULT_TOPIC;
  static const DDS::Duration_t DEFAULT_TIMEOUT;
  static const ACE_TCHAR* DEFAULT_TRANSPORT;

  virtual ~TestBase();

  virtual DDS::ReturnCode_t init_participant(
    DDS::DomainId_t& domain,
    DDS::DomainParticipantQos& qos,
    DDS::DomainParticipantListener_ptr& listener,
    DDS::StatusMask& status);

  virtual DDS::ReturnCode_t init_topic(
    const char*& name,
    const char*& type_name,
    DDS::TopicQos& qos,
    DDS::TopicListener_ptr& listener,
    DDS::StatusMask& status);

  virtual DDS::ReturnCode_t init_transport(
    OpenDDS::DCPS::TransportIdType& transport_id,
    ACE_TString& transport_type);

  int run(int &argc, ACE_TCHAR* arg[]);

  virtual int test() = 0;

protected:
  DDS::DomainParticipant_var participant_;
  DDS::Topic_var topic_;

  TestBase();

  void init()
  ACE_THROW_SPEC((CORBA::SystemException));

  void fini()
  ACE_THROW_SPEC((CORBA::SystemException));

  virtual void init_i()
  ACE_THROW_SPEC((CORBA::SystemException)) = 0;

  virtual void fini_i()
  ACE_THROW_SPEC((CORBA::SystemException)) = 0;

  DDS::DomainParticipant_var create_participant()
  ACE_THROW_SPEC((CORBA::SystemException));

  DDS::Topic_var create_topic()
  ACE_THROW_SPEC((CORBA::SystemException));

  OpenDDS::DCPS::TransportImpl_rch create_transport()
  ACE_THROW_SPEC((CORBA::SystemException));

  static OpenDDS::DCPS::TransportIdType next_transport_id();
};

template<typename Writer>
class TestFramework_Export TestPublisher : public virtual TestBase {
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
    size_t count = 1,
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
class TestFramework_Export TestSubscriber : public virtual TestBase {
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
    size_t count = 1,
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
class TestFramework_Export TestPair : public virtual TestSubscriber<Reader>,
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

#endif  /* DCPS_TESTFRAMEWORK_H */
