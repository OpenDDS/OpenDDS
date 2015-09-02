/*
 *
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

#include "TestFrameworkC.h"
#include "TestFrameworkTypeSupportC.h"
#include "TestFrameworkTypeSupportImpl.h"

#include "TestFramework_Export.h"

class TestFramework_Export TestBase {
public:
  static const DDS::DomainId_t DEFAULT_DOMAIN;
  static const char* DEFAULT_TOPIC;
  static const DDS::Duration_t DEFAULT_TIMEOUT;

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

  virtual DDS::ReturnCode_t init_publisher(
    DDS::PublisherQos& /*qos*/,
    DDS::PublisherListener_ptr& /*listener*/,
    DDS::StatusMask& /*status*/)
  {
    return DDS::RETCODE_OK;
  }

  virtual DDS::ReturnCode_t init_datawriter(
    DDS::DataWriterQos& /*qos*/,
    DDS::DataWriterListener_ptr& /*listener*/,
    DDS::StatusMask& /*status*/)
  {
    return DDS::RETCODE_OK;
  }

  virtual DDS::ReturnCode_t init_subscriber(
    DDS::SubscriberQos& /*qos*/,
    DDS::SubscriberListener_ptr& /*listener*/,
    DDS::StatusMask& /*status*/)
  {
    return DDS::RETCODE_OK;
  }

  virtual DDS::ReturnCode_t init_datareader(
    DDS::DataReaderQos& /*qos*/,
    DDS::DataReaderListener_ptr& /*listener*/,
    DDS::StatusMask& /*status*/)
  {
    return DDS::RETCODE_OK;
  }

  DDS::DomainParticipant_var& get_participant() { return participant_; }
  DDS::Topic_var& get_topic() { return topic_; }

  int run(int &argc, ACE_TCHAR* arg[]);

  virtual int test() = 0;

protected:
  DDS::DomainParticipant_var participant_;
  DDS::Topic_var topic_;

  TestBase();

  void init(int argc, ACE_TCHAR* argv[]);

  void fini();

  virtual void init_i(int /*argc*/, ACE_TCHAR*[] /*argv*/) {}

  virtual void fini_i() = 0;

  DDS::DomainParticipant_var create_participant();

  DDS::Topic_var create_topic();
};

#endif  /* DCPS_TESTFRAMEWORK_H */
