/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <dds/DCPS/BuiltInTopicUtils.h>
#include <dds/DCPS/DCPS_Utils.h>
#include <dds/DCPS/Definitions.h>
#include <dds/DCPS/GuardCondition.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/WaitSet.h>

#include <dds/DCPS/XTypes/DynamicDataXcdrReadImpl.h>
#include <dds/DCPS/XTypes/DynamicTypeSupport.h>

#if defined ACE_AS_STATIC_LIBS && !OPENDDS_CONFIG_SAFETY_PROFILE
#  include <dds/DCPS/RTPS/RtpsDiscovery.h>
#  include <dds/DCPS/transport/rtps_udp/RtpsUdp.h>
#endif

#include <ace/Semaphore.h>
#include <ace/Thread_Semaphore.h>

#include <iostream>
#include <sstream>

using namespace OpenDDS::DCPS;
bool sample_read = false;
class TestListener : public DDS::DataReaderListener {
public:
  explicit TestListener(DDS::GuardCondition_var gc)
    : ret_val_(0)
    , gc_(gc)
  {
  }

  void on_requested_deadline_missed(DDS::DataReader_ptr,
                                    const DDS::RequestedDeadlineMissedStatus&)
  {}

  void on_requested_incompatible_qos(DDS::DataReader_ptr,
                                     const DDS::RequestedIncompatibleQosStatus&)
  {}

  void on_sample_rejected(DDS::DataReader_ptr,
                          const DDS::SampleRejectedStatus&)
  {}

  void on_liveliness_changed(DDS::DataReader_ptr,
                             const DDS::LivelinessChangedStatus&)
  {}

  void on_data_available(DDS::DataReader_ptr reader)
  {
    DDS::DynamicDataReader_var r = DDS::DynamicDataReader::_narrow(reader);

    DDS::DynamicDataSeq datas;
    DDS::SampleInfoSeq infos;

    const DDS::ReturnCode_t ret = r->read(datas, infos, DDS::LENGTH_UNLIMITED, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);
    if (ret != DDS::RETCODE_OK) {
      return;
    }

    for (unsigned int idx = 0; idx != datas.length(); ++idx) {
      const DDS::DynamicData_ptr data = datas[idx];

      String my_type;
      String indent;
      if (!OpenDDS::XTypes::print_dynamic_data(data, my_type, indent)) {
        if (log_level >= LogLevel::Error) {
          ACE_ERROR((LM_ERROR, "(%P|%t) Error: TestListener::on_sample_data_received:"
                     " Failed to read DynamicData\n"));
        }
      }

      const String struct_string_final =
        "struct Dynamic::my_struct_final\n"
        "  Dynamic::long_struct_arr_final my_long_struct_arr Dynamic::long_struct_final[2] =\n"
        "    [0] struct Dynamic::long_struct_final\n"
        "      Int32 my_long = 1\n"
        "    [1] struct Dynamic::long_struct_final\n"
        "      Int32 my_long = 2\n"
        "  Dynamic::inner_union_seq_final my_inner_union_seq Dynamic::inner_union_final[2] =\n"
        "    [0] union Dynamic::inner_union_final\n"
        "      Int32 discriminator = -2147483647\n"
        "      Boolean b = true\n"
        "    [1] union Dynamic::inner_union_final\n"
        "      Int32 discriminator = 2\n"
        "      Dynamic::bool_seq my_alias_seq Boolean[2] =\n"
        "        [0] = true\n"
        "        [1] = false\n"
        "  Dynamic::enum_arr my_enum_arr Dynamic::EnumType[2] =\n"
        "    [0] = V1\n"
        "    [1] = V2\n"
        "  Int8 my_int8 = 1\n"
        "  UInt8 my_uint8 = 2\n"
        "  Int16 my_short = 3\n"
        "  UInt16 my_ushort = 4\n"
        "  Int32 my_long = 5\n"
        "  UInt32 my_ulong = 6\n"
        "  Int64 my_longlong = 7\n"
        "  UInt64 my_ulonglong = 8\n"
        "  Float32 my_float = 9.25\n"
        "  Float64 my_double = 10.5\n"
        "  Float128 my_longdouble = 11.075\n"
        "  Boolean my_boolean = true\n"
        "  Byte my_byte = 0x0c\n"
        "  Char8 my_char = 'd'\n"
        "  Char16 my_wchar = L'e'\n"
        "  String8 my_string = \"Hello\"\n"
        "  WString16 my_wstring = L\"World\"\n"
        "  Dynamic::bool_seq my_alias_seq Boolean[2] =\n"
        "    [0] = true\n"
        "    [1] = false\n"
        "  Dynamic::char_arr my_alias_array Char8[2] =\n"
        "    [0] = 'a'\n"
        "    [1] = 'b'\n"
        "  Sequence my_anon_seq Dynamic::EnumType[2] =\n"
        "    [0] = V2\n"
        "    [1] = V1\n"
        "  Array my_anon_arr Int16[2] =\n"
        "    [0] = 5\n"
        "    [1] = 6\n";

      const String nested_struct_string_final =
        "struct Dynamic::outer_struct_final\n"
        "  struct Dynamic::inner_struct_final is\n"
        "    union Dynamic::inner_union_final iu\n"
        "      Int32 discriminator = 2\n"
        "      Dynamic::bool_seq my_alias_seq Boolean[2] =\n"
        "        [0] = false\n"
        "        [1] = true\n";

      const String default_union_string_final =
        "union Dynamic::inner_union_final\n"
        "  Int32 discriminator = -2147483647\n"
        "  Boolean b = true\n";

      const String nested_union_string_final =
        "union Dynamic::outer_union_final\n"
        "  Dynamic::EnumType discriminator = V1\n"
        "  struct Dynamic::inner_struct_final is\n"
        "    union Dynamic::inner_union_final iu\n"
        "      Int32 discriminator = 1\n"
        "      Int32 l = 5\n";

      const String struct_string_appendable =
        "struct Dynamic::my_struct_appendable\n"
        "  Dynamic::long_struct_arr_appendable my_long_struct_arr Dynamic::long_struct_appendable[2] =\n"
        "    [0] struct Dynamic::long_struct_appendable\n"
        "      Int32 my_long = 1\n"
        "    [1] struct Dynamic::long_struct_appendable\n"
        "      Int32 my_long = 2\n"
        "  Dynamic::inner_union_seq_appendable my_inner_union_seq Dynamic::inner_union_appendable[2] =\n"
        "    [0] union Dynamic::inner_union_appendable\n"
        "      Int32 discriminator = -2147483647\n"
        "      Boolean b = true\n"
        "    [1] union Dynamic::inner_union_appendable\n"
        "      Int32 discriminator = 2\n"
        "      Dynamic::bool_seq my_alias_seq Boolean[2] =\n"
        "        [0] = true\n"
        "        [1] = false\n"
        "  Dynamic::enum_arr my_enum_arr Dynamic::EnumType[2] =\n"
        "    [0] = V1\n"
        "    [1] = V2\n"
        "  Int8 my_int8 = 1\n"
        "  UInt8 my_uint8 = 2\n"
        "  Int16 my_short = 3\n"
        "  UInt16 my_ushort = 4\n"
        "  Int32 my_long = 5\n"
        "  UInt32 my_ulong = 6\n"
        "  Int64 my_longlong = 7\n"
        "  UInt64 my_ulonglong = 8\n"
        "  Float32 my_float = 9.25\n"
        "  Float64 my_double = 10.5\n"
        "  Float128 my_longdouble = 11.075\n"
        "  Boolean my_boolean = true\n"
        "  Byte my_byte = 0x0c\n"
        "  Char8 my_char = 'd'\n"
        "  Char16 my_wchar = L'e'\n"
        "  String8 my_string = \"Hello\"\n"
        "  WString16 my_wstring = L\"World\"\n"
        "  Dynamic::bool_seq my_alias_seq Boolean[2] =\n"
        "    [0] = true\n"
        "    [1] = false\n"
        "  Dynamic::char_arr my_alias_array Char8[2] =\n"
        "    [0] = 'a'\n"
        "    [1] = 'b'\n"
        "  Sequence my_anon_seq Dynamic::EnumType[2] =\n"
        "    [0] = V2\n"
        "    [1] = V1\n"
        "  Array my_anon_arr Int16[2] =\n"
        "    [0] = 5\n"
        "    [1] = 6\n";

      const String nested_struct_string_appendable =
        "struct Dynamic::outer_struct_appendable\n"
        "  struct Dynamic::inner_struct_appendable is\n"
        "    union Dynamic::inner_union_appendable iu\n"
        "      Int32 discriminator = 2\n"
        "      Dynamic::bool_seq my_alias_seq Boolean[2] =\n"
        "        [0] = false\n"
        "        [1] = true\n";

      const String default_union_string_appendable =
        "union Dynamic::inner_union_appendable\n"
        "  Int32 discriminator = -2147483647\n"
        "  Boolean b = true\n";

      const String nested_union_string_appendable =
        "union Dynamic::outer_union_appendable\n"
        "  Dynamic::EnumType discriminator = V1\n"
        "  struct Dynamic::inner_struct_appendable is\n"
        "    union Dynamic::inner_union_appendable iu\n"
        "      Int32 discriminator = 1\n"
        "      Int32 l = 5\n";

      const String struct_string_mutable =
        "struct Dynamic::my_struct_mutable\n"
        "  Dynamic::long_struct_arr_mutable my_long_struct_arr Dynamic::long_struct_mutable[2] =\n"
        "    [0] struct Dynamic::long_struct_mutable\n"
        "      Int32 my_long = 1\n"
        "    [1] struct Dynamic::long_struct_mutable\n"
        "      Int32 my_long = 2\n"
        "  Dynamic::inner_union_seq_mutable my_inner_union_seq Dynamic::inner_union_mutable[2] =\n"
        "    [0] union Dynamic::inner_union_mutable\n"
        "      Int32 discriminator = -2147483647\n"
        "      Boolean b = true\n"
        "    [1] union Dynamic::inner_union_mutable\n"
        "      Int32 discriminator = 2\n"
        "      Dynamic::bool_seq my_alias_seq Boolean[2] =\n"
        "        [0] = true\n"
        "        [1] = false\n"
        "  Dynamic::enum_arr my_enum_arr Dynamic::EnumType[2] =\n"
        "    [0] = V1\n"
        "    [1] = V2\n"
        "  Int8 my_int8 = 1\n"
        "  UInt8 my_uint8 = 2\n"
        "  Int16 my_short = 3\n"
        "  UInt16 my_ushort = 4\n"
        "  Int32 my_long = 5\n"
        "  UInt32 my_ulong = 6\n"
        "  Int64 my_longlong = 7\n"
        "  UInt64 my_ulonglong = 8\n"
        "  Float32 my_float = 9.25\n"
        "  Float64 my_double = 10.5\n"
        "  Float128 my_longdouble = 11.075\n"
        "  Boolean my_boolean = true\n"
        "  Byte my_byte = 0x0c\n"
        "  Char8 my_char = 'd'\n"
        "  Char16 my_wchar = L'e'\n"
        "  String8 my_string = \"Hello\"\n"
        "  WString16 my_wstring = L\"World\"\n"
        "  Dynamic::bool_seq my_alias_seq Boolean[2] =\n"
        "    [0] = true\n"
        "    [1] = false\n"
        "  Dynamic::char_arr my_alias_array Char8[2] =\n"
        "    [0] = 'a'\n"
        "    [1] = 'b'\n"
        "  Sequence my_anon_seq Dynamic::EnumType[2] =\n"
        "    [0] = V2\n"
        "    [1] = V1\n"
        "  Array my_anon_arr Int16[2] =\n"
        "    [0] = 5\n"
        "    [1] = 6\n";

      const String nested_struct_string_mutable =
        "struct Dynamic::outer_struct_mutable\n"
        "  struct Dynamic::inner_struct_mutable is\n"
        "    union Dynamic::inner_union_mutable iu\n"
        "      Int32 discriminator = 2\n"
        "      Dynamic::bool_seq my_alias_seq Boolean[2] =\n"
        "        [0] = false\n"
        "        [1] = true\n";

      const String default_union_string_mutable =
        "union Dynamic::inner_union_mutable\n"
        "  Int32 discriminator = -2147483647\n"
        "  Boolean b = true\n";

      const String nested_union_string_mutable =
        "union Dynamic::outer_union_mutable\n"
        "  Dynamic::EnumType discriminator = V1\n"
        "  struct Dynamic::inner_struct_mutable is\n"
        "    union Dynamic::inner_union_mutable iu\n"
        "      Int32 discriminator = 1\n"
        "      Int32 l = 5\n";

      if (my_type != struct_string_final &&
          my_type != nested_struct_string_final &&
        my_type != default_union_string_final &&
        my_type != nested_union_string_final &&
        my_type != struct_string_appendable &&
        my_type != nested_struct_string_appendable &&
        my_type != default_union_string_appendable &&
        my_type != nested_union_string_appendable &&
        my_type != struct_string_mutable &&
        my_type != nested_struct_string_mutable &&
        my_type != default_union_string_mutable &&
        my_type != nested_union_string_mutable &&
        log_level >= LogLevel::Error) {
        ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: TestListener::on_sample_data_received:"
                   " Type did not match\n"));
      }
      std::cout << my_type << "\n";
      gc_->set_trigger_value(true);
    }
  }

  void on_subscription_matched(DDS::DataReader_ptr,
                               const DDS::SubscriptionMatchedStatus& status)
  {
    if (status.current_count == 1 && DCPS_debug_level >= 4) {
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) TestListener::on_subscription_matched:"
                                    " a writer connected to recorder\n")));
    } else if (status.current_count == 0 && status.total_count > 0) {
      if (DCPS_debug_level >= 4) {
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) TestListener::on_subscription_matched:"
                                      " a writer disconnected with recorder\n")));
      }
    }
  }

  void on_sample_lost(DDS::DataReader_ptr,
                      const DDS::SampleLostStatus&)
  {}

  int ret_val_;

private:
  DDS::GuardCondition_var gc_;
};

DDS::ReturnCode_t wait_for_writer(DDS::DomainParticipant_var participant,
                                  DDS::DynamicType_var& type)
{

  if (type) {
    return DDS::RETCODE_OK;
  }

  DDS::Subscriber_var bit_subscriber = participant->get_builtin_subscriber();
  DDS::DataReader_var dr = bit_subscriber->lookup_datareader(BUILT_IN_PUBLICATION_TOPIC);
  DDS::PublicationBuiltinTopicDataDataReader_var pub_dr = DDS::PublicationBuiltinTopicDataDataReader::_narrow(dr);
  DDS::ReadCondition_var rc = pub_dr->create_readcondition(DDS::NOT_READ_SAMPLE_STATE,
                                                           DDS::ANY_VIEW_STATE,
                                                           DDS::ALIVE_INSTANCE_STATE);

  DDS::WaitSet_var ws = new DDS::WaitSet;
  DDS::ReturnCode_t ret = ws->attach_condition(rc);
  if (ret != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: attach_condition failed: %C\n", retcode_to_string(ret)));
    return ret;
  }

  DDS::Duration_t timeout = { DDS::DURATION_INFINITE_SEC, DDS::DURATION_INFINITE_NSEC };
  DDS::ConditionSeq conditions;
  while (!type && (ret = ws->wait(conditions, timeout)) == DDS::RETCODE_OK) {
    DDS::PublicationBuiltinTopicDataSeq pub_data;
    DDS::SampleInfoSeq infos;
    ret = pub_dr->read_w_condition(pub_data, infos, DDS::LENGTH_UNLIMITED, rc);
    if (ret == DDS::RETCODE_OK) {
      for (unsigned int idx = 0; !type && idx != pub_data.length(); ++idx) {
        if (std::strcmp(pub_data[idx].topic_name, "recorder_topic") == 0) {
          TheServiceParticipant->get_dynamic_type(type, participant, pub_data[idx].key);
        }
      }
    } else {
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: read_w_condition failed: %C\n", retcode_to_string(ret)));
    }
  }

  ws->detach_condition(rc);
  pub_dr->delete_readcondition(rc);

  return DDS::RETCODE_OK;
}

int run_test(int argc, ACE_TCHAR* argv[])
{
  int ret_val = 0;
  try {
    // Initialize DomainParticipantFactory
    DDS::DomainParticipantFactory_var dpf =
      TheParticipantFactoryWithArgs(argc, argv);
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
    // Create DomainParticipant
    DDS::DomainParticipant_var participant =
      dpf->create_participant(153,
                              PARTICIPANT_QOS_DEFAULT,
                              0,
                              DEFAULT_STATUS_MASK);

    if (!participant) {
      if (log_level >= LogLevel::Error) {
        ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: main(): create_participant failed!\n"));
      }
      return 1;
    }

    {
      DDS::DynamicType_var type;
      if (wait_for_writer(participant, type) != DDS::RETCODE_OK) {
        return 1;
      }

      DDS::WaitSet_var ws = new DDS::WaitSet;
      DDS::GuardCondition_var gc = new DDS::GuardCondition;
      ws->attach_condition(gc);

      DDS::DataRepresentationIdSeq allowable_representations;
      allowable_representations.length(1);
      if (!ACE_OS::strcmp(xcdr_version, ACE_TEXT("1"))) {
        allowable_representations[0] = DDS::XCDR_DATA_REPRESENTATION;
      } else {
        allowable_representations[0] = DDS::XCDR2_DATA_REPRESENTATION;
      }

      DDS::TypeSupport_var ts = new DDS::DynamicTypeSupport(type, allowable_representations);
      ts->register_type(participant, ACE_TEXT_ALWAYS_CHAR(type_name));

      DDS::Topic_var topic = participant->create_topic("recorder_topic",
                                                       ACE_TEXT_ALWAYS_CHAR(type_name),
                                                       TOPIC_QOS_DEFAULT,
                                                       0,
                                                       DEFAULT_STATUS_MASK);

      if (!topic) {
        ws->detach_condition(gc);
        ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: main(): create_topic failed!\n"));
        return 1;
      }

      DDS::Subscriber_var subscriber = participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT,
                                                                      0,
                                                                      DEFAULT_STATUS_MASK);

      TestListener* listener = new TestListener(gc);
      DDS::DataReaderListener_var listener_var = listener;
      DDS::DataReaderQos dr_qos;
      subscriber->get_default_datareader_qos(dr_qos);
      dr_qos.representation.value.length(1);
      if (!ACE_OS::strcmp(xcdr_version, ACE_TEXT("1"))) {
        dr_qos.representation.value[0] = DDS::XCDR_DATA_REPRESENTATION;
      } else {
        dr_qos.representation.value[0] = DDS::XCDR2_DATA_REPRESENTATION;
      }
      dr_qos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;
      DDS::DataReader_var dr = subscriber->create_datareader(topic,
                                                             dr_qos,
                                                             listener_var,
                                                             DEFAULT_STATUS_MASK);
      if (!dr) {
        ws->detach_condition(gc);
        ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: main(): create_datareader failed!\n"));
        return 1;
      }

      DDS::Duration_t timeout = { DDS::DURATION_INFINITE_SEC, DDS::DURATION_INFINITE_NSEC };
      DDS::ConditionSeq conditions;
      ACE_DEBUG((LM_DEBUG, "(%P|%t) DEBUG: main(): calling wait\n"));
      DDS::ReturnCode_t ret = ws->wait(conditions, timeout);

      ret_val = listener->ret_val_;
      ws->detach_condition(gc);

      if (ret != DDS::RETCODE_OK) {
        if (log_level >= LogLevel::Error) {
          ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: main(): wait failed!\n"));
        }
        return 1;
      }
      ACE_DEBUG((LM_DEBUG, "(%P|%t) DEBUG: main(): cleaning up\n"));
    }
    participant->delete_contained_entities();
    dpf->delete_participant(participant);
    TheServiceParticipant->shutdown();
  } catch (const CORBA::Exception& e) {
    e._tao_print_exception("Exception caught in main():");
    return 1;
  }
  if (ret_val == 1 && log_level >= LogLevel::Error) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: main(): failed to properly analyze sample!\n"));
  }
  return ret_val;
}

int ACE_TMAIN(int argc, ACE_TCHAR* argv[])
{
  int ret = run_test(argc, argv);
  ACE_Thread_Manager::instance()->wait();
  return ret;
}
