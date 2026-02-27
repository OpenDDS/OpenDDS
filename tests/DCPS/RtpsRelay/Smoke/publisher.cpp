/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "Args.h"
#include "DataWriterListenerImpl.h"
#include "MessengerTypeSupportImpl.h"
#include "ParticipantBit.h"

#include <dds/DCPS/JsonValueWriter.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/PublisherImpl.h>
#include <dds/DCPS/Qos_Helper.h>
#include <dds/DCPS/Service_Participant.h>

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

#include <ace/Log_Msg.h>
#include <ace/OS_NS_stdlib.h>
#include <ace/OS_NS_unistd.h>

#if OPENDDS_CONFIG_SECURITY
const char auth_ca_file[] = "file:../../../security/certs/identity/identity_ca_cert.pem";
const char perm_ca_file[] = "file:../../../security/certs/permissions/permissions_ca_cert.pem";
const char id_cert_file[] = "file:../../../security/certs/identity/test_participant_02_cert.pem";
const char id_key_file[] = "file:../../../security/certs/identity/test_participant_02_private_key.pem";
const char governance_file[] = "file:./governance_signed.p7s";
const char permissions_file[] = "file:./permissions_publisher_signed.p7s";
#endif

Args args;

const char USER_DATA[] = "The Publisher";
const char OTHER_USER_DATA[] = "The Subscriber";

void writer_test(DistributedConditionSet_rch dcs,
                 const DDS::DataWriter_var& dw)
{
  // Write samples
  Messenger::MessageDataWriter_var message_dw = Messenger::MessageDataWriter::_narrow(dw.in());

  Messenger::Message message;
  message.subject_id = 99;
  message.from         = "Comic Book Guy";
  message.subject      = "Review";
  message.text         = "Worst. Movie. Ever.";
  message.count        = 0;

  {
    const DDS::ReturnCode_t error = message_dw->write(message, DDS::HANDLE_NIL);
    if (error != DDS::RETCODE_OK) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("%N:%l: svc()")
                 ACE_TEXT(" ERROR: write returned %C!\n"), OpenDDS::DCPS::retcode_to_string(error)));
    }
  }

  if (args.check_lease_recovery) {
    dcs->wait_for("Publisher", "Subscriber", "count_0");
    if (args.expect_unmatch) {
      dcs->wait_for("Publisher", "Publisher", "on_publication_matched_2_1_1_1");
    } else {
      dcs->wait_for("Publisher", "Subscriber", "on_subscription_matched_2_1_1_1");
    }
  }

  message.count        = 1;
  {
    const DDS::ReturnCode_t error = message_dw->write(message, DDS::HANDLE_NIL);
    if (error != DDS::RETCODE_OK) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("%N:%l: svc()")
                 ACE_TEXT(" ERROR: write returned %C!\n"), OpenDDS::DCPS::retcode_to_string(error)));
    }
  }
  dcs->wait_for("Publisher", "Subscriber", "count_1");
}

void stress_test(const DDS::DataWriter_var& dw,
                 const DDS::DomainParticipant_var& participant,
                 const CORBA::String_var& type_name,
                 const DDS::Publisher_var& publisher)
{
  // Create additional topics, writers, and populate with data.
  DDS::DataWriterQos qos;
  dw->get_qos(qos);

  Messenger::Message message;
  message.subject_id = 99;
  message.from         = "Comic Book Guy";
  message.subject      = "Review";
  message.text         = "Worst. Movie. Ever.";
  message.count        = 0;

  for (std::size_t idx = 0; idx != 24; ++idx) {
    std::string topic_name = "Movie Discussion List ";
    topic_name += OpenDDS::DCPS::to_dds_string(idx);

    DDS::Topic_var topic =
      participant->create_topic(topic_name.c_str(),
                                type_name.in(),
                                TOPIC_QOS_DEFAULT,
                                DDS::TopicListener::_nil(),
                                OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    DDS::DataWriter_var local_dw =
      publisher->create_datawriter(topic.in(),
                                   qos,
                                   0,
                                   OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    Messenger::MessageDataWriter_var message_local_dw = Messenger::MessageDataWriter::_narrow(local_dw.in());

    for (std::size_t instance = 0, instance_limit = rand() % 26; instance != instance_limit; ++instance) {
      message.subject_id = static_cast<DDS::Int32>(instance);
      message.text = std::string(rand() % (1400 * 6), 'c').c_str();
      message_local_dw->write(message, DDS::HANDLE_NIL);
    }
  }

  // Write samples
  Messenger::MessageDataWriter_var message_dw = Messenger::MessageDataWriter::_narrow(dw.in());

  message.subject_id = 99;
  message.from         = "Comic Book Guy";
  message.subject      = "Review";
  message.text         = "Worst. Movie. Ever.";
  message.count        = 0;

  DDS::WaitSet_var waiter = new DDS::WaitSet;

  ShutdownHandler shutdown_handler;
  waiter->attach_condition(shutdown_handler.guard_);

  DDS::ConditionSeq active;
  const DDS::Duration_t timeout = {1, 0};

  bool running = true;
  DDS::ReturnCode_t ret;
  while (running) {
    ret = waiter->wait(active, timeout);

    if (ret == DDS::RETCODE_TIMEOUT) {
      ret = message_dw->write(message, DDS::HANDLE_NIL);
      if (ret != DDS::RETCODE_OK) {
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("%N:%l: svc()")
                   ACE_TEXT(" ERROR: write returned %C!\n"), OpenDDS::DCPS::retcode_to_string(ret)));
      }
      ++message.count;
      continue;
    }

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

void drain_test(DistributedConditionSet_rch dcs, const DDS::DomainParticipant_var& participant)
{
  DDS::Subscriber_var bits = participant->get_builtin_subscriber();
  DDS::DataReader_var dr = bits->lookup_datareader(OpenDDS::DCPS::BUILT_IN_CONNECTION_RECORD_TOPIC);
  DDS::TopicDescription_var topic = dr->get_topicdescription();
  ACE_UNUSED_ARG(topic);
  DDS::ReadCondition_var read_cond =
    dr->create_readcondition(DDS::NOT_READ_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);
  OpenDDS::DCPS::ConnectionRecordDataReader_var connection_records = OpenDDS::DCPS::ConnectionRecordDataReader::_narrow(dr);

  DDS::WaitSet_var waiter = new DDS::WaitSet;
  waiter->attach_condition(read_cond);
  struct WaitSetCleanup {
    DDS::WaitSet_var& waiter_;
    WaitSetCleanup(DDS::WaitSet_var& waiter) : waiter_(waiter) {}
    ~WaitSetCleanup()
    {
      DDS::ConditionSeq conditions;
      waiter_->get_conditions(conditions);
      waiter_->detach_conditions(conditions);
    }
  } cleanup(waiter);

  bool connected = false;
  constexpr DDS::Duration_t timeout{DDS::DURATION_INFINITE_SEC, DDS::DURATION_INFINITE_NSEC};
  DDS::ConditionSeq active_conditions;

  while (waiter->wait(active_conditions, timeout) == DDS::RETCODE_OK) {
    while (true) {
      OpenDDS::DCPS::ConnectionRecord record;
      DDS::SampleInfo info;
      if (connection_records->take_next_sample(record, info) != DDS::RETCODE_OK) {
        break;
      }
      if (std::string(record.address).find(":4444") == std::string::npos) {
        continue; // not interested in this instance
      }
#if OPENDDS_HAS_JSON_VALUE_WRITER
      ACE_DEBUG((LM_DEBUG, "%C\n", OpenDDS::DCPS::to_json(topic, record, info).c_str()));
#endif
      if (!connected && info.instance_state == DDS::ALIVE_INSTANCE_STATE) {
        connected = true;
        dcs->post("publisher", "connected");
      } else if (connected && (info.instance_state & DDS::NOT_ALIVE_INSTANCE_STATE)) {
        return; // exit on transition from connected to disconnected
      }
    }
  }
  ACE_ERROR((LM_ERROR, "ERROR: failed to wait()\n"));
}

int ACE_TMAIN(int argc, ACE_TCHAR *argv[])
{
  DDS::DomainParticipantFactory_var dpf;
  DDS::DomainParticipant_var participant;

  int status = EXIT_SUCCESS;
  try {

    {
      DistributedConditionSet_rch dcs = OpenDDS::DCPS::make_rch<FileBasedDistributedConditionSet>();

      // Initialize DomainParticipantFactory
      dpf = TheParticipantFactoryWithArgs(argc, argv);

      if ((status = args.parse(argc, argv)) != EXIT_SUCCESS) {
        return status;
      }

      OpenDDS::DCPS::RcHandle<OpenDDS::DCPS::TransportInst> transport_inst = TheTransportRegistry->get_inst("pub_rtps");
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
      participant = dpf->create_participant(42,
                                part_qos,
                                DDS::DomainParticipantListener::_nil(),
                                OpenDDS::DCPS::DEFAULT_STATUS_MASK);

      if (CORBA::is_nil(participant.in())) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("%N:%l: main()")
                          ACE_TEXT(" ERROR: create_participant failed!\n")),
                         1);
      }

      OpenDDS::DCPS::DomainParticipantImpl* dp_impl =
        dynamic_cast<OpenDDS::DCPS::DomainParticipantImpl*>(participant.in());

      OpenDDS::DCPS::RcHandle<OpenDDS::RTPS::RtpsDiscovery> disc = OpenDDS::DCPS::dynamic_rchandle_cast<OpenDDS::RTPS::RtpsDiscovery>(TheServiceParticipant->get_discovery(42));
      const OpenDDS::DCPS::GUID_t guid = dp_impl->get_id();
      OpenDDS::DCPS::RcHandle<OpenDDS::DCPS::TransportInst> discovery_inst = disc->sedp_transport_inst(42, guid);
      discovery_inst->count_messages(true);

      // Register TypeSupport (Messenger::Message)
      Messenger::MessageTypeSupport_var mts =
        new Messenger::MessageTypeSupportImpl();

      if (mts->register_type(participant.in(), "") != DDS::RETCODE_OK) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("%N:%l: main()")
                          ACE_TEXT(" ERROR: register_type failed!\n")),
                         1);
      }

      // Create Topic
      CORBA::String_var type_name = mts->get_type_name();
      DDS::Topic_var topic =
        participant->create_topic("Movie Discussion List",
                                  type_name.in(),
                                  TOPIC_QOS_DEFAULT,
                                  DDS::TopicListener::_nil(),
                                  OpenDDS::DCPS::DEFAULT_STATUS_MASK);

      if (CORBA::is_nil(topic.in())) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("%N:%l: main()")
                          ACE_TEXT(" ERROR: create_topic failed!\n")),
                         1);
      }

      // Create Publisher
      DDS::PublisherQos publisher_qos;
      participant->get_default_publisher_qos(publisher_qos);
      publisher_qos.partition.name.length(1);
      publisher_qos.partition.name[0] = args.override_partition
        ? ACE_TEXT_ALWAYS_CHAR(args.override_partition) : "OCI";

      DDS::Publisher_var pub =
        participant->create_publisher(publisher_qos,
                                      DDS::PublisherListener::_nil(),
                                      OpenDDS::DCPS::DEFAULT_STATUS_MASK);

      if (CORBA::is_nil(pub.in())) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("%N:%l: main()")
                          ACE_TEXT(" ERROR: create_publisher failed!\n")),
                         1);
      }

      DataWriterListenerImpl* const listener_servant = new DataWriterListenerImpl(dcs);
      DDS::DataWriterListener_var listener(listener_servant);

      DDS::DataWriterQos qos;
      pub->get_default_datawriter_qos(qos);
      qos.history.kind = DDS::KEEP_LAST_HISTORY_QOS;
      qos.history.depth = 1;
      qos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;
      qos.durability.kind = DDS::TRANSIENT_LOCAL_DURABILITY_QOS;

      // Create DataWriter
      DDS::DataWriter_var dw =
        pub->create_datawriter(topic.in(),
                               qos,
                               listener.in(),
                               OpenDDS::DCPS::DEFAULT_STATUS_MASK);

      if (CORBA::is_nil(dw.in())) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("%N:%l: main()")
                          ACE_TEXT(" ERROR: create_datawriter failed!\n")),
                         1);
      }

      if (args.participant_bit_expected_instances > 0) {
        status = test_participant_discovery(dcs, participant, args.participant_bit_expected_instances,
                                            USER_DATA, OTHER_USER_DATA, disc->config()->resend_period());

      } else if (args.stress_test) {
        stress_test(dw, participant, type_name, pub);
      } else if (args.drain_test || args.deny_partitions_test) {
        drain_test(dcs, participant);
      } else {
        writer_test(dcs, dw);

#if OPENDDS_HAS_JSON_VALUE_WRITER
        ACE_DEBUG((LM_DEBUG, "Publisher Guid: %C\n", OpenDDS::DCPS::LogGuid(guid).c_str()));
        OpenDDS::DCPS::TransportStatisticsSequence stats;
        disc->append_transport_statistics(42, guid, stats);
        transport_inst->append_transport_statistics(stats, 42, dp_impl);

        for (unsigned int i = 0; i != stats.length(); ++i) {
          ACE_DEBUG((LM_DEBUG, "Publisher Transport Statistics: %C\n", OpenDDS::DCPS::to_json(stats[i]).c_str()));
        }
#endif
      }
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
