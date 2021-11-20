/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "dynamicTypeSupportImpl.h"

#include <tests/DCPS/common/TestException.h>
#include <tests/Utils/StatusMatching.h>

#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/transport/framework/TransportRegistry.h>
#include <dds/DCPS/transport/framework/TransportSendStrategy.h>
#if defined ACE_AS_STATIC_LIBS && !defined OPENDDS_SAFETY_PROFILE
#include <dds/DCPS/RTPS/RtpsDiscovery.h>
#include <dds/DCPS/transport/rtps_udp/RtpsUdp.h>
#endif

#include <ace/Arg_Shifter.h>
#include <ace/OS_NS_unistd.h>

using namespace OpenDDS::DCPS;
using namespace DDS;

void stru_narrow_write(DataWriter_var dw)
{
  Dynamic::stru foo;
  foo.my_int8 = 1;
  foo.my_uint8 = 2;
  foo.my_short = 3;
  foo.my_ushort = 4;
  foo.my_long = 5;
  foo.my_ulong = 6;
  foo.my_longlong = 7;
  foo.my_ulonglong = 8;
  foo.my_float = 9.25;
  foo.my_double = 10.5;
  ACE_CDR_LONG_DOUBLE_ASSIGNMENT(foo.my_longdouble, 11.075);
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
}

void nested_stru_narrow_write(DataWriter_var dw)
{
  Dynamic::inner_struct is;
  Dynamic::outer_struct os;
  is.l = 5;
  os.is = is;
  Dynamic::outer_structDataWriter_var narrow_dw = Dynamic::outer_structDataWriter::_narrow(dw);
  InstanceHandle_t handle = narrow_dw->register_instance(os);
  narrow_dw->write(os, handle);
}

void union_narrow_write(DataWriter_var dw)
{
  Dynamic::my_union foo;
  foo._d(2);
  Dynamic::bool_seq bs;
  bs.length(2);
  bs[0] = false;
  bs[1] = true;
  foo.my_alias_seq(bs);
  Dynamic::my_unionDataWriter_var narrow_dw = Dynamic::my_unionDataWriter::_narrow(dw);
  InstanceHandle_t handle = narrow_dw->register_instance(foo);
  narrow_dw->write(foo, handle);
}

void union_default_narrow_write(DataWriter_var dw)
{
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
  const ACE_TCHAR* type_name = argv[1];
  DomainParticipantFactory_var dpf = TheParticipantFactoryWithArgs(argc, argv);
  DomainParticipant_var dp =
    dpf->create_participant(153,
                            PARTICIPANT_QOS_DEFAULT,
                            DomainParticipantListener::_nil(),
                            DEFAULT_STATUS_MASK);

  TransportConfig_rch cfg = TheTransportRegistry->get_config("rtps");
  if (!cfg.is_nil()) {
    TheTransportRegistry->bind_config(cfg, dp);
  }

  //this needs modularization
  DDS::TypeSupport_var ts_var;
  if (!ACE_OS::strcmp(type_name, ACE_TEXT("stru"))) {
    ts_var = new Dynamic::struTypeSupportImpl;
  } else if (!ACE_OS::strcmp(type_name, ACE_TEXT("nested"))) {
    ts_var = new Dynamic::outer_structTypeSupportImpl;
  } else if (!ACE_OS::strcmp(type_name, ACE_TEXT("union")) ||
             !ACE_OS::strcmp(type_name, ACE_TEXT("union_default"))) {
    ts_var = new Dynamic::my_unionTypeSupportImpl;
  }
  ts_var->register_type(dp, ACE_TEXT_ALWAYS_CHAR(type_name));

  Topic_var topic =
      dp->create_topic("recorder_topic", ACE_TEXT_ALWAYS_CHAR(type_name), TOPIC_QOS_DEFAULT,
                       0, DEFAULT_STATUS_MASK);

  // Create the publisher
  Publisher_var pub =
      dp->create_publisher(PUBLISHER_QOS_DEFAULT,
                           PublisherListener::_nil(), DEFAULT_STATUS_MASK);

  // Create the datawriters
  DataWriterQos dw_qos;
  pub->get_default_datawriter_qos (dw_qos);
  dw_qos.representation.value.length(1);
  dw_qos.representation.value[0] = DDS::XCDR2_DATA_REPRESENTATION;
  DataWriter_var dw =
      pub->create_datawriter(topic, dw_qos,
                             DDS::DataWriterListener::_nil(), DEFAULT_STATUS_MASK);
  if (Utils::wait_match(dw, 1, Utils::EQ)) {
    if (log_level >= LogLevel::Error) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("Error waiting for match for dw\n")));
    }
    return 1;
  }
  if (!ACE_OS::strcmp(type_name, ACE_TEXT("stru"))) {
    stru_narrow_write(dw);
  } else if (!ACE_OS::strcmp(type_name, ACE_TEXT("nested"))) {
    nested_stru_narrow_write(dw);
  } else if (!ACE_OS::strcmp(type_name, ACE_TEXT("union"))) {
    union_narrow_write(dw);
  } else if (!ACE_OS::strcmp(type_name, ACE_TEXT("union_default"))) {
    union_default_narrow_write(dw);
  }
  if (Utils::wait_match(dw, 0, Utils::EQ)) {
    if (log_level >= LogLevel::Error) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("Error waiting for unmatch for dw\n")));
    }
    return 1;
  }
  pub->delete_contained_entities();
  dp->delete_publisher(pub);

  dp->delete_topic(topic);
  dpf->delete_participant(dp);
  TheServiceParticipant->shutdown();

  return status;
}
