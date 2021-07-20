// -*- C++ -*-
// ============================================================================
/**
 *  @file   findtopic.cpp
 *
 * Demonstrate an issue with find_topic. According to the DDS spec for each call
 * to find_topic a call to delete_topic must be done. This is because a topic can
 * only be created once, so when it is already there another part of the program
 * can call find_topic to reuse an existing topic. It gets now problematic when
 * within the program there are two pieces that each use the same topic and create a
 * DDS entity referring to it, at the moment the first piece deletes his DDS
 * entity it will call delete_topic but that should work (return RETCODE_OK), because
 * he is unaware of the other piece.
 */
// ============================================================================

#include "LocalDiscovery.h"
#include "MessengerTypeSupportImpl.h"

#include "tests/Utils/ExceptionStreams.h"

#include <dds/DCPS/RTPS/RtpsDiscovery.h>
#ifdef ACE_AS_STATIC_LIBS
#  include <dds/DCPS/transport/rtps_udp/RtpsUdp.h>
#endif

#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/Qos_Helper.h>

#include <ace/Get_Opt.h>
#include <ace/streams.h>

#include <memory>

using namespace std;

int sub_program(DDS::DomainParticipant* participant)
{
  const DDS::Duration_t timeout = {0, 0};
  // Doing a find_topic will require us to call delete topic twice, see
  // 7.1.2.2.1.11 from the dds spec
  DDS::Topic_var topic2 = participant->find_topic("Movie Discussion List", timeout);
  if (!topic2) {
    ACE_ERROR_RETURN((LM_ERROR, "%N:%l: main() ERROR: Not able to find topic\n"), EXIT_FAILURE);
  }

  // Now using topic2 we create a second publisher and datawriter
  DDS::Publisher_var pub2 = participant->create_publisher(PUBLISHER_QOS_DEFAULT, 0, 0);
  if (!pub2) {
    ACE_ERROR_RETURN((LM_ERROR, "%N:%l: main() ERROR: create_publisher 2 failed!\n"), EXIT_FAILURE);
  }

  DDS::DataWriter_var dw2 = pub2->create_datawriter(topic2, DATAWRITER_QOS_DEFAULT, 0, 0);
  if (!dw2) {
    ACE_ERROR_RETURN((LM_ERROR, "%N:%l: main() ERROR: create_datawriter 2 failed!\n"), EXIT_FAILURE);
  }

  // Do now a find for topic "Movie Discussion List", that should return a new topic which we directly
  // delete again which should work because this new topic is not used.
  DDS::Topic_var topic3 = participant->find_topic("Movie Discussion List", timeout);
  if (!topic3) {
    ACE_ERROR_RETURN((LM_ERROR, "%N:%l: main() ERROR: Not able to find topic 3\n"), EXIT_FAILURE);
  }
  // topic2 and topic3 should have the same instance handles
  if (topic2->get_instance_handle() != topic3->get_instance_handle()) {
    ACE_ERROR_RETURN((LM_ERROR, "%N:%l: main() ERROR: topic2 and topic3 should have the same instance handles\n"), EXIT_FAILURE);
  }
  // Now delete the topic3 again
  const DDS::ReturnCode_t retcode6 = participant->delete_topic(topic3);
  if (retcode6 != DDS::RETCODE_OK) {
    ACE_ERROR_RETURN((LM_ERROR, "%N:%l: main() ERROR: should be able to delete topic 3\n"), EXIT_FAILURE);
  }
  topic3 = 0;

  // Now the second part will cleanup its datawriter/publisher
  const DDS::ReturnCode_t retcode12 = pub2->delete_datawriter(dw2);
  if (retcode12 != DDS::RETCODE_OK) {
    ACE_ERROR_RETURN((LM_ERROR, "%N:%l: main() ERROR: should be able to delete datawriter 2\n"), EXIT_FAILURE);
  }
  dw2 = 0;

  const DDS::ReturnCode_t retcode9 = participant->delete_publisher(pub2);
  if (retcode9 != DDS::RETCODE_OK) {
    ACE_ERROR_RETURN((LM_ERROR, "%N:%l: main() ERROR: should be able to delete publisher 2\n"), EXIT_FAILURE);
  }
  pub2 = 0;

  // Now we should be able to delete out topic2, we deleted the entities we created
  const DDS::ReturnCode_t retcode4 = participant->delete_topic(topic2);
  if (retcode4 != DDS::RETCODE_OK) {
    ACE_ERROR_RETURN((LM_ERROR, "%N:%l: main() ERROR: should be able to delete topic\n"), EXIT_FAILURE);
  }

  return EXIT_SUCCESS;
}

int double_delete(DDS::DomainParticipant* participant, const char* type_name)
{
  // Currently OpenDDS returns the same topic when we call create_topic twice
  DDS::Topic_var topic1 = participant->create_topic("Test",
                                                    type_name, TOPIC_QOS_DEFAULT, 0, 0);
  if (!topic1) {
    ACE_ERROR_RETURN((LM_ERROR, "%N:%l: double_delete() ERROR: create_topic 1 failed!\n"), EXIT_FAILURE);
  }

  DDS::Topic_var topic2 = participant->create_topic("Test",
                                                    type_name, TOPIC_QOS_DEFAULT, 0, 0);
  if (!topic2) {
    ACE_ERROR_RETURN((LM_ERROR, "%N:%l: double_delete() ERROR: create_topic 2 failed!\n"), EXIT_FAILURE);
  }

  if (topic1->get_instance_handle() != topic2->get_instance_handle()) {
    ACE_ERROR_RETURN((LM_ERROR, "%N:%l: double_delete() ERROR: instance handle of topic 1 and 2 are different\n"), EXIT_FAILURE);
  }

  const DDS::ReturnCode_t retcode1 = participant->delete_topic(topic1);
  if (retcode1 != DDS::RETCODE_OK) {
    ACE_ERROR_RETURN((LM_ERROR, "%N:%l: double_delete() ERROR: should be able to delete topic 1\n"), EXIT_FAILURE);
  }

  const DDS::ReturnCode_t retcode2 = participant->delete_topic(topic2);
  if (retcode2 != DDS::RETCODE_OK) {
    ACE_ERROR_RETURN((LM_ERROR, "%N:%l: double_delete() ERROR: should be able to delete topic 2\n"), EXIT_FAILURE);
  }

  return EXIT_SUCCESS;
}

int ACE_TMAIN(int argc, ACE_TCHAR* argv[])
{
  try {
    DDS::DomainParticipantFactory_var dpf = TheParticipantFactoryWithArgs(argc, argv);

    OpenDDS::DCPS::Discovery_rch discovery;
    if (argc > 1 && ACE_TString(argv[1]) == ACE_TEXT("rtps")) {
      OpenDDS::RTPS::RtpsDiscovery_rch rtps_disc =
        OpenDDS::DCPS::make_rch<OpenDDS::RTPS::RtpsDiscovery>("RTPS");
      rtps_disc->sedp_multicast(false);
      discovery = rtps_disc;
      ACE_DEBUG((LM_DEBUG, "%N:%l main() using RTPS Discovery\n"));
    } else {
      discovery = OpenDDS::DCPS::make_rch<LocalDiscovery>();
      ACE_DEBUG((LM_DEBUG, "%N:%l main() using Local Discovery\n"));
    }
    TheServiceParticipant->add_discovery(discovery);
    TheServiceParticipant->set_default_discovery(discovery->key());

    TransportInst_rch default_inst = TheTransportRegistry->create_inst("transport", "rtps_udp");
    TheTransportRegistry->get_config(TransportRegistry::DEFAULT_CONFIG_NAME)->sorted_insert(default_inst);

    DDS::DomainParticipant_var participant = dpf->create_participant(11, PARTICIPANT_QOS_DEFAULT, 0, 0);
    if (!participant) {
      ACE_ERROR_RETURN((LM_ERROR, "%N:%l: main() ERROR: create_participant failed!\n"), EXIT_FAILURE);
    }

    Messenger::MessageTypeSupport_var mts = new Messenger::MessageTypeSupportImpl;
    if (mts->register_type(participant, "") != DDS::RETCODE_OK) {
      ACE_ERROR_RETURN((LM_ERROR, "%N:%l: main() ERROR: register_type failed!\n"), EXIT_FAILURE);
    }

    CORBA::String_var type_name = mts->get_type_name();
    DDS::Topic_var topic1 = participant->create_topic("Movie Discussion List",
                                                      type_name, TOPIC_QOS_DEFAULT, 0, 0);
    if (!topic1) {
      ACE_ERROR_RETURN((LM_ERROR, "%N:%l: main() ERROR: create_topic failed!\n"), EXIT_FAILURE);
    }

    DDS::Publisher_var pub1 = participant->create_publisher(PUBLISHER_QOS_DEFAULT, 0, 0);
    if (!pub1) {
      ACE_ERROR_RETURN((LM_ERROR, "%N:%l: main() ERROR: create_publisher 1 failed!\n"), EXIT_FAILURE);
    }

    DDS::DataWriter_var dw1 = pub1->create_datawriter(topic1, DATAWRITER_QOS_DEFAULT, 0, 0);
    if (!dw1) {
      ACE_ERROR_RETURN((LM_ERROR, "%N:%l: main() ERROR: create_datawriter 1 failed!\n"), EXIT_FAILURE);
    }

    // Now the second part of the program will try to do its setup
    if (sub_program(participant) != EXIT_SUCCESS) {
      ACE_ERROR_RETURN((LM_ERROR, "%N:%l: main() ERROR: sub program 1 failed!\n"), EXIT_FAILURE);
    }

    // Now test double create/delete
    if (double_delete(participant, type_name.in ()) != EXIT_SUCCESS) {
      ACE_ERROR_RETURN((LM_ERROR, "%N:%l: main() ERROR: double_delete failed!\n"), EXIT_FAILURE);
    }

    const DDS::ReturnCode_t retcode2 = pub1->delete_datawriter(dw1);
    if (retcode2 != DDS::RETCODE_OK) {
      ACE_ERROR_RETURN((LM_ERROR, "%N:%l: main() ERROR: should be able to delete datawriter\n"), EXIT_FAILURE);
    }
    dw1 = 0;

    const DDS::ReturnCode_t retcode8 = participant->delete_publisher(pub1);
    if (retcode8 != DDS::RETCODE_OK) {
      ACE_ERROR_RETURN((LM_ERROR, "%N:%l: main() ERROR: should be able to delete publisher\n"), EXIT_FAILURE);
    }
    pub1 = 0;

    const DDS::ReturnCode_t retcode6 = participant->delete_topic(topic1);
    if (retcode6 != DDS::RETCODE_OK) {
      ACE_ERROR_RETURN((LM_ERROR, "%N:%l: main() ERROR: should be able to delete topic\n"), EXIT_FAILURE);
    }
    topic1 = 0;

    dpf->delete_participant(participant);
    participant = 0;
    TheServiceParticipant->shutdown();

  } catch (const CORBA::Exception& e) {
    cerr << "Exception caught in main.cpp:" << endl
         << e << endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
