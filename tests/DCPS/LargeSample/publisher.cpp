/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <ace/Get_Opt.h>
#include <ace/Log_Msg.h>
#include <ace/OS_NS_stdlib.h>
#include <ace/OS_NS_unistd.h>

#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/PublisherImpl.h>
#include <dds/DCPS/Service_Participant.h>

#include "dds/DCPS/StaticIncludes.h"
#if defined ACE_AS_STATIC_LIBS && !defined OPENDDS_SAFETY_PROFILE
#include <dds/DCPS/transport/udp/Udp.h>
#include <dds/DCPS/transport/multicast/Multicast.h>
#include <dds/DCPS/transport/shmem/Shmem.h>
#include <dds/DCPS/transport/rtps_udp/RtpsUdp.h>
#include <dds/DCPS/RTPS/RtpsDiscovery.h>
#endif

#include "MessengerTypeSupportImpl.h"
#include "Writer.h"

namespace {

void parse_args(int argc, ACE_TCHAR* argv[],
                bool& reliable,
                int& num_msgs,
                int& my_pid)
{
  ACE_Get_Opt getopt(argc, argv, "r:n:p:");
  for (int opt = 0; (opt = getopt()) != EOF;) {
    if (opt == 'r') {
      reliable = ACE_OS::atoi(getopt.opt_arg());
    } else if (opt == 'n') {
      num_msgs = ACE_OS::atoi(getopt.opt_arg());
    } else if (opt == 'p') {
      my_pid = ACE_OS::atoi(getopt.opt_arg());
    }
  }
}

}

int ACE_TMAIN(int argc, ACE_TCHAR *argv[])
{
  int status = 0;

  try {
    // Initialize DomainParticipantFactory
    DDS::DomainParticipantFactory_var dpf =
      TheParticipantFactoryWithArgs(argc, argv);

    bool reliable = true;
    int num_msgs = 10;
    int my_pid = ACE_OS::getpid();

    parse_args(argc, argv, reliable, num_msgs, my_pid);

    // Create DomainParticipant
    DDS::DomainParticipant_var participant =
      dpf->create_participant(411,
                              PARTICIPANT_QOS_DEFAULT,
                              DDS::DomainParticipantListener::_nil(),
                              OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (CORBA::is_nil(participant.in())) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l: main()")
                        ACE_TEXT(" ERROR: create_participant failed!\n")),
                       -1);
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
    DDS::Topic_var topic =
      participant->create_topic("Movie Discussion List",
                                type_name.in(),
                                TOPIC_QOS_DEFAULT,
                                DDS::TopicListener::_nil(),
                                OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (CORBA::is_nil(topic.in())) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l: main()")
                        ACE_TEXT(" ERROR: create_topic failed!\n")),
                       -1);
    }

    // Create Publisher
    DDS::Publisher_var pub =
      participant->create_publisher(PUBLISHER_QOS_DEFAULT,
                                    DDS::PublisherListener::_nil(),
                                    OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (CORBA::is_nil(pub.in())) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l: main()")
                        ACE_TEXT(" ERROR: create_publisher failed!\n")),
                       -1);
    }

    DDS::DataWriterQos qos;
    pub->get_default_datawriter_qos(qos);
    qos.liveliness.kind = DDS::AUTOMATIC_LIVELINESS_QOS;
    qos.liveliness.lease_duration.sec = 5;
    qos.liveliness.lease_duration.nanosec = 0;
    qos.history.kind = DDS::KEEP_ALL_HISTORY_QOS;
    qos.durability.kind = DDS::TRANSIENT_LOCAL_DURABILITY_QOS;

    // Create DataWriter
    DDS::DataWriter_var dw =
      pub->create_datawriter(topic.in(),
                             qos,
                             DDS::DataWriterListener::_nil(),
                             OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (CORBA::is_nil(dw.in())) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l: main()")
                        ACE_TEXT(" ERROR: create_datawriter failed!\n")),
                       -1);
    }

    DDS::DataWriter_var dw2 =
      pub->create_datawriter(topic.in(),
                             qos,
                             DDS::DataWriterListener::_nil(),
                             OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (CORBA::is_nil(dw2.in())) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l: main()")
                        ACE_TEXT(" ERROR: create_datawriter 2 failed!\n")),
                       -1);
    }

    {
      Writer writer(dw, dw2, my_pid);
      writer.write(reliable, num_msgs);
    }

    // Sleep to give subscriber a chance to nak before exiting
    ACE_OS::sleep(3);

    ACE_DEBUG((LM_DEBUG, "Publisher delete contained entities\n"));
    // Clean-up!
    participant->delete_contained_entities();
    ACE_DEBUG((LM_DEBUG, "Publisher delete participant\n"));
    dpf->delete_participant(participant.in());

    ACE_DEBUG((LM_DEBUG, "Publisher shutdown\n"));
    TheServiceParticipant->shutdown();

    ACE_DEBUG((LM_DEBUG, "Publisher vars going out of scope\n"));
  } catch (const CORBA::Exception& e) {
    e._tao_print_exception("Exception caught in main():");
    status = -1;
  }

  ACE_DEBUG((LM_DEBUG, "Publisher exiting with status=%d\n", status));
  return status;
}
