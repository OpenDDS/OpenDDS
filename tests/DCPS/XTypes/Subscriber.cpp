#include "Common.h"
#include "SubscriberTypeSupportImpl.h"
#include "CommonTypeSupportImpl.h"

#include <dds/DCPS/DCPS_Utils.h>
#include <dds/DCPS/Definitions.h>

struct ReadTest {
  DDS::TypeSupport* ts;
  DDS::DataReader* dr;
  const std::string& topic_name;
  bool dynamic;
#if !OPENDDS_CONFIG_SAFETY_PROFILE
  DDS::DynamicData_var dd;
#endif
  CORBA::String_var type_name;
  bool success;

  ReadTest(DDS::TypeSupport* ts, DDS::DataReader* dr, const std::string& topic_name, bool dynamic)
    : ts(ts)
    , dr(dr)
    , topic_name(topic_name)
    , dynamic(dynamic)
#if !OPENDDS_CONFIG_SAFETY_PROFILE
    , dd(0)
#endif
    , type_name(ts->get_type_name())
    , success(true)
  {
  }

  void check_int32(const std::string& path, CORBA::Int32 expected, CORBA::Int32 actual)
  {
    if (expected != actual) {
      ACE_ERROR((LM_ERROR, "ERROR: check_int32 for %C.%C: expected %d, received %d\n",
        type_name.in(), path.c_str(), expected, actual));
      success = false;
    }
  }

  void check_int32(const std::string& path, CORBA::Int32 expected)
  {
#if OPENDDS_CONFIG_SAFETY_PROFILE
    ACE_UNUSED_ARG(path);
    ACE_UNUSED_ARG(expected);
    success = false;
#else
    DDS::DynamicData_var use_dd;
    DDS::MemberId use_id;
    CORBA::Int32 actual;
    OpenDDS::XTypes::MemberPath member_path;
    DDS::DynamicType_var dt = ts->get_type();
    if (!(dd && check_rc(member_path.resolve_string_path(dt, path), "check_int32: resolve_string_path") &&
        check_rc(member_path.get_member_from_data(dd, use_dd, use_id), "check_int32: get_member_from_data") &&
        check_rc(use_dd->get_int32_value(actual, use_id), "check_int32: get_int32_value"))) {
      ACE_ERROR((LM_ERROR, "ERROR: check_int32 for %C.%C failed\n", type_name.in(), path.c_str()));
      success = false;
      return;
    }
    check_int32(path, expected, actual);
#endif
  }

  void check_string(const std::string& path, const std::string& expected, const char* actual)
  {
    if (expected != actual) {
      ACE_ERROR((LM_ERROR, "ERROR: check_string for %C.%C: expected \"%C\", received \"%C\"\n",
        type_name.in(), path.c_str(), expected.c_str(), actual));
      success = false;
    }
  }

  void check_string(const std::string& path, const std::string& expected)
  {
#if OPENDDS_CONFIG_SAFETY_PROFILE
    ACE_UNUSED_ARG(path);
    ACE_UNUSED_ARG(expected);
    success = false;
#else
    DDS::DynamicData_var use_dd;
    DDS::MemberId use_id;
    CORBA::String_var actual;
    OpenDDS::XTypes::MemberPath member_path;
    DDS::DynamicType_var dt = ts->get_type();
    if (!(dd && check_rc(member_path.resolve_string_path(dt, path), "check_string: resolve_string_path") &&
        check_rc(member_path.get_member_from_data(dd, use_dd, use_id), "check_string: get_member_from_data") &&
        check_rc(use_dd->get_string_value(actual, use_id), "check_int32: get_int32_value"))) {
      ACE_ERROR((LM_ERROR, "ERROR: check_string for %C.%C failed\n", type_name.in(), path.c_str()));
      success = false;
      return;
    }
    check_string(path, expected, actual);
#endif
  }

  template <typename DataReaderType, typename SeqType>
  bool read_single(SeqType& seq)
  {
    if (!read_i<DataReaderType>(dr, seq)) {
      ACE_ERROR((LM_ERROR, "ERROR: read_single %C failed\n", type_name.in()));
      return false;
    }
    if (seq.length() != 1) {
      ACE_ERROR((LM_ERROR, "ERROR: read_single %C expected one, but sample got %u\n",
        type_name.in(), seq.length()));
      return false;
    }
    return true;
  }

  template <typename SampleType>
  bool read(SampleType& sample)
  {
    typedef typename OpenDDS::DCPS::DDSTraits<SampleType> Traits;
    typedef typename Traits::MessageSequenceType SeqType;
    SeqType seq;
    if (!read_single<typename Traits::DataReaderType, SeqType>(seq)) {
      return false;
    }
    sample = seq[0];
    return true;
  }

  bool read_dynamic()
  {
#if OPENDDS_CONFIG_SAFETY_PROFILE
    success = false;
    return false;
#else
    DDS::DynamicDataSeq seq;
    if (!read_single<DDS::DynamicDataReader, DDS::DynamicDataSeq>(seq)) {
      return false;
    }
    // TODO: the duplicate shouldn't be required, but a direct assignment isn't
    // incrementing the reference count.
    // https://github.com/DOCGroup/ACE_TAO/issues/2037
    dd = DDS::DynamicData::_duplicate(seq[0]);
    return true;
#endif
  }

  template <typename TopicType>
  void read_kf_struct()
  {
    if (dynamic) {
      if (read_dynamic()) {
        check_int32("key_field", key_value);
      }
    } else {
      TopicType data;
      if (read(data)) {
        check_int32("key_field", key_value, data.key_field);
      }
    }
  }

  template <typename TopicType>
  void read_kf_af_struct(AdditionalFieldValue afv)
  {
    if (dynamic) {
      if (read_dynamic()) {
        check_int32("key_field", key_value);
        check_int32("additional_field", afv);
      }
    } else {
      TopicType data;
      if (read(data)) {
        check_int32("key_field", key_value, data.key_field);
        check_int32("additional_field", afv, data.additional_field);
      }
    }
  }

  template <typename TopicType>
  void read_trim_struct()
  {
    if (dynamic) {
      if (read_dynamic()) {
        check_string("trim_string", STRING_20);
      }
    } else {
      TopicType data;
      if (read(data)) {
        check_string("trim_string", STRING_20, data.trim_string);
      }
    }
  }

  template <typename TopicType>
  void read_union()
  {
    const UnionDisc expected = E_KEY;
    TopicType cpp_data;
    if (dynamic) {
      if (read_dynamic()) {
        check_int32("discriminator", expected);
      }
    } else if (read(cpp_data)) {
      check_int32("discriminator", expected, cpp_data._d());
    }
    if (!success) {
      return;
    }
    if (dynamic) {
      check_int32("key_field", key_value);
    } else {
      check_int32("key_field", key_value, cpp_data.key_field());
    }
  }
};

int ACE_TMAIN(int argc, ACE_TCHAR* argv[])
{
  std::string type;
  std::string topic_name = "";
  std::string registered_type_name = "";
  bool dynamic = false;
  bool skip_read = false;

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
    } else if (arg == ACE_TEXT("--disallow-type-coercion")) {
      disallow_type_coercion =  true;
    } else if (arg == ACE_TEXT("--ignore-member-names")) {
      ignore_member_names = true;
    } else if (arg == ACE_TEXT("--force-type-validation")) {
      force_type_validation = true;
    } else if (arg == ACE_TEXT("--dynamic-ts")) {
      dynamic = true;
    } else if (arg == ACE_TEXT("--skip-read")) {
      skip_read = true;
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
  bool success = true;
  if (type == "PlainCdrStruct") {
    get_topic<PlainCdrStruct>(success, ts, dp, topic_name, topic, registered_type_name, dynamic);
  } else if (type == "FinalStructSub") {
    get_topic<FinalStructSub>(success, ts, dp, topic_name, topic, registered_type_name, dynamic);
  } else if (type == "AppendableStruct") {
    get_topic<AppendableStruct>(success, ts, dp, topic_name, topic, registered_type_name, dynamic);
  } else if (type == "ExtendedAppendableStruct") {
    get_topic<ExtendedAppendableStruct>(success, ts, dp, topic_name, topic, registered_type_name, dynamic);
  } else if (type == "AppendableStructNoXTypes") {
    get_topic<AppendableStructNoXTypes>(success, ts, dp, topic_name, topic, registered_type_name, dynamic);
  } else if (type == "MutableStruct") {
    get_topic<MutableStruct>(success, ts, dp, topic_name, topic, registered_type_name, dynamic);
  } else if (type == "MutableUnion") {
    get_topic<MutableUnion>(success, ts, dp, topic_name, topic, registered_type_name, dynamic);
  } else if (type == "Trim20Struct") {
    get_topic<Trim20Struct>(success, ts, dp, topic_name, topic, registered_type_name, dynamic);
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

  ACE_DEBUG((LM_DEBUG, "Reader starting at %T\n"));

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
    success = wait_inconsistent_topic(ws, topic);
  } else if (status_mask == DDS::REQUESTED_INCOMPATIBLE_QOS_STATUS) {
    success = wait_requested_incompatible_qos(ws, dr);
  } else {
    success = wait_subscription_matched(ws, dr);
  }
  ws->detach_condition(condition);
  if (!success) {
    ACE_ERROR((LM_ERROR, "ERROR: Reader failed for type %C\n", type.c_str()));
    return 1;
  }

  if (!check_inconsistent_topic_status(topic) ||
      !check_requested_incompatible_qos_status(dr, type)) {
    return 1;
  }

  ReadTest rt(ts, dr, topic_name, dynamic);
  if (!expect_inconsistent_topic && !expect_incompatible_qos && !skip_read) {
    if (type == "PlainCdrStruct") {
      rt.read_kf_struct<PlainCdrStruct>();
    } else if (type == "AppendableStruct") {
      rt.read_kf_struct<AppendableStruct>();
    } else if (type == "ExtendedAppendableStruct") {
      rt.read_kf_af_struct<ExtendedAppendableStruct>(FINAL_STRUCT_AF);
    } else if (type == "AppendableStructNoXTypes") {
      rt.read_kf_struct<AppendableStructNoXTypes>();
    } else if (type == "FinalStructSub") {
      rt.read_kf_struct<FinalStructSub>();
    } else if (type == "MutableStruct") {
      rt.read_kf_af_struct<MutableStruct>(
        topic_name == "MutableBaseStructT" ? FINAL_STRUCT_AF : MUTABLE_STRUCT_AF);
    } else if (type == "MutableUnion") {
      rt.read_union<MutableUnion>();
    } else if (type == "Trim20Struct") {
      rt.read_trim_struct<Trim20Struct>();
    }
  }
  if (!rt.success) {
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

  if (!read_control(control_dr, true)) {
    ACE_ERROR((LM_ERROR, "ERROR: control read failed\n"));
    return 1;
  }

  ACE_DEBUG((LM_DEBUG, "Reader cleanup at %T\n"));

  topic = 0;
  dp->delete_contained_entities();
  dpf->delete_participant(dp);
  TheServiceParticipant->shutdown();
  return 0;
}
