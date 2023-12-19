// -*- C++ -*-
// ============================================================================
/**
 *  @file   tpreuse.cpp
 *
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

int ACE_TMAIN(int argc, ACE_TCHAR *argv[])
{
  DDS::DomainParticipantFactory_var dpf =
    TheParticipantFactoryWithArgs(argc, argv);
  DDS::DomainParticipant_var participant =
    dpf->create_participant(11,
                            PARTICIPANT_QOS_DEFAULT,
                            0,
                            ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  DDS::DomainParticipant_var participant2 =
    dpf->create_participant(11,
                            PARTICIPANT_QOS_DEFAULT,
                            0,
                            ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  // Register TypeSupport (Messenger::Message)
  Messenger::MessageTypeSupport_var mts =
    new Messenger::MessageTypeSupportImpl();

  mts->register_type(participant, "");

  // Create Topic
  CORBA::String_var type_name = mts->get_type_name();
  DDS::Topic_var topic =
    participant->create_topic("Movie Discussion List",
                              type_name,
                              TOPIC_QOS_DEFAULT,
                              0,
                              OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  // Create Publisher
  DDS::Publisher_var pub =
    participant->create_publisher(PUBLISHER_QOS_DEFAULT,
                                  0,
                                  OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  // Create DataWriter
  DDS::DataWriter_var dw =
    pub->create_datawriter(topic,
                           DATAWRITER_QOS_DEFAULT,
                           0,
                           OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  const DDS::ReturnCode_t retcode = participant2->delete_topic(topic);
  if (retcode != DDS::RETCODE_PRECONDITION_NOT_MET) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("%N:%l: main()")
                      ACE_TEXT(" ERROR: should not be able to delete topic, not part of this participant!\n")),
                     -1);
  }

  const DDS::ReturnCode_t retcode5 = participant->delete_topic(topic);
  if (retcode5 != DDS::RETCODE_PRECONDITION_NOT_MET) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("%N:%l: main()")
                      ACE_TEXT(" ERROR: should not be able to delete topic, still referenced by datawriter!\n")),
                     -1);
  }

  const DDS::ReturnCode_t retcode2 = pub->delete_datawriter(dw);
  if (retcode2 != DDS::RETCODE_OK) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("%N:%l: main()")
                      ACE_TEXT(" ERROR: should be able to delete datawriter\n")),
                     -1);
  }

  DDS::Publisher_var pub2 =
    participant2->create_publisher(PUBLISHER_QOS_DEFAULT,
                                   0,
                                   OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  DDS::DataWriter_var dw2 =
    pub2->create_datawriter(topic,
                            DATAWRITER_QOS_DEFAULT,
                            0,
                            OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (dw2) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("%N:%l: main()")
                      ACE_TEXT(" ERROR: should not be able to create_datawriter with a topic from another participant\n")),
                     -1);
  }

  DDS::Subscriber_var sub2 =
    participant2->create_subscriber(SUBSCRIBER_QOS_DEFAULT,
                                    0,
                                    OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  DDS::DataReader_var dr2 =
    sub2->create_datareader(topic,
                            DATAREADER_QOS_DEFAULT,
                            0,
                            OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (dr2) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("%N:%l: main()")
                      ACE_TEXT(" ERROR: should not be able to create_datareader with a topic from another participant\n")),
                     -1);
  }

  DDS::Duration_t timeout;
  timeout.sec = 0;
  timeout.nanosec = 0;
  // Doing a find_topic will require us to call delete topic twice, see
  // 2.2.2.2.1.11 from the dds spec
  DDS::Topic_var topic2 = participant->find_topic("Movie Discussion List", timeout);
  if (!topic2) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("%N:%l: main()")
                      ACE_TEXT(" ERROR: Not able to find topic\n")),
                     -1);
  }

  const DDS::ReturnCode_t retcode4 = participant->delete_topic(topic);
  if (retcode4 != DDS::RETCODE_OK) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("%N:%l: main()")
                      ACE_TEXT(" ERROR: should be able to delete topic\n")),
                     -1);
  }

  const DDS::ReturnCode_t retcode6 = participant->delete_topic(topic2);
  if (retcode6 != DDS::RETCODE_OK) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("%N:%l: main()")
                      ACE_TEXT(" ERROR: should be able to delete topic\n")),
                     -1);
  }

  const DDS::ReturnCode_t retcode8 = participant->delete_publisher(pub);
  if (retcode8 != DDS::RETCODE_OK) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("%N:%l: main()")
                      ACE_TEXT(" ERROR: should be able to delete publisher\n")),
                     -1);
  }

  participant->delete_contained_entities();
  participant2->delete_contained_entities();
  dpf->delete_participant(participant);
  dpf->delete_participant(participant2);
  TheServiceParticipant->shutdown();

  return 0;
}
