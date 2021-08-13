#include "Domain.h"
#include <tests/DCPS/ConsolidatedMessengerIdl/MessengerTypeSupportImpl.h>
#include <dds/DCPS/Marked_Default_Qos.h>

const char* Domain::sTopic = "DeadlineTestTopic";
const char* Domain::sTopicType  = "DeadlineTestTopicType";
const DDS::Duration_t Domain::W_Deadline = {4, 0}; //writer deadline period 4 second (required .sec > 1)
const DDS::Duration_t Domain::R_Deadline = {5, 0}; //reader deadline period 5 second (required .sec > 1)
const OpenDDS::DCPS::TimeDuration Domain::W_Sleep(W_Deadline.sec * N_Expiration + 1);
const OpenDDS::DCPS::TimeDuration Domain::R_Sleep(R_Deadline.sec * N_Expiration + 1);

Domain::Domain(int argc, ACE_TCHAR* argv[], const std::string& app_mame)
  : app_mame_(app_mame)
  , dpf_()
  , dp_()
  , topic_()
{
  try {
    dpf_ = TheParticipantFactoryWithArgs(argc, argv);
    if (!dpf_) {
      throw std::runtime_error("TheParticipantFactoryWithArgs is null.");
    }

    dp_ = dpf_->create_participant(sID, PARTICIPANT_QOS_DEFAULT, 0, OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (!dp_) {
      throw std::runtime_error("create_participant failed.");
    }

    Messenger::MessageTypeSupport_var ts(new Messenger::MessageTypeSupportImpl);
    if (ts->register_type(dp_.in(), sTopicType) != DDS::RETCODE_OK) {
      throw std::runtime_error("register_type failed.");
    }

    DDS::TopicQos topic_qos;
    dp_->get_default_topic_qos(topic_qos);
    topic_ = dp_->create_topic(sTopic, sTopicType, topic_qos, 0, OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (!topic_) {
      throw std::runtime_error("create_topic failed.");
    }
  } catch (...) {
    ACE_ERROR((LM_ERROR, (app_mame_ + " Domain ERROR: ...\n").c_str()));
    cleanup();
    throw;
  }
}

void Domain::cleanup()
{
  ACE_DEBUG((LM_INFO, (app_mame_ + " Domain::cleanup\n").c_str()));
  if (dp_) {
    ACE_DEBUG((LM_INFO, (app_mame_ + " Domain delete_contained_entities\n").c_str()));
    dp_->delete_contained_entities();
    if (dpf_) {
      ACE_DEBUG((LM_INFO, (app_mame_ + " Domain delete_participant\n").c_str()));
      dpf_->delete_participant(dp_.in());
      dpf_ = 0;
    }
    dp_ = 0;
  }
}
