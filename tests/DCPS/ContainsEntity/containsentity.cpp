// -*- C++ -*-
// ============================================================================
/**
 *  @file   containsentity.cpp
 *
 */
// ============================================================================

#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/PublisherImpl.h>
#include <dds/DCPS/Qos_Helper.h>
#include <dds/DCPS/transport/framework/TransportType_rch.h>
#include <dds/DCPS/transport/framework/TransportRegistry.h>
#include <dds/DCPS/transport/framework/TransportConfig_rch.h>
#include <dds/DCPS/transport/framework/TransportExceptions.h>
#include "MessengerTypeSupportImpl.h"

#ifndef DDS_HAS_MINIMUM_BIT
#include <dds/DCPS/RTPS/RtpsDiscovery.h>
#endif

#ifdef ACE_AS_STATIC_LIBS
#include <dds/DCPS/transport/rtps_udp/RtpsUdp.h>
#include <dds/DCPS/transport/shmem/Shmem.h>
#include <dds/DCPS/transport/udp/Udp.h>
#include <dds/DCPS/transport/multicast/Multicast.h>
#include <dds/DCPS/RTPS/RtpsDiscovery.h>
#endif

#include "dds/DCPS/StaticIncludes.h"

#include <ace/Arg_Shifter.h>
#include <ace/streams.h>
#include "tests/Utils/ExceptionStreams.h"
#include "ace/Get_Opt.h"

#include <memory>
using namespace std;

int ACE_TMAIN (int argc, ACE_TCHAR *argv[]) {

  OPENDDS_STRING transport("rtps_udp");

  ACE_Arg_Shifter arg_shifter(argc, argv);
  while (arg_shifter.is_anything_left())
  {
    const ACE_TCHAR* current_arg = 0;
    // The '-t' option for transport
    if ((current_arg = arg_shifter.get_the_parameter(ACE_TEXT("-t")))) {
      transport = ACE_TEXT_ALWAYS_CHAR(current_arg);
      ACE_DEBUG((LM_DEBUG, "Using transport:%C\n", transport.c_str()));
      arg_shifter.consume_arg();
    }
    else {
      arg_shifter.ignore_arg();
    }
  }

  OPENDDS_STRING config_1("dds4ccm_");
  config_1 += transport + "_1";

  OPENDDS_STRING instance_1("the_");
  instance_1 += transport + "_transport_1";

  OPENDDS_STRING config_2("dds4ccm_");
  config_2 += transport + "_2";

  OPENDDS_STRING instance_2("the_");
  instance_2 += transport + "_transport_2";

  try
    {
      DDS::DomainParticipantFactory_var dpf =
        TheParticipantFactoryWithArgs(argc, argv);

      OpenDDS::DCPS::TransportConfig_rch config =
        OpenDDS::DCPS::TransportRegistry::instance()->get_config(config_1.c_str());

      if (config.is_nil())
        {
          config =
            OpenDDS::DCPS::TransportRegistry::instance()->create_config(config_1.c_str());
        }

      OpenDDS::DCPS::TransportInst_rch inst =
        OpenDDS::DCPS::TransportRegistry::instance()->get_inst(instance_1.c_str());

      if (inst.is_nil())
        {
          inst =
            OpenDDS::DCPS::TransportRegistry::instance()->create_inst(instance_1.c_str(),
                                                                      transport.c_str());

          config->instances_.push_back(inst);

          OpenDDS::DCPS::TransportRegistry::instance()->global_config(config);
        }

      // Create another transport instance for participant2 since RTPS transport instances
      // cannot be shared by domain participants.
      OpenDDS::DCPS::TransportConfig_rch config2 =
        OpenDDS::DCPS::TransportRegistry::instance()->get_config(config_2.c_str());

      if (config2.is_nil())
        {
          config2 =
            OpenDDS::DCPS::TransportRegistry::instance()->create_config(config_2.c_str());
        }

      OpenDDS::DCPS::TransportInst_rch inst2 =
        OpenDDS::DCPS::TransportRegistry::instance()->get_inst(instance_2.c_str());

      if (inst2.is_nil())
        {
          inst2 =
            OpenDDS::DCPS::TransportRegistry::instance()->create_inst(instance_2.c_str(),
                                                                      transport.c_str());
          config2->instances_.push_back(inst2);

        }

#ifndef DDS_HAS_MINIMUM_BIT
      OpenDDS::RTPS::RtpsDiscovery_rch disc =
        OpenDDS::DCPS::make_rch<OpenDDS::RTPS::RtpsDiscovery>(OpenDDS::DCPS::Discovery::DEFAULT_RTPS);

      // The recommended value for the resend period is 2 seconds for
      // the current implementation of OpenDDS.
      disc->resend_period(OpenDDS::DCPS::TimeDuration(2));

      TheServiceParticipant->add_discovery(disc);
      TheServiceParticipant->set_repo_domain(11, disc->key());
#endif
      TheServiceParticipant->set_default_discovery (OpenDDS::DCPS::Discovery::DEFAULT_RTPS);

      DDS::DomainParticipant_var participant1 =
        dpf->create_participant(11,
                                PARTICIPANT_QOS_DEFAULT,
                                DDS::DomainParticipantListener::_nil(),
                                ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      if (CORBA::is_nil (participant1.in ())) {
        cerr << "create_participant1 failed." << endl;
        return 1;
      }
      else
      {
        ACE_DEBUG ((LM_DEBUG, "Created participant 1 with instance handle %d\n",
                    participant1->get_instance_handle ()));
      }
      try
      {
        OpenDDS::DCPS::TransportRegistry::instance()->bind_config(config, participant1.in());
      }
      catch (const OpenDDS::DCPS::Transport::MiscProblem &) {
        ACE_ERROR_RETURN((LM_ERROR,
          ACE_TEXT("%N:%l: main()")
          ACE_TEXT(" ERROR: TransportRegistry::bind_config() throws")
          ACE_TEXT(" Transport::MiscProblem exception\n")),
          -1);
      }
      catch (const OpenDDS::DCPS::Transport::NotFound &) {
        ACE_ERROR_RETURN((LM_ERROR,
          ACE_TEXT("%N:%l: main()")
          ACE_TEXT(" ERROR: TransportRegistry::bind_config() throws")
          ACE_TEXT(" Transport::NotFound exception\n")),
          -1);
      }

      DDS::DomainParticipant_var participant2 =
        dpf->create_participant(11,
                                PARTICIPANT_QOS_DEFAULT,
                                DDS::DomainParticipantListener::_nil(),
                                ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      if (CORBA::is_nil (participant2.in ())) {
        cerr << "create_participant failed." << endl;
        return 1;
      }
      else
      {
        ACE_DEBUG ((LM_DEBUG, "Created participant 2 with instance handle %d\n",
                    participant2->get_instance_handle ()));
      }

      try {
        OpenDDS::DCPS::TransportRegistry::instance()->bind_config(config2, participant2.in());
      }
      catch (const OpenDDS::DCPS::Transport::MiscProblem &) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("%N:%l: main()")
                          ACE_TEXT(" ERROR: TransportRegistry::bind_config() throws")
                          ACE_TEXT(" Transport::MiscProblem exception\n")),
                          -1);
      }
      catch (const OpenDDS::DCPS::Transport::NotFound &) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("%N:%l: main()")
                          ACE_TEXT(" ERROR: TransportRegistry::bind_config() throws")
                          ACE_TEXT(" Transport::NotFound exception\n")),
                          -1);
      }

      if (participant1->get_instance_handle () == participant2->get_instance_handle ())
      {
         ACE_ERROR_RETURN((LM_ERROR,
                           ACE_TEXT("%N:%l: main()")
                           ACE_TEXT(" ERROR: participant1 and participant2 do have the same instance handle!\n")),
                           -1);
      }
      // Register TypeSupport (Messenger::Message)
      Messenger::MessageTypeSupport_var mts =
        new Messenger::MessageTypeSupportImpl();

      if (mts->register_type(participant1.in(), "") != DDS::RETCODE_OK) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("%N:%l: main()")
                          ACE_TEXT(" ERROR: register_type 1 failed!\n")),
                        -1);
      }

      if (mts->register_type(participant2.in(), "") != DDS::RETCODE_OK) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("%N:%l: main()")
                          ACE_TEXT(" ERROR: register_type 2 failed!\n")),
                        -1);
      }

      // Create Topic
      CORBA::String_var type_name = mts->get_type_name();
      DDS::Topic_var topic1 =
        participant1->create_topic("Movie Discussion List",
                                  type_name.in(),
                                  TOPIC_QOS_DEFAULT,
                                  DDS::TopicListener::_nil(),
                                  OpenDDS::DCPS::DEFAULT_STATUS_MASK);

      if (CORBA::is_nil(topic1.in())) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("%N:%l: main()")
                          ACE_TEXT(" ERROR: create_topic1 failed!\n")),
                        -1);
      }

      DDS::Topic_var topic2 =
        participant2->create_topic("Movie Discussion List",
                                  type_name.in(),
                                  TOPIC_QOS_DEFAULT,
                                  DDS::TopicListener::_nil(),
                                  OpenDDS::DCPS::DEFAULT_STATUS_MASK);

      if (CORBA::is_nil(topic2.in())) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("%N:%l: main()")
                          ACE_TEXT(" ERROR: create_topic2 failed!\n")),
                        -1);
      }

      // Now check contains_entity for topic
      if (!participant1->contains_entity(topic1->get_instance_handle()))
      {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("%N:%l: main()")
                          ACE_TEXT(" ERROR: Participant 1 reports incorrectly that topic 1 does not belong to participant 1!\n")),
                        -1);
      }
      if (!participant2->contains_entity(topic2->get_instance_handle()))
      {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("%N:%l: main()")
                          ACE_TEXT(" ERROR: Participant 2 reports incorrectly that topic 2 does not belong to participant 1!\n")),
                        -1);
      }
      if (participant1->contains_entity(topic2->get_instance_handle()))
      {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("%N:%l: main()")
                          ACE_TEXT(" ERROR: Participant 1 reports incorrectly that topic 2 does belong to participant 1!\n")),
                        -1);
      }
      if (participant2->contains_entity(topic1->get_instance_handle()))
      {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("%N:%l: main()")
                          ACE_TEXT(" ERROR: Participant 2 reports incorrectly that topic 1 does belong to participant 2!\n")),
                        -1);
      }

      // Create Publisher on participant 1
      DDS::Publisher_var pub1 =
        participant1->create_publisher(PUBLISHER_QOS_DEFAULT,
                                      DDS::PublisherListener::_nil(),
                                      OpenDDS::DCPS::DEFAULT_STATUS_MASK);

      if (CORBA::is_nil(pub1.in())) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("%N:%l: main()")
                          ACE_TEXT(" ERROR: create_publisher 1 failed!\n")),
                        -1);
      }

      // Create DataWriter on participant 1
      DDS::DataWriter_var dw1 =
        pub1->create_datawriter(topic1.in(),
                              DATAWRITER_QOS_DEFAULT,
                              DDS::DataWriterListener::_nil(),
                              OpenDDS::DCPS::DEFAULT_STATUS_MASK);

      if (CORBA::is_nil(dw1.in())) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("%N:%l: main()")
                          ACE_TEXT(" ERROR: create_datawriter 1 failed!\n")),
                        -1);
      }

      // Create Subscriber on participant 2
      DDS::Subscriber_var sub2 =
        participant2->create_subscriber(SUBSCRIBER_QOS_DEFAULT,
                                        DDS::SubscriberListener::_nil(),
                                        OpenDDS::DCPS::DEFAULT_STATUS_MASK);

      if (CORBA::is_nil(sub2.in())) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("%N:%l: main()")
                          ACE_TEXT(" ERROR: create_subscriber 2 failed!\n")),
                        -1);
      }

      // Create DataReader on participant 2
      DDS::DataReader_var dr2 =
        sub2->create_datareader(topic2.in(),
                              DATAREADER_QOS_DEFAULT,
                              DDS::DataReaderListener::_nil(),
                              OpenDDS::DCPS::DEFAULT_STATUS_MASK);

      if (CORBA::is_nil(dr2.in())) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("%N:%l: main()")
                          ACE_TEXT(" ERROR: create_datareader 2 failed!\n")),
                        -1);
      }

      // Now validate that the contains entity works correctly
      if (!participant1->contains_entity(pub1->get_instance_handle()))
      {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("%N:%l: main()")
                          ACE_TEXT(" ERROR: Participant 1 reports incorrectly that publisher 1 does not belong to participant 1!\n")),
                        -1);
      }
      if (!participant1->contains_entity(dw1->get_instance_handle()))
      {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("%N:%l: main()")
                          ACE_TEXT(" ERROR: Participant 1 reports incorrectly that datawriter 1 does not belong to participant 1!\n")),
                        -1);
      }
      if (!participant2->contains_entity(sub2->get_instance_handle()))
      {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("%N:%l: main()")
                          ACE_TEXT(" ERROR: Participant 1 reports incorrectly that subscriber 2 does not belong to participant 2!\n")),
                        -1);
      }
      if (!participant2->contains_entity(dr2->get_instance_handle()))
      {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("%N:%l: main()")
                          ACE_TEXT(" ERROR: Participant 1 reports incorrectly that datareader 2 does not belong to participant 2!\n")),
                        -1);
      }
      if (participant1->contains_entity(sub2->get_instance_handle()))
      {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("%N:%l: main()")
                          ACE_TEXT(" ERROR: Participant 1 reports incorrectly that subscriber 2 does belong to participant 1!\n")),
                        -1);
      }
      if (participant1->contains_entity(dr2->get_instance_handle()))
      {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("%N:%l: main()")
                          ACE_TEXT(" ERROR: Participant 1 reports incorrectly that datareader 2 does belong to participant 1!\n")),
                        -1);
      }
      if (participant2->contains_entity(pub1->get_instance_handle()))
      {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("%N:%l: main()")
                          ACE_TEXT(" ERROR: Participant 1 reports incorrectly that publisher 1 does belong to participant 2!\n")),
                        -1);
      }
      if (participant2->contains_entity(dw1->get_instance_handle()))
      {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("%N:%l: main()")
                          ACE_TEXT(" ERROR: Participant 2 reports incorrectly that datawriter 1 does belong to participant 2!\n")),
                        -1);
      }


      DDS::ReturnCode_t retcode2 = pub1->delete_datawriter (dw1.in ());
      if (retcode2 != DDS::RETCODE_OK) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("%N:%l: main()")
                          ACE_TEXT(" ERROR: should be able to delete datawriter\n")),
                        -1);
      }

      DDS::ReturnCode_t retcode5 = dpf->delete_participant(participant1.in ());
      if (retcode5 != DDS::RETCODE_PRECONDITION_NOT_MET) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("%N:%l: main()")
                          ACE_TEXT(" ERROR: should not be able to delete participant1\n")),
                        -1);
      }

      DDS::ReturnCode_t retcode3 = participant1->delete_publisher (pub1.in ());
      if (retcode3 != DDS::RETCODE_OK) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("%N:%l: main()")
                          ACE_TEXT(" ERROR: should be able to delete publisher\n")),
                        -1);
      }

      DDS::ReturnCode_t retcode4 = participant1->delete_topic (topic1.in ());
      if (retcode4 != DDS::RETCODE_OK) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("%N:%l: main()")
                          ACE_TEXT(" ERROR: should be able to delete topic1\n")),
                        -1);
      }

      DDS::ReturnCode_t retcode6 = dpf->delete_participant(participant1.in ());
      if (retcode6 != DDS::RETCODE_OK) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("%N:%l: main()")
                          ACE_TEXT(" ERROR: should be able to delete participant1\n")),
                        -1);
      }


      if (participant2->delete_contained_entities () != ::DDS::RETCODE_OK) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("%N:%l: main()")
                          ACE_TEXT(" ERROR: participant2 delete_contained_entities failed\n")),
                        -1);
      }

      DDS::ReturnCode_t retcode7 = dpf->delete_participant(participant2.in ());
      if (retcode7 != DDS::RETCODE_OK) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("%N:%l: main()")
                          ACE_TEXT(" ERROR: should be able to delete participant2\n")),
                        -1);
      }

      ACE_DEBUG ((LM_DEBUG, "Shutting down the service participant with one participant still registered\n"));
      TheServiceParticipant->shutdown ();
  }
  catch (CORBA::Exception& e)
  {
    cerr << "dp: Exception caught in main.cpp:" << endl
         << e << endl;
    exit(1);
  }

  return 0;
}
