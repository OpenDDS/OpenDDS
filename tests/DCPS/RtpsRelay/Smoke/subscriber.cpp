/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "Args.h"
#include "DataReaderListener.h"
#include "MessengerTypeSupportImpl.h"
#include "../../common/ConnectionRecordLogger.h"

#include <dds/DCPS/JsonValueWriter.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/RTPS/RtpsDiscovery.h>
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/SubscriberImpl.h>
#include <dds/DCPS/WaitSet.h>
#include <dds/DCPS/transport/framework/TransportRegistry.h>
#include <dds/OpenddsDcpsExtTypeSupportImpl.h>

#ifdef ACE_AS_STATIC_LIBS
#  include <dds/DCPS/RTPS/RtpsDiscovery.h>
#  include <dds/DCPS/transport/rtps_udp/RtpsUdp.h>
#  ifdef OPENDDS_SECURITY
#    include <dds/DCPS/security/BuiltInPlugins.h>
#  endif
#endif
#ifdef OPENDDS_SECURITY
#  include <dds/DCPS/security/framework/Properties.h>
#endif

#include <iostream>

#ifdef OPENDDS_SECURITY
const char auth_ca_file[] = "file:../../../security/certs/identity/identity_ca_cert.pem";
const char perm_ca_file[] = "file:../../../security/certs/permissions/permissions_ca_cert.pem";
const char id_cert_file[] = "file:../../../security/certs/identity/test_participant_03_cert.pem";
const char id_key_file[] = "file:../../../security/certs/identity/test_participant_03_private_key.pem";
const char governance_file[] = "file:./governance_signed.p7s";
const char permissions_file[] = "file:./permissions_subscriber_signed.p7s";

void append(DDS::PropertySeq& props, const char* name, const char* value, bool propagate = false)
{
  const DDS::Property_t prop = {name, value, propagate};
  const unsigned int len = props.length();
  props.length(len + 1);
  props[len] = prop;
}
#endif

bool check_lease_recovery = false;
bool expect_unmatch = false;

bool reliable = false;
bool wait_for_acks = false;

const char USER_DATA[] = "The Subscriber";

int
ACE_TMAIN(int argc, ACE_TCHAR *argv[])
{
  int status = 0;
  try {
    // Initialize DomainParticipantFactory
    DDS::DomainParticipantFactory_var dpf =
      TheParticipantFactoryWithArgs(argc, argv);

    int status = EXIT_SUCCESS;
    if ((status = parse_args(argc, argv)) != EXIT_SUCCESS) {
      return status;
    }

    OpenDDS::DCPS::RcHandle<OpenDDS::DCPS::TransportInst> transport_inst = TheTransportRegistry->get_inst("sub_rtps");
    if (transport_inst) {
      transport_inst->count_messages(true);
    }

    DDS::DomainParticipantQos part_qos;
    dpf->get_default_participant_qos(part_qos);
    part_qos.user_data.value.length(static_cast<unsigned int>(std::strlen(USER_DATA)));
    std::memcpy(part_qos.user_data.value.get_buffer(), USER_DATA, std::strlen(USER_DATA));

#if defined(OPENDDS_SECURITY)
    if (TheServiceParticipant->get_security()) {
      DDS::PropertySeq& props = part_qos.property.value;
      append(props, DDS::Security::Properties::AuthIdentityCA, auth_ca_file);
      append(props, DDS::Security::Properties::AuthIdentityCertificate, id_cert_file);
      append(props, DDS::Security::Properties::AuthPrivateKey, id_key_file);
      append(props, DDS::Security::Properties::AccessPermissionsCA, perm_ca_file);
      append(props, DDS::Security::Properties::AccessGovernance, governance_file);
      append(props, DDS::Security::Properties::AccessPermissions, permissions_file);
    }
#endif

    // Create DomainParticipant
    DDS::DomainParticipant_var participant =
      dpf->create_participant(42,
                              part_qos,
                              DDS::DomainParticipantListener::_nil(),
                              OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (CORBA::is_nil(participant.in())) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l main()")
                        ACE_TEXT(" ERROR: create_participant() failed!\n")), 1);
    }

      OpenDDS::DCPS::DomainParticipantImpl* dp_impl =
        dynamic_cast<OpenDDS::DCPS::DomainParticipantImpl*>(participant.in());

      OpenDDS::DCPS::RcHandle<OpenDDS::RTPS::RtpsDiscovery> disc = OpenDDS::DCPS::static_rchandle_cast<OpenDDS::RTPS::RtpsDiscovery>(TheServiceParticipant->get_discovery(42));
      const OpenDDS::DCPS::GUID_t guid = dp_impl->get_id();
      OpenDDS::DCPS::RcHandle<OpenDDS::DCPS::TransportInst> discovery_inst = disc->sedp_transport_inst(42, guid);
      discovery_inst->count_messages(true);

    OpenDDS::Test::install_connection_record_logger(participant);

    // Register Type (Messenger::Message)
    Messenger::MessageTypeSupport_var ts =
      new Messenger::MessageTypeSupportImpl();

    if (ts->register_type(participant.in(), "") != DDS::RETCODE_OK) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l main()")
                        ACE_TEXT(" ERROR: register_type() failed!\n")), 1);
    }

    // Create Topic (Movie Discussion List)
    CORBA::String_var type_name = ts->get_type_name();
    DDS::Topic_var topic =
      participant->create_topic("Movie Discussion List",
                                type_name.in(),
                                TOPIC_QOS_DEFAULT,
                                DDS::TopicListener::_nil(),
                                OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (CORBA::is_nil(topic.in())) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l main()")
                        ACE_TEXT(" ERROR: create_topic() failed!\n")), 1);
    }

    // Create Subscriber
    DDS::SubscriberQos subscriber_qos;
    participant->get_default_subscriber_qos(subscriber_qos);
    subscriber_qos.partition.name.length(1);
    subscriber_qos.partition.name[0] = "?CI";

    DDS::Subscriber_var sub =
      participant->create_subscriber(subscriber_qos,
                                     DDS::SubscriberListener::_nil(),
                                     OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (CORBA::is_nil(sub.in())) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l main()")
                        ACE_TEXT(" ERROR: create_subscriber() failed!\n")), 1);
    }

    // Create DataReader
    DataReaderListenerImpl* const listener_servant = new DataReaderListenerImpl;
    DDS::DataReaderListener_var listener(listener_servant);

    DDS::DataReaderQos dr_qos;
    sub->get_default_datareader_qos(dr_qos);
    std::cout << "Reliable DataReader" << std::endl;
    dr_qos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;

    DDS::DataReader_var reader =
      sub->create_datareader(topic.in(),
                             dr_qos,
                             listener.in(),
                             OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (CORBA::is_nil(reader.in())) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l main()")
                        ACE_TEXT(" ERROR: create_datareader() failed!\n")), 1);
    }

    // Block until Publisher completes
    DDS::StatusCondition_var condition = reader->get_statuscondition();
    condition->set_enabled_statuses(DDS::SUBSCRIPTION_MATCHED_STATUS);

    DDS::WaitSet_var ws = new DDS::WaitSet;
    ws->attach_condition(condition);

    DDS::Duration_t timeout =
      { DDS::DURATION_INFINITE_SEC, DDS::DURATION_INFINITE_NSEC };

    DDS::ConditionSeq conditions;
    DDS::SubscriptionMatchedStatus matches = { 0, 0, 0, 0, 0 };

    while (true) {
      if (reader->get_subscription_matched_status(matches) != DDS::RETCODE_OK) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("%N:%l main()")
                          ACE_TEXT(" ERROR: get_subscription_matched_status() failed!\n")), 1);
      }
      if (!expect_unmatch) {
        if (matches.current_count == 0 && matches.total_count > 0) {
          break;
        }
      } else {
        if (matches.current_count == 1 && matches.total_count > 1) {
          listener_servant->mark_rediscovered();
        }
        if (matches.current_count == 0 && matches.total_count > 1) {
          break;
        }
      }
      if (ws->wait(conditions, timeout) != DDS::RETCODE_OK) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("%N:%l main()")
                          ACE_TEXT(" ERROR: wait() failed!\n")), 1);
      }
    }

    status = listener_servant->is_valid(check_lease_recovery, expect_unmatch) ? EXIT_SUCCESS : EXIT_FAILURE;

    ws->detach_condition(condition);

#if OPENDDS_HAS_JSON_VALUE_WRITER
    std::cout << "Subscriber Guid: " << OpenDDS::DCPS::LogGuid(guid).c_str() << std::endl;
    OpenDDS::DCPS::TransportStatisticsSequence stats;
    disc->append_transport_statistics(42, guid, stats);
    if (transport_inst) {
      transport_inst->append_transport_statistics(stats);
    }

    for (unsigned int i = 0; i != stats.length(); ++i) {
      std::cout << "Subscriber Transport Statistics: " << OpenDDS::DCPS::to_json(stats[i]) << std::endl;
    }
#endif

    // Clean-up!
    participant->delete_contained_entities();
    dpf->delete_participant(participant.in());
    TheServiceParticipant->shutdown();

  } catch (const CORBA::Exception& e) {
    e._tao_print_exception("Exception caught in main():");
    status = 1;
  }

  return status;
}
