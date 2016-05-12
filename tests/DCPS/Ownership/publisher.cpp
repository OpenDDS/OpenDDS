/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <ace/Get_Opt.h>
#include <ace/Log_Msg.h>
#include <ace/OS_NS_stdlib.h>

#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/PublisherImpl.h>
#include <dds/DCPS/Service_Participant.h>

#include "dds/DCPS/StaticIncludes.h"
#ifdef ACE_AS_STATIC_LIBS
#include <dds/DCPS/transport/udp/Udp.h>
#include <dds/DCPS/transport/multicast/Multicast.h>
#include <dds/DCPS/RTPS/RtpsDiscovery.h>
#include <dds/DCPS/transport/rtps_udp/RtpsUdp.h>
#endif

#include "MessengerTypeSupportImpl.h"
#include "Writer.h"

DDS::Duration_t deadline = {DDS::DURATION_INFINITE_SEC,
                            DDS::DURATION_INFINITE_NSEC};
DDS::Duration_t liveliness = {DDS::DURATION_INFINITE_SEC,
                              DDS::DURATION_INFINITE_NSEC};
ACE_Time_Value dds_delay(1);
ACE_Time_Value reset_delay(ACE_Time_Value::zero);
int ownership_strength = 0;
int reset_ownership_strength = -1;
ACE_CString ownership_dw_id = "OwnershipDataWriter";
bool delay_reset = false;

namespace {

int
parse_args(int argc, ACE_TCHAR *argv[])
{
  ACE_Get_Opt get_opts(argc, argv, ACE_TEXT("s:i:r:d:y:l:c"));

  int c;
  while ((c = get_opts()) != -1) {
    switch (c) {
    case 's':
      ownership_strength = ACE_OS::atoi (get_opts.opt_arg());
      break;
    case 'i':
      ownership_dw_id = ACE_TEXT_ALWAYS_CHAR(get_opts.opt_arg());
      break;
    case 'r':
      reset_ownership_strength = ACE_OS::atoi (get_opts.opt_arg());
      break;
    case 'd':
      deadline.sec = ACE_OS::atoi (get_opts.opt_arg());
      deadline.nanosec = 0;
      break;
    case 'y':
      dds_delay.msec (ACE_OS::atoi (get_opts.opt_arg()));
      break;
    case 'l':
      liveliness.sec = ACE_OS::atoi (get_opts.opt_arg());
      liveliness.nanosec = 0;
      break;
    case 'c':
      delay_reset = true;
      break;
    case '?':
    default:
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("usage: %C -s <ownership_strength> ")
                        ACE_TEXT("-i <ownership_dw_id> -r <reset_ownership_strength> ")
                        ACE_TEXT("-d <deadline> -y <delay> -l <liveliness>\n"),
                        argv[0]),
                        -1);
    }
  }

  if (delay_reset) {
    if (deadline.sec != ::DDS::DURATION_INFINITE_SEC) {
      reset_delay.sec(deadline.sec);
    }
    if (liveliness.sec != ::DDS::DURATION_INFINITE_SEC
        && reset_delay.sec() < static_cast<time_t>(liveliness.sec)) {
      reset_delay.sec(liveliness.sec);
    }

    if (reset_delay > ACE_Time_Value::zero) {
      reset_delay += ACE_Time_Value (4);
    }
  }

  return 0;
}

} // namespace

int ACE_TMAIN(int argc, ACE_TCHAR *argv[])
{
  try {
    // Initialize DomainParticipantFactory
    DDS::DomainParticipantFactory_var dpf =
      TheParticipantFactoryWithArgs(argc, argv);

    int error;
    if ((error = parse_args(argc, argv)) != 0) {
      return error;
    }

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
    DDS::Topic_var topic =
      participant->create_topic("Movie Discussion List",
                                CORBA::String_var(mts->get_type_name()),
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

    ::DDS::DataWriterQos dw_qos;
    pub->get_default_datawriter_qos (dw_qos);
    dw_qos.ownership.kind = ::DDS::EXCLUSIVE_OWNERSHIP_QOS;
    dw_qos.ownership_strength.value = ownership_strength;
    dw_qos.deadline.period.sec = deadline.sec;
    dw_qos.deadline.period.nanosec = deadline.nanosec;
    dw_qos.liveliness.kind = DDS::MANUAL_BY_TOPIC_LIVELINESS_QOS;
    dw_qos.liveliness.lease_duration.sec = liveliness.sec;
    dw_qos.liveliness.lease_duration.nanosec = liveliness.nanosec;

    // Create DataWriter
    DDS::DataWriter_var dw =
      pub->create_datawriter(topic.in(),
                             dw_qos,
                             DDS::DataWriterListener::_nil(),
                             OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (CORBA::is_nil(dw.in())) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l: main()")
                        ACE_TEXT(" ERROR: create_datawriter failed!\n")),
                       -1);
    }

    // Start writing threads
    Writer* writer = new Writer(dw.in(), ownership_dw_id.c_str());
    writer->start();

    while (!writer->is_finished()) {
      ACE_Time_Value small_time(0, 250000);
      ACE_OS::sleep(small_time);
    }

    // Wait for acks failing in static build...
    writer->wait_for_acks();
    writer->end();
    delete writer;

    // Clean-up!
    participant->delete_contained_entities();
    dpf->delete_participant(participant.in());

    TheServiceParticipant->shutdown();

  } catch (const CORBA::Exception& e) {
    e._tao_print_exception("Exception caught in main():");
    ACE_OS::exit(-1);
  }

  return 0;
}
