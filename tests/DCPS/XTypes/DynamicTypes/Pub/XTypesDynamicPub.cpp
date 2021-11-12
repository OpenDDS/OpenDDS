#include "dynamicTypeSupportImpl.h"

#include <tests/DCPS/common/TestException.h>
#include <tests/Utils/StatusMatching.h>

#include <dds/DCPS/DCPS_Utils.h>
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/Qos_Helper.h>
#include <dds/DCPS/TopicDescriptionImpl.h>
#include <dds/DCPS/PublisherImpl.h>
#include <dds/DCPS/transport/framework/TransportRegistry.h>
#include <dds/DCPS/transport/framework/TransportSendStrategy.h>
#include <dds/DCPS/StaticIncludes.h>
#include <dds/DCPS/WaitSet.h>
#include <dds/DCPS/BuiltInTopicUtils.h>
#if defined ACE_AS_STATIC_LIBS && !defined OPENDDS_SAFETY_PROFILE
#include <dds/DCPS/transport/udp/Udp.h>
#endif

#include <ace/Arg_Shifter.h>
#include <ace/OS_NS_unistd.h>

using namespace DDS;
/// parse the command line arguments
int parse_args (int argc, ACE_TCHAR *argv[])
{
  u_long mask =  ACE_LOG_MSG->priority_mask(ACE_Log_Msg::PROCESS);
  ACE_LOG_MSG->priority_mask(mask | LM_TRACE | LM_DEBUG, ACE_Log_Msg::PROCESS);
  ACE_Arg_Shifter arg_shifter (argc, argv);
  return 0;
}
void stru_narrow_write(DataWriter_var dw) {
  Dynamic::stru foo;
  foo.my_int8 = 1;
  foo.my_uint8 = 2;
  foo.my_short = 3;
  foo.my_ushort = 4;
  foo.my_long = 5;
  foo.my_ulong = 6;
  foo.my_longlong = 7;
  foo.my_ulonglong = 8;
  foo.my_float = 9;
  foo.my_double = 10;
  foo.my_longdouble = 11;
  foo.my_boolean = true;
  foo.my_byte = 12;
  foo.my_char = 'd';
  foo.my_wchar = L'e';
  foo.my_string = "Hello";
  foo.my_wstring = L"World";
  foo.my_alias_seq.length(2);
  foo.my_alias_seq[0] = true;
  foo.my_alias_seq[1] = false;
  foo.my_alias_array[0] = 'a';
  foo.my_alias_array[1] = 'b';
  foo.my_anon_seq.length(2);
  foo.my_anon_seq[0] = Dynamic::V2;
  foo.my_anon_seq[1] = Dynamic::V1;
  foo.my_anon_arr[0] = 5;
  foo.my_anon_arr[1] = 6;
  Dynamic::struDataWriter_var narrow_dw = Dynamic::struDataWriter::_narrow(dw);
  InstanceHandle_t handle = narrow_dw->register_instance(foo);
  narrow_dw->write(foo, handle);
  ACE_DEBUG((LM_DEBUG, "WRITE\n"));
}

void nested_stru_narrow_write(DataWriter_var dw) {
  Dynamic::inner_struct is;
  Dynamic::outer_struct os;
  is.l = 5;
  os.is = is;
  Dynamic::outer_structDataWriter_var narrow_dw = Dynamic::outer_structDataWriter::_narrow(dw);
  InstanceHandle_t handle = narrow_dw->register_instance(os);
  narrow_dw->write(os, handle);
}

void union_narrow_write(DataWriter_var dw) {
  Dynamic::my_union foo;
  foo._d(1);
  foo.ld(10);
  Dynamic::my_unionDataWriter_var narrow_dw = Dynamic::my_unionDataWriter::_narrow(dw);
  InstanceHandle_t handle = narrow_dw->register_instance(foo);
  narrow_dw->write(foo, handle);
}

void union_default_narrow_write(DataWriter_var dw) {
  Dynamic::my_union foo;
  foo._d(3);
  foo.b(true);
  Dynamic::my_unionDataWriter_var narrow_dw = Dynamic::my_unionDataWriter::_narrow(dw);
  InstanceHandle_t handle = narrow_dw->register_instance(foo);
  narrow_dw->write(foo, handle);
}

int ACE_TMAIN(int argc, ACE_TCHAR *argv[])
{
  int status = 0;
  OpenDDS::DCPS::String type_name = argv[1];
  DomainParticipantFactory_var dpf = TheParticipantFactoryWithArgs(argc, argv);
  parse_args (argc, argv);
  DomainParticipant_var dp =
    dpf->create_participant(153,
                            PARTICIPANT_QOS_DEFAULT,
                            DomainParticipantListener::_nil(),
                            OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  OpenDDS::DCPS::TransportConfig_rch cfg = TheTransportRegistry->get_config("rtps");
  if (!cfg.is_nil()) {
    TheTransportRegistry->bind_config(cfg, dp);
  }

  //this needs modularization
  OpenDDS::DCPS::TypeSupport_var ts_var;
  if (type_name == "stru") {
    ts_var = new Dynamic::struTypeSupportImpl;
  } else if (type_name == "nested") {
    ts_var = new Dynamic::outer_structTypeSupportImpl;
  } else if (type_name == "union" || type_name == "union_default") {
    ts_var = new Dynamic::my_unionTypeSupportImpl;
  }
  ts_var->register_type(dp, type_name.c_str());

  Topic_var topic =
      dp->create_topic ("recorder_topic", type_name.c_str(), TOPIC_QOS_DEFAULT,
                        0, OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  // Create the publisher
  Publisher_var pub =
      dp->create_publisher(PUBLISHER_QOS_DEFAULT,
                           PublisherListener::_nil(), OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  // Create the datawriters
  DataWriterQos dw_qos;
  pub->get_default_datawriter_qos (dw_qos);
  dw_qos.representation.value.length(1);
  dw_qos.representation.value[0] = DDS::XCDR2_DATA_REPRESENTATION;
  DataWriter_var dw =
      pub->create_datawriter(topic, dw_qos,
                             DataWriterListener::_nil(), OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (Utils::wait_match(dw, 1, Utils::EQ)) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("Error waiting for match for dw\n")));
    return 1;
  }
  // TODO CLAYTON: There is currently a race between get_dynamic_data and the creation of the
  // dynamic type used by it.  A more elegant solution than a delaying sleep is needed.
  sleep(3);
  if (type_name == "stru") {
    stru_narrow_write(dw);
  } else if (type_name == "nested") {
    nested_stru_narrow_write(dw);
  } else if (type_name == "union") {
    union_narrow_write(dw);
  } else if (type_name == "union_default") {
    union_default_narrow_write(dw);
  }
  pub->delete_contained_entities();
  dp->delete_publisher(pub);

  dp->delete_topic(topic);
  dpf->delete_participant(dp);
  TheServiceParticipant->shutdown();

  return status;
}
