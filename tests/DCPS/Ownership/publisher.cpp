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
#include <dds/DCPS/RTPS/RtpsDiscovery.h>
#include <dds/DCPS/transport/rtps_udp/RtpsUdp.h>
#endif

#include "MessengerTypeSupportImpl.h"

#include <tests/Utils/StatusMatching.h>
#include <tests/Utils/DistributedConditionSet.h>

DDS::Duration_t deadline = {DDS::DURATION_INFINITE_SEC,
                            DDS::DURATION_INFINITE_NSEC};
DDS::Duration_t liveliness = {DDS::DURATION_INFINITE_SEC,
                              DDS::DURATION_INFINITE_NSEC};
ACE_Time_Value dds_delay(1);
ACE_Time_Value reset_delay; // default is zero
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
    DistributedConditionSet_rch dcs =
      OpenDDS::DCPS::make_rch<FileBasedDistributedConditionSet>();

    // Initialize DomainParticipantFactory
    DDS::DomainParticipantFactory_var dpf =
      TheParticipantFactoryWithArgs(argc, argv);

    int error;
    if ((error = parse_args(argc, argv)) != 0) {
      return error;
    }

    // Create DomainParticipant
    DDS::DomainParticipant_var participant =
      dpf->create_participant(111,
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

    // Block until Subscribers are available
    if (wait_match(dw, 2, Utils::EQ)) {
      ACE_OS::exit(-1);
    }

    dcs->wait_for(ownership_dw_id.c_str(), "reader1", "ready");
    dcs->wait_for(ownership_dw_id.c_str(), "reader2", "ready");

    // Write samples
    Messenger::MessageDataWriter_var message_dw
      = Messenger::MessageDataWriter::_narrow(dw.in());

    if (CORBA::is_nil(message_dw.in())) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("%N:%l: svc()")
                 ACE_TEXT(" ERROR: _narrow failed!\n")));
      ACE_OS::exit(-1);
    }

    Messenger::Message message;

    message.from       = CORBA::string_dup(ownership_dw_id.c_str());
    message.subject    = CORBA::string_dup("Review");
    message.text       = CORBA::string_dup("Worst. Movie. Ever.");
    message.count      = 1;
    message.strength   = ownership_strength;

    for (int i = 0; i != 20; ++i) {
      message.subject_id = message.count % 2;  // 0 or 1
      ACE_DEBUG ((LM_DEBUG, "(%P|%t) %C writes instance %d count %d str %d\n",
                  ownership_dw_id.c_str(), message.subject_id, message.count, message.strength));
      DDS::ReturnCode_t rc_error = message_dw->write(message, ::DDS::HANDLE_NIL);

      if (rc_error != DDS::RETCODE_OK) {
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("%N:%l: svc()")
                   ACE_TEXT(" ERROR: write returned %d!\n"), rc_error));
      }

      if (message.count == 5) {
        ::DDS::DataWriterQos qos;
        rc_error = dw->get_qos(qos);
        if (rc_error != ::DDS::RETCODE_OK) {
          ACE_ERROR((LM_ERROR,
                     ACE_TEXT("%N:%l: svc()")
                     ACE_TEXT(" ERROR: get_qos returned %d!\n"), rc_error));
        }
        CORBA::Long old = qos.ownership_strength.value;
        if (reset_ownership_strength != -1 && old != reset_ownership_strength) {
          dcs->wait_for("publisher", "subscriber", "strength change " + OpenDDS::DCPS::to_dds_string(old));
          qos.ownership_strength.value = reset_ownership_strength;
          // Wait for the change in qos to propagate. This helps
          // simplify result verification on subscriber side.
          ACE_DEBUG ((LM_DEBUG, ACE_TEXT ("(%P|%t) %C : reset ownership strength from %d to %d\n"),
                      ownership_dw_id.c_str(), old, reset_ownership_strength));
          rc_error = dw->set_qos (qos);
          if (rc_error != ::DDS::RETCODE_OK) {
            ACE_ERROR((LM_ERROR,
                       ACE_TEXT("%N:%l: svc()")
                       ACE_TEXT(" ERROR: set_qos returned %d!\n"), rc_error));
          } else {
            message.strength   =  reset_ownership_strength;
            ACE_DEBUG ((LM_DEBUG, ACE_TEXT ("(%P|%t) ownership strength in message is now %d\n"),
                        message.strength));
            rc_error = dw->get_qos(qos);
            if (rc_error != ::DDS::RETCODE_OK) {
              ACE_ERROR((LM_ERROR,
                         ACE_TEXT("%N:%l: svc()")
                         ACE_TEXT(" ERROR: get_qos returned %d!\n"), rc_error));
            } else {
              ACE_DEBUG ((LM_DEBUG, ACE_TEXT ("(%P|%t) ownership strength in qos is now %d\n"),
                          qos.ownership_strength.value));
            }
          }
          dcs->wait_for("publisher", "subscriber", "strength change " + OpenDDS::DCPS::to_dds_string(reset_ownership_strength));
        }
      }

      if ((message.count == 5)
          && reset_delay > ACE_Time_Value::zero) {
        ACE_DEBUG ((LM_DEBUG, ACE_TEXT ("(%P|%t) %C : reset delay from %d to %d at sample %d\n"),
                    ownership_dw_id.c_str(), dds_delay.msec(), reset_delay.msec(), message.count));
        ACE_OS::sleep(reset_delay);
      } else if (dds_delay > ACE_Time_Value::zero) {
        ACE_OS::sleep(dds_delay);
      }

      message.count++;
    }

    const DDS::Duration_t max_wait = { DDS::DURATION_INFINITE_SEC, DDS::DURATION_INFINITE_NSEC };
    if (dw->wait_for_acknowledgments(max_wait) != DDS::RETCODE_OK) {
      ACE_ERROR((LM_ERROR, "(%P|%t) %C : wait for acknowledgement failed\n",
                 ownership_dw_id.c_str()));
    }

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
