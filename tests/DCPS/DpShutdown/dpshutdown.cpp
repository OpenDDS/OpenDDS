#include <MessengerTypeSupportImpl.h>
#include <tests/Utils/ExceptionStreams.h>

#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/PublisherImpl.h>
#include <dds/DCPS/Qos_Helper.h>
#include <dds/DCPS/DCPS_Utils.h>
#include <dds/DCPS/transport/framework/TransportType_rch.h>
#include <dds/DCPS/transport/framework/TransportRegistry.h>
#include <dds/DCPS/transport/framework/TransportConfig_rch.h>
#include <dds/DCPS/transport/framework/TransportExceptions.h>
#ifndef DDS_HAS_MINIMUM_BIT
#  include <dds/DCPS/RTPS/RtpsDiscovery.h>
#endif
#ifdef ACE_AS_STATIC_LIBS
#  include <dds/DCPS/transport/rtps_udp/RtpsUdp.h>
#  include <dds/DCPS/transport/shmem/Shmem.h>
#  include <dds/DCPS/transport/udp/Udp.h>
#  include <dds/DCPS/transport/tcp/Tcp.h>
#  include <dds/DCPS/transport/multicast/Multicast.h>
#  include <dds/DCPS/RTPS/RtpsDiscovery.h>
#endif

#include <ace/Arg_Shifter.h>
#include <ace/streams.h>
#include <ace/Get_Opt.h>

#include <memory>
using namespace std;
using OpenDDS::DCPS::TransportConfig_rch;
using OpenDDS::DCPS::TransportRegistry;
using OpenDDS::DCPS::String;
using OpenDDS::DCPS::retcode_to_string;

TransportConfig_rch create_transport(const String& transport, const String& suffix)
{
  const String prefix("dpshutdown_");
  const String config_name = prefix + transport + "_config_" + suffix;
  TransportConfig_rch config = TransportRegistry::instance()->get_config(config_name);
  if (!config) {
    config = TransportRegistry::instance()->create_config(config_name);
  }
  if (!config) {
    ACE_ERROR((LM_ERROR, "%N:%l ERROR: could not get TransportConfig for %C\n",
      transport.c_str()));
    return TransportConfig_rch();
  }

  const String inst_name = prefix + transport + "_inst_" + suffix;
  OpenDDS::DCPS::TransportInst_rch inst = TransportRegistry::instance()->get_inst(inst_name);
  if (!inst) {
    inst = TransportRegistry::instance()->create_inst(inst_name, transport);
    config->instances_.push_back(inst);
  }
  if (!inst) {
    ACE_ERROR((LM_ERROR, "%N:%l ERROR: could not get TransportInst for %C\n",
      transport.c_str()));
    return TransportConfig_rch();
  }

  return config;
}

int ACE_TMAIN(int argc, ACE_TCHAR* argv[])
{
  OPENDDS_STRING transport("rtps_udp");

  ACE_Arg_Shifter arg_shifter(argc, argv);
  while (arg_shifter.is_anything_left()) {
    const ACE_TCHAR* current_arg = 0;
    // The '-t' option for transport
    if ((current_arg = arg_shifter.get_the_parameter(ACE_TEXT("-t")))) {
      transport = ACE_TEXT_ALWAYS_CHAR(current_arg);
      ACE_DEBUG((LM_DEBUG, "Using transport:%C\n", transport.c_str()));
      arg_shifter.consume_arg();
    } else {
      arg_shifter.ignore_arg();
    }
  }

  try {
    DDS::DomainParticipantFactory_var dpf = TheParticipantFactoryWithArgs(argc, argv);

    TransportConfig_rch config1 = create_transport(transport, "1");
    if (!config1) {
      return 1;
    }
    TransportRegistry::instance()->global_config(config1);

    // Create another transport instance for participant2 since RTPS transport instances
    // cannot be shared by domain participants.
    TransportConfig_rch config2 = create_transport(transport, "2");
    if (!config2) {
      return 1;
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
    TheServiceParticipant->set_default_discovery(OpenDDS::DCPS::Discovery::DEFAULT_RTPS);

    DDS::DomainParticipant_var participant1 =
      dpf->create_participant(11, PARTICIPANT_QOS_DEFAULT, 0, 0);
    if (!participant1) {
      cerr << "create_participant failed." << endl;
      return 1;
    } else {
      ACE_DEBUG((LM_DEBUG, "Created participant 1 with instance handle %d\n",
        participant1->get_instance_handle()));
    }
    try {
      OpenDDS::DCPS::TransportRegistry::instance()->bind_config(config1, participant1);
    } catch (const OpenDDS::DCPS::Transport::MiscProblem&) {
      ACE_ERROR((LM_ERROR, "%N:%l: main() ERROR: TransportRegistry::bind_config() throws"
        " Transport::MiscProblem exception\n"));
      return 1;
    } catch (const OpenDDS::DCPS::Transport::NotFound&) {
      ACE_ERROR((LM_ERROR, "%N:%l: main() ERROR: "
        "TransportRegistry::bind_config() throws Transport::NotFound exception\n"));
      return 1;
    }

    DDS::DomainParticipant_var participant2 =
      dpf->create_participant(11, PARTICIPANT_QOS_DEFAULT, 0, 0);
    if (!participant2) {
      cerr << "create_participant failed." << endl;
      return 1;
    } else {
      ACE_DEBUG((LM_DEBUG,
        "Created participant 2 with instance handle %d\n",
        participant2->get_instance_handle()));
    }

    try {
      OpenDDS::DCPS::TransportRegistry::instance()->bind_config(config2, participant2);
    } catch (const OpenDDS::DCPS::Transport::MiscProblem&) {
      ACE_ERROR((LM_ERROR, "%N:%l: main() ERROR: TransportRegistry::bind_config() throws "
        "Transport::MiscProblem exception\n"));
      return 1;
    } catch (const OpenDDS::DCPS::Transport::NotFound&) {
      ACE_ERROR((LM_ERROR, "%N:%l: main() ERROR: TransportRegistry::bind_config() throws"
        " Transport::NotFound exception\n"));
      return 1;
    }

    if (participant1->get_instance_handle() == participant2->get_instance_handle()) {
      ACE_ERROR((LM_ERROR, "%N:%l: main() ERROR: "
        "participant1 and participant2 do have the same instance handle!\n"));
      return 1;
    }

    // Register TypeSupport (Messenger::Message)
    Messenger::MessageTypeSupport_var mts = new Messenger::MessageTypeSupportImpl();
    if (mts->register_type(participant1, "") != DDS::RETCODE_OK) {
      ACE_ERROR((LM_ERROR, "%N:%l: main() ERROR: register_type failed!\n"));
      return 1;
    }

    // Create Topic
    CORBA::String_var type_name = mts->get_type_name();
    DDS::Topic_var topic1 =
      participant1->create_topic("Movie Discussion List", type_name, TOPIC_QOS_DEFAULT, 0, 0);
    if (!topic1) {
      ACE_ERROR((LM_ERROR, "%N:%l: main() ERROR: create_topic topic1 failed!\n"));
      return 1;
    }

    // Create Publisher
    DDS::Publisher_var pub = participant1->create_publisher(PUBLISHER_QOS_DEFAULT, 0, 0);
    if (!pub) {
      ACE_ERROR((LM_ERROR, "%N:%l: main() ERROR: create_publisher failed!\n"));
      return 1;
    }

    // Create DataWriter
    DDS::DataWriter_var dw = pub->create_datawriter(topic1, DATAWRITER_QOS_DEFAULT, 0, 0);
    if (!dw) {
      ACE_ERROR((LM_ERROR, "%N:%l: main() ERROR: create_datawriter failed!\n"));
      return 1;
    }

    if (mts->register_type(participant2, "") != DDS::RETCODE_OK) {
      ACE_ERROR((LM_ERROR, "%N:%l: main() ERROR: register_type failed!\n"));
      return 1;
    }

    // Create Topic2
    DDS::Topic_var topic2 =
      participant2->create_topic("Movie Discussion List", type_name, TOPIC_QOS_DEFAULT, 0, 0);
    if (!topic2) {
      ACE_ERROR((LM_ERROR, "%N:%l: main() ERROR: create_topic for topic2 failed!\n"));
      return 1;
    }

    // Create Subscriber
    DDS::Subscriber_var sub = participant2->create_subscriber(SUBSCRIBER_QOS_DEFAULT, 0, 0);
    if (!sub) {
      ACE_ERROR((LM_ERROR, "%N:%l: main() ERROR: create_subscriber failed!\n"));
      return 1;
    }

    // Create DataReader
    DDS::DataReader_var dr = sub->create_datareader(topic2, DATAREADER_QOS_DEFAULT, 0, 0);
    if (!dr) {
      ACE_ERROR((LM_ERROR, "%N:%l: main() ERROR: create_datareader failed!\n"));
      return 1;
    }

    // Try to shutdown, but participant1 and participant2 still exist
    DDS::ReturnCode_t rc = TheServiceParticipant->shutdown();
    if (rc != DDS::RETCODE_PRECONDITION_NOT_MET) {
      ACE_ERROR((LM_ERROR, "%N:%l: ERROR: "
        "Expected precondition not met from 1st shutdown attempt, but got %C\n",
        retcode_to_string(rc)));
      return 1;
    }

    /*
     * Try to delete participant1, going one entity at a time to check that we
     * can't delete the participant until every child entity is deleted.
     */
    rc = dpf->delete_participant(participant1);
    if (rc != DDS::RETCODE_PRECONDITION_NOT_MET) {
      ACE_ERROR((LM_ERROR, "%N:%l: ERROR: "
        "Expected precondition not met from 1st delete participant1 attempt, but got %C\n",
        retcode_to_string(rc)));
      return 1;
    }

    /**
     * Intentionally try to use the wrong participant to delete the publisher.
     * This used to cause hard to diagnose errors if the mistake wasn't caught.
     */
    rc = participant2->delete_publisher(pub);
    if (rc != DDS::RETCODE_PRECONDITION_NOT_MET) {
      ACE_ERROR((LM_ERROR, "%N:%l: ERROR: "
        "Expected precondition not met from 1st delete publisher attempt, but got %C\n",
        retcode_to_string(rc)));
      return 1;
    }

    rc = participant1->delete_publisher(pub);
    if (rc != DDS::RETCODE_PRECONDITION_NOT_MET) { // dw still exists
      ACE_ERROR((LM_ERROR, "%N:%l: ERROR: "
        "Expected precondition not met from 2nd delete publisher attempt, but got %C\n",
        retcode_to_string(rc)));
      return 1;
    }

    rc = pub->delete_datawriter(dw);
    if (rc != DDS::RETCODE_OK) {
      ACE_ERROR((LM_ERROR, "%N:%l: ERROR: Failed to delete datawriter: %C\n",
        retcode_to_string(rc)));
      return 1;
    }

    rc = participant1->delete_publisher(pub);
    if (rc != DDS::RETCODE_OK) {
      ACE_ERROR((LM_ERROR, "%N:%l: ERROR: 3rd delete publisher attempt failed: %C\n",
        retcode_to_string(rc)));
      return 1;
    }

    rc = dpf->delete_participant(participant1);
    if (rc != DDS::RETCODE_PRECONDITION_NOT_MET) { // topic1 still exists
      ACE_ERROR((LM_ERROR, "%N:%l: ERROR: "
        "Expected precondition not met from 2nd delete participant1 attempt, but got %C\n",
        retcode_to_string(rc)));
      return 1;
    }

    rc = participant1->delete_topic(topic1);
    if (rc != DDS::RETCODE_OK) {
      ACE_ERROR((LM_ERROR, "%N:%l: ERROR: failed to delete topic1: %C",
        retcode_to_string(rc)));
      return 1;
    }

    rc = dpf->delete_participant(participant1);
    if (rc != DDS::RETCODE_OK) {
      ACE_ERROR((LM_ERROR, "%N:%l: ERROR: 3nd delete participant1 attempt failed: %C\n",
        retcode_to_string(rc)));
      return 1;
    }

    // Try to shutdown, but participant2 still exists
    rc = TheServiceParticipant->shutdown();
    if (rc != DDS::RETCODE_PRECONDITION_NOT_MET) {
      ACE_ERROR((LM_ERROR, "%N:%l: ERROR: "
        "Expected precondition not met from 2nd shutdown attempt, but got %C\n",
        retcode_to_string(rc)));
      return 1;
    }

    /*
     * Try to delete participant2, going one entity at a time to check that we
     * can't delete the participant until every child entity is deleted.
     */
    rc = dpf->delete_participant(participant2);
    if (rc != DDS::RETCODE_PRECONDITION_NOT_MET) {
      ACE_ERROR((LM_ERROR, "%N:%l: ERROR: "
        "Expected precondition not met from 1st delete participant2 attempt, but got %C\n",
        retcode_to_string(rc)));
      return 1;
    }

    /**
     * Intentionally try to use the wrong participant to delete the subscriber.
     * This used to cause hard to diagnose errors if the mistake wasn't caught.
     */
    rc = participant1->delete_subscriber(sub);
    if (rc != DDS::RETCODE_PRECONDITION_NOT_MET) {
      ACE_ERROR((LM_ERROR, "%N:%l: ERROR: "
        "Expected precondition not met from 1st delete subscriber attempt, but got %C\n",
        retcode_to_string(rc)));
      return 1;
    }

    rc = participant2->delete_subscriber(sub);
    if (rc != DDS::RETCODE_PRECONDITION_NOT_MET) { // dr still exists
      ACE_ERROR((LM_ERROR, "%N:%l: ERROR: "
        "Expected precondition not met from 2nd delete subscriber attempt, but got %C\n",
        retcode_to_string(rc)));
      return 1;
    }

    rc = sub->delete_datareader(dr);
    if (rc != DDS::RETCODE_OK) {
      ACE_ERROR((LM_ERROR, "%N:%l: ERROR: Failed to delete datareader: %C\n",
        retcode_to_string(rc)));
      return 1;
    }

    rc = participant2->delete_subscriber(sub);
    if (rc != DDS::RETCODE_OK) {
      ACE_ERROR((LM_ERROR, "%N:%l: ERROR: 3rd delete subscriber attempt failed: %C\n",
        retcode_to_string(rc)));
      return 1;
    }

    rc = dpf->delete_participant(participant2);
    if (rc != DDS::RETCODE_PRECONDITION_NOT_MET) { // topic2 still exists
      ACE_ERROR((LM_ERROR, "%N:%l: ERROR: "
        "Expected precondition not met from 2nd delete participant2 attempt, but got %C\n",
        retcode_to_string(rc)));
      return 1;
    }

    rc = participant2->delete_topic(topic2);
    if (rc != DDS::RETCODE_OK) {
      ACE_ERROR((LM_ERROR, "%N:%l: ERROR: failed to delete topic2: %C",
        retcode_to_string(rc)));
      return 1;
    }

    rc = dpf->delete_participant(participant2);
    if (rc != DDS::RETCODE_OK) {
      ACE_ERROR((LM_ERROR, "%N:%l: ERROR: 3nd delete participant2 attempt failed: %C\n",
        retcode_to_string(rc)));
      return 1;
    }

    // Now shutdown should return OK
    rc = TheServiceParticipant->shutdown();
    if (rc != DDS::RETCODE_OK) {
      ACE_ERROR((LM_ERROR, "%N:%l: ERROR: 3rd shutdown attempt failed: %C\n",
        retcode_to_string(rc)));
      return 1;
    }

    // Calling shutdown again after a successful shutdown should return already deleted
    rc = TheServiceParticipant->shutdown();
    if (rc != DDS::RETCODE_ALREADY_DELETED) {
      ACE_ERROR((LM_ERROR, "%N:%l: ERROR: "
        "Expected already deleted from 4th shutdown attempt, but got %C\n",
        retcode_to_string(rc)));
      return 1;
    }
  } catch (CORBA::Exception& e) {
    cerr << "dp: Exception caught in main.cpp:" << endl << e << endl;
    exit(1);
  }

  return 0;
}
