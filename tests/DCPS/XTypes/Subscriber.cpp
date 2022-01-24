#include "Common.h"
#include "SubscriberTypeSupportImpl.h"
#include "CommonTypeSupportImpl.h"

#include <dds/DCPS/DCPS_Utils.h>

template<typename T1>
ReturnCode_t check_additional_field_value(const T1& data,
  AdditionalFieldValue expected_additional_field_value)
{
  ReturnCode_t ret = RETCODE_OK;
  if (data[0].additional_field != expected_additional_field_value) {
    ACE_DEBUG((LM_DEBUG, "reader: expected additional field value: %d, received: %d\n",
               expected_additional_field_value, data[0].additional_field));
    ret = RETCODE_ERROR;
  }

  return ret;
}

template<typename T1, typename T2>
ReturnCode_t read_tryconstruct_struct(const DataReader_var& dr, const T1& pdr, T2& data,
                                      const std::string& expected_value)
{
  ReturnCode_t ret = RETCODE_OK;
  if ((ret = read_i(dr, pdr, data)) == RETCODE_OK) {
    if (data.length() != 1) {
      ACE_ERROR((LM_ERROR, "ERROR: reader: unexpected data length: %d\n", data.length()));
      ret = RETCODE_ERROR;
    } else if (ACE_OS::strcmp(data[0].trim_string, expected_value.c_str()) != 0) {
      ACE_ERROR((LM_ERROR, "ERROR: reader: expected key value: %C, received: %C\n",
                 expected_value.c_str(), data[0].trim_string.in()));
      ret = RETCODE_ERROR;
    } else if (verbose) {
      ACE_ERROR((LM_DEBUG, "reader: %C\n", data[0].trim_string.in()));
    }
  } else {
    ACE_ERROR((LM_ERROR, "ERROR: Reader: read_i returned %C\n",
               OpenDDS::DCPS::retcode_to_string(ret)));
  }

  return ret;
}

template<typename T1, typename T2>
ReturnCode_t read_struct(const DataReader_var& dr, const T1& pdr, T2& data)
{
  ReturnCode_t ret = RETCODE_OK;
  if ((ret = read_i(dr, pdr, data)) == RETCODE_OK) {
    if (data.length() != 1) {
      ACE_ERROR((LM_ERROR, "reader: unexpected data length: %d\n", data.length()));
      ret = RETCODE_ERROR;
    } else if (data[0].key_field != key_value) {
      ACE_ERROR((LM_ERROR, "reader: expected key value: %d, received: %d\n", key_value, data[0].key_field));
      ret = RETCODE_ERROR;
    } else if (verbose) {
      ACE_DEBUG((LM_DEBUG, "reader: %d\n", data[0].key_field));
    }
  } else {
    ACE_ERROR((LM_ERROR, "ERROR: Reader: read_i returned %C\n",
               OpenDDS::DCPS::retcode_to_string(ret)));
  }

  return ret;
}

template<typename T1, typename T2>
ReturnCode_t read_union(const DataReader_var& dr, const T1& pdr, T2& data)
{
  ReturnCode_t ret = RETCODE_OK;
  if ((ret = read_i(dr, pdr, data)) == RETCODE_OK) {
    if (data.length() != 1) {
      ACE_ERROR((LM_ERROR, "reader: unexpected data length: %d\n", data.length()));
      ret = RETCODE_ERROR;
    } else {
      switch (data[0]._d()) {
      case E_KEY:
        if (data[0].key_field() != key_value) {
          ACE_ERROR((LM_ERROR, "reader: expected union key value: %d, received: %d\n",
            key_value, data[0].key_field()));
          ret = RETCODE_ERROR;
        }
        if (verbose) {
          ACE_DEBUG((LM_DEBUG, "reader: union key %d\n", data[0].key_field()));
        }
        break;
      case E_ADDITIONAL_FIELD:
        if (data[0].additional_field() != key_value) {
          ACE_ERROR((LM_ERROR, "reader: expected additional_field value: %d, received: %d\n",
            key_value, data[0].additional_field()));
          ret = RETCODE_ERROR;
        }
        if (verbose) {
          ACE_DEBUG((LM_DEBUG, "reader: union additional_field %d\n", data[0].additional_field()));
        }
        break;
      default:
        break;
      }
    }
  } else {
    ACE_ERROR((LM_ERROR, "ERROR: Reader: read_i returned %C\n",
               OpenDDS::DCPS::retcode_to_string(ret)));
  }

  return ret;
}

ReturnCode_t read_plain_cdr_struct(const DataReader_var& dr)
{
  PlainCdrStructDataReader_var pdr = PlainCdrStructDataReader::_narrow(dr);
  ::PlainCdrStructSeq data;
  return read_struct(dr, pdr, data);
}


ReturnCode_t read_final_struct(const DataReader_var& dr)
{
  FinalStructSubDataReader_var pdr = FinalStructSubDataReader::_narrow(dr);
  ::FinalStructSubSeq data;
  return read_struct(dr, pdr, data);
}

ReturnCode_t read_appendable_struct(const DataReader_var& dr)
{
  AppendableStructDataReader_var pdr = AppendableStructDataReader::_narrow(dr);
  ::AppendableStructSeq data;
  return read_struct(dr, pdr, data);
}

ReturnCode_t read_appendable_struct_no_xtypes(const DataReader_var& dr)
{
  AppendableStructNoXTypesDataReader_var pdr = AppendableStructNoXTypesDataReader::_narrow(dr);
  ::AppendableStructNoXTypesSeq data;
  return read_struct(dr, pdr, data);
}

ReturnCode_t read_mutable_struct(const DataReader_var& dr, const AdditionalFieldValue& afv)
{
  MutableStructDataReader_var pdr = MutableStructDataReader::_narrow(dr);
  ::MutableStructSeq data;
  ReturnCode_t ret;
  ret = read_struct(dr, pdr, data);
  if (ret == RETCODE_OK) {
    ret = check_additional_field_value(data, afv);
  }
  return ret;
}

ReturnCode_t read_mutable_union(const DataReader_var& dr)
{
  MutableUnionDataReader_var pdr = MutableUnionDataReader::_narrow(dr);
  ::MutableUnionSeq data;
  return read_union(dr, pdr, data);
}

ReturnCode_t read_trim20_struct(const DataReader_var& dr)
{
  Trim20StructDataReader_var pdr = Trim20StructDataReader::_narrow(dr);
  ::Trim20StructSeq data;
  return read_tryconstruct_struct(dr, pdr, data, STRING_20);
}


int ACE_TMAIN(int argc, ACE_TCHAR* argv[])
{
  std::string type;
  std::string topic_name = "";
  std::string registered_type_name = "";

  // Default properties of type consistency enforcement Qos currently supported
  bool disallow_type_coercion = false;
  bool ignore_member_names = false;
  bool force_type_validation = false;

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
    } else if (arg == ACE_TEXT("--expect_inconsistent_topic")) {
      expect_inconsistent_topic = true;
    } else if (arg == ACE_TEXT("--expect_incompatible_qos")) {
      expect_incompatible_qos = true;
    } else if (arg == ACE_TEXT("--disallow_type_coercion")) {
      disallow_type_coercion =  true;
    } else if (arg == ACE_TEXT("--ignore_member_names")) {
      ignore_member_names = true;
    } else if (arg == ACE_TEXT("--force_type_validation")) {
      force_type_validation = true;
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
  } else if (type == "FinalStructSub") {
    FinalStructSubTypeSupport_var ts = new FinalStructSubTypeSupportImpl;
    failed = !get_topic(ts, dp, topic_name, topic, registered_type_name);
  } else if (type == "AppendableStruct") {
    AppendableStructTypeSupport_var ts = new AppendableStructTypeSupportImpl;
    failed = !get_topic(ts, dp, topic_name, topic, registered_type_name);
  } else if (type == "AppendableStructNoXTypes") {
    AppendableStructNoXTypesTypeSupport_var ts = new AppendableStructNoXTypesTypeSupportImpl;
    failed = !get_topic(ts, dp, topic_name, topic, registered_type_name);
  } else if (type == "MutableStruct") {
    MutableStructTypeSupport_var ts = new MutableStructTypeSupportImpl;
    failed = !get_topic(ts, dp, topic_name, topic, registered_type_name);
  } else if (type == "MutableUnion") {
    MutableUnionTypeSupport_var ts = new MutableUnionTypeSupportImpl;
    failed = !get_topic(ts, dp, topic_name, topic, registered_type_name);
  } else if (type == "Trim20Struct") {
    Trim20StructTypeSupport_var ts = new Trim20StructTypeSupportImpl;
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

  ACE_DEBUG((LM_DEBUG, "Reader starting at %T\n"));
  Topic_var ack_control_topic;
  Topic_var echo_control_topic;
  ControlStructTypeSupport_var ack_control_ts = new ControlStructTypeSupportImpl;
  ControlStructTypeSupport_var echo_control_ts = new ControlStructTypeSupportImpl;
  get_topic(ack_control_ts, dp, "SET_PD_OL_OA_OM_OD_Ack", ack_control_topic, "ControlStruct");
  get_topic(echo_control_ts, dp, "SET_PD_OL_OA_OM_OD_Echo", echo_control_topic, "ControlStruct");

  // For publishing ack control topic.
  Publisher_var control_pub = dp->create_publisher(PUBLISHER_QOS_DEFAULT, 0,
                                                   DEFAULT_STATUS_MASK);
  if (!control_pub) {
    ACE_ERROR((LM_ERROR, "ERROR: create_publisher failed for control_pub\n"));
    return 1;
  }

  DataWriterQos control_dw_qos;
  control_pub->get_default_datawriter_qos(control_dw_qos);
  control_dw_qos.durability.kind = TRANSIENT_LOCAL_DURABILITY_QOS;

  DataWriter_var control_dw = control_pub->create_datawriter(ack_control_topic, control_dw_qos, 0,
                                                             DEFAULT_STATUS_MASK);
  if (!control_dw) {
    ACE_ERROR((LM_ERROR, "ERROR: create_datawriter for control_pub failed\n"));
    return 1;
  }

  ControlStructDataWriter_var control_typed_dw = ControlStructDataWriter::_narrow(control_dw);
  if (!control_typed_dw) {
    ACE_ERROR((LM_ERROR, "ERROR: _narrow ack datawriter failed\n"));
    return 1;
  }

  // For subscribing echo control topic.
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

  DataReader_var control_dr = control_sub->create_datareader(echo_control_topic, control_dr_qos, 0,
                                                             DEFAULT_STATUS_MASK);
  if (!control_dr) {
    ACE_ERROR((LM_ERROR, "ERROR: create_datareader failed\n"));
    return 1;
  }

  ControlStructDataReader_var control_pdr = ControlStructDataReader::_narrow(control_dr);
  if (!control_pdr) {
    ACE_ERROR((LM_ERROR, "ERROR: _narrow echo datareader failed\n"));
    return 1;
  }

  // For subscribing user topic.
  Subscriber_var sub = dp->create_subscriber(SUBSCRIBER_QOS_DEFAULT, 0, DEFAULT_STATUS_MASK);
  if (!sub) {
    ACE_ERROR((LM_ERROR, "ERROR: create_subscriber failed\n"));
    return 1;
  }

  DataReaderQos dr_qos;
  sub->get_default_datareader_qos(dr_qos);
  dr_qos.reliability.kind = RELIABLE_RELIABILITY_QOS;
  dr_qos.durability.kind = TRANSIENT_LOCAL_DURABILITY_QOS;

  if (disallow_type_coercion) {
    dr_qos.type_consistency.kind = DISALLOW_TYPE_COERCION;
  }
  dr_qos.type_consistency.ignore_member_names = ignore_member_names;
  dr_qos.type_consistency.force_type_validation = force_type_validation;
  if (expect_incompatible_qos) {
    dr_qos.representation.value.length(1);
    dr_qos.representation.value[0] = XCDR2_DATA_REPRESENTATION;
  }

  DataReader_var dr = sub->create_datareader(topic, dr_qos, 0, DEFAULT_STATUS_MASK);
  if (!dr) {
    ACE_ERROR((LM_ERROR, "ERROR: create_datareader failed\n"));
    return 1;
  }

  DDS::StatusCondition_var condition = expect_inconsistent_topic ? topic->get_statuscondition() : dr->get_statuscondition();
  const DDS::StatusMask status_mask = expect_inconsistent_topic ? DDS::INCONSISTENT_TOPIC_STATUS :
    (expect_incompatible_qos ? DDS::REQUESTED_INCOMPATIBLE_QOS_STATUS : DDS::SUBSCRIPTION_MATCHED_STATUS);
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
  } else if (status_mask == DDS::REQUESTED_INCOMPATIBLE_QOS_STATUS) {
    failed = !wait_requested_incompatible_qos(ws, dr);
  } else {
    failed = !wait_subscription_matched(ws, dr);
  }
  ws->detach_condition(condition);

  if (failed) {
    ACE_ERROR((LM_ERROR, "ERROR: Reader failed for type %C\n", type.c_str()));
    return 1;
  }

  if (!check_inconsistent_topic_status(topic) ||
      !check_requested_incompatible_qos_status(dr, type)) {
    return 1;
  }

  if (!expect_inconsistent_topic && !expect_incompatible_qos) {
    if (type == "PlainCdrStruct") {
      failed = (read_plain_cdr_struct(dr) != RETCODE_OK);
    } else if (type == "AppendableStruct") {
      failed = (read_appendable_struct(dr) != RETCODE_OK);
    } else if (type == "AppendableStructNoXTypes") {
      failed = (read_appendable_struct_no_xtypes(dr) != RETCODE_OK);
    } else if (type == "FinalStructSub") {
      failed = (read_final_struct(dr) != RETCODE_OK);
    } else if (type == "MutableStruct") {
      AdditionalFieldValue afv = MUTABLE_STRUCT_AF;
      if (topic_name == "MutableBaseStructT") {
        afv = FINAL_STRUCT_AF;
      }
      failed = (read_mutable_struct(dr, afv) != RETCODE_OK);
    } else if (type == "MutableUnion") {
      failed = (read_mutable_union(dr) != RETCODE_OK);
    } else if (type == "Trim20Struct") {
      failed = (read_trim20_struct(dr) != RETCODE_OK);
    }
  }
  if (failed) {
    return 1;
  }

  //
  // As the subscriber is now about to exit, let the publisher know it can exit too
  //
  if (!wait_for_reader(true, control_dw)) {
    return 1;
  }

  ACE_DEBUG((LM_DEBUG, "Reader sending ack at %T\n"));

  ControlStruct cs;
  ret = control_typed_dw->write(cs, HANDLE_NIL);
  if (ret != RETCODE_OK) {
    ACE_ERROR((LM_ERROR, "ERROR: control write returned %C\n",
      OpenDDS::DCPS::retcode_to_string(ret)));
    return 1;
  }

  ACE_DEBUG((LM_DEBUG, "Reader waiting for echo at %T\n"));

  ::ControlStructSeq control_data;
  ret = read_i(control_dr, control_pdr, control_data, true);
  if (ret != RETCODE_OK) {
    ACE_ERROR((LM_ERROR, "ERROR: control read returned %C\n",
      OpenDDS::DCPS::retcode_to_string(ret)));
    return 1;
  }

  ACE_DEBUG((LM_DEBUG, "Reader cleanup at %T\n"));

  topic = 0;
  dp->delete_contained_entities();
  dpf->delete_participant(dp);
  TheServiceParticipant->shutdown();
  return 0;
}
