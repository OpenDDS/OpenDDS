#include "XTypes.h"
#include "XTypesPubTypeSupportImpl.h"

#include <dds/DCPS/DCPS_Utils.h>

void write_plain_cdr_struct(const DataWriter_var& dw)
{
  PlainCdrStructDataWriter_var pdw = PlainCdrStructDataWriter::_narrow(dw);

  PlainCdrStruct pcs;
  pcs.key = key_value;
  pcs.value = 1;

  const ReturnCode_t ret = pdw->write(pcs, HANDLE_NIL);
  if (ret != RETCODE_OK) {
    ACE_ERROR((LM_ERROR, "ERROR: write_plain_cdr_struct returned %C\n",
               OpenDDS::DCPS::retcode_to_string(ret)));
  }
  if (verbose) {
    ACE_DEBUG((LM_DEBUG, "writer: PlainCdrStruct\n"));
  }
}

void write_final_struct(const DataWriter_var& dw)
{
  FinalStructPubDataWriter_var pdw = FinalStructPubDataWriter::_narrow(dw);

  FinalStructPub fs;
  fs.key = key_value;

  const ReturnCode_t ret = pdw->write(fs, HANDLE_NIL);
  if (ret != RETCODE_OK) {
    ACE_ERROR((LM_ERROR, "ERROR: write_final_struct returned %C\n",
               OpenDDS::DCPS::retcode_to_string(ret)));
  }
  if (verbose) {
    ACE_DEBUG((LM_DEBUG, "writer: FinalStructPub\n"));
  }
}

void write_modified_final_struct(const DataWriter_var& dw)
{
  ModifiedFinalStructDataWriter_var pdw = ModifiedFinalStructDataWriter::_narrow(dw);

  ModifiedFinalStruct mfs;
  mfs.key = key_value;
  mfs.additional_field = FINAL_STRUCT_AF;

  const ReturnCode_t ret = pdw->write(mfs, HANDLE_NIL);
  if (ret != RETCODE_OK) {
    ACE_ERROR((LM_ERROR, "ERROR: write_modified_final_struct returned %C\n",
               OpenDDS::DCPS::retcode_to_string(ret)));
  }
  if (verbose) {
    ACE_DEBUG((LM_DEBUG, "writer: ModifiedFinalStruct\n"));
  }
}

void write_appendable_struct_no_xtypes(const DataWriter_var& dw)
{
  AppendableStructNoXTypesDataWriter_var typed_dw = AppendableStructNoXTypesDataWriter::_narrow(dw);

  AppendableStructNoXTypes as;
  as.key = key_value;

  const ReturnCode_t ret = typed_dw->write(as, HANDLE_NIL);
  if (ret != RETCODE_OK) {
    ACE_ERROR((LM_ERROR, "ERROR: write_appendable_struct_no_xtypes returned %C\n",
               OpenDDS::DCPS::retcode_to_string(ret)));
  }
  if (verbose) {
    ACE_DEBUG((LM_DEBUG, "writer: AppendableStructNoXTypes\n"));
  }
}

void write_additional_prefix_field_struct(const DataWriter_var& dw)
{
  AdditionalPrefixFieldStructDataWriter_var typed_dw = AdditionalPrefixFieldStructDataWriter::_narrow(dw);

  AdditionalPrefixFieldStruct apfs;
  apfs.key = key_value;
  apfs.additional_field = APPENDABLE_STRUCT_AF;

  const ReturnCode_t ret = typed_dw->write(apfs, HANDLE_NIL);
  if (ret != RETCODE_OK) {
    ACE_ERROR((LM_ERROR, "ERROR: write_additional_prefix_field_struct returned %C\n",
               OpenDDS::DCPS::retcode_to_string(ret)));
  }
  if (verbose) {
    ACE_DEBUG((LM_DEBUG, "writer: AdditionalPrefixFieldStruct\n"));
  }
}

void write_additional_postfix_field_struct(const DataWriter_var& dw)
{
  AdditionalPostfixFieldStructDataWriter_var typed_dw = AdditionalPostfixFieldStructDataWriter::_narrow(dw);

  AdditionalPostfixFieldStruct apfs;
  apfs.key = key_value;
  apfs.additional_field = APPENDABLE_STRUCT_AF;

  const ReturnCode_t ret = typed_dw->write(apfs, HANDLE_NIL);
  if (ret != RETCODE_OK) {
    ACE_ERROR((LM_ERROR, "ERROR: write_additional_postfix_field_struct returned %C\n",
               OpenDDS::DCPS::retcode_to_string(ret)));
  }
  if (verbose) {
    ACE_DEBUG((LM_DEBUG, "writer: AdditionalPostfixFieldStruct\n"));
  }
}

void write_modified_mutable_struct(const DataWriter_var& dw)
{
  ModifiedMutableStructDataWriter_var typed_dw = ModifiedMutableStructDataWriter::_narrow(dw);

  ModifiedMutableStruct ams;
  ams.key = key_value;
  ams.additional_field = MUTABLE_STRUCT_AF;

  const ReturnCode_t ret = typed_dw->write(ams, HANDLE_NIL);
  if (ret != RETCODE_OK) {
    ACE_ERROR((LM_ERROR, "ERROR: write_modified_mutable_struct returned %C\n",
               OpenDDS::DCPS::retcode_to_string(ret)));
  }
  if (verbose) {
    ACE_DEBUG((LM_DEBUG, "writer: ModifiedMutableStruct\n"));
  }
}

void write_modified_mutable_union(const DataWriter_var& dw)
{
  ModifiedMutableUnionDataWriter_var typed_dw = ModifiedMutableUnionDataWriter::_narrow(dw);

  ModifiedMutableUnion mmu;
  mmu.key(key_value);

  const ReturnCode_t ret = typed_dw->write(mmu, HANDLE_NIL);
  if (ret != RETCODE_OK) {
    ACE_ERROR((LM_ERROR, "ERROR: write_modified_mutable_union returned %C\n",
               OpenDDS::DCPS::retcode_to_string(ret)));
  }
  if (verbose) {
    ACE_DEBUG((LM_DEBUG, "writer: ModifiedMutableUnion\n"));
  }
}

void write_trim64_struct(const DataWriter_var& dw)
{
  Trim64StructDataWriter_var typed_dw = Trim64StructDataWriter::_narrow(dw);

  Trim64Struct tcs;
  tcs.trim_string = STRING_26.c_str();

  const ReturnCode_t ret = typed_dw->write(tcs, HANDLE_NIL);
  if (ret != RETCODE_OK) {
    ACE_ERROR((LM_ERROR, "ERROR: write_trim64_struct returned %C\n",
               OpenDDS::DCPS::retcode_to_string(ret)));
  }
  if (verbose) {
    ACE_DEBUG((LM_DEBUG, "writer: Trim64Struct\n"));
  }
}

void write_appendable_struct_with_dependency(const DataWriter_var& dw)
{
  AppendableStructWithDependencyDataWriter_var typed_dw = AppendableStructWithDependencyDataWriter::_narrow(dw);

  AppendableStructWithDependency as;
  as.key = key_value;
  as.additional_nested_struct.additional_field = NESTED_STRUCT_AF;

  const ReturnCode_t ret = typed_dw->write(as, HANDLE_NIL);
  if (ret != RETCODE_OK) {
    ACE_ERROR((LM_ERROR, "ERROR: write_appendable_struct_with_dependency returned %C\n",
               OpenDDS::DCPS::retcode_to_string(ret)));
  }
  if (verbose) {
    ACE_DEBUG((LM_DEBUG, "writer: AppendableStructWithDependency\n"));
  }
}

void write_modified_name_mutable_struct(const DataWriter_var& dw)
{
  ModifiedNameMutableStructDataWriter_var typed_dw = ModifiedNameMutableStructDataWriter::_narrow(dw);
  ModifiedNameMutableStruct sample;
  sample.key_modified = key_value;
  sample.additional_field_modified = MUTABLE_STRUCT_AF;

  const ReturnCode_t ret = typed_dw->write(sample, HANDLE_NIL);
  if (ret != RETCODE_OK) {
    ACE_ERROR((LM_ERROR, "ERROR: write_modified_name_mutable_struct returned %C\n",
               OpenDDS::DCPS::retcode_to_string(ret)));
  }
  if (verbose) {
    ACE_DEBUG((LM_DEBUG, "writer: ModifiedNameMutableStruct\n"));
  }
}

void write_modified_name_mutable_union(const DataWriter_var& dw)
{
  ModifiedNameMutableUnionDataWriter_var typed_dw = ModifiedNameMutableUnionDataWriter::_narrow(dw);
  ModifiedNameMutableUnion sample;
  sample.key_modified(key_value);

  const ReturnCode_t ret = typed_dw->write(sample, HANDLE_NIL);
  if (ret != RETCODE_OK) {
    ACE_ERROR((LM_ERROR, "ERROR: write_modified_name_mutable_union returned %C\n",
               OpenDDS::DCPS::retcode_to_string(ret)));
  }
  if (verbose) {
    ACE_DEBUG((LM_DEBUG, "writer:  ModifiedNameMutableUnion\n"));
  }
}

int ACE_TMAIN(int argc, ACE_TCHAR* argv[])
{
  std::string type;
  std::string topic_name = "";
  std::string registered_type_name = "";

  DomainParticipantFactory_var dpf = TheParticipantFactoryWithArgs(argc, argv);

  // Arguments "--type" and "--expect_to_fail" must be specified
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
    } else if (arg == ACE_TEXT("--reg_type")) {
      if (i + 1 < argc) {
        registered_type_name = ACE_TEXT_ALWAYS_CHAR(argv[++i]);
      } else {
        ACE_ERROR((LM_ERROR, "ERROR: Invalid registered type argument\n"));
        return 1;
      }
    } else if (arg == ACE_TEXT("--key_val")) {
      if (i + 1 < argc) {
        key_value = ACE_OS::atoi(argv[++i]);
      } else {
        ACE_ERROR((LM_ERROR, "ERROR: Invalid key value argument\n"));
        return 1;
      }
    } else if (arg == ACE_TEXT("--expect_to_fail")) {
      expect_to_match = false;
    } else {
      ACE_ERROR((LM_ERROR, "ERROR: Invalid argument: %s\n", argv[i]));
      return 1;
    }
  }

  DomainParticipant_var dp;
  create_participant(dpf, dp);
  if (!dp) {
    ACE_ERROR((LM_ERROR, "ERROR: create_participant failed\n"));
    return 1;
  }

  bool failed = false;

  Topic_var topic;

  if (topic_name.empty()) {
    topic_name = !registered_type_name.empty() ? registered_type_name : type;
  } else if (registered_type_name.empty()) {
    registered_type_name = topic_name;
  }

  if (type == "PlainCdrStruct") {
    PlainCdrStructTypeSupport_var ts = new PlainCdrStructTypeSupportImpl;
    failed = !get_topic(ts, dp, topic_name, topic, registered_type_name);
  } else if (type == "FinalStructPub") {
    FinalStructPubTypeSupport_var ts = new FinalStructPubTypeSupportImpl;
    failed = !get_topic(ts, dp, topic_name, topic, registered_type_name);
  } else if (type == "ModifiedFinalStruct") {
    ModifiedFinalStructTypeSupport_var ts = new ModifiedFinalStructTypeSupportImpl;
    failed = !get_topic(ts, dp, topic_name, topic, registered_type_name);
  } else if (type == "AppendableStructNoXTypes") {
    AppendableStructNoXTypesTypeSupport_var ts = new AppendableStructNoXTypesTypeSupportImpl;
    failed = !get_topic(ts, dp, topic_name, topic, registered_type_name);
  } else if (type == "AdditionalPrefixFieldStruct") {
    AdditionalPrefixFieldStructTypeSupport_var ts = new AdditionalPrefixFieldStructTypeSupportImpl;
    failed = !get_topic(ts, dp, topic_name, topic, registered_type_name);
  } else if (type == "AdditionalPostfixFieldStruct") {
    AdditionalPostfixFieldStructTypeSupport_var ts = new AdditionalPostfixFieldStructTypeSupportImpl;
    failed = !get_topic(ts, dp, topic_name, topic, registered_type_name);
  } else if (type == "ModifiedMutableStruct") {
    ModifiedMutableStructTypeSupport_var ts = new ModifiedMutableStructTypeSupportImpl;
    failed = !get_topic(ts, dp, topic_name, topic, registered_type_name);
  } else if (type == "ModifiedIdMutableStruct") {
    ModifiedIdMutableStructTypeSupport_var ts = new ModifiedIdMutableStructTypeSupportImpl;
    failed = !get_topic(ts, dp, topic_name, topic, registered_type_name);
  } else if (type == "ModifiedTypeMutableStruct") {
    ModifiedTypeMutableStructTypeSupport_var ts = new ModifiedTypeMutableStructTypeSupportImpl;
    failed = !get_topic(ts, dp, topic_name, topic, registered_type_name);
  } else if (type == "ModifiedNameMutableStruct") {
    ModifiedNameMutableStructTypeSupport_var ts = new ModifiedNameMutableStructTypeSupportImpl;
    failed = !get_topic(ts, dp, topic_name, topic, registered_type_name);
  } else if (type == "ModifiedMutableUnion") {
    ModifiedMutableUnionTypeSupport_var ts = new ModifiedMutableUnionTypeSupportImpl;
    failed = !get_topic(ts, dp, topic_name, topic, registered_type_name);
  } else if (type == "ModifiedDiscMutableUnion") {
    ModifiedDiscMutableUnionTypeSupport_var ts = new ModifiedDiscMutableUnionTypeSupportImpl;
    failed = !get_topic(ts, dp, topic_name, topic, registered_type_name);
  } else if (type == "ModifiedTypeMutableUnion") {
    ModifiedTypeMutableUnionTypeSupport_var ts = new ModifiedTypeMutableUnionTypeSupportImpl;
    failed = !get_topic(ts, dp, topic_name, topic, registered_type_name);
  } else if (type == "ModifiedNameMutableUnion") {
    ModifiedNameMutableUnionTypeSupport_var ts = new ModifiedNameMutableUnionTypeSupportImpl;
    failed = !get_topic(ts, dp, topic_name, topic, registered_type_name);
  } else if (type == "Trim64Struct") {
    Trim64StructTypeSupport_var ts = new Trim64StructTypeSupportImpl;
    failed = !get_topic(ts, dp, topic_name, topic, registered_type_name);
  } else if (type == "AppendableStructWithDependency") {
    AppendableStructWithDependencyTypeSupport_var ts = new AppendableStructWithDependencyTypeSupportImpl;
    failed = !get_topic(ts, dp, topic_name, topic, registered_type_name);
  } else if (type.empty()) {
    ACE_ERROR((LM_ERROR, "ERROR: Must specify a type name\n"));
    return 1;
  } else {
    ACE_ERROR((LM_ERROR, "ERROR: Type %C is not supported\n", type.c_str()));
    return 1;
  }

  if (failed) {
    return 1;
  }

  ACE_DEBUG((LM_DEBUG, "Writer starting at %T\n"));
  Topic_var ack_control_topic;
  Topic_var echo_control_topic;
  ControlStructTypeSupport_var ack_control_ts = new ControlStructTypeSupportImpl;
  ControlStructTypeSupport_var echo_control_ts = new ControlStructTypeSupportImpl;
  get_topic(ack_control_ts, dp, "SET_PD_OL_OA_OM_OD_Ack", ack_control_topic, "ControlStruct");
  get_topic(echo_control_ts, dp, "SET_PD_OL_OA_OM_OD_Echo", echo_control_topic, "ControlStruct");

  Subscriber_var control_sub = dp->create_subscriber(SUBSCRIBER_QOS_DEFAULT, 0,
                                                     DEFAULT_STATUS_MASK);
  if (!control_sub) {
    ACE_ERROR((LM_ERROR, "ERROR: create_subscriber failed\n"));
    return 1;
  }
  Publisher_var control_pub = dp->create_publisher(PUBLISHER_QOS_DEFAULT, 0,
                                                   DEFAULT_STATUS_MASK);
  if (!control_pub) {
    ACE_ERROR((LM_ERROR, "ERROR: create_publisher failed for control_pub\n"));
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
  ControlStructDataReader_var control_pdr = ControlStructDataReader::_narrow(control_dr);

  Publisher_var pub = dp->create_publisher(PUBLISHER_QOS_DEFAULT, 0,
    DEFAULT_STATUS_MASK);
  if (!pub) {
    ACE_ERROR((LM_ERROR, "ERROR: create_publisher failed\n"));
    return 1;
  }

  DataWriterQos dw_qos;
  pub->get_default_datawriter_qos(dw_qos);
  dw_qos.durability.kind = TRANSIENT_LOCAL_DURABILITY_QOS;

  DataWriter_var dw = pub->create_datawriter(topic, dw_qos, 0,
    DEFAULT_STATUS_MASK);
  if (!dw) {
    ACE_ERROR((LM_ERROR, "ERROR: create_datawriter failed\n"));
    return 1;
  }

  DDS::StatusCondition_var condition = expect_to_match ? dw->get_statuscondition() : topic->get_statuscondition();
  condition->set_enabled_statuses(expect_to_match ? DDS::PUBLICATION_MATCHED_STATUS : DDS::INCONSISTENT_TOPIC_STATUS);
  DDS::WaitSet_var ws = new DDS::WaitSet;
  ws->attach_condition(condition);

  DDS::ConditionSeq conditions;
  DDS::Duration_t timeout = { 10, 0 };
  ReturnCode_t ret = ws->wait(conditions, timeout);
  if (ret != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, "ERROR: %C condition wait failed for type %C: %C\n",
      expect_to_match ? "PUBLICATION_MATCHED_STATUS" : "INCONSISTENT_TOPIC_STATUS", type.c_str(),
      OpenDDS::DCPS::retcode_to_string(ret)));
    failed = true;
  }

  ws->detach_condition(condition);

  if (!failed) {
    failed = !check_inconsistent_topic_status(topic);
  }

  if (failed) {
    ACE_ERROR((LM_ERROR, "ERROR: Writer failed for type %C\n", type.c_str()));
  } else if (expect_to_match) {
    if (type == "PlainCdrStruct") {
      write_plain_cdr_struct(dw);
    } else if (type == "FinalStructPub") {
      write_final_struct(dw);
    } else if (type == "ModifiedFinalStruct") {
      write_modified_final_struct(dw);
    } else if (type == "AppendableStructNoXTypes") {
      write_appendable_struct_no_xtypes(dw);
    } else if (type == "AdditionalPrefixFieldStruct") {
      write_additional_prefix_field_struct(dw);
    } else if (type == "AdditionalPostfixFieldStruct") {
      write_additional_postfix_field_struct(dw);
    } else if (type == "ModifiedMutableStruct") {
      write_modified_mutable_struct(dw);
    } else if (type == "ModifiedMutableUnion") {
      write_modified_mutable_union(dw);
    } else if (type == "Trim64Struct") {
      write_trim64_struct(dw);
    } else if (type == "AppendableStructWithDependency") {
      write_appendable_struct_with_dependency(dw);
    } else if (type == "ModifiedNameMutableStruct") {
      write_modified_name_mutable_struct(dw);
    } else if (type == "ModifiedNameMutableUnion") {
      write_modified_name_mutable_union(dw);
    }
  }

  ACE_DEBUG((LM_DEBUG, "Writer waiting for ack at %T\n"));

  ::ControlStructSeq control_data;
  ret = read_i(control_dr, control_pdr, control_data);
  if (ret != RETCODE_OK) {
    ACE_ERROR((LM_ERROR, "ERROR: control read returned %C\n",
      OpenDDS::DCPS::retcode_to_string(ret)));
    return 1;
  }

  if (!wait_for_reader (true, control_dw)) {
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

  if (!wait_for_reader (false, control_dw)) {
    return 1;
  }
  topic = 0;
  ACE_DEBUG((LM_DEBUG, "Writer cleanup at %T\n"));

  dp->delete_contained_entities();
  dpf->delete_participant(dp);
  TheServiceParticipant->shutdown();

  ACE_DEBUG((LM_DEBUG, "writer exiting at %T\n"));
  return failed ? 1 : 0;
}
