#include "Domain.h"
#include <dds/DCPS/Service_Participant.h>
#include "TestMsgTypeSupportImpl.h"
#include <dds/DCPS/Marked_Default_Qos.h>
#include <iostream>

const char* Domain::TESTMSG_TYPE  = "TestReliableBestEffortReaders Type";
const char* Domain::TESTMSG_TOPIC = "TestReliableBestEffortReaders Topic";

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

    TestMsgTypeSupport_var ts = new TestMsgTypeSupportImpl();
    if (ts->register_type(participant.in(), TESTMSG_TYPE) != DDS::RETCODE_OK) {
      throw ACE_TEXT("register_type failed.");
    }

    topic = participant->create_topic(TESTMSG_TOPIC, TESTMSG_TYPE,
      TOPIC_QOS_DEFAULT, DDS::TopicListener::_nil(), OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (CORBA::is_nil(topic.in())) {
      throw ACE_TEXT("create_topic failed.");
    }
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
      //std::cerr << "delete_contained_entities" << std::endl;
      participant->delete_contained_entities();
      //std::cerr << "delete_participant" << std::endl;
      dpf->delete_participant(participant);
    }
    //std::cerr << "TheServiceParticipant.shutdown" << std::endl;
    TheServiceParticipant->shutdown();
  }
  participant = 0;
  dpf = 0;
}
