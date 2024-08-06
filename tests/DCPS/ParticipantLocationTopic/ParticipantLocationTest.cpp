/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "BitListener.h"

#include <MessengerTypeSupportImpl.h>
#include <tests/Utils/StatusMatching.h>

#include <dds/DCPS/BuiltInTopicUtils.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/PublisherImpl.h>
#include <dds/DCPS/Service_Participant.h>

#include <dds/OpenDDSConfigWrapper.h>

#include <dds/DCPS/transport/framework/TransportExceptions.h>
#include <dds/DCPS/transport/framework/TransportRegistry.h>
#ifdef ACE_AS_STATIC_LIBS
#  include <dds/DCPS/RTPS/RtpsDiscovery.h>
#  include <dds/DCPS/transport/rtps_udp/RtpsUdp.h>
#  include <dds/DCPS/security/BuiltInPlugins.h>
#endif
#include <dds/DCPS/security/framework/Properties.h>

#include <ace/Get_Opt.h>

// common
const char auth_ca_file_from_tests[] = "security/certs/identity/identity_ca_cert.pem";
const char perm_ca_file_from_tests[] = "security/certs/permissions/permissions_ca_cert.pem";
const char governance_file[] = "file:./governance_signed.p7s";

// per participant
const char pub_id_cert_file_from_tests[] = "security/certs/identity/test_participant_02_cert.pem";
const char pub_id_key_file_from_tests[] = "security/certs/identity/test_participant_02_private_key.pem";
const char pub_permissions_file[] = "file:./permissions_publisher_signed.p7s";

const char sub_id_cert_file_from_tests[] = "security/certs/identity/test_participant_03_cert.pem";
const char sub_id_key_file_from_tests[] = "security/certs/identity/test_participant_03_private_key.pem";
const char sub_permissions_file[] = "file:./permissions_subscriber_signed.p7s";

bool no_ice = false;
bool ipv6 = false;

int parse_args(int argc, ACE_TCHAR* argv[])
{
  ACE_Get_Opt get_opts(argc, argv, ACE_TEXT("n6"));
  int c;

  while ((c = get_opts()) != -1)
    switch (c) {
    case 'n':
      no_ice = true;
      break;
    case '6':
      ipv6 = true;
      break;
    case '?':
    default:
      ACE_ERROR_RETURN((LM_ERROR, "usage: %s\n"
                        "\t-n do not check for ICE connections\n"
                        "\t-6 use IPv6\n",
                        argv[0]),
                       -1);
    }
  // Indicates successful parsing of the command line
  return 0;
}

void append(DDS::PropertySeq& props, const char* name, const char* value, bool propagate = false)
{
  const DDS::Property_t prop = {name, value, propagate};
  const unsigned int len = props.length();
  props.length(len + 1);
  props[len] = prop;
}

ACE_Thread_Mutex participants_done_lock;
ACE_Condition_Thread_Mutex participants_done_cond(participants_done_lock);
int participants_done = 0;

void participants_done_callback()
{
  ACE_Guard<ACE_Thread_Mutex> g(participants_done_lock);
  ++participants_done;
  participants_done_cond.signal();
}

int ACE_TMAIN(int argc, ACE_TCHAR* argv[])
{
  int status = EXIT_SUCCESS;

  try {

    std::cout << "Starting publisher" << std::endl;

    DDS::DomainParticipantFactory_var dpf = TheParticipantFactoryWithArgs(argc, argv);

    if (parse_args(argc, argv) != 0)
      return 1;

    DDS::DomainParticipantQos pub_qos;
    dpf->get_default_participant_qos(pub_qos);

    DDS::DomainParticipantQos sub_qos;
    dpf->get_default_participant_qos(sub_qos);

#if OPENDDS_CONFIG_SECURITY
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
      const OPENDDS_STRING pub_id_cert_file = path_to_tests + pub_id_cert_file_from_tests;
      const OPENDDS_STRING pub_id_key_file = path_to_tests + pub_id_key_file_from_tests;
      const OPENDDS_STRING sub_id_cert_file = path_to_tests + sub_id_cert_file_from_tests;
      const OPENDDS_STRING sub_id_key_file = path_to_tests + sub_id_key_file_from_tests;

      if (TheServiceParticipant->get_security()) {
        DDS::PropertySeq& pub_props = pub_qos.property.value;
        append(pub_props, DDS::Security::Properties::AuthIdentityCA, auth_ca_file.c_str());
        append(pub_props, DDS::Security::Properties::AuthIdentityCertificate, pub_id_cert_file.c_str());
        append(pub_props, DDS::Security::Properties::AuthPrivateKey, pub_id_key_file.c_str());
        append(pub_props, DDS::Security::Properties::AccessPermissionsCA, perm_ca_file.c_str());
        append(pub_props, DDS::Security::Properties::AccessGovernance, governance_file);
        append(pub_props, DDS::Security::Properties::AccessPermissions, pub_permissions_file);

        DDS::PropertySeq& sub_props = sub_qos.property.value;
        append(sub_props, DDS::Security::Properties::AuthIdentityCA, auth_ca_file.c_str());
        append(sub_props, DDS::Security::Properties::AuthIdentityCertificate, sub_id_cert_file.c_str());
        append(sub_props, DDS::Security::Properties::AuthPrivateKey, sub_id_key_file.c_str());
        append(sub_props, DDS::Security::Properties::AccessPermissionsCA, perm_ca_file.c_str());
        append(sub_props, DDS::Security::Properties::AccessGovernance, governance_file);
        append(sub_props, DDS::Security::Properties::AccessPermissions, sub_permissions_file);
      }

#endif

    DDS::DomainParticipant_var participant = dpf->create_participant(42,
                                                                     pub_qos,
                                                                     DDS::DomainParticipantListener::_nil(),
                                                                     OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (!participant) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l: main()")
                        ACE_TEXT(" ERROR: create_participant failed!\n")),
                       EXIT_FAILURE);
    }

    Messenger::MessageTypeSupport_var mts =
      new Messenger::MessageTypeSupportImpl();

    if (mts->register_type(participant, "") != DDS::RETCODE_OK) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l: main()")
                        ACE_TEXT(" ERROR: register_type failed!\n")),
                       EXIT_FAILURE);
    }

    CORBA::String_var type_name = mts->get_type_name();
    DDS::Topic_var topic =
      participant->create_topic("Movie Discussion List",
                                type_name,
                                TOPIC_QOS_DEFAULT,
                                DDS::TopicListener::_nil(),
                                OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (!topic) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l: main()")
                        ACE_TEXT(" ERROR: create_topic failed!\n")),
                       EXIT_FAILURE);
    }

    DDS::PublisherQos publisher_qos;
    participant->get_default_publisher_qos(publisher_qos);
    publisher_qos.partition.name.length(1);
    publisher_qos.partition.name[0] = "OCI";

    DDS::Publisher_var pub =
      participant->create_publisher(publisher_qos,
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
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("%N:%l main()")
               ACE_TEXT(" Reliable DataWriter\n")));
    qos.history.kind = DDS::KEEP_ALL_HISTORY_QOS;
    qos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;

    DDS::DataWriter_var dw =
      pub->create_datawriter(topic,
                             qos,
                             DDS::DataWriterListener::_nil(),
                             OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (!dw) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l: main()")
                        ACE_TEXT(" ERROR: create_datawriter failed!\n")),
                       EXIT_FAILURE);
    }

    DDS::Subscriber_var bit_subscriber = participant->get_builtin_subscriber();

    DDS::DataReader_var pub_loc_dr = bit_subscriber->lookup_datareader(OpenDDS::DCPS::BUILT_IN_PARTICIPANT_LOCATION_TOPIC);
    if (0 == pub_loc_dr) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("%N:%l main()")
                 ACE_TEXT(" ERROR: Could not get %C DataReader\n"),
                 OpenDDS::DCPS::BUILT_IN_PARTICIPANT_LOCATION_TOPIC));
      ACE_OS::exit(EXIT_FAILURE);
    }

    BitListener* listener = new BitListener("Publisher", no_ice, ipv6, participants_done_callback);
    DDS::DataReaderListener_var listener_var(listener);

    CORBA::Long retcode =
      pub_loc_dr->set_listener(listener,
                               OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (retcode != DDS::RETCODE_OK) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("%N:%l main()")
                 ACE_TEXT(" ERROR: set_listener for %C failed\n"),
                 OpenDDS::DCPS::BUILT_IN_PARTICIPANT_LOCATION_TOPIC));
      ACE_OS::exit(EXIT_FAILURE);
    }
    // No need to call on_data_available since subscriber doesn't exist.

    DDS::DataReader_var part_dr = bit_subscriber->lookup_datareader(OpenDDS::DCPS::BUILT_IN_PARTICIPANT_TOPIC);
    if (!part_dr) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("%N:%l main()")
                 ACE_TEXT(" ERROR: Could not get %C DataReader\n"),
                 OpenDDS::DCPS::BUILT_IN_PARTICIPANT_TOPIC));
      ACE_OS::exit(EXIT_FAILURE);
    }

    retcode = part_dr->set_listener(listener, OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (retcode != DDS::RETCODE_OK) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("%N:%l main()")
                 ACE_TEXT(" ERROR: set_listener for %C failed\n"),
                 OpenDDS::DCPS::BUILT_IN_PARTICIPANT_TOPIC));
      ACE_OS::exit(EXIT_FAILURE);
    }

    std::cout << "Starting subscriber" << std::endl;

    DDS::DomainParticipant_var sub_participant = dpf->create_participant(42,
                                                                         sub_qos,
                                                                         DDS::DomainParticipantListener::_nil(),
                                                                         OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (!sub_participant) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l: main()")
                        ACE_TEXT(" ERROR: subscriber create_participant failed!\n")),
                       EXIT_FAILURE);
    }

    Messenger::MessageTypeSupport_var sub_ts =
      new Messenger::MessageTypeSupportImpl();

    if (sub_ts->register_type(sub_participant, "") != DDS::RETCODE_OK) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l main()")
                        ACE_TEXT(" ERROR: register_type() failed!\n")), EXIT_FAILURE);
    }

    CORBA::String_var sub_type_name = sub_ts->get_type_name();
    DDS::Topic_var sub_topic =
      sub_participant->create_topic("Movie Discussion List",
                                    sub_type_name,
                                    TOPIC_QOS_DEFAULT,
                                    DDS::TopicListener::_nil(),
                                    OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (!sub_topic) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l main()")
                        ACE_TEXT(" ERROR: create_topic() failed!\n")), EXIT_FAILURE);
    }

    DDS::SubscriberQos subscriber_qos;
    sub_participant->get_default_subscriber_qos(subscriber_qos);
    subscriber_qos.partition.name.length(1);
    subscriber_qos.partition.name[0] = "?CI";

    DDS::Subscriber_var sub =
      sub_participant->create_subscriber(subscriber_qos,
                                     DDS::SubscriberListener::_nil(),
                                     OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (!sub) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l main()")
                        ACE_TEXT(" ERROR: create_subscriber() failed!\n")), EXIT_FAILURE);
    }

    TheTransportRegistry->bind_config("subscriber_config", sub);

    DDS::DataReaderQos dr_qos;
    sub->get_default_datareader_qos(dr_qos);
    std::cout << "Reliable DataReader" << std::endl;
    dr_qos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;

    DDS::DataReader_var reader =
      sub->create_datareader(sub_topic,
                             dr_qos,
                             0,
                             OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (!reader) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l main()")
                        ACE_TEXT(" ERROR: create_datareader() failed!\n")), EXIT_FAILURE);
    }

    DDS::Subscriber_var sub_bit_subscriber = sub_participant->get_builtin_subscriber();

    DDS::DataReader_var sub_loc_dr = sub_bit_subscriber->lookup_datareader(OpenDDS::DCPS::BUILT_IN_PARTICIPANT_LOCATION_TOPIC);
    if (0 == sub_loc_dr) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("%N:%l main()")
                 ACE_TEXT(" ERROR: Could not get %C DataReader\n"),
                 OpenDDS::DCPS::BUILT_IN_PARTICIPANT_LOCATION_TOPIC));
      ACE_OS::exit(EXIT_FAILURE);
    }

    BitListener* sub_listener = new BitListener("Subscriber", no_ice, ipv6, participants_done_callback);
    DDS::DataReaderListener_var sub_listener_var(sub_listener);

    retcode =
      sub_loc_dr->set_listener(sub_listener,
                               OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (retcode != DDS::RETCODE_OK) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("%N:%l main()")
                 ACE_TEXT(" ERROR: set_listener for %C failed\n"),
                 OpenDDS::DCPS::BUILT_IN_PARTICIPANT_LOCATION_TOPIC));
      ACE_OS::exit(EXIT_FAILURE);
    }

    // Call on_data_available in case there are samples which are waiting
    sub_listener->on_data_available(sub_loc_dr);

    DDS::DataReader_var sub_part_dr = sub_bit_subscriber->lookup_datareader(OpenDDS::DCPS::BUILT_IN_PARTICIPANT_TOPIC);
    if (!sub_loc_dr) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("%N:%l main()")
                 ACE_TEXT(" ERROR: Could not get %C DataReader\n"),
                 OpenDDS::DCPS::BUILT_IN_PARTICIPANT_TOPIC));
      ACE_OS::exit(EXIT_FAILURE);
    }

    retcode = sub_part_dr->set_listener(sub_listener, OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (retcode != DDS::RETCODE_OK) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("%N:%l main()")
                 ACE_TEXT(" ERROR: set_listener for %C failed\n"),
                 OpenDDS::DCPS::BUILT_IN_PARTICIPANT_TOPIC));
      ACE_OS::exit(EXIT_FAILURE);
    }

    // Call on_data_available in case there are samples which are waiting
    sub_listener->on_data_available(sub_part_dr);

    while (participants_done != 2) {
      participants_done_cond.wait();
    }

    // check that all locations received
    if (!listener->check()) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("%N:%l main()")
                 ACE_TEXT(" ERROR: Check for all publisher locations failed\n")));
      status = EXIT_FAILURE;
    }

    if (!sub_listener->check()) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("%N:%l main()")
                 ACE_TEXT(" ERROR: Check for all subscriber locations failed\n")));
      status = EXIT_FAILURE;
    }

    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("%N:%l main()")
               ACE_TEXT(" publisher participant deleting contained entities\n")));
    participant->delete_contained_entities();
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("%N:%l main()")
               ACE_TEXT(" subscriber participant deleting contained entities\n")));
    sub_participant->delete_contained_entities();
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("%N:%l main()")
               ACE_TEXT(" domain participant factory deleting participants\n")));
    dpf->delete_participant(participant);
    dpf->delete_participant(sub_participant);
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("%N:%l main()")
               ACE_TEXT(" shutdown service participant\n")));
    TheServiceParticipant->shutdown();

  } catch (const CORBA::Exception& e) {
    e._tao_print_exception("Exception caught in main():");
    status = EXIT_FAILURE;
  } catch (const OpenDDS::DCPS::Transport::Exception&) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("%N:%l main()")
               ACE_TEXT(" ERROR: Transport Exception\n")));
    status = EXIT_FAILURE;
  }

  return status;
}
