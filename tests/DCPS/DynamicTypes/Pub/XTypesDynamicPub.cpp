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
#  include <dds/DCPS/RTPS/RtpsDiscovery.h>
#  include <dds/DCPS/transport/rtps_udp/RtpsUdp.h>
#endif

#include <ace/Arg_Shifter.h>
#include <ace/OS_NS_unistd.h>

using namespace OpenDDS::DCPS;
using namespace DDS;

void my_struct_final_narrow_write(DataWriter_var dw)
{
  Dynamic::my_struct_final foo;
  Dynamic::long_struct_final ls;
  ls.my_long = 1;
  Dynamic::long_struct_final ls2;
  ls2.my_long = 2;
  Dynamic::inner_union_final iu;
  iu._d(3);
  iu.b(true);
  Dynamic::inner_union_final iu2;
  iu2._d(2);
  Dynamic::bool_seq bs;
  bs.length(2);
  bs[0] = true;
  bs[1] = false;
  iu2.my_alias_seq(bs);
  foo.my_long_struct_arr[0] = ls;
  foo.my_long_struct_arr[1] = ls2;
  foo.my_inner_union_seq.length(2);
  foo.my_inner_union_seq[0] = iu;
  foo.my_inner_union_seq[1] = iu2;
  foo.my_enum_arr[0] = Dynamic::V1;
  foo.my_enum_arr[1] = Dynamic::V2;
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
  Dynamic::my_struct_finalDataWriter_var narrow_dw = Dynamic::my_struct_finalDataWriter::_narrow(dw);
  InstanceHandle_t handle = narrow_dw->register_instance(foo);
  narrow_dw->write(foo, handle);
}

void my_struct_appendable_narrow_write(DataWriter_var dw)
{
  Dynamic::my_struct_appendable foo;
  Dynamic::long_struct_appendable ls;
  ls.my_long = 1;
  Dynamic::long_struct_appendable ls2;
  ls2.my_long = 2;
  Dynamic::inner_union_appendable iu;
  iu._d(3);
  iu.b(true);
  Dynamic::inner_union_appendable iu2;
  iu2._d(2);
  Dynamic::bool_seq bs;
  bs.length(2);
  bs[0] = true;
  bs[1] = false;
  iu2.my_alias_seq(bs);
  foo.my_long_struct_arr[0] = ls;
  foo.my_long_struct_arr[1] = ls2;
  foo.my_inner_union_seq.length(2);
  foo.my_inner_union_seq[0] = iu;
  foo.my_inner_union_seq[1] = iu2;
  foo.my_enum_arr[0] = Dynamic::V1;
  foo.my_enum_arr[1] = Dynamic::V2;
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
  Dynamic::my_struct_appendableDataWriter_var narrow_dw = Dynamic::my_struct_appendableDataWriter::_narrow(dw);
  InstanceHandle_t handle = narrow_dw->register_instance(foo);
  narrow_dw->write(foo, handle);
}


void my_struct_mutable_narrow_write(DataWriter_var dw)
{
  Dynamic::my_struct_mutable foo;
  Dynamic::long_struct_mutable ls;
  ls.my_long = 1;
  Dynamic::long_struct_mutable ls2;
  ls2.my_long = 2;
  Dynamic::inner_union_mutable iu;
  iu._d(3);
  iu.b(true);
  Dynamic::inner_union_mutable iu2;
  iu2._d(2);
  Dynamic::bool_seq bs;
  bs.length(2);
  bs[0] = true;
  bs[1] = false;
  iu2.my_alias_seq(bs);
  foo.my_long_struct_arr[0] = ls;
  foo.my_long_struct_arr[1] = ls2;
  foo.my_inner_union_seq.length(2);
  foo.my_inner_union_seq[0] = iu;
  foo.my_inner_union_seq[1] = iu2;
  foo.my_enum_arr[0] = Dynamic::V1;
  foo.my_enum_arr[1] = Dynamic::V2;
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
  Dynamic::my_struct_mutableDataWriter_var narrow_dw = Dynamic::my_struct_mutableDataWriter::_narrow(dw);
  InstanceHandle_t handle = narrow_dw->register_instance(foo);
  narrow_dw->write(foo, handle);
}

void outer_struct_final_narrow_write(DataWriter_var dw)
{
  Dynamic::outer_struct_final os;
  Dynamic::inner_struct_final is;
  Dynamic::inner_union_final foo;
  foo._d(2);
  Dynamic::bool_seq bs;
  bs.length(2);
  bs[0] = false;
  bs[1] = true;
  foo.my_alias_seq(bs);
  is.iu = foo;
  os.is = is;
  Dynamic::outer_struct_finalDataWriter_var narrow_dw = Dynamic::outer_struct_finalDataWriter::_narrow(dw);
  InstanceHandle_t handle = narrow_dw->register_instance(os);
  narrow_dw->write(os, handle);
}

void outer_struct_appendable_narrow_write(DataWriter_var dw)
{
  Dynamic::outer_struct_appendable os;
  Dynamic::inner_struct_appendable is;
  Dynamic::inner_union_appendable foo;
  foo._d(2);
  Dynamic::bool_seq bs;
  bs.length(2);
  bs[0] = false;
  bs[1] = true;
  foo.my_alias_seq(bs);
  is.iu = foo;
  os.is = is;
  Dynamic::outer_struct_appendableDataWriter_var narrow_dw = Dynamic::outer_struct_appendableDataWriter::_narrow(dw);
  InstanceHandle_t handle = narrow_dw->register_instance(os);
  narrow_dw->write(os, handle);
}

void outer_struct_mutable_narrow_write(DataWriter_var dw)
{
  Dynamic::outer_struct_mutable os;
  Dynamic::inner_struct_mutable is;
  Dynamic::inner_union_mutable foo;
  foo._d(2);
  Dynamic::bool_seq bs;
  bs.length(2);
  bs[0] = false;
  bs[1] = true;
  foo.my_alias_seq(bs);
  is.iu = foo;
  os.is = is;
  Dynamic::outer_struct_mutableDataWriter_var narrow_dw = Dynamic::outer_struct_mutableDataWriter::_narrow(dw);
  InstanceHandle_t handle = narrow_dw->register_instance(os);
  narrow_dw->write(os, handle);
}

void inner_union_final_narrow_write(DataWriter_var dw)
{
  Dynamic::inner_union_final foo;
  foo._d(3);
  foo.b(true);
  Dynamic::inner_union_finalDataWriter_var narrow_dw = Dynamic::inner_union_finalDataWriter::_narrow(dw);
  InstanceHandle_t handle = narrow_dw->register_instance(foo);
  narrow_dw->write(foo, handle);
}

void inner_union_appendable_narrow_write(DataWriter_var dw)
{
  Dynamic::inner_union_appendable foo;
  foo._d(3);
  foo.b(true);
  Dynamic::inner_union_appendableDataWriter_var narrow_dw = Dynamic::inner_union_appendableDataWriter::_narrow(dw);
  InstanceHandle_t handle = narrow_dw->register_instance(foo);
  narrow_dw->write(foo, handle);
}

void inner_union_mutable_narrow_write(DataWriter_var dw)
{
  Dynamic::inner_union_mutable foo;
  foo._d(3);
  foo.b(true);
  Dynamic::inner_union_mutableDataWriter_var narrow_dw = Dynamic::inner_union_mutableDataWriter::_narrow(dw);
  InstanceHandle_t handle = narrow_dw->register_instance(foo);
  narrow_dw->write(foo, handle);
}


void outer_union_final_narrow_write(DataWriter_var dw)
{
  Dynamic::outer_union_final ou;
  Dynamic::inner_struct_final is;
  Dynamic::inner_union_final iu;
  iu._d(1);
  iu.l(5);
  is.iu = iu;
  ou._d(Dynamic::V1);
  ou.is(is);
  Dynamic::outer_union_finalDataWriter_var narrow_dw = Dynamic::outer_union_finalDataWriter::_narrow(dw);
  InstanceHandle_t handle = narrow_dw->register_instance(ou);
  narrow_dw->write(ou, handle);
}

void outer_union_appendable_narrow_write(DataWriter_var dw)
{
  Dynamic::outer_union_appendable ou;
  Dynamic::inner_struct_appendable is;
  Dynamic::inner_union_appendable iu;
  iu._d(1);
  iu.l(5);
  is.iu = iu;
  ou._d(Dynamic::V1);
  ou.is(is);
  Dynamic::outer_union_appendableDataWriter_var narrow_dw = Dynamic::outer_union_appendableDataWriter::_narrow(dw);
  InstanceHandle_t handle = narrow_dw->register_instance(ou);
  narrow_dw->write(ou, handle);
}

void outer_union_mutable_narrow_write(DataWriter_var dw)
{
  Dynamic::outer_union_mutable ou;
  Dynamic::inner_struct_mutable is;
  Dynamic::inner_union_mutable iu;
  iu._d(1);
  iu.l(5);
  is.iu = iu;
  ou._d(Dynamic::V1);
  ou.is(is);
  Dynamic::outer_union_mutableDataWriter_var narrow_dw = Dynamic::outer_union_mutableDataWriter::_narrow(dw);
  InstanceHandle_t handle = narrow_dw->register_instance(ou);
  narrow_dw->write(ou, handle);
}

int ACE_TMAIN(int argc, ACE_TCHAR * argv[])
{
  DomainParticipantFactory_var dpf = TheParticipantFactoryWithArgs(argc, argv);
  const ACE_TCHAR* type_name = argv[1];
  const ACE_TCHAR* xcdr_version = argv[2];
  if (argc < 3) {
    ACE_ERROR((LM_ERROR, "ERROR: Must pass type name and xcdr version\n"));
    return 1;
  } else if (argc > 3) {
    ACE_ERROR((LM_ERROR, "ERROR: Too many arguments\n"));
    return 1;
  }
  ACE_DEBUG((LM_DEBUG, "(%P|%t) DEBUG: main(): calling create_participant\n"));
  DomainParticipant_var dp =
    dpf->create_participant(153,
                            PARTICIPANT_QOS_DEFAULT,
                            DomainParticipantListener::_nil(),
                            DEFAULT_STATUS_MASK);

  TransportConfig_rch cfg = TheTransportRegistry->get_config("rtps");
  if (cfg) {
    TheTransportRegistry->bind_config(cfg, dp);
  }

  DDS::TypeSupport_var ts_var;
  if (!ACE_OS::strcmp(type_name, ACE_TEXT("my_struct_final"))) {
    ts_var = new Dynamic::my_struct_finalTypeSupportImpl;
  } else if (!ACE_OS::strcmp(type_name, ACE_TEXT("outer_struct_final"))) {
    ts_var = new Dynamic::outer_struct_finalTypeSupportImpl;
  } else if (!ACE_OS::strcmp(type_name, ACE_TEXT("inner_union_final"))) {
    ts_var = new Dynamic::inner_union_finalTypeSupportImpl;
  } else if (!ACE_OS::strcmp(type_name, ACE_TEXT("outer_union_final"))) {
    ts_var = new Dynamic::outer_union_finalTypeSupportImpl;
  } else if (!ACE_OS::strcmp(type_name, ACE_TEXT("my_struct_appendable"))) {
    ts_var = new Dynamic::my_struct_appendableTypeSupportImpl;
  } else if (!ACE_OS::strcmp(type_name, ACE_TEXT("outer_struct_appendable"))) {
    ts_var = new Dynamic::outer_struct_appendableTypeSupportImpl;
  } else if (!ACE_OS::strcmp(type_name, ACE_TEXT("inner_union_appendable"))) {
    ts_var = new Dynamic::inner_union_appendableTypeSupportImpl;
  } else if (!ACE_OS::strcmp(type_name, ACE_TEXT("outer_union_appendable"))) {
    ts_var = new Dynamic::outer_union_appendableTypeSupportImpl;
  } else if (!ACE_OS::strcmp(type_name, ACE_TEXT("my_struct_mutable"))) {
    ts_var = new Dynamic::my_struct_mutableTypeSupportImpl;
  } else if (!ACE_OS::strcmp(type_name, ACE_TEXT("outer_struct_mutable"))) {
    ts_var = new Dynamic::outer_struct_mutableTypeSupportImpl;
  } else if (!ACE_OS::strcmp(type_name, ACE_TEXT("inner_union_mutable"))) {
    ts_var = new Dynamic::inner_union_mutableTypeSupportImpl;
  } else if (!ACE_OS::strcmp(type_name, ACE_TEXT("outer_union_mutable"))) {
    ts_var = new Dynamic::outer_union_mutableTypeSupportImpl;
  } else {
    ACE_ERROR((LM_ERROR, "ERROR: Invalid type name: \"%s\"\n", type_name));
    return 1;
  }
  ts_var->register_type(dp, ACE_TEXT_ALWAYS_CHAR(type_name));

  ACE_DEBUG((LM_DEBUG, "(%P|%t) DEBUG: main(): calling create_topic\n"));
  Topic_var topic =
    dp->create_topic("recorder_topic", ACE_TEXT_ALWAYS_CHAR(type_name), TOPIC_QOS_DEFAULT,
                     0, DEFAULT_STATUS_MASK);

  ACE_DEBUG((LM_DEBUG, "(%P|%t) DEBUG: main(): calling create_publisher\n"));
  // Create the publisher
  Publisher_var pub =
    dp->create_publisher(PUBLISHER_QOS_DEFAULT,
                         PublisherListener::_nil(), DEFAULT_STATUS_MASK);

  // Create the datawriters
  DataWriterQos dw_qos;
  pub->get_default_datawriter_qos(dw_qos);
  dw_qos.representation.value.length(1);
  if (!ACE_OS::strcmp(xcdr_version, ACE_TEXT("1"))) {
    dw_qos.representation.value[0] = DDS::XCDR_DATA_REPRESENTATION;
  } else {
    dw_qos.representation.value[0] = DDS::XCDR2_DATA_REPRESENTATION;
  }
  ACE_DEBUG((LM_DEBUG, "(%P|%t) DEBUG: main(): calling create_datawriter\n"));
  DataWriter_var dw =
    pub->create_datawriter(topic, dw_qos,
                           DDS::DataWriterListener::_nil(), DEFAULT_STATUS_MASK);

  ACE_DEBUG((LM_DEBUG, "(%P|%t) DEBUG: main(): calling wait_match 1\n"));
  if (Utils::wait_match(dw, 1, Utils::EQ)) {
    if (log_level >= LogLevel::Error) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("Error waiting for match for dw\n")));
    }
    return 1;
  }

  if (!ACE_OS::strcmp(type_name, ACE_TEXT("my_struct_final"))) {
    my_struct_final_narrow_write(dw);
  } else if (!ACE_OS::strcmp(type_name, ACE_TEXT("outer_struct_final"))) {
    outer_struct_final_narrow_write(dw);
  } else if (!ACE_OS::strcmp(type_name, ACE_TEXT("inner_union_final"))) {
    inner_union_final_narrow_write(dw);
  } else if (!ACE_OS::strcmp(type_name, ACE_TEXT("outer_union_final"))) {
    outer_union_final_narrow_write(dw);
  } else if (!ACE_OS::strcmp(type_name, ACE_TEXT("my_struct_appendable"))) {
    my_struct_appendable_narrow_write(dw);
  } else if (!ACE_OS::strcmp(type_name, ACE_TEXT("outer_struct_appendable"))) {
    outer_struct_appendable_narrow_write(dw);
  } else if (!ACE_OS::strcmp(type_name, ACE_TEXT("inner_union_appendable"))) {
    inner_union_appendable_narrow_write(dw);
  } else if (!ACE_OS::strcmp(type_name, ACE_TEXT("outer_union_appendable"))) {
    outer_union_appendable_narrow_write(dw);
  } else if (!ACE_OS::strcmp(type_name, ACE_TEXT("my_struct_mutable"))) {
    my_struct_mutable_narrow_write(dw);
  } else if (!ACE_OS::strcmp(type_name, ACE_TEXT("outer_struct_mutable"))) {
    outer_struct_mutable_narrow_write(dw);
  } else if (!ACE_OS::strcmp(type_name, ACE_TEXT("inner_union_mutable"))) {
    inner_union_mutable_narrow_write(dw);
  } else if (!ACE_OS::strcmp(type_name, ACE_TEXT("outer_union_mutable"))) {
    outer_union_mutable_narrow_write(dw);
  }

  ACE_DEBUG((LM_DEBUG, "(%P|%t) DEBUG: main(): calling wait_match 0\n"));
  if (Utils::wait_match(dw, 0, Utils::EQ)) {
    if (log_level >= LogLevel::Error) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("Error waiting for unmatch for dw\n")));
    }
    return 1;
  }

  ACE_DEBUG((LM_DEBUG, "(%P|%t) DEBUG: main(): cleaning up\n"));
  pub->delete_contained_entities();
  dp->delete_publisher(pub);

  dp->delete_topic(topic);
  dpf->delete_participant(dp);
  TheServiceParticipant->shutdown();

  return 0;
}
