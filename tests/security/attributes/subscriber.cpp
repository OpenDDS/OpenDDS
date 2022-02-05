/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DataReaderListener.h"
#include "SecurityAttributesMessageTypeSupportImpl.h"
#include "Args.h"

#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/SubscriberImpl.h>
#include <dds/DCPS/WaitSet.h>
#include <dds/DCPS/BuiltInTopicUtils.h>
#ifdef ACE_AS_STATIC_LIBS
#  include <dds/DCPS/RTPS/RtpsDiscovery.h>
#  include <dds/DCPS/transport/rtps_udp/RtpsUdp.h>
#  include <dds/DCPS/security/BuiltInPlugins.h>
#endif
#include <dds/DCPS/transport/framework/TransportRegistry.h>
#include <dds/DCPS/transport/framework/TransportConfig.h>
#include <dds/DCPS/transport/framework/TransportInst.h>
#include <dds/DCPS/security/framework/Properties.h>

#include <dds/DdsDcpsInfrastructureC.h>

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

int run_test(int argc, ACE_TCHAR *argv[], Args& my_args)
{
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
      using namespace DDS::Security::Properties;
      DDS::PropertySeq& props = part_qos.property.value;
      append(props, AuthIdentityCA, my_args.auth_ca_file_.data());
      append(props, AuthIdentityCertificate, my_args.id_cert_file_.data());
      append(props, AuthPrivateKey, my_args.id_key_file_.data());
      append(props, AccessPermissionsCA, my_args.perm_ca_file_.data());
      append(props, AccessGovernance, my_args.governance_file_.data());
      append(props, AccessPermissions, my_args.permissions_file_.data());
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

    DDS::SubscriberQos sub_qos = SUBSCRIBER_QOS_DEFAULT;
    if (!my_args.partition_.empty()) {
      participant->get_default_subscriber_qos(sub_qos);
      my_args.partition_to_qos(sub_qos.partition);
    }

    // Create Subscriber
    DDS::Subscriber_var sub =
      participant->create_subscriber(sub_qos,
                                     DDS::SubscriberListener::_nil(),
                                     OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (CORBA::is_nil(sub.in())) {
      CLEAN_ERROR_RETURN((LM_WARNING,
                          ACE_TEXT("(%P|%t) %N:%l - WARNING: ")
                          ACE_TEXT("main() - create_subscriber() failed!\n")), -24);
    }

    // Create DataReaderListener
    DDS::DataReader_var part_reader;
#ifndef DDS_HAS_MINIMUM_BIT
    DDS::Subscriber_var bit_subscriber = participant->get_builtin_subscriber();
    part_reader = bit_subscriber->lookup_datareader(OpenDDS::DCPS::BUILT_IN_PARTICIPANT_TOPIC);
#endif
    DataReaderListenerImpl* const listener_servant = new DataReaderListenerImpl(my_args, part_reader.in());
    DDS::DataReaderListener_var listener(listener_servant);
#ifndef DDS_HAS_MINIMUM_BIT
    part_reader->set_listener(listener.in(), OpenDDS::DCPS::DEFAULT_STATUS_MASK);
#endif

    // Create DataReader
    DDS::DataReaderQos dr_qos;
    sub->get_default_datareader_qos(dr_qos);
    if (DataReaderListenerImpl::is_reliable()) {
      std::cerr << "Reliable DataReader" << std::endl;
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

    const DDS::Duration_t infinite = { DDS::DURATION_INFINITE_SEC, DDS::DURATION_INFINITE_NSEC };
    const DDS::Duration_t one_second = { 1, 0 };

    bool wait_forever = false;
    DDS::Duration_t timeout;
    if (my_args.timeout_ == 0) {
      timeout = infinite;
      wait_forever = true;
    } else {
      timeout.sec = my_args.timeout_;
      timeout.nanosec = 0;
    }

    const ACE_Time_Value deadline = OpenDDS::DCPS::duration_to_absolute_time_value(timeout, ACE_OS::gettimeofday());

    DDS::ConditionSeq conditions;
    DDS::SubscriptionMatchedStatus matches = { 0, 0, 0, 0, 0 };

    ACE_Time_Value current_time = ACE_OS::gettimeofday();
    while (wait_forever || current_time < deadline) {
      if (reader->get_subscription_matched_status(matches) != DDS::RETCODE_OK) {
        CLEAN2_ERROR_RETURN((LM_WARNING,
                             ACE_TEXT("(%P|%t) %N:%l - WARNING: ")
                             ACE_TEXT("main() - get_subscription_matched_status() failed!\n")), -26);
      }
      if (matches.current_count == 0 && matches.total_count > 0) {
        break;
      }

      DDS::ReturnCode_t rc = ws->wait(conditions, one_second);
      if (rc != DDS::RETCODE_TIMEOUT && rc != DDS::RETCODE_OK) {
        CLEAN2_ERROR_RETURN((LM_WARNING,
                             ACE_TEXT("(%P|%t) %N:%l - WARNING: ")
                             ACE_TEXT("main() - wait() failed!\n")), -28);
      }
      current_time = ACE_OS::gettimeofday();
    }
    if (!wait_forever && deadline <= current_time && !(matches.current_count == 0 && matches.total_count > 0)) {
      CLEAN2_ERROR_RETURN((LM_WARNING,
                           ACE_TEXT("(%P|%t) %N:%l - WARNING: ")
                           ACE_TEXT("main() - wait() timed out!\n")), -27);
    }

    status = listener_servant->is_valid() ? 0 : -29;

    ws->detach_condition(condition);

    // Clean-up!

#ifndef DDS_HAS_MINIMUM_BIT
    part_reader->set_listener(0, OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    part_reader = 0;
    bit_subscriber = 0;
#endif

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

int ACE_TMAIN(int argc, ACE_TCHAR *argv[])
{
  Args my_args;
  const int result = run_test(argc, argv, my_args);
  if (result != my_args.expected_result_) {
    std::cerr << "Subscriber exiting with unexpected result: " << result << std::endl;
    return 1;
  }
  return 0;
}
