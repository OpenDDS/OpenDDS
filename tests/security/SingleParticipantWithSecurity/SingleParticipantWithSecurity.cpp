/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <tests/DCPS/ConsolidatedMessengerIdl/MessengerTypeSupportC.h>
#include <tests/DCPS/ConsolidatedMessengerIdl/MessengerTypeSupportImpl.h>

#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/PublisherImpl.h>
#include <dds/DCPS/SubscriberImpl.h>
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/WaitSet.h>
#include <dds/DCPS/DCPS_Utils.h>
#include <dds/DCPS/security/framework/Properties.h>
#ifdef ACE_AS_STATIC_LIBS
#  include <dds/DCPS/transport/rtps_udp/RtpsUdp.h>
#  include <dds/DCPS/RTPS/RtpsDiscovery.h>
#  include <dds/DCPS/security/BuiltInPlugins.h>
#endif

#include <ace/Get_Opt.h>
#include <ace/Log_Msg.h>
#include <ace/OS_NS_stdlib.h>
#include <ace/OS_NS_unistd.h>

#include <cstdlib>

const char auth_ca_file_from_tests[] = "certs/identity/identity_ca_cert.pem";
const char perm_ca_file_from_tests[] = "certs/permissions/permissions_ca_cert.pem";
const char id_cert_file_from_tests[] = "certs/identity/test_participant_01_cert.pem";
const char id_key_file_from_tests[] = "certs/identity/test_participant_01_private_key.pem";
const char governance_file[] = "file:./governance_signed.p7s";
const char permissions_file[] = "file:./permissions_1_signed.p7s";

void append(DDS::PropertySeq& props, const char* name, const char* value, bool propagate = false)
{
  const DDS::Property_t prop = {name, value, propagate};
  const unsigned int len = props.length();
  props.length(len + 1);
  props[len] = prop;
}

void read(DDS::DataReader_ptr reader)
{
  try {
    Messenger::MessageDataReader_var message_dr =
      Messenger::MessageDataReader::_narrow(reader);

    if (!message_dr) {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("%N:%l: read()")
        ACE_TEXT(" ERROR: _narrow failed!\n")));
      return;
    }

    DDS::ReadCondition_var dr_rc = reader->create_readcondition(DDS::NOT_READ_SAMPLE_STATE,
      DDS::ANY_VIEW_STATE,
      DDS::ALIVE_INSTANCE_STATE);
    DDS::WaitSet_var ws = new DDS::WaitSet;
    ws->attach_condition(dr_rc);
    std::set<int> instances;

    DDS::ConditionSeq active;
    const DDS::Duration_t max_wait = { 10, 0 };
    DDS::ReturnCode_t ret = ws->wait(active, max_wait);

    if (ret == DDS::RETCODE_TIMEOUT) {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("%N:%l: read()")
        ACE_TEXT(" ERROR: reader timedout\n")));
      return;
    } else if (ret != DDS::RETCODE_OK) {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("%N:%l: read()")
        ACE_TEXT(" ERROR: Reader: wait returned %C\n"), OpenDDS::DCPS::retcode_to_string(ret)));
      return;
    }

    ws->detach_condition(dr_rc);
    reader->delete_readcondition(dr_rc);

    Messenger::Message message;
    DDS::SampleInfo si;

    ret = message_dr->take_next_sample(message, si);

    if (ret != DDS::RETCODE_OK) {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("%N:%l: read()")
        ACE_TEXT(" ERROR: unexpected status:%C\n"), OpenDDS::DCPS::retcode_to_string(ret)));
      return;
    }

    std::cout << "SampleInfo.sample_rank = " << si.sample_rank << std::endl;
    std::cout << "SampleInfo.instance_state = " << OpenDDS::DCPS::InstanceState::instance_state_string(si.instance_state) << std::endl;

    if (si.valid_data) {

      std::cout << "Message: subject    = " << message.subject.in() << std::endl
        << "         subject_id = " << message.subject_id << std::endl
        << "         from       = " << message.from.in() << std::endl
        << "         count      = " << message.count << std::endl
        << "         text       = " << message.text.in() << std::endl;

      if (std::string("Comic Book Guy") != message.from.in() &&
        std::string("OpenDDS-Java") != message.from.in()) {
        std::cout << "ERROR: Invalid message.from" << std::endl;
      }
      if (std::string("Review") != message.subject.in()) {
        std::cout << "ERROR: Invalid message.subject" << std::endl;
      }
      if (std::string("Worst. Movie. Ever.") != message.text.in()) {
        std::cout << "ERROR: Invalid message.text" << std::endl;
      }
      if (message.subject_id != 99) {
        std::cout << "ERROR: Invalid message.subject_id" << std::endl;
      }
    } else if (si.instance_state == DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE) {
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("%N:%l: INFO: instance is disposed\n")));
    } else if (si.instance_state == DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE) {
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("%N:%l: INFO: instance is unregistered\n")));
    } else {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("%N:%l: read()")
        ACE_TEXT(" ERROR: unknown instance state: %d\n"),
        si.instance_state));
    }

    return;
  }
  catch (const CORBA::Exception& e) {
    e._tao_print_exception("Exception caught in on_data_available():");
    ACE_OS::exit(EXIT_FAILURE);
    return;
  }
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

      // Determine the path to the keys
      const OpenDDS::DCPS::String path_to_tests = "file:../";

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
      Messenger::MessageTypeSupport_var mts =
        new Messenger::MessageTypeSupportImpl();

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
      qos.history.kind = DDS::KEEP_ALL_HISTORY_QOS;
      qos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;

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
      DDS::DataReaderQos dr_qos;
      sub->get_default_datareader_qos(dr_qos);
      dr_qos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;

      DDS::DataReader_var reader =
        sub->create_datareader(topic.in(),
          dr_qos,
          0,
          OpenDDS::DCPS::DEFAULT_STATUS_MASK);

      if (!reader) {
        ACE_ERROR_RETURN((LM_ERROR,
          ACE_TEXT("%N:%l main()")
          ACE_TEXT(" ERROR: create_datareader() failed!\n")),
          EXIT_FAILURE);
      }

      // Wait for the writer to match the reader
      DDS::StatusCondition_var condition = dw->get_statuscondition();
      condition->set_enabled_statuses(DDS::PUBLICATION_MATCHED_STATUS);
      DDS::WaitSet_var ws = new DDS::WaitSet;
      ws->attach_condition(condition);

      DDS::ConditionSeq conditions;
      DDS::Duration_t timeout = { 10, 0 };
      if (ws->wait(conditions, timeout) != DDS::RETCODE_OK) {
        ACE_ERROR_RETURN((LM_ERROR,
          ACE_TEXT("%N:%l: main()")
          ACE_TEXT(" ERROR: PUBLICATION_MATCHED_STATUS condition wait failed\n")),
          EXIT_FAILURE);
      }

      ws->detach_condition(condition);

      // Write the sample
      Messenger::Message message;
      message.from = "Comic Book Guy";
      message.subject = "Review";
      message.text = "Worst. Movie. Ever.";
      message.count = 0;
      message.subject_id = 99;

      Messenger::MessageDataWriter_var message_dw
        = Messenger::MessageDataWriter::_narrow(dw);
      message_dw->write(message, DDS::HANDLE_NIL);

      // Read the sample
      read(reader);
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
