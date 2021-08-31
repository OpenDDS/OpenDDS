#include "Domain.h"

#include <dds/DCPS/Marked_Default_Qos.h>

#include <tests/DCPS/ConsolidatedMessengerIdl/MessengerTypeSupportImpl.h>

#include <stdexcept>

const int Domain::s_id = 11;
const char* Domain::s_topic = "DeadlineTestTopic";
const char* Domain::s_topic_type  = "DeadlineTestTopicType";
const int Domain::s_key1 = 11;
const int Domain::s_key2 = 13;
const int Domain::n_msg = 10;
const int Domain::n_instance = 2;
const int Domain::n_expiration = 2;
const DDS::Duration_t Domain::w_deadline = {4, 0}; //writer deadline period 4 second (required .sec > 1)
const DDS::Duration_t Domain::r_deadline = {5, 0}; //reader deadline period 5 second (required .sec > 1)
const OpenDDS::DCPS::TimeDuration Domain::w_sleep(w_deadline.sec * n_expiration + 1);
const OpenDDS::DCPS::TimeDuration Domain::r_sleep(r_deadline.sec * n_expiration + 1);
const OpenDDS::DCPS::TimeDuration Domain::w_interval(0, 500000);

Domain::Domain(int argc, ACE_TCHAR* argv[], const std::string& app_name)
  : app_name_(app_name)
  , dpf_()
  , dp_()
  , topic_()
{
  try {
    dpf_ = TheParticipantFactoryWithArgs(argc, argv);
    if (!dpf_) {
      throw std::runtime_error("TheParticipantFactoryWithArgs is null.");
    }

    dp_ = dpf_->create_participant(s_id, PARTICIPANT_QOS_DEFAULT, 0, OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (!dp_) {
      throw std::runtime_error("create_participant failed.");
    }

    Messenger::MessageTypeSupport_var ts(new Messenger::MessageTypeSupportImpl);
    if (ts->register_type(dp_.in(), s_topic_type) != DDS::RETCODE_OK) {
      throw std::runtime_error("register_type failed.");
    }

    DDS::TopicQos topic_qos;
    dp_->get_default_topic_qos(topic_qos);
    topic_ = dp_->create_topic(s_topic, s_topic_type, topic_qos, 0, OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (!topic_) {
      throw std::runtime_error("create_topic failed.");
    }
  } catch (...) {
    ACE_ERROR((LM_ERROR, (app_name_ + " Domain ERROR: ...\n").c_str()));
    cleanup();
    throw;
  }
}

void Domain::cleanup()
{
  ACE_DEBUG((LM_INFO, (app_name_ + " Domain::cleanup\n").c_str()));
  if (dp_) {
    ACE_DEBUG((LM_INFO, (app_name_ + " Domain delete_contained_entities\n").c_str()));
    dp_->delete_contained_entities();
    if (dpf_) {
      ACE_DEBUG((LM_INFO, (app_name_ + " Domain delete_participant\n").c_str()));
      dpf_->delete_participant(dp_.in());
      dpf_ = 0;
    }
    dp_ = 0;
  }
}
