/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <dds/DCPS/Service_Participant.h>

#include "TestFramework.h"

const DDS::DomainId_t TestBase::DEFAULT_DOMAIN = 42;
const char*           TestBase::DEFAULT_TOPIC = "TestFramework";
const DDS::Duration_t TestBase::DEFAULT_TIMEOUT = { 30, 0 }; // 30 seconds

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


int
TestBase::run(int& argc, ACE_TCHAR* argv[])
{
  DDS::DomainParticipantFactory_var dpf =
    TheParticipantFactoryWithArgs(argc, argv);

  int error(0);
  try {
    init(argc, argv);
    error = test();
    fini();

  } catch (const CORBA::SystemException& e) {
    e._tao_print_exception("ERROR: Caught in run() -");
    return -1;
  }

  TheServiceParticipant->shutdown();

  return error;
}

void
TestBase::init(int argc, ACE_TCHAR* argv[])
{
  this->participant_ = create_participant();
  this->topic_ = create_topic();

  init_i(argc, argv);  // delegate to child
}

void
TestBase::fini()
{
  fini_i();  // delegate to child

  this->participant_->delete_contained_entities();

  DDS::DomainParticipantFactory_var dpf = TheParticipantFactory;
  dpf->delete_participant(this->participant_);
}

DDS::DomainParticipant_var
TestBase::create_participant()
{
  DDS::DomainId_t domain = DEFAULT_DOMAIN;

  DDS::DomainParticipantQos qos;
  DDS::DomainParticipantFactory_var dpf = TheParticipantFactory;
  if (dpf->get_default_participant_qos(qos) != DDS::RETCODE_OK) {
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
    dpf->create_participant(domain, qos, listener, status);

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

