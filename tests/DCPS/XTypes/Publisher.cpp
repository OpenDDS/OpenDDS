#include "Common.h"
#include "PublisherNonMutableStructsTypeSupportImpl.h"
#include "PublisherMutableStructsTypeSupportImpl.h"
#include "PublisherUnionsTypeSupportImpl.h"
#include "CommonTypeSupportImpl.h"

#include <dds/DCPS/DCPS_Utils.h>
#include <dds/DCPS/XTypes/DynamicDataFactory.h>
#include <dds/DCPS/XTypes/Utils.h>

using OpenDDS::DCPS::retcode_to_string;
using OpenDDS::XTypes::MemberNamePath;

template <typename Sample,
  typename DataWriter = typename OpenDDS::DCPS::DDSTraits<Sample>::DataWriterType >
void write_sample(const TypeSupport_var& ts, const DataWriter_var& dw, const Sample& sample)
{
  typename DataWriter::_var_type typed_dw = DataWriter::_narrow(dw);
  CORBA::String_var name = ts->get_type_name();
  if (!typed_dw) {
    ACE_ERROR((LM_ERROR, "ERROR: type_dw for %C was null\n", name.in()));
    return;
  }
  check_rc(typed_dw->write(sample, HANDLE_NIL), std::string("write for ") + name.in() + " failed");
  if (verbose) {
    ACE_DEBUG((LM_DEBUG, "writer: {}\n", name.in()));
  }
}

class DynamicWriter {
public:
  const TypeSupport_var& ts;
  const DataWriter_var& dw;
  DynamicData_var dd;
  bool success;

  DynamicWriter(const TypeSupport_var& ts, const DataWriter_var& dw)
    : ts(ts)
    , dw(dw)
    , success(true)
  {
    DDS::DynamicType_var dt = ts->get_type();
    if (OpenDDS::XTypes::dynamic_type_is_valid(dt)) {
      dd = DDS::DynamicDataFactory::get_instance()->create_data(dt);
    } else {
      ACE_ERROR((LM_ERROR, "ERROR: Got invalid DynamicType from DynamicTypeSupport!\n"));
    }
  }

  void set_int32(const MemberNamePath& path, CORBA::Int32 value)
  {
    if (!dd) {
      success = false;
      return;
    }
    DDS::DynamicData_var use_dd;
    DDS::MemberId use_id;
    if (check_rc(path.get_member_from_data(dd, use_dd, use_id), "set_int32: get_member_from_data")) {
      success &= check_rc(use_dd->set_int32_value(use_id, value), "set_int32: set_int32_value");
    } else {
      success = false;
    }
  }

  void set_string(const MemberNamePath& path, const std::string& value)
  {
    if (!dd) {
      success = false;
      return;
    }
    DDS::DynamicData_var use_dd;
    DDS::MemberId use_id;
    if (check_rc(path.get_member_from_data(dd, use_dd, use_id), "set_string: get_member_from_data")) {
      success &= check_rc(use_dd->set_string_value(use_id, value.c_str()), "set_string: set_string_value");
    } else {
      success = false;
    }
  }

  ~DynamicWriter()
  {
    if (success) {
      write_sample<DynamicData_var, DynamicDataWriter>(ts, dw, dd);
    }
  }
};

void write_plain_cdr_struct(const TypeSupport_var& ts, const DataWriter_var& dw, bool dynamic)
{
  if (dynamic) {
    DynamicWriter d(ts, dw);
    d.set_int32("key_field", key_value);
    d.set_int32("value_field", 1);
  } else {
    PlainCdrStruct sample;
    sample.key_field = key_value;
    sample.value_field = 1;
    write_sample(ts, dw, sample);
  }
}

void write_final_struct(const TypeSupport_var& ts, const DataWriter_var& dw, bool dynamic)
{
  if (dynamic) {
    DynamicWriter d(ts, dw);
    d.set_int32("key_field", key_value);
  } else {
    FinalStructPub sample;
    sample.key_field = key_value;
    write_sample(ts, dw, sample);
  }
}

void write_modified_final_struct(const TypeSupport_var& ts, const DataWriter_var& dw, bool dynamic)
{
  if (dynamic) {
    DynamicWriter d(ts, dw);
    d.set_int32("key_field", key_value);
    d.set_int32("additional_field", FINAL_STRUCT_AF);
  } else {
    ModifiedFinalStruct sample;
    sample.key_field = key_value;
    sample.additional_field = FINAL_STRUCT_AF;
    write_sample(ts, dw, sample);
  }
}

void write_appendable_struct_no_xtypes(const TypeSupport_var& ts, const DataWriter_var& dw, bool dynamic)
{
  if (dynamic) {
    DynamicWriter d(ts, dw);
    d.set_int32("key_field", key_value);
  } else {
    AppendableStructNoXTypes sample;
    sample.key_field = key_value;
    write_sample(ts, dw, sample);
  }
}

void write_additional_prefix_field_struct(const TypeSupport_var& ts, const DataWriter_var& dw, bool dynamic)
{
  if (dynamic) {
    DynamicWriter d(ts, dw);
    d.set_int32("key_field", key_value);
    d.set_int32("additional_field", APPENDABLE_STRUCT_AF);
  } else {
    AdditionalPrefixFieldStruct sample;
    sample.key_field = key_value;
    sample.additional_field = APPENDABLE_STRUCT_AF;
    write_sample(ts, dw, sample);
  }
}

void write_base_appendable_struct(const TypeSupport_var& ts, const DataWriter_var& dw, bool dynamic)
{
  if (dynamic) {
    DynamicWriter d(ts, dw);
    d.set_int32("key_field", key_value);
  } else {
    BaseAppendableStruct sample;
    sample.key_field = key_value;
    write_sample(ts, dw, sample);
  }
}

void write_additional_postfix_field_struct(const TypeSupport_var& ts, const DataWriter_var& dw, bool dynamic)
{
  AdditionalPostfixFieldStructDataWriter_var typed_dw = AdditionalPostfixFieldStructDataWriter::_narrow(dw);

  if (dynamic) {
    DynamicWriter d(ts, dw);
    d.set_int32("key_field", key_value);
    d.set_int32("additional_field", APPENDABLE_STRUCT_AF);
  } else {
    AdditionalPostfixFieldStruct sample;
    sample.key_field = key_value;
    sample.additional_field = APPENDABLE_STRUCT_AF;
    write_sample(ts, dw, sample);
  }
}

void write_modified_mutable_struct(const TypeSupport_var& ts, const DataWriter_var& dw, bool dynamic)
{
  if (dynamic) {
    DynamicWriter d(ts, dw);
    d.set_int32("key_field", key_value);
    d.set_int32("additional_field", MUTABLE_STRUCT_AF);
  } else {
    ModifiedMutableStruct sample;
    sample.key_field = key_value;
    sample.additional_field = MUTABLE_STRUCT_AF;
    write_sample(ts, dw, sample);
  }
}

void write_mutable_base_struct(const TypeSupport_var& ts, const DataWriter_var& dw, bool dynamic)
{
  if (dynamic) {
    DynamicWriter d(ts, dw);
    d.set_int32("key_field", key_value);
  } else {
    MutableBaseStruct sample;
    sample.key_field = key_value;
    write_sample(ts, dw, sample);
  }
}

void write_modified_mutable_union(const TypeSupport_var& ts, const DataWriter_var& dw, bool dynamic)
{
  if (dynamic) {
    DynamicWriter d(ts, dw);
    d.set_int32("key_field", key_value);
  } else {
    ModifiedMutableUnion sample;
    sample.key_field(key_value);
    write_sample(ts, dw, sample);
  }
}

void write_trim64_struct(const TypeSupport_var& ts, const DataWriter_var& dw, bool dynamic)
{
  if (dynamic) {
    DynamicWriter d(ts, dw);
    d.set_string("trim_string", STRING_26);
  } else {
    Trim64Struct sample;
    sample.trim_string = STRING_26.c_str();
    write_sample(ts, dw, sample);
  }
}

void write_appendable_struct_with_dependency(const TypeSupport_var& ts, const DataWriter_var& dw, bool dynamic)
{
  if (dynamic) {
    DynamicWriter d(ts, dw);
    d.set_int32("key_field", key_value);
    d.set_int32(MemberNamePath("additional_nested_struct").add("additional_field"), NESTED_STRUCT_AF);
  } else {
    AppendableStructWithDependency sample;
    sample.key_field = key_value;
    sample.additional_nested_struct.additional_field = NESTED_STRUCT_AF;
    write_sample(ts, dw, sample);
  }
}

void write_modified_name_mutable_struct(const TypeSupport_var& ts, const DataWriter_var& dw, bool dynamic)
{
  if (dynamic) {
    DynamicWriter d(ts, dw);
    d.set_int32("key_field_modified", key_value);
    d.set_int32("additional_field_modified", MUTABLE_STRUCT_AF);
  } else {
    ModifiedNameMutableStruct sample;
    sample.key_field_modified = key_value;
    sample.additional_field_modified = MUTABLE_STRUCT_AF;
    write_sample(ts, dw, sample);
  }
}

void write_modified_name_mutable_union(const TypeSupport_var& ts, const DataWriter_var& dw, bool dynamic)
{
  if (dynamic) {
    DynamicWriter d(ts, dw);
    d.set_int32("key_field_modified", key_value);
  } else {
    ModifiedNameMutableUnion sample;
    sample.key_field_modified(key_value);
    write_sample(ts, dw, sample);
  }
}

int ACE_TMAIN(int argc, ACE_TCHAR* argv[])
{
  std::string type;
  std::string topic_name = "";
  std::string registered_type_name = "";
  bool dynamic = false;

  DomainParticipantFactory_var dpf = TheParticipantFactoryWithArgs(argc, argv);

  // Arguments "--type" must be specified
  for (int i = 1; i < argc; ++i) {
    ACE_TString arg(argv[i]);
    if (arg == ACE_TEXT("--verbose")) {
      verbose = true;
    } else if (arg == ACE_TEXT("--type")) {
      if (i + 1 < argc) {
        type = ACE_TEXT_ALWAYS_CHAR(argv[++i]);
      } else {
        ACE_ERROR((LM_ERROR, "ERROR: Invalid type argument\n"));
        return 1;
      }
    } else if (arg == ACE_TEXT("--topic")) {
      if (i + 1 < argc) {
        topic_name = ACE_TEXT_ALWAYS_CHAR(argv[++i]);
      } else {
        ACE_ERROR((LM_ERROR, "ERROR: Invalid topic name argument\n"));
        return 1;
      }
    } else if (arg == ACE_TEXT("--reg-type")) {
      if (i + 1 < argc) {
        registered_type_name = ACE_TEXT_ALWAYS_CHAR(argv[++i]);
      } else {
        ACE_ERROR((LM_ERROR, "ERROR: Invalid registered type argument\n"));
        return 1;
      }
    } else if (arg == ACE_TEXT("--key-val")) {
      if (i + 1 < argc) {
        key_value = ACE_OS::atoi(argv[++i]);
      } else {
        ACE_ERROR((LM_ERROR, "ERROR: Invalid key value argument\n"));
        return 1;
      }
    } else if (arg == ACE_TEXT("--expect-inconsistent-topic")) {
      expect_inconsistent_topic = true;
    } else if (arg == ACE_TEXT("--expect-incompatible-qos")) {
      expect_incompatible_qos = true;
    } else if (arg == ACE_TEXT("--dynamic-ts")) {
      dynamic = true;
    } else {
      ACE_ERROR((LM_ERROR, "ERROR: Invalid argument: %s\n", argv[i]));
      return 1;
    }
  }

  ACE_DEBUG((LM_DEBUG, "TYPE: %C\n", type.c_str()));

  DomainParticipant_var dp;
  create_participant(dpf, dp);
  if (!dp) {
    ACE_ERROR((LM_ERROR, "ERROR: create_participant failed\n"));
    return 1;
  }

  if (topic_name.empty()) {
    topic_name = !registered_type_name.empty() ? registered_type_name : type;
  } else if (registered_type_name.empty()) {
    registered_type_name = topic_name;
  }

  Topic_var topic;
  TypeSupport_var ts;
  bool success = true;
  if (type == "PlainCdrStruct") {
    get_topic<PlainCdrStruct>(success, ts, dp, topic_name, topic, registered_type_name, dynamic);
  } else if (type == "FinalStructPub") {
    get_topic<FinalStructPub>(success, ts, dp, topic_name, topic, registered_type_name, dynamic);
  } else if (type == "ModifiedFinalStruct") {
    get_topic<ModifiedFinalStruct>(success, ts, dp, topic_name, topic, registered_type_name, dynamic);
  } else if (type == "BaseAppendableStruct") {
    get_topic<BaseAppendableStruct>(success, ts, dp, topic_name, topic, registered_type_name, dynamic);
  } else if (type == "AppendableStructNoXTypes") {
    get_topic<AppendableStructNoXTypes>(success, ts, dp, topic_name, topic, registered_type_name, dynamic);
  } else if (type == "AdditionalPrefixFieldStruct") {
    get_topic<AdditionalPrefixFieldStruct>(success, ts, dp, topic_name, topic, registered_type_name, dynamic);
  } else if (type == "AdditionalPostfixFieldStruct") {
    get_topic<AdditionalPostfixFieldStruct>(success, ts, dp, topic_name, topic, registered_type_name, dynamic);
  } else if (type == "ModifiedMutableStruct") {
    get_topic<ModifiedMutableStruct>(success, ts, dp, topic_name, topic, registered_type_name, dynamic);
  } else if (type == "ModifiedIdMutableStruct") {
    get_topic<ModifiedIdMutableStruct>(success, ts, dp, topic_name, topic, registered_type_name, dynamic);
  } else if (type == "ModifiedTypeMutableStruct") {
    get_topic<ModifiedTypeMutableStruct>(success, ts, dp, topic_name, topic, registered_type_name, dynamic);
  } else if (type == "ModifiedNameMutableStruct") {
    get_topic<ModifiedNameMutableStruct>(success, ts, dp, topic_name, topic, registered_type_name, dynamic);
  } else if (type == "ModifiedMutableUnion") {
    get_topic<ModifiedMutableUnion>(success, ts, dp, topic_name, topic, registered_type_name, dynamic);
  } else if (type == "ModifiedDiscMutableUnion") {
    get_topic<ModifiedDiscMutableUnion>(success, ts, dp, topic_name, topic, registered_type_name, dynamic);
  } else if (type == "ModifiedTypeMutableUnion") {
    get_topic<ModifiedTypeMutableUnion>(success, ts, dp, topic_name, topic, registered_type_name, dynamic);
  } else if (type == "ModifiedNameMutableUnion") {
    get_topic<ModifiedNameMutableUnion>(success, ts, dp, topic_name, topic, registered_type_name, dynamic);
  } else if (type == "Trim64Struct") {
    get_topic<Trim64Struct>(success, ts, dp, topic_name, topic, registered_type_name, dynamic);
  } else if (type == "AppendableStructWithDependency") {
    get_topic<AppendableStructWithDependency>(success, ts, dp, topic_name, topic, registered_type_name, dynamic);
  } else if (type == "MutableBaseStruct") {
    get_topic<MutableBaseStruct>(success, ts, dp, topic_name, topic, registered_type_name, dynamic);
  } else if (type.empty()) {
    ACE_ERROR((LM_ERROR, "ERROR: Must specify a type name\n"));
    return 1;
  } else {
    ACE_ERROR((LM_ERROR, "ERROR: Type %C is not supported\n", type.c_str()));
    return 1;
  }

  Topic_var ack_control_topic;
  Topic_var echo_control_topic;
  TypeSupport_var ack_control_ts;
  TypeSupport_var echo_control_ts;
  get_topic<ControlStruct>(success,
    ack_control_ts, dp, "SET_PD_OL_OA_OM_OD_Ack", ack_control_topic, "ControlStruct");
  get_topic<ControlStruct>(success,
    echo_control_ts, dp, "SET_PD_OL_OA_OM_OD_Echo", echo_control_topic, "ControlStruct");

  if (!success) {
    return 1;
  }

  ACE_DEBUG((LM_DEBUG, "Writer starting at %T\n"));

  // For subscribing ack control topic.
  Subscriber_var control_sub = dp->create_subscriber(SUBSCRIBER_QOS_DEFAULT, 0,
                                                     DEFAULT_STATUS_MASK);
  if (!control_sub) {
    ACE_ERROR((LM_ERROR, "ERROR: create_subscriber failed\n"));
    return 1;
  }

  DataReaderQos control_dr_qos;
  control_sub->get_default_datareader_qos(control_dr_qos);
  control_dr_qos.reliability.kind = RELIABLE_RELIABILITY_QOS;
  control_dr_qos.durability.kind = TRANSIENT_LOCAL_DURABILITY_QOS;

  DataReader_var control_dr = control_sub->create_datareader(ack_control_topic, control_dr_qos, 0,
                                                             DEFAULT_STATUS_MASK);
  if (!control_dr) {
    ACE_ERROR((LM_ERROR, "ERROR: create_datareader failed\n"));
    return 1;
  }

  // For publishing echo control topic.
  Publisher_var control_pub = dp->create_publisher(PUBLISHER_QOS_DEFAULT, 0,
                                                   DEFAULT_STATUS_MASK);
  if (!control_pub) {
    ACE_ERROR((LM_ERROR, "ERROR: create_publisher failed for control_pub\n"));
    return 1;
  }

  DataWriterQos control_dw_qos;
  control_pub->get_default_datawriter_qos(control_dw_qos);
  control_dw_qos.durability.kind = TRANSIENT_LOCAL_DURABILITY_QOS;

  DataWriter_var control_dw = control_pub->create_datawriter(echo_control_topic, control_dw_qos, 0,
                                                             DEFAULT_STATUS_MASK);
  if (!control_dw) {
    ACE_ERROR((LM_ERROR, "ERROR: create_datawriter for control_pub failed\n"));
    return 1;
  }

  ControlStructDataWriter_var control_typed_dw = ControlStructDataWriter::_narrow(control_dw);
  if (!control_typed_dw) {
    ACE_ERROR((LM_ERROR, "ERROR: _narrow echo datawriter failed\n"));
    return 1;
  }

  // For publishing user topic.
  Publisher_var pub = dp->create_publisher(PUBLISHER_QOS_DEFAULT, 0, DEFAULT_STATUS_MASK);
  if (!pub) {
    ACE_ERROR((LM_ERROR, "ERROR: create_publisher failed\n"));
    return 1;
  }

  DataWriterQos dw_qos;
  pub->get_default_datawriter_qos(dw_qos);
  dw_qos.durability.kind = TRANSIENT_LOCAL_DURABILITY_QOS;
  if (!expect_incompatible_qos) {
    dw_qos.representation.value.length(1);
    dw_qos.representation.value[0] = XCDR2_DATA_REPRESENTATION;
  }

  DataWriter_var dw = pub->create_datawriter(topic, dw_qos, 0, DEFAULT_STATUS_MASK);
  if (!dw) {
    ACE_ERROR((LM_ERROR, "ERROR: create_datawriter failed\n"));
    return 1;
  }

  DDS::StatusCondition_var condition = expect_inconsistent_topic ? topic->get_statuscondition() : dw->get_statuscondition();
  const DDS::StatusMask status_mask = expect_inconsistent_topic ? DDS::INCONSISTENT_TOPIC_STATUS :
    (expect_incompatible_qos ? DDS::OFFERED_INCOMPATIBLE_QOS_STATUS : DDS::PUBLICATION_MATCHED_STATUS);
  DDS::ReturnCode_t ret = condition->set_enabled_statuses(status_mask);
  if (ret != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, "ERROR: set_enabled_statuses failed: %C\n", OpenDDS::DCPS::retcode_to_string(ret)));
    return 1;
  }

  DDS::WaitSet_var ws = new DDS::WaitSet;
  ret = ws->attach_condition(condition);
  if (ret != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, "ERROR: attach_condition failed: %C\n",
               OpenDDS::DCPS::retcode_to_string(ret)));
    return 1;
  }

  if (status_mask == DDS::INCONSISTENT_TOPIC_STATUS) {
    success = wait_inconsistent_topic(ws, topic);
  } else if (status_mask == DDS::OFFERED_INCOMPATIBLE_QOS_STATUS) {
    success = wait_offered_incompatible_qos(ws, dw);
  } else {
    success = wait_publication_matched(ws, dw);
  }
  ws->detach_condition(condition);

  if (!success) {
    ACE_ERROR((LM_ERROR, "ERROR: Writer failed for type %C\n", type.c_str()));
    return 1;
  }

  if (!check_inconsistent_topic_status(topic) ||
      !check_offered_incompatible_qos_status(dw, type)) {
    return 1;
  }

  if (!expect_inconsistent_topic && !expect_incompatible_qos) {
    if (type == "PlainCdrStruct") {
      write_plain_cdr_struct(ts, dw, dynamic);
    } else if (type == "FinalStructPub") {
      write_final_struct(ts, dw, dynamic);
    } else if (type == "ModifiedFinalStruct") {
      write_modified_final_struct(ts, dw, dynamic);
    } else if (type == "BaseAppendableStruct") {
      write_base_appendable_struct(ts, dw, dynamic);
    } else if (type == "AppendableStructNoXTypes") {
      write_appendable_struct_no_xtypes(ts, dw, dynamic);
    } else if (type == "AdditionalPrefixFieldStruct") {
      write_additional_prefix_field_struct(ts, dw, dynamic);
    } else if (type == "AdditionalPostfixFieldStruct") {
      write_additional_postfix_field_struct(ts, dw, dynamic);
    } else if (type == "ModifiedMutableStruct") {
      write_modified_mutable_struct(ts, dw, dynamic);
    } else if (type == "ModifiedMutableUnion") {
      write_modified_mutable_union(ts, dw, dynamic);
    } else if (type == "Trim64Struct") {
      write_trim64_struct(ts, dw, dynamic);
    } else if (type == "AppendableStructWithDependency") {
      write_appendable_struct_with_dependency(ts, dw, dynamic);
    } else if (type == "ModifiedNameMutableStruct") {
      write_modified_name_mutable_struct(ts, dw, dynamic);
    } else if (type == "ModifiedNameMutableUnion") {
      write_modified_name_mutable_union(ts, dw, dynamic);
    } else if (type == "MutableBaseStruct") {
      write_mutable_base_struct(ts, dw, dynamic);
    }
  }
  ACE_DEBUG((LM_DEBUG, "Writer waiting for ack at %T\n"));

  if (!read_control(control_dr)) {
    ACE_ERROR((LM_ERROR, "ERROR: control read failed\n"));
    return 1;
  }

  // Send echo when the subscriber's control reader joins.
  if (!wait_for_reader(true, control_dw)) {
    return 1;
  }

  ACE_DEBUG((LM_DEBUG, "Writer sending echo at %T\n"));

  ControlStruct cs;
  ret = control_typed_dw->write(cs, HANDLE_NIL);
  if (ret != RETCODE_OK) {
    ACE_ERROR((LM_ERROR, "ERROR: control write returned %C\n",
      OpenDDS::DCPS::retcode_to_string(ret)));
    return 1;
  }

  // When the subscriber's control reader leaves, we can leave.
  if (!wait_for_reader(false, control_dw)) {
    return 1;
  }

  ACE_DEBUG((LM_DEBUG, "Writer cleanup at %T\n"));
  topic = 0;
  dp->delete_contained_entities();
  dpf->delete_participant(dp);
  TheServiceParticipant->shutdown();

  ACE_DEBUG((LM_DEBUG, "Writer exiting at %T\n"));
  return 0;
}
