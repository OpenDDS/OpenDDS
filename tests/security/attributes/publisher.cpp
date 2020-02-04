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
#include <dds/DCPS/Time_Helper.h>

#include "dds/DCPS/StaticIncludes.h"
#include <dds/DCPS/transport/framework/TransportRegistry.h>

#ifdef ACE_AS_STATIC_LIBS
# ifndef OPENDDS_SAFETY_PROFILE
#include <dds/DCPS/transport/udp/Udp.h>
#include <dds/DCPS/transport/multicast/Multicast.h>
#include <dds/DCPS/RTPS/RtpsDiscovery.h>
#include <dds/DCPS/transport/shmem/Shmem.h>
# endif
#include <dds/DCPS/transport/rtps_udp/RtpsUdp.h>
#endif

#include "SecurityAttributesMessageTypeSupportImpl.h"
#include "Writer.h"
#include "Args.h"

#include <iostream>

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

bool dw_reliable() {
  OpenDDS::DCPS::TransportConfig_rch gc = TheTransportRegistry->global_config();
  return !(gc->instances_[0]->transport_type_ == "udp");
}

void append(DDS::PropertySeq& props, const char* name, const char* value)
{
  const DDS::Property_t prop = {name, value, false /*propagate*/};
  const unsigned int len = props.length();
  props.length(len + 1);
  props[len] = prop;
}

using SecurityAttributes::Args;

int run_test(int argc, ACE_TCHAR *argv[], Args& my_args)
{
  try {

    std::cerr << "Starting publisher" << std::endl;

    DDS::DomainParticipantFactory_var dpf;
    DDS::DomainParticipant_var participant;
    {
      // Initialize DomainParticipantFactory
      dpf = TheParticipantFactoryWithArgs(argc, argv);

      int error = Args::parse_args(argc, argv, my_args);
      if (error < 0) {
        return error;
      }

      std::cerr << "Starting publisher with " << argc << " args" << std::endl;

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
      participant = dpf->create_participant(my_args.domain_,
                                part_qos,
                                DDS::DomainParticipantListener::_nil(),
                                OpenDDS::DCPS::DEFAULT_STATUS_MASK);

      if (CORBA::is_nil(participant.in())) {
        ACE_ERROR_RETURN((LM_WARNING,
                          ACE_TEXT("(%P|%t) %N:%l - WARNING: ")
                          ACE_TEXT("main() - create_participant() failed!\n")),
                          -11);
      }

      // Register TypeSupport (SecurityAttributes::Message)
      SecurityAttributes::MessageTypeSupport_var mts =
        new SecurityAttributes::MessageTypeSupportImpl();

      if (mts->register_type(participant.in(), "") != DDS::RETCODE_OK) {
        CLEAN_ERROR_RETURN((LM_WARNING,
                            ACE_TEXT("(%P|%t) %N:%l - WARNING: ")
                            ACE_TEXT("main() - register_type() failed!\n")),
                            -12);
      }

      // Create Topic
      CORBA::String_var type_name = mts->get_type_name();
      DDS::Topic_var topic =
        participant->create_topic(my_args.topic_name_.data(),
                                  type_name.in(),
                                  TOPIC_QOS_DEFAULT,
                                  DDS::TopicListener::_nil(),
                                  OpenDDS::DCPS::DEFAULT_STATUS_MASK);

      if (CORBA::is_nil(topic.in())) {
        CLEAN_ERROR_RETURN((LM_WARNING,
                            ACE_TEXT("(%P|%t) %N:%l - WARNING: ")
                            ACE_TEXT("main() - create_topic failed!\n")),
                            -13);
      }

      // Create Publisher
      DDS::Publisher_var pub =
        participant->create_publisher(PUBLISHER_QOS_DEFAULT,
                                      DDS::PublisherListener::_nil(),
                                      OpenDDS::DCPS::DEFAULT_STATUS_MASK);

      if (CORBA::is_nil(pub.in())) {
        CLEAN_ERROR_RETURN((LM_WARNING,
                            ACE_TEXT("(%P|%t) %N:%l - WARNING: ")
                            ACE_TEXT("main() - create_publisher failed!\n")),
                            -14);
      }

      DDS::DataWriterQos qos;
      pub->get_default_datawriter_qos(qos);
      if (dw_reliable()) {
        std::cerr << "Reliable DataWriter" << std::endl;
        qos.history.kind = DDS::KEEP_ALL_HISTORY_QOS;
        qos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;
      }

      // Create DataWriter
      DDS::DataWriter_var dw =
        pub->create_datawriter(topic.in(),
                               qos,
                               DDS::DataWriterListener::_nil(),
                               OpenDDS::DCPS::DEFAULT_STATUS_MASK);

      if (CORBA::is_nil(dw.in())) {
        CLEAN_ERROR_RETURN((LM_WARNING,
                            ACE_TEXT("(%P|%t) %N:%l - WARNING: ")
                            ACE_TEXT("main() - create_datawriter failed!\n")),
                            -15);
      }

      // Start writing threads
      std::cerr << "Creating Writer" << std::endl;
      Writer* writer = new Writer(dw.in(), my_args);

      DDS::Duration_t timeout;
      if (my_args.timeout_ == 0) {
        timeout.sec = DDS::DURATION_INFINITE_SEC;
        timeout.nanosec = DDS::DURATION_INFINITE_NSEC;
      } else {
        timeout.sec = my_args.timeout_;
        timeout.nanosec = 0;
      }
      ACE_Time_Value deadline = OpenDDS::DCPS::duration_to_absolute_time_value(timeout, ACE_OS::gettimeofday());

      std::cerr << "Starting Writer" << std::endl;
      writer->start();

      ACE_Time_Value current_time = ACE_OS::gettimeofday();
      while (current_time < deadline && !writer->is_finished()) {
        ACE_Time_Value small_time(0, 250000);
        ACE_OS::sleep(small_time);
        current_time = ACE_OS::gettimeofday();
      }

      if (!(current_time < deadline)) {
        CLEAN_ERROR_RETURN((LM_WARNING,
                            ACE_TEXT("(%P|%t) %N:%l - WARNING: ")
                            ACE_TEXT("main() - timeout exceeded!\n")),
                            -16);
      }

      std::cerr << "Writer finished " << std::endl;
      writer->end();

      if (my_args.wait_for_acks_) {
        std::cerr << "Writer wait for ACKS" << std::endl;
        ACE_Time_Value difference = deadline - ACE_OS::gettimeofday();
        DDS::Duration_t ack_timeout = {static_cast<CORBA::Long>(difference.sec()), static_cast<CORBA::ULong>(difference.usec() * 1000)};
        dw->wait_for_acknowledgments(ack_timeout);
      } else {
        // let any missed multicast/rtps messages get re-delivered
        std::cerr << "Writer wait small time" << std::endl;
        ACE_Time_Value small_time(0, 250000);
        ACE_OS::sleep(small_time);
      }

      std::cerr << "deleting DW" << std::endl;
      delete writer;
    }
    // Clean-up!
    std::cerr << "deleting contained entities" << std::endl;
    participant->delete_contained_entities();
    std::cerr << "deleting participant" << std::endl;
    dpf->delete_participant(participant.in());
    std::cerr << "shutdown" << std::endl;
    TheServiceParticipant->shutdown();

  } catch (const CORBA::Exception& e) {
    e._tao_print_exception("Exception caught in main():");
    return -19;
  }

  return 0;
}

int ACE_TMAIN(int argc, ACE_TCHAR *argv[])
{
  Args my_args;

  int result = run_test(argc, argv, my_args);
  if (result == my_args.expected_result_) {
    return 0;
  } else {
    std::cerr << "Publisher exiting with unexpected result: " << result << std::endl;
    return result == 0 ? -1 : result; // If unexpected result is zero (we expected a failure, but got a success), return -1 to signal error
  }
}
