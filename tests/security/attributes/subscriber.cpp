/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */


#include <dds/DdsDcpsInfrastructureC.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/SubscriberImpl.h>
#include <dds/DCPS/WaitSet.h>

#include "dds/DCPS/StaticIncludes.h"
#ifdef ACE_AS_STATIC_LIBS
# ifndef OPENDDS_SAFETY_PROFILE
#include <dds/DCPS/transport/udp/Udp.h>
#include <dds/DCPS/transport/multicast/Multicast.h>
#include <dds/DCPS/RTPS/RtpsDiscovery.h>
#include <dds/DCPS/transport/shmem/Shmem.h>
# endif
#include <dds/DCPS/transport/rtps_udp/RtpsUdp.h>
#endif

#include <dds/DCPS/transport/framework/TransportRegistry.h>
#include <dds/DCPS/transport/framework/TransportConfig.h>
#include <dds/DCPS/transport/framework/TransportInst.h>

#include "DataReaderListener.h"
#include "SecurityAttributesMessageTypeSupportImpl.h"
#include "Args.h"

#include <dds/DCPS/security/framework/Properties.h>

#define CLEAN_ERROR_RETURN(stuff, val) \
do { \
  ACE_ERROR(stuff); \
  std::cerr << "deleting contained entities" << std::endl; \
  participant->delete_contained_entities(); \
  std::cerr << "deleting participant" << std::endl; \
  dpf->delete_participant(participant.in()); \
  std::cerr << "shutdown" << std::endl; \
  TheServiceParticipant->shutdown(); \
  return val; \
} while (0);

#define CLEAN2_ERROR_RETURN(stuff, val) \
do { \
  ACE_ERROR(stuff); \
  ws->detach_condition(condition); \
  std::cerr << "deleting contained entities" << std::endl; \
  participant->delete_contained_entities(); \
  std::cerr << "deleting participant" << std::endl; \
  dpf->delete_participant(participant.in()); \
  std::cerr << "shutdown" << std::endl; \
  TheServiceParticipant->shutdown(); \
  return val; \
} while (0);

void append(DDS::PropertySeq& props, const char* name, const char* value)
{
  const DDS::Property_t prop = {name, value, false /*propagate*/};
  const unsigned int len = props.length();
  props.length(len + 1);
  props[len] = prop;
}

using SecurityAttributes::Args;

int run_test(int argc, ACE_TCHAR *argv[], Args& my_args) {
  int status = 0;
  try {
    // Initialize DomainParticipantFactory
    DDS::DomainParticipantFactory_var dpf =
      TheParticipantFactoryWithArgs(argc, argv);

    int error;
    if ((error = Args::parse_args(argc, argv, my_args)) != 0) {
      return error;
    }

    DDS::DomainParticipantQos part_qos;
    dpf->get_default_participant_qos(part_qos);

    if (TheServiceParticipant->get_security()) {
      DDS::PropertySeq& props = part_qos.property.value;
      append(props, DDS::Security::Properties::AuthIdentityCA, my_args.auth_ca_file_.data());
      append(props, DDS::Security::Properties::AuthIdentityCertificate, my_args.id_cert_file_.data());
      append(props, DDS::Security::Properties::AuthPrivateKey, my_args.id_key_file_.data());
      append(props, DDS::Security::Properties::AccessPermissionsCA, my_args.perm_ca_file_.data());
      append(props, DDS::Security::Properties::AccessGovernance, my_args.governance_file_.data());
      append(props, DDS::Security::Properties::AccessPermissions, my_args.permissions_file_.data());
    }

    // Create DomainParticipant
    DDS::DomainParticipant_var participant =
      dpf->create_participant(my_args.domain_,
                              part_qos,
                              DDS::DomainParticipantListener::_nil(),
                              OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (CORBA::is_nil(participant.in())) {
      ACE_ERROR_RETURN((LM_WARNING,
                        ACE_TEXT("(%P|%t) %N:%l - WARNING: ")
                        ACE_TEXT("main() - create_participant() failed!\n")), -21);
    }

    // Register Type (SecurityAttributes::Message)
    SecurityAttributes::MessageTypeSupport_var ts =
      new SecurityAttributes::MessageTypeSupportImpl();

    if (ts->register_type(participant.in(), "") != DDS::RETCODE_OK) {
      CLEAN_ERROR_RETURN((LM_WARNING,
                          ACE_TEXT("(%P|%t) %N:%l - WARNING: ")
                          ACE_TEXT("main() - register_type() failed!\n")), -22);
    }

    // Create Topic (Movie Discussion List)
    CORBA::String_var type_name = ts->get_type_name();
    DDS::Topic_var topic =
      participant->create_topic(my_args.topic_name_.data(),
                                type_name.in(),
                                TOPIC_QOS_DEFAULT,
                                DDS::TopicListener::_nil(),
                                OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (CORBA::is_nil(topic.in())) {
      CLEAN_ERROR_RETURN((LM_WARNING,
                          ACE_TEXT("(%P|%t) %N:%l - WARNING: ")
                          ACE_TEXT("main() - create_topic() failed!\n")), -23);
    }

    // Create Subscriber
    DDS::Subscriber_var sub =
      participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT,
                                     DDS::SubscriberListener::_nil(),
                                     OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (CORBA::is_nil(sub.in())) {
      CLEAN_ERROR_RETURN((LM_WARNING,
                          ACE_TEXT("(%P|%t) %N:%l - WARNING: ")
                          ACE_TEXT("main() - create_subscriber() failed!\n")), -24);
    }

    // Create DataReader
    DataReaderListenerImpl* const listener_servant = new DataReaderListenerImpl(my_args);
    DDS::DataReaderListener_var listener(listener_servant);

    DDS::DataReaderQos dr_qos;
    sub->get_default_datareader_qos(dr_qos);
    if (DataReaderListenerImpl::is_reliable()) {
      std::cout << "Reliable DataReader" << std::endl;
      dr_qos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;
    }

    DDS::DataReader_var reader =
      sub->create_datareader(topic.in(),
                             dr_qos,
                             listener.in(),
                             OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (CORBA::is_nil(reader.in())) {
      CLEAN_ERROR_RETURN((LM_WARNING,
                          ACE_TEXT("(%P|%t) %N:%l - WARNING: ")
                          ACE_TEXT("main() - create_datareader() failed!\n")), -25);
    }

    // Block until Publisher completes
    DDS::StatusCondition_var condition = reader->get_statuscondition();
    condition->set_enabled_statuses(DDS::SUBSCRIPTION_MATCHED_STATUS);

    DDS::WaitSet_var ws = new DDS::WaitSet;
    ws->attach_condition(condition);

    DDS::Duration_t timeout;
    if (my_args.timeout_ == 0) {
      timeout.sec = DDS::DURATION_INFINITE_SEC;
      timeout.nanosec = DDS::DURATION_INFINITE_NSEC;
    } else {
      timeout.sec = my_args.timeout_;
      timeout.nanosec = 0;
    }
    ACE_Time_Value deadline = OpenDDS::DCPS::duration_to_absolute_time_value(timeout, ACE_OS::gettimeofday());

    DDS::ConditionSeq conditions;
    DDS::SubscriptionMatchedStatus matches = { 0, 0, 0, 0, 0 };

    ACE_Time_Value current_time = ACE_OS::gettimeofday();
    while (current_time < deadline) {
      if (reader->get_subscription_matched_status(matches) != DDS::RETCODE_OK) {
        CLEAN2_ERROR_RETURN((LM_WARNING,
                             ACE_TEXT("(%P|%t) %N:%l - WARNING: ")
                             ACE_TEXT("main() - get_subscription_matched_status() failed!\n")), -26);
      }
      if (matches.current_count == 0 && matches.total_count > 0) {
        break;
      }

      DDS::ReturnCode_t rc = ws->wait(conditions, timeout);
      if (rc == DDS::RETCODE_TIMEOUT) {
        CLEAN2_ERROR_RETURN((LM_WARNING,
                             ACE_TEXT("(%P|%t) %N:%l - WARNING: ")
                             ACE_TEXT("main() - wait() timed out!\n")), -27);
      } else if (rc != DDS::RETCODE_OK) {
        CLEAN2_ERROR_RETURN((LM_WARNING,
                             ACE_TEXT("(%P|%t) %N:%l - WARNING: ")
                             ACE_TEXT("main() - wait() failed!\n")), -28);
      }
      current_time = ACE_OS::gettimeofday();
    }

    status = listener_servant->is_valid() ? 0 : -29;

    ws->detach_condition(condition);

    // Clean-up!
    std::cerr << "deleting contained entities" << std::endl;
    participant->delete_contained_entities();
    std::cerr << "deleting participant" << std::endl;
    dpf->delete_participant(participant.in());
    std::cerr << "shutdown" << std::endl;
    TheServiceParticipant->shutdown();

  } catch (const CORBA::Exception& e) {
    e._tao_print_exception("Exception caught in main():");
    status = -30;
  }

  return status;
}

int
ACE_TMAIN(int argc, ACE_TCHAR *argv[])
{
  Args my_args;

  int result = run_test(argc, argv, my_args);
  if (result == my_args.expected_result_) {
    return 0;
  } else {
    std::cerr << "Subscriber exiting with unexpected result: " << result << std::endl;
    return result == 0 ? -1 : result; // If unexpected result is zero (we expected a failure, but got a success), return -1 to signal error
  }
}
