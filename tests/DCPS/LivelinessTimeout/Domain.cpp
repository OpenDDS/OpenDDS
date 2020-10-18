// -*- C++ -*-
//
#include "Domain.h"

#include <dds/DdsDcpsInfrastructureC.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/transport/framework/EntryExit.h>
#include <tests/DCPS/FooType4/FooDefTypeSupportImpl.h>

#include <ace/Arg_Shifter.h>

#include <iostream>

const char* Domain::TOPIC = "foo";
const char* Domain::TOPIC_TYPE  = "foo";

Domain::Domain(int argc, ACE_TCHAR* argv[], const std::string& app_mame)
  : app_name_(app_mame)
  , lease_duration_sec_(1)
  , test_duration_sec_(40, 0)
  , threshold_liveliness_lost_(1000)
{
  try {
    dpf_ = TheParticipantFactoryWithArgs(argc, argv);
    if (!dpf_) {
      throw ACE_TEXT("(%P|%t) Domain Participant Factory not found.");
    }
    parse_args(argc, argv);

    participant_ = dpf_->create_participant(ID, PARTICIPANT_QOS_DEFAULT,
      DDS::DomainParticipantListener::_nil(), OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (!participant_) {
      throw ACE_TEXT("(%P|%t) create_participant failed.");
    }

    Xyz::FooTypeSupport_var ts(new Xyz::FooTypeSupportImpl);
    if (ts->register_type(participant_.in(), TOPIC_TYPE) != DDS::RETCODE_OK) {
      throw ACE_TEXT("(%P|%t) register_type failed.");
    }

    topic_ = participant_->create_topic(TOPIC, TOPIC_TYPE, TOPIC_QOS_DEFAULT,
      DDS::TopicListener::_nil(), OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (!topic_) {
      throw ACE_TEXT("(%P|%t) create_topic failed.");
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

void Domain::cleanup()
{
  ACE_DEBUG((LM_INFO, ACE_TEXT("%C (%P) cleanup\n"), app_name_.c_str()));
  if (dpf_) {
    if (participant_) {
      participant_->delete_contained_entities();
      dpf_->delete_participant(participant_);
      participant_ = 0;
    }
    TheServiceParticipant->shutdown();
    dpf_ = 0;
  }
}

void Domain::parse_args(int argc, ACE_TCHAR* argv[])
{
  u_long mask = ACE_LOG_MSG->priority_mask(ACE_Log_Msg::PROCESS);
  ACE_LOG_MSG->priority_mask(mask | LM_TRACE | LM_DEBUG, ACE_Log_Msg::PROCESS);
  ACE_Arg_Shifter shifter(argc, argv);
  while (shifter.is_anything_left()) {
    // options:
    //  -l lease duration              defaults to 10
    //  -x test duration in sec        defaults to 40
    //  -t threshold liveliness lost   defaults to 1000
    //  -z                             verbose transport debug
    const ACE_TCHAR* arg = 0;
    if ((arg = shifter.get_the_parameter(ACE_TEXT("-l"))) != 0) {
      lease_duration_sec_ = ACE_OS::atoi(arg);
      shifter.consume_arg();
    }
    else if ((arg = shifter.get_the_parameter(ACE_TEXT("-x"))) != 0) {
      test_duration_sec_.set(ACE_OS::atof(arg));
      shifter.consume_arg();
    }
    else if ((arg = shifter.get_the_parameter(ACE_TEXT("-t"))) != 0) {
      threshold_liveliness_lost_ = ACE_OS::atoi(arg);
      shifter.consume_arg();
    }
    else if (shifter.cur_arg_strncasecmp(ACE_TEXT("-z")) == 0) {
      TURN_ON_VERBOSE_DEBUG;
      shifter.consume_arg();
    }
    else {
      shifter.ignore_arg();
    }
  }
}
