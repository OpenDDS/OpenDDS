#include "Common.h"
#include "PublisherNonMutableStructsTypeSupportImpl.h"
#include "PublisherMutableStructsTypeSupportImpl.h"
#include "PublisherUnionsTypeSupportImpl.h"
#include "CommonTypeSupportImpl.h"

#include <dds/DCPS/DCPS_Utils.h>

void write_plain_cdr_struct(const DataWriter_var& dw)
{
  PlainCdrStructDataWriter_var typed_dw = PlainCdrStructDataWriter::_narrow(dw);

  PlainCdrStruct pcs;
  pcs.key_field = key_value;
  pcs.value_field = 1;

  const ReturnCode_t ret = typed_dw->write(pcs, HANDLE_NIL);
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
  FinalStructPubDataWriter_var typed_dw = FinalStructPubDataWriter::_narrow(dw);

  FinalStructPub fs;
  fs.key_field = key_value;

  const ReturnCode_t ret = typed_dw->write(fs, HANDLE_NIL);
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
  ModifiedFinalStructDataWriter_var typed_dw = ModifiedFinalStructDataWriter::_narrow(dw);

  ModifiedFinalStruct mfs;
  mfs.key_field = key_value;
  mfs.additional_field = FINAL_STRUCT_AF;

  const ReturnCode_t ret = typed_dw->write(mfs, HANDLE_NIL);
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
  as.key_field = key_value;

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
  apfs.key_field = key_value;
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

void write_base_appendable_struct(const DataWriter_var& dw)
{
  BaseAppendableStructDataWriter_var typed_dw = BaseAppendableStructDataWriter::_narrow(dw);

  BaseAppendableStruct bas;
  bas.key_field = key_value;

  const ReturnCode_t ret = typed_dw->write(bas, HANDLE_NIL);
  if (ret != RETCODE_OK) {
    ACE_ERROR((LM_ERROR, "ERROR: write_base_appendable_struct returned %C\n",
               OpenDDS::DCPS::retcode_to_string(ret)));
  }
  if (verbose) {
    ACE_DEBUG((LM_DEBUG, "writer: BaseAppendableStruct\n"));
  }
}

void write_additional_postfix_field_struct(const DataWriter_var& dw)
{
  AdditionalPostfixFieldStructDataWriter_var typed_dw = AdditionalPostfixFieldStructDataWriter::_narrow(dw);

  AdditionalPostfixFieldStruct apfs;
  apfs.key_field = key_value;
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
  ams.key_field = key_value;
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

void write_mutable_base_struct(const DataWriter_var& dw)
{
  MutableBaseStructDataWriter_var typed_dw = MutableBaseStructDataWriter::_narrow(dw);

  MutableBaseStruct mbs;
  mbs.key_field = key_value;

  const ReturnCode_t ret = typed_dw->write(mbs, HANDLE_NIL);
  if (ret != RETCODE_OK) {
    ACE_ERROR((LM_ERROR, "ERROR: write_mutable_base_struct returned %C\n",
               OpenDDS::DCPS::retcode_to_string(ret)));
  }
  if (verbose) {
    ACE_DEBUG((LM_DEBUG, "writer: MutableBaseStruct\n"));
  }
}

void write_modified_mutable_union(const DataWriter_var& dw)
{
  ModifiedMutableUnionDataWriter_var typed_dw = ModifiedMutableUnionDataWriter::_narrow(dw);

  ModifiedMutableUnion mmu;
  mmu.key_field(key_value);

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
  as.key_field = key_value;
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
  sample.key_field_modified = key_value;
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
  sample.key_field_modified(key_value);

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
  bool failed = false;
  if (type == "PlainCdrStruct") {
    failed = !get_topic<PlainCdrStruct>(ts, dp, topic_name, topic, registered_type_name, dynamic);
  } else if (type == "FinalStructPub") {
    failed = !get_topic<FinalStructPub>(ts, dp, topic_name, topic, registered_type_name, dynamic);
  } else if (type == "ModifiedFinalStruct") {
    failed = !get_topic<ModifiedFinalStruct>(ts, dp, topic_name, topic, registered_type_name, dynamic);
  } else if (type == "BaseAppendableStruct") {
    failed = !get_topic<BaseAppendableStruct>(ts, dp, topic_name, topic, registered_type_name, dynamic);
  } else if (type == "AppendableStructNoXTypes") {
    failed = !get_topic<AppendableStructNoXTypes>(ts, dp, topic_name, topic, registered_type_name, dynamic);
  } else if (type == "AdditionalPrefixFieldStruct") {
    failed = !get_topic<AdditionalPrefixFieldStruct>(ts, dp, topic_name, topic, registered_type_name, dynamic);
  } else if (type == "AdditionalPostfixFieldStruct") {
    failed = !get_topic<AdditionalPostfixFieldStruct>(ts, dp, topic_name, topic, registered_type_name, dynamic);
  } else if (type == "ModifiedMutableStruct") {
    failed = !get_topic<ModifiedMutableStruct>(ts, dp, topic_name, topic, registered_type_name, dynamic);
  } else if (type == "ModifiedIdMutableStruct") {
    failed = !get_topic<ModifiedIdMutableStruct>(ts, dp, topic_name, topic, registered_type_name, dynamic);
  } else if (type == "ModifiedTypeMutableStruct") {
    failed = !get_topic<ModifiedTypeMutableStruct>(ts, dp, topic_name, topic, registered_type_name, dynamic);
  } else if (type == "ModifiedNameMutableStruct") {
    failed = !get_topic<ModifiedNameMutableStruct>(ts, dp, topic_name, topic, registered_type_name, dynamic);
  } else if (type == "ModifiedMutableUnion") {
    failed = !get_topic<ModifiedMutableUnion>(ts, dp, topic_name, topic, registered_type_name, dynamic);
  } else if (type == "ModifiedDiscMutableUnion") {
    failed = !get_topic<ModifiedDiscMutableUnion>(ts, dp, topic_name, topic, registered_type_name, dynamic);
  } else if (type == "ModifiedTypeMutableUnion") {
    failed = !get_topic<ModifiedTypeMutableUnion>(ts, dp, topic_name, topic, registered_type_name, dynamic);
  } else if (type == "ModifiedNameMutableUnion") {
    failed = !get_topic<ModifiedNameMutableUnion>(ts, dp, topic_name, topic, registered_type_name, dynamic);
  } else if (type == "Trim64Struct") {
    failed = !get_topic<Trim64Struct>(ts, dp, topic_name, topic, registered_type_name, dynamic);
  } else if (type == "AppendableStructWithDependency") {
    failed = !get_topic<AppendableStructWithDependency>(ts, dp, topic_name, topic, registered_type_name, dynamic);
  } else if (type == "MutableBaseStruct") {
    failed = !get_topic<MutableBaseStruct>(ts, dp, topic_name, topic, registered_type_name, dynamic);
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
  failed |= !get_topic<ControlStruct>(
    ack_control_ts, dp, "SET_PD_OL_OA_OM_OD_Ack", ack_control_topic, "ControlStruct");
  failed |= !get_topic<ControlStruct>(
    echo_control_ts, dp, "SET_PD_OL_OA_OM_OD_Echo", echo_control_topic, "ControlStruct");

  if (failed) {
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

  ControlStructDataReader_var control_pdr = ControlStructDataReader::_narrow(control_dr);
  if (!control_pdr) {
    ACE_ERROR((LM_ERROR, "ERROR: _narrow ack datareader failed\n"));
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
    failed = !wait_inconsistent_topic(ws, topic);
  } else if (status_mask == DDS::OFFERED_INCOMPATIBLE_QOS_STATUS) {
    failed = !wait_offered_incompatible_qos(ws, dw);
  } else {
    failed = !wait_publication_matched(ws, dw);
  }
  ws->detach_condition(condition);

  if (failed) {
    ACE_ERROR((LM_ERROR, "ERROR: Writer failed for type %C\n", type.c_str()));
    return 1;
  }

  if (!check_inconsistent_topic_status(topic) ||
      !check_offered_incompatible_qos_status(dw, type)) {
    return 1;
  }

  if (!expect_inconsistent_topic && !expect_incompatible_qos && !dynamic) {
    if (type == "PlainCdrStruct") {
      write_plain_cdr_struct(dw);
    } else if (type == "FinalStructPub") {
      write_final_struct(dw);
    } else if (type == "ModifiedFinalStruct") {
      write_modified_final_struct(dw);
    } else if (type == "BaseAppendableStruct") {
      write_base_appendable_struct(dw);
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
    } else if (type == "MutableBaseStruct") {
      write_mutable_base_struct(dw);
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
