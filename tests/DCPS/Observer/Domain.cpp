#include "Domain.h"
#include "TestObserver.h"
#include <dds/DCPS/Observer.h>
#include <dds/DCPS/EntityImpl.h>

#include <tests/DCPS/ConsolidatedMessengerIdl/MessengerTypeSupportImpl.h>
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include <iostream>

const char* Domain::TEST_TOPIC = "TestObserver Topic";
const char* Domain::TEST_TOPIC_TYPE  = "TestObserver Type";

Domain::Domain(int argc, ACE_TCHAR* argv[], const std::string& app_mame) : appName(app_mame) {
  try {
    dpf = TheParticipantFactoryWithArgs(argc, argv);
    if (CORBA::is_nil(dpf.in())) {
      throw ACE_TEXT("Domain Participant Factory not found.");
    }

    participant = dpf->create_participant(ID, PARTICIPANT_QOS_DEFAULT,
      DDS::DomainParticipantListener::_nil(), OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (CORBA::is_nil(participant.in())) {
      throw ACE_TEXT("create_participant failed.");
    }

    Messenger::MessageTypeSupport_var ts(new Messenger::MessageTypeSupportImpl);
    if (ts->register_type(participant.in(), TEST_TOPIC_TYPE) != DDS::RETCODE_OK) {
      throw ACE_TEXT("register_type failed.");
    }

    topic = participant->create_topic(TEST_TOPIC, TEST_TOPIC_TYPE,
      TOPIC_QOS_DEFAULT, DDS::TopicListener::_nil(), OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (CORBA::is_nil(topic.in())) {
      throw ACE_TEXT("create_topic failed.");
    }

    auto entity = dynamic_cast<OpenDDS::DCPS::EntityImpl*>(participant.ptr());
  //entity->set_observer(OpenDDS::DCPS::make_rch<TestObserver>(), OpenDDS::DCPS::Observer::e_ALL);
    entity->set_observer(OpenDDS::DCPS::make_rch<TestObserver>(),
        OpenDDS::DCPS::Observer::e_ENABLED | OpenDDS::DCPS::Observer::e_DELETED
      | OpenDDS::DCPS::Observer::e_ASSOCIATED | OpenDDS::DCPS::Observer::e_DISASSOCIATED
      | OpenDDS::DCPS::Observer::e_QOS_CHANGED | OpenDDS::DCPS::Observer::e_ASSOCIATED_QOS_CHANGED
      );
  } catch (const CORBA::Exception& e) {
    ACE_ERROR((LM_ERROR, "CORBA::Exception: %C\n", e._info().c_str()));
    cleanup();
    throw;
  } catch (...) {
    cleanup();
    throw;
  }
}

Domain::~Domain() {
  cleanup();
}

void Domain::cleanup() {
  std::cout << appName << " cleanup" << std::endl;
  if(!CORBA::is_nil(dpf.in())){
    if(!CORBA::is_nil(participant.in())){
      std::cout << appName << " delete_contained_entities" << std::endl;
      participant->delete_contained_entities();
      std::cout << appName << " delete_participant" << std::endl;
      dpf->delete_participant(participant);
    }
    std::cout << appName << " TheServiceParticipant.shutdown" << std::endl;
    TheServiceParticipant->shutdown();
  }
  participant = 0;
  dpf = 0;
  std::cout << appName << " cleanup done" << std::endl;
}
