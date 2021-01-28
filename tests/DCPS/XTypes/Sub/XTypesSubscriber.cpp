#include "XTypes.h"
#include "XTypesSubTypeSupportImpl.h"

template<typename T1>
ReturnCode_t check_additional_field_value(const T1& data, AdditionalFieldValue expected_additional_field_value)
{
  ReturnCode_t ret = RETCODE_OK;
  if (data[0].additional_field != expected_additional_field_value) {
    ACE_DEBUG((LM_DEBUG, "reader: expected additional field value: %d, received: %d\n", expected_additional_field_value, data[0].additional_field));
    ret = RETCODE_ERROR;
  }

  return ret;
}

template<typename T1, typename T2>
ReturnCode_t read_i(const DataReader_var& dr, const T1& pdr, T2& data)
{
  ReadCondition_var dr_rc = dr->create_readcondition(NOT_READ_SAMPLE_STATE,
    ANY_VIEW_STATE,
    ALIVE_INSTANCE_STATE);
  WaitSet_var ws = new WaitSet;
  ws->attach_condition(dr_rc);
  std::set<int> instances;

  ConditionSeq active;
  const Duration_t max_wait = { 10, 0 };
  ReturnCode_t ret = ws->wait(active, max_wait);

  if (ret == RETCODE_TIMEOUT) {
    ACE_ERROR((LM_ERROR, "ERROR: reader timedout\n"));
  } else if (ret != RETCODE_OK) {
    ACE_ERROR((LM_ERROR, "ERROR: Reader: wait returned %d\n", ret));
  } else {
    SampleInfoSeq info;
    if ((ret = pdr->take_w_condition(data, info, LENGTH_UNLIMITED, dr_rc)) != RETCODE_OK) {
      ACE_ERROR((LM_ERROR, "ERROR: Reader: take_w_condition returned %d\n", ret));
    }
  }

  ws->detach_condition(dr_rc);
  dr->delete_readcondition(dr_rc);

  return ret;
}

template<typename T1, typename T2>
ReturnCode_t read_tryconstruct_struct(const DataReader_var& dr, const T1& pdr, T2& data, const std::string& expected_value)
{
  ReturnCode_t ret = RETCODE_OK;
  if ((ret = read_i(dr, pdr, data)) == RETCODE_OK) {
    if (data.length() != 1) {
      ACE_ERROR((LM_ERROR, "ERROR: reader: unexpected data length: %d", data.length()));
      ret = RETCODE_ERROR;
    } else if (ACE_OS::strcmp(data[0].trim_string, expected_value.c_str()) != 0) {
      ACE_ERROR((LM_ERROR, "ERROR: reader: expected key value: %C, received: %C\n", expected_value.c_str(), data[0].trim_string.in()));
      ret = RETCODE_ERROR;
    } else if (verbose) {
      ACE_ERROR((LM_DEBUG, "reader: %C\n", data[0].trim_string.in()));
    }
  } else {
    ACE_ERROR((LM_ERROR, "ERROR: Reader: read_i returned %d\n", ret));
  }

  return ret;
}

template<typename T1, typename T2>
ReturnCode_t read_struct(const DataReader_var& dr, const T1& pdr, T2& data)
{
  ReturnCode_t ret = RETCODE_OK;
  if ((ret = read_i(dr, pdr, data)) == RETCODE_OK) {
    if (data.length() != 1) {
      ACE_ERROR((LM_ERROR, "reader: unexpected data length: %d", data.length()));
      ret = RETCODE_ERROR;
    } else if (data[0].key != key_value) {
      ACE_ERROR((LM_ERROR, "reader: expected key value: %d, received: %d\n", key_value, data[0].key));
      ret = RETCODE_ERROR;
    } else if (verbose) {
      ACE_DEBUG((LM_DEBUG, "reader: %d\n", data[0].key));
    }
  } else {
    ACE_ERROR((LM_ERROR, "ERROR: Reader: read_i returned %d\n", ret));
  }

  return ret;
}

template<typename T1, typename T2>
ReturnCode_t read_union(const DataReader_var& dr, const T1& pdr, T2& data)
{
  ReturnCode_t ret = RETCODE_OK;
  if ((ret = read_i(dr, pdr, data)) == RETCODE_OK) {
    if (data.length() != 1) {
      ACE_ERROR((LM_ERROR, "reader: unexpected data length: %d", data.length()));
      ret = RETCODE_ERROR;
    } else {
      switch (data[0]._d()) {
      case E_KEY:
        if (data[0].key() != key_value) {
          ACE_ERROR((LM_ERROR, "reader: expected union key value: %d, received: %d\n",
            key_value, data[0].key()));
          ret = RETCODE_ERROR;
        }
        if (verbose) {
          ACE_DEBUG((LM_DEBUG, "reader: union key %d\n", data[0].key()));
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
    ACE_ERROR((LM_ERROR, "ERROR: Reader: read_i returned %d\n", ret));
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

ReturnCode_t read_mutable_struct(const DataReader_var& dr)
{
  MutableStructDataReader_var pdr = MutableStructDataReader::_narrow(dr);
  ::MutableStructSeq data;
  ReturnCode_t ret;
  ret = read_struct(dr, pdr, data);
  if (ret == RETCODE_OK) {
    ret = check_additional_field_value(data, MUTABLE_STRUCT_AF);
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
  std::string registered_type_name = "";

  DomainParticipantFactory_var dpf = TheParticipantFactoryWithArgs(argc, argv);

  for (int i = 1; i < argc; ++i) {
    ACE_TString arg(argv[i]);
    if (arg == ACE_TEXT("--verbose")) {
      verbose = true;
    } else if (arg == ACE_TEXT("--writer")) {
    } else if (arg == ACE_TEXT("--reader")) {
    } else if (arg == ACE_TEXT("--type")) {
      if (i + 1 < argc) {
        type = ACE_TEXT_ALWAYS_CHAR(argv[++i]);
      } else {
        ACE_ERROR((LM_ERROR, "ERROR: Invalid type argument"));
        return 1;
      }
    } else if (arg == ACE_TEXT("--type_r")) {
      if (i + 1 < argc) {
        registered_type_name = ACE_TEXT_ALWAYS_CHAR(argv[++i]);
      } else {
        ACE_ERROR((LM_ERROR, "ERROR: Invalid registered type argument"));
        return 1;
      }
    } else if (arg == ACE_TEXT("--key_val")) {
      if (i + 1 < argc) {
        key_value = ACE_OS::atoi(argv[++i]);
      } else {
        ACE_ERROR((LM_ERROR, "ERROR: Invalid key value argument"));
        return 1;
      }
    } else if (arg == ACE_TEXT("--expect_to_fail")) {
      expect_to_match = false;
    } else if (arg == ACE_TEXT("--enable_security")) {
      security_requested = true;
    } else {
      ACE_ERROR((LM_ERROR, "ERROR: Invalid argument: %s\n", argv[i]));
      return 1;
    }
  }

  DomainParticipant_var dp;
  create_participant(dpf, dp);
  if (!dp) {
    ACE_ERROR((LM_ERROR, "ERROR: create_participant() failed"));
    return 1;
  }

  bool failed = false;

  Topic_var topic;

  const std::string topic_name =
    !registered_type_name.empty() ? registered_type_name : type;

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
  } else {
    ACE_ERROR((LM_ERROR, "ERROR: Type %C is not supported\n", type.c_str()));
    return 1;
  }

  if (failed) {
    return 1;
  }

  ACE_DEBUG((LM_DEBUG, "Reader starting at %T\n"));

  Subscriber_var sub = dp->create_subscriber(SUBSCRIBER_QOS_DEFAULT, 0,
    DEFAULT_STATUS_MASK);
  DataReaderQos dr_qos;
  sub->get_default_datareader_qos(dr_qos);
  dr_qos.reliability.kind = RELIABLE_RELIABILITY_QOS;
  dr_qos.durability.kind = TRANSIENT_LOCAL_DURABILITY_QOS;

  DataReader_var dr = sub->create_datareader(topic, dr_qos, 0,
    DEFAULT_STATUS_MASK);

  DDS::StatusCondition_var condition = expect_to_match ? dr->get_statuscondition() : topic->get_statuscondition();
  condition->set_enabled_statuses(expect_to_match ? DDS::SUBSCRIPTION_MATCHED_STATUS : DDS::INCONSISTENT_TOPIC_STATUS);
  DDS::WaitSet_var ws = new DDS::WaitSet;
  ws->attach_condition(condition);

  DDS::ConditionSeq conditions;
  DDS::Duration_t timeout = { 10, 0 };
  if (ws->wait(conditions, timeout) != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, "ERROR: %C condition wait failed for type %C\n",
      expect_to_match ? "SUBSCRIPTION_MATCHED_STATUS" : "INCONSISTENT_TOPIC_STATUS", type.c_str()));
    failed = 1;
  }

  ws->detach_condition(condition);

  if (!failed) {
    failed = !check_inconsistent_topic_status(topic);
  }

  if (failed) {
    ACE_ERROR((LM_ERROR, "ERROR: Reader failed for type %C\n", type.c_str()));
  } else if (expect_to_match) {
    if (type == "PlainCdrStruct") {
      failed = (read_plain_cdr_struct(dr) != RETCODE_OK);
    } else if (type == "AppendableStruct") {
      failed = (read_appendable_struct(dr) != RETCODE_OK);
    } else if (type == "AppendableStructNoXTypes") {
      failed = (read_appendable_struct_no_xtypes(dr) != RETCODE_OK);
    } else if (type == "FinalStructSub") {
      failed = (read_final_struct(dr) != RETCODE_OK);
    } else if (type == "MutableStruct") {
      failed = (read_mutable_struct(dr) != RETCODE_OK);
    } else if (type == "MutableUnion") {
      failed = (read_mutable_union(dr) != RETCODE_OK);
    } else if (type == "Trim20Struct") {
      failed = (read_trim20_struct(dr) != RETCODE_OK);
    }
  }

  topic = 0;
  dp->delete_contained_entities();
  dpf->delete_participant(dp);
  TheServiceParticipant->shutdown();
  return failed ? 1 : 0;
}
