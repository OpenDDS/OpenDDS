/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "Args.h"
#include "DataReaderListener.h"
#include "MessengerTypeSupportImpl.h"
#include "ParticipantBit.h"

#include <dds/DCPS/JsonValueWriter.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/Qos_Helper.h>
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/SubscriberImpl.h>
#include <dds/DCPS/WaitSet.h>

#include <dds/DCPS/RTPS/RtpsDiscovery.h>

#include <dds/DCPS/transport/framework/TransportRegistry.h>

#include <dds/OpenDDSConfigWrapper.h>
#include <dds/OpenddsDcpsExtTypeSupportImpl.h>

#ifdef ACE_AS_STATIC_LIBS
#  include <dds/DCPS/RTPS/RtpsDiscovery.h>
#  include <dds/DCPS/transport/rtps_udp/RtpsUdp.h>
#  if OPENDDS_CONFIG_SECURITY
#    include <dds/DCPS/security/BuiltInPlugins.h>
#  endif
#endif
#if OPENDDS_CONFIG_SECURITY
#  include <dds/DCPS/security/framework/Properties.h>
#endif

#include <iostream>
#include <cstdlib>

#if OPENDDS_CONFIG_SECURITY
const char auth_ca_file[] = "file:../../../security/certs/identity/identity_ca_cert.pem";
const char perm_ca_file[] = "file:../../../security/certs/permissions/permissions_ca_cert.pem";
const char id_cert_file[] = "file:../../../security/certs/identity/test_participant_03_cert.pem";
const char id_key_file[] = "file:../../../security/certs/identity/test_participant_03_private_key.pem";
const char governance_file[] = "file:./governance_signed.p7s";
const char permissions_file[] = "file:./permissions_subscriber_signed.p7s";
#endif

Args args;

const char USER_DATA[] = "The Subscriber";
const char OTHER_USER_DATA[] = "The Publisher";

void stress_test(const DDS::DataReader_var& dr,
                 const DDS::DomainParticipant_var& participant,
                 const CORBA::String_var& type_name,
                 const DDS::Subscriber_var& subscriber)
{
  // Create additional topics and readers.
  DDS::DataReaderQos qos;
  dr->get_qos(qos);

  for (std::size_t idx = 0; idx != 24; ++idx) {
    std::string topic_name = "Movie Discussion List ";
    topic_name += OpenDDS::DCPS::to_dds_string(idx);

    DDS::Topic_var topic =
      participant->create_topic(topic_name.c_str(),
                                type_name.in(),
                                TOPIC_QOS_DEFAULT,
                                DDS::TopicListener::_nil(),
                                OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    DataReaderListenerImpl* const listener_servant = new DataReaderListenerImpl(DistributedConditionSet_rch());
    DDS::DataReaderListener_var listener(listener_servant);

    DDS::DataReader_var local_dr =
      subscriber->create_datareader(topic.in(),
                                    qos,
                                    listener,
                                    OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  }

  DDS::WaitSet_var waiter = new DDS::WaitSet;

  ShutdownHandler shutdown_handler;
  waiter->attach_condition(shutdown_handler.guard_);

  DDS::ConditionSeq active;
  const DDS::Duration_t timeout = {DDS::DURATION_INFINITE_SEC, DDS::DURATION_INFINITE_NSEC};

  bool running = true;
  DDS::ReturnCode_t ret;
  while (running) {
    ret = waiter->wait(active, timeout);

    if (ret != DDS::RETCODE_OK) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("%N:%l: svc()")
                 ACE_TEXT(" ERROR: wait returned %C!\n"), OpenDDS::DCPS::retcode_to_string(ret)));
      running = false;
      break;
    }

    for (unsigned i = 0; running && active.length() > i; ++i) {
      if (active[i] == shutdown_handler.guard_) {
        running = false;
      }
    }
  }
}

int
ACE_TMAIN(int argc, ACE_TCHAR *argv[])
{
  int status = EXIT_SUCCESS;
  try {
    DistributedConditionSet_rch dcs = OpenDDS::DCPS::make_rch<FileBasedDistributedConditionSet>();

    // Initialize DomainParticipantFactory
    DDS::DomainParticipantFactory_var dpf =
      TheParticipantFactoryWithArgs(argc, argv);

    if ((status = args.parse(argc, argv)) != EXIT_SUCCESS) {
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

#if OPENDDS_CONFIG_SECURITY
    if (TheServiceParticipant->get_security()) {
      DDS::PropertySeq& props = part_qos.property.value;
      OpenDDS::DCPS::Qos_Helper::append(props, DDS::Security::Properties::AuthIdentityCA, TheServiceParticipant->config_store()->get("AuthIdentityCA", auth_ca_file));
      OpenDDS::DCPS::Qos_Helper::append(props, DDS::Security::Properties::AuthIdentityCertificate, TheServiceParticipant->config_store()->get("AuthIdentityCertificate", id_cert_file));
      OpenDDS::DCPS::Qos_Helper::append(props, DDS::Security::Properties::AuthPrivateKey, TheServiceParticipant->config_store()->get("AuthPrivateKey", id_key_file));
      OpenDDS::DCPS::Qos_Helper::append(props, DDS::Security::Properties::AccessPermissionsCA, TheServiceParticipant->config_store()->get("AccessPermissionsCA", perm_ca_file));
      OpenDDS::DCPS::Qos_Helper::append(props, DDS::Security::Properties::AccessGovernance, TheServiceParticipant->config_store()->get("AccessGovernance", governance_file));
      OpenDDS::DCPS::Qos_Helper::append(props, DDS::Security::Properties::AccessPermissions, TheServiceParticipant->config_store()->get("AccessPermissions", permissions_file));
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

    OpenDDS::DCPS::RcHandle<OpenDDS::RTPS::RtpsDiscovery> disc = OpenDDS::DCPS::dynamic_rchandle_cast<OpenDDS::RTPS::RtpsDiscovery>(TheServiceParticipant->get_discovery(42));
    const OpenDDS::DCPS::GUID_t guid = dp_impl->get_id();
    OpenDDS::DCPS::RcHandle<OpenDDS::DCPS::TransportInst> discovery_inst = disc->sedp_transport_inst(42, guid);
    discovery_inst->count_messages(true);

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
    subscriber_qos.partition.name[0] = args.override_partition
      ? ACE_TEXT_ALWAYS_CHAR(args.override_partition) : "?CI";

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
    DataReaderListenerImpl* const listener_servant = new DataReaderListenerImpl(dcs);
    DDS::DataReaderListener_var listener(listener_servant);

    DDS::DataReaderQos dr_qos;
    sub->get_default_datareader_qos(dr_qos);
    dr_qos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;
    dr_qos.durability.kind = DDS::TRANSIENT_LOCAL_DURABILITY_QOS;

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

    if (args.participant_bit_expected_instances > 0) {
      status = test_participant_discovery(dcs, participant, args.participant_bit_expected_instances,
                                          USER_DATA, OTHER_USER_DATA, disc->config()->resend_period());
    } else if (args.stress_test) {
      stress_test(reader, participant, type_name, sub);
      status = EXIT_SUCCESS;
    } else {
      dcs->wait_for("Subscriber", "Subscriber", "count_1");
      status = listener_servant->is_valid() ? EXIT_SUCCESS : EXIT_FAILURE;

#if OPENDDS_HAS_JSON_VALUE_WRITER
      ACE_DEBUG((LM_DEBUG, "Subscriber Guid: %C\n", OpenDDS::DCPS::LogGuid(guid).c_str()));
      OpenDDS::DCPS::TransportStatisticsSequence stats;
      disc->append_transport_statistics(42, guid, stats);
      if (transport_inst) {
        transport_inst->append_transport_statistics(stats, 42, dp_impl);
      }

      for (unsigned int i = 0; i != stats.length(); ++i) {
        ACE_DEBUG((LM_DEBUG, "Subscriber Transport Statistics: %C\n", OpenDDS::DCPS::to_json(stats[i]).c_str()));
      }
#endif
    }

    // Clean-up!
    participant->delete_contained_entities();
    dpf->delete_participant(participant.in());
    TheServiceParticipant->shutdown();
  } catch (const CORBA::Exception& e) {
    e._tao_print_exception("Exception caught in main():");
    status = EXIT_FAILURE;
  }

  return status;
}
