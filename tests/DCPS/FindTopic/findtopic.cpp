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

#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/PublisherImpl.h>
#include <dds/DCPS/Qos_Helper.h>

#ifdef ACE_AS_STATIC_LIBS
#include <dds/DCPS/RTPS/RtpsDiscovery.h>
#include <dds/DCPS/transport/rtps_udp/RtpsUdp.h>
#endif

#include "dds/DCPS/StaticIncludes.h"
#include "MessengerTypeSupportImpl.h"

#include <ace/streams.h>
#include "tests/Utils/ExceptionStreams.h"
#include "ace/Get_Opt.h"

#include <memory>

using namespace std;

int sub_program (DDS::DomainParticipant_ptr participant)
{
  DDS::Duration_t timeout;
  timeout.sec = 0;
  timeout.nanosec = 0;
  // Doing a find_topic will require us to call delete topic twice, see
  // 7.1.2.2.1.11 from the dds spec
  DDS::Topic_var topic2 = participant->find_topic ("Movie Discussion List", timeout);
  if (CORBA::is_nil (topic2.in ())) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("%N:%l: main()")
                      ACE_TEXT(" ERROR: Not able to find topic\n")),
                    -1);
  }

  // Now using topic2 we create a second publisher and datawriter
  // Create Publisher
  DDS::Publisher_var pub2 =
    participant->create_publisher(PUBLISHER_QOS_DEFAULT,
                                  DDS::PublisherListener::_nil(),
                                  OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  if (CORBA::is_nil(pub2.in())) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("%N:%l: main()")
                      ACE_TEXT(" ERROR: create_publisher 2 failed!\n")),
                    -1);
  }

  // Create DataWriter
  DDS::DataWriter_var dw2 =
    pub2->create_datawriter(topic2.in(),
                          DATAWRITER_QOS_DEFAULT,
                          DDS::DataWriterListener::_nil(),
                          OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  if (CORBA::is_nil(dw2.in())) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("%N:%l: main()")
                      ACE_TEXT(" ERROR: create_datawriter 2 failed!\n")),
                    -1);
  }

  // Now the second part will cleanup its datawriter/publisher
  DDS::ReturnCode_t retcode12 = pub2->delete_datawriter (dw2.in ());
  if (retcode12 != DDS::RETCODE_OK) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("%N:%l: main()")
                      ACE_TEXT(" ERROR: should be able to delete datawriter 2\n")),
                    -1);
  }
  dw2 = DDS::DataWriter::_nil ();

  const DDS::ReturnCode_t retcode9 = participant->delete_publisher (pub2.in ());
  if (retcode9 != DDS::RETCODE_OK) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("%N:%l: main()")
                      ACE_TEXT(" ERROR: should be able to delete publisher 2\n")),
                    -1);
  }
  pub2 = DDS::Publisher::_nil ();

  // Now we should be able to delete out topic2, we deleted the entities we created
  const DDS::ReturnCode_t retcode4 = participant->delete_topic (topic2.in ());
  if (retcode4 != DDS::RETCODE_OK) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("%N:%l: main()")
                      ACE_TEXT(" ERROR: should be able to delete topic\n")),
                    -1);
  }
  topic2 = DDS::Topic::_nil ();

  return 0;
}

int ACE_TMAIN (int argc, ACE_TCHAR *argv[]){
  try
    {
      DDS::DomainParticipantFactory_var dpf =
        TheParticipantFactoryWithArgs(argc, argv);
      DDS::DomainParticipant_var participant =
        dpf->create_participant(11,
                                PARTICIPANT_QOS_DEFAULT,
                                DDS::DomainParticipantListener::_nil(),
                                ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      if (CORBA::is_nil (participant.in ())) {
        cerr << "create_participant failed." << endl;
        return 1;
      }
      else
      {
        ACE_DEBUG ((LM_DEBUG, "Created participant 1 with instance handle %d\n",
                    participant->get_instance_handle ()));
      }

      // Register TypeSupport (Messenger::Message)
      Messenger::MessageTypeSupport_var mts =
        new Messenger::MessageTypeSupportImpl();

      if (mts->register_type(participant.in(), "") != DDS::RETCODE_OK) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("%N:%l: main()")
                          ACE_TEXT(" ERROR: register_type failed!\n")),
                        -1);
      }

      // Create Topic
      CORBA::String_var type_name = mts->get_type_name();
      DDS::Topic_var topic1 =
        participant->create_topic("Movie Discussion List",
                                  type_name.in(),
                                  TOPIC_QOS_DEFAULT,
                                  DDS::TopicListener::_nil(),
                                  OpenDDS::DCPS::DEFAULT_STATUS_MASK);

      if (CORBA::is_nil(topic1.in())) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("%N:%l: main()")
                          ACE_TEXT(" ERROR: create_topic failed!\n")),
                        -1);
      }

      // Create Publisher
      DDS::Publisher_var pub1 =
        participant->create_publisher(PUBLISHER_QOS_DEFAULT,
                                      DDS::PublisherListener::_nil(),
                                      OpenDDS::DCPS::DEFAULT_STATUS_MASK);

      if (CORBA::is_nil(pub1.in())) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("%N:%l: main()")
                          ACE_TEXT(" ERROR: create_publisher 1 failed!\n")),
                        -1);
      }

      // Create DataWriter
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

      // Now the second part of the program will try to do its setup
      if (sub_program(participant.in ()) != 0)
      {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("%N:%l: main()")
                          ACE_TEXT(" ERROR: sub program failed\n")),
                        -1);
      }

      DDS::ReturnCode_t retcode2 = pub1->delete_datawriter (dw1.in ());
      if (retcode2 != DDS::RETCODE_OK) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("%N:%l: main()")
                          ACE_TEXT(" ERROR: should be able to delete datawriter\n")),
                        -1);
      }
      dw1 = DDS::DataWriter::_nil ();

      DDS::ReturnCode_t retcode8 = participant->delete_publisher (pub1.in ());
      if (retcode8 != DDS::RETCODE_OK) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("%N:%l: main()")
                          ACE_TEXT(" ERROR: should be able to delete publisher\n")),
                        -1);
      }
      pub1 = DDS::Publisher::_nil ();

      const DDS::ReturnCode_t retcode6 = participant->delete_topic (topic1.in ());
      if (retcode6 != DDS::RETCODE_OK) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("%N:%l: main()")
                          ACE_TEXT(" ERROR: should be able to delete topic\n")),
                        -1);
      }
      topic1 = DDS::Topic::_nil ();

      dpf->delete_participant(participant.in ());
      participant = DDS::DomainParticipant::_nil ();
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
