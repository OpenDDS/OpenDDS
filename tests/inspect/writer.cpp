#include "inspectTypeSupportImpl.h"

#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/WaitSet.h>
#include <dds/DCPS/RTPS/RtpsDiscovery.h>
#include <dds/DCPS/RTPS/RtpsDiscoveryConfig.h>
#include <dds/DCPS/transport/framework/TransportRegistry.h>
#include <dds/DCPS/transport/framework/TransportConfig.h>
#include <dds/DCPS/transport/framework/TransportInst.h>

#include <dds/DdsDcpsInfrastructureC.h>
#include <dds/DdsDcpsPublicationC.h>

using namespace OpenDDS::DCPS;
using namespace OpenDDS::RTPS;

int main(int argc, char* argv[])
{
  try {
    TransportConfig_rch transport_config =
      TheTransportRegistry->create_config("default_rtps_transport_config");
    TransportInst_rch transport_inst =
      TheTransportRegistry->create_inst("default_rtps_transport", "rtps_udp");
    transport_config->instances_.push_back(transport_inst);

    TheTransportRegistry->global_config(transport_config);
    DDS::DomainParticipantFactory_var dpf = TheParticipantFactoryWithArgs(argc, argv);

    OpenDDS::RTPS::RtpsDiscovery_rch disc =
      make_rch<OpenDDS::RTPS::RtpsDiscovery>("rtps_disc");
    disc->use_xtypes(OpenDDS::RTPS::RtpsDiscoveryConfig::XTYPES_COMPLETE);
    Service_Participant* const service = TheServiceParticipant;
    service->add_discovery(static_rchandle_cast<Discovery>(disc));
    service->set_repo_domain(0, disc->key());

    DDS::DomainParticipant_var participant = dpf->create_participant(
      0, PARTICIPANT_QOS_DEFAULT, 0, OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (!participant) {
      return 1;
    }

    StructTopicTypeSupport_var struct_ts = new StructTopicTypeSupportImpl;
    if (struct_ts->register_type(participant, "") != DDS::RETCODE_OK) {
      return 1;
    }

    UnionTopicTypeSupport_var union_ts = new UnionTopicTypeSupportImpl;
    if (union_ts->register_type(participant, "") != DDS::RETCODE_OK) {
      return 1;
    }

    CORBA::String_var struct_name = struct_ts->get_type_name();
    DDS::Topic_var struct_topic = participant->create_topic("Struct Topic",
      struct_name, TOPIC_QOS_DEFAULT, 0, OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (!struct_topic) {
      return 1;
    }

    DDS::Topic_var another_struct_topic = participant->create_topic("Another Struct Topic",
      struct_name, TOPIC_QOS_DEFAULT, 0, OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (!another_struct_topic) {
      return 1;
    }

    CORBA::String_var union_name = union_ts->get_type_name();
    DDS::Topic_var union_topic = participant->create_topic("Union Topic",
      union_name, TOPIC_QOS_DEFAULT, 0, OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (!union_topic) {
      return 1;
    }

    DDS::Publisher_var publisher = participant->create_publisher(
      PUBLISHER_QOS_DEFAULT, 0, OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (!publisher) {
      return 1;
    }

    DDS::DataWriterQos dw_qos;
    publisher->get_default_datawriter_qos(dw_qos);
    dw_qos.history.kind = DDS::KEEP_ALL_HISTORY_QOS;
    dw_qos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;

    DDS::DataWriter_var struct_writer = publisher->create_datawriter(
      struct_topic, dw_qos, 0, OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (!struct_writer) {
      return 1;
    }
    StructTopicDataWriter_var struct_writer_i =
      StructTopicDataWriter::_narrow(struct_writer);
    if (!struct_writer_i) {
      return 1;
    }

    DDS::DataWriter_var another_struct_writer = publisher->create_datawriter(
      another_struct_topic, dw_qos, 0, OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (!another_struct_writer) {
      return 1;
    }
    StructTopicDataWriter_var another_struct_writer_i =
      StructTopicDataWriter::_narrow(another_struct_writer);
    if (!another_struct_writer_i) {
      return 1;
    }

    StructTopic struct_sample;
    struct_sample.u32 = 0;
    struct_sample.str = "Hello Struct";
    struct_sample.array[0] = 1;
    struct_sample.array[1] = 2;
    struct_sample.array[2] = 3;
    struct_sample.array[3] = 4;
    struct_sample.seq.length(4);
    struct_sample.seq[0] = 1;
    struct_sample.seq[1] = 2;
    struct_sample.seq[2] = 3;
    struct_sample.seq[3] = 4;
    struct_sample.inner.value = -16;

    DDS::DataWriter_var union_writer = publisher->create_datawriter(
      union_topic, dw_qos, 0, OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (!union_writer) {
      return 1;
    }
    UnionTopicDataWriter_var union_writer_i =
      UnionTopicDataWriter::_narrow(union_writer);
    if (!union_writer_i) {
      return 1;
    }

    UnionTopic union_sample;
    union_sample.str("Hello Union");

    while (true) {
      ACE_DEBUG((LM_DEBUG, "Writing samples with u32 == %u\n", struct_sample.u32));
      struct_writer_i->write(struct_sample, DDS::HANDLE_NIL);
      another_struct_writer_i->write(struct_sample, DDS::HANDLE_NIL);
      ++struct_sample.u32;
      union_writer_i->write(union_sample, DDS::HANDLE_NIL);
      ACE_OS::sleep(1);
    }

    participant->delete_contained_entities();
    dpf->delete_participant(participant);

    service->shutdown();

  } catch (const CORBA::Exception& e) {
    e._tao_print_exception("Exception caught in main():");
    return 1;
  }

  return 0;
}
