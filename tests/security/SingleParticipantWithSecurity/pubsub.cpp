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
#include <dds/DCPS/SubscriberImpl.h>
#include <dds/DCPS/Service_Participant.h>

#include "dds/DCPS/StaticIncludes.h"

#ifdef ACE_AS_STATIC_LIBS
# ifndef OPENDDS_SAFETY_PROFILE
#include <dds/DCPS/transport/udp/Udp.h>
#include <dds/DCPS/transport/multicast/Multicast.h>
#include <dds/DCPS/RTPS/RtpsDiscovery.h>
#include <dds/DCPS/transport/shmem/Shmem.h>
#  ifdef OPENDDS_SECURITY
#  include "dds/DCPS/security/BuiltInPlugins.h"
#  endif
# endif
#include <dds/DCPS/transport/rtps_udp/RtpsUdp.h>
#endif

#include <cstdlib>

#include "SingleParticipantWithSecurityTypeSupportImpl.h"
#include "Writer.h"
#include "SingleParticipantWithSecurity.h"
#include "DataReaderListener.h"

#ifdef OPENDDS_SECURITY
#include <dds/DCPS/security/framework/Properties.h>

const char auth_ca_file_from_tests[] = "security/certs/identity/identity_ca_cert.pem";
const char perm_ca_file_from_tests[] = "security/certs/permissions/permissions_ca_cert.pem";
const char id_cert_file_from_tests[] = "security/certs/identity/test_participant_01_cert.pem";
const char id_key_file_from_tests[] = "security/certs/identity/test_participant_01_private_key.pem";
const char governance_file[] = "file:./governance_signed.p7s";
const char permissions_file[] = "file:./permissions_1_signed.p7s";
#endif

bool dw_reliable() {
  OpenDDS::DCPS::TransportConfig_rch gc = TheTransportRegistry->global_config();
  return !(gc->instances_[0]->transport_type_ == "udp");
}

void append(DDS::PropertySeq& props, const char* name, const char* value, bool propagate = false)
{
  const DDS::Property_t prop = {name, value, propagate};
  const unsigned int len = props.length();
  props.length(len + 1);
  props[len] = prop;
}

int ACE_TMAIN(int argc, ACE_TCHAR *argv[])
{
  int status = EXIT_SUCCESS;
  DDS::DomainParticipantFactory_var dpf;
  DDS::DomainParticipant_var participant;

  try {

    std::cout << "Starting publisher" << std::endl;
    {
      // Initialize DomainParticipantFactory
      dpf = TheParticipantFactoryWithArgs(argc, argv);

      std::cout << "Starting publisher with " << argc << " args" << std::endl;

      DDS::DomainParticipantQos part_qos;
      dpf->get_default_participant_qos(part_qos);

      DDS::PropertySeq& props = part_qos.property.value;

#ifdef OPENDDS_SECURITY
      // Determine the path to the keys
      OPENDDS_STRING path_to_tests;
      const char* dds_root = ACE_OS::getenv("DDS_ROOT");
      if (dds_root && dds_root[0]) {
        // Use DDS_ROOT in case we are one of the CMake tests
        path_to_tests = OPENDDS_STRING("file:") + dds_root + "/tests/";
      } else {
        // Else if DDS_ROOT isn't defined try to do it relative to the traditional location
        path_to_tests = "file:../../";
      }
      const OPENDDS_STRING auth_ca_file = path_to_tests + auth_ca_file_from_tests;
      const OPENDDS_STRING perm_ca_file = path_to_tests + perm_ca_file_from_tests;
      const OPENDDS_STRING id_cert_file = path_to_tests + id_cert_file_from_tests;
      const OPENDDS_STRING id_key_file = path_to_tests + id_key_file_from_tests;
      if (TheServiceParticipant->get_security()) {
        append(props, DDS::Security::Properties::AuthIdentityCA, auth_ca_file.c_str());
        append(props, DDS::Security::Properties::AuthIdentityCertificate, id_cert_file.c_str());
        append(props, DDS::Security::Properties::AuthPrivateKey, id_key_file.c_str());
        append(props, DDS::Security::Properties::AccessPermissionsCA, perm_ca_file.c_str());
        append(props, DDS::Security::Properties::AccessGovernance, governance_file);
        append(props, DDS::Security::Properties::AccessPermissions, permissions_file);
      }
#endif

      // Create DomainParticipant
      participant = dpf->create_participant(4,
                                part_qos,
                                DDS::DomainParticipantListener::_nil(),
                                OpenDDS::DCPS::DEFAULT_STATUS_MASK);

      if (!participant) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("%N:%l: main()")
                          ACE_TEXT(" ERROR: create_participant failed!\n")),
                         EXIT_FAILURE);
      }

      // Register TypeSupport (SingleParticipantWithSecurity::Message)
      SingleParticipantWithSecurity::MessageTypeSupport_var mts =
        new SingleParticipantWithSecurity::MessageTypeSupportImpl();

      if (mts->register_type(participant.in(), "") != DDS::RETCODE_OK) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("%N:%l: main()")
                          ACE_TEXT(" ERROR: register_type failed!\n")),
                         EXIT_FAILURE);
      }

      // Create Topic
      CORBA::String_var type_name = mts->get_type_name();
      DDS::Topic_var topic =
        participant->create_topic("Movie Discussion List",
                                  type_name.in(),
                                  TOPIC_QOS_DEFAULT,
                                  DDS::TopicListener::_nil(),
                                  OpenDDS::DCPS::DEFAULT_STATUS_MASK);

      if (!topic) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("%N:%l: main()")
                          ACE_TEXT(" ERROR: create_topic failed!\n")),
                         EXIT_FAILURE);
      }

      // Create Publisher
      DDS::Publisher_var pub =
        participant->create_publisher(PUBLISHER_QOS_DEFAULT,
                                      DDS::PublisherListener::_nil(),
                                      OpenDDS::DCPS::DEFAULT_STATUS_MASK);

      if (!pub) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("%N:%l: main()")
                          ACE_TEXT(" ERROR: create_publisher failed!\n")),
                         EXIT_FAILURE);
      }

      DDS::DataWriterQos qos;
      pub->get_default_datawriter_qos(qos);
      if (dw_reliable()) {
        std::cout << "Reliable DataWriter" << std::endl;
        qos.history.kind = DDS::KEEP_ALL_HISTORY_QOS;
        qos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;
      }

      // Create DataWriter
      DDS::DataWriter_var dw =
        pub->create_datawriter(topic.in(),
                               qos,
                               DDS::DataWriterListener::_nil(),
                               OpenDDS::DCPS::DEFAULT_STATUS_MASK);

      if (!dw) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("%N:%l: main()")
                          ACE_TEXT(" ERROR: create_datawriter failed!\n")),
                         EXIT_FAILURE);
      }

      // Create Subscriber
      DDS::Subscriber_var sub =
        participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT,
          DDS::SubscriberListener::_nil(),
          OpenDDS::DCPS::DEFAULT_STATUS_MASK);

      if (!sub) {
        ACE_ERROR_RETURN((LM_ERROR,
          ACE_TEXT("%N:%l main()")
          ACE_TEXT(" ERROR: create_subscriber() failed!\n")),
          EXIT_FAILURE);
      }

      // Create DataReader
      DataReaderListenerImpl* const listener_servant = new DataReaderListenerImpl;
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

      if (!reader) {
        ACE_ERROR_RETURN((LM_ERROR,
          ACE_TEXT("%N:%l main()")
          ACE_TEXT(" ERROR: create_datareader() failed!\n")),
          EXIT_FAILURE);
      }

      // Start writing threads
      std::cout << "Creating Writer" << std::endl;
      Writer* writer = new Writer(dw.in());
      std::cout << "Starting Writer" << std::endl;
      writer->start();

      while (!writer->is_finished()) {
        ACE_Time_Value small_time(0, 250000);
        ACE_OS::sleep(small_time);
      }

      std::cout << "Writer finished " << std::endl;
      writer->end();

      if (dw_reliable()) {
        std::cout << "Writer wait for ACKS" << std::endl;

        DDS::Duration_t timeout =
          { DDS::DURATION_INFINITE_SEC, DDS::DURATION_INFINITE_NSEC };
        dw->wait_for_acknowledgments(timeout);
      } else {
        // let any missed multicast/rtps messages get re-delivered
        std::cout << "Writer wait small time" << std::endl;
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
    status = EXIT_FAILURE;
  }

  return status;
}
