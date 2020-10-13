#include "XTypesTypeSupportImpl.h"

#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/WaitSet.h>
#include <dds/DCPS/transport/framework/TransportSendStrategy.h>
#ifdef ACE_AS_STATIC_LIBS
#  include <dds/DCPS/RTPS/RtpsDiscovery.h>
#  include <dds/DCPS/transport/rtps_udp/RtpsUdp.h>
#endif

using namespace DDS;
using OpenDDS::DCPS::DEFAULT_STATUS_MASK;

bool verbose = false;
bool expect_to_fail = false;

enum KeyValue
{
  PROPERTY_1_KEY,
  PROPERTY_2_KEY,
  APPENDABLE_STRUCT_KEY,
  ADDITIONAL_PREFIX_FIELD_STRUCT_KEY,
  ADDITIONAL_POSTFIX_FIELD_STRUCT_KEY,
  MUTABLE_STRUCT_KEY,
  MODIFIED_MUTABLE_STRUCT_KEY,
  MUTABLE_UNION_KEY,
  MODIFIED_MUTABLE_UNION_KEY
};

enum AdditionalFieldValue
{
  ADDITIONAL_PREFIX_FIELD_STRUCT_AF,
  ADDITIONAL_POSTFIX_FIELD_STRUCT_AF,
  MUTABLE_STRUCT_AF,
  MODIFIED_MUTABLE_STRUCT_AF,
  MODIFIED_MUTABLE_UNION_AF
};

const std::string STRING_26 = "abcdefghijklmnopqrstuvwxyz";
const std::string STRING_20 = "abcdefghijklmnopqrst";

template<typename T1>
ReturnCode_t check_additional_field_value(const T1& data, AdditionalFieldValue expected_additional_field_value)
{
  ReturnCode_t ret = RETCODE_OK;
  if (data[0].additional_field != expected_additional_field_value) {
    ACE_DEBUG((LM_DEBUG, "reader: expected key value: %d, received: %d\n", expected_additional_field_value, data[0].additional_field));
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
  const Duration_t max_wait = { 5, 0 };
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
ReturnCode_t read_struct(const DataReader_var& dr, const T1& pdr, T2& data, KeyValue expected_key_value)
{
  ReturnCode_t ret = RETCODE_OK;
  if ((ret = read_i(dr, pdr, data)) == RETCODE_OK) {
    if (data.length() != 1) {
      ACE_ERROR((LM_ERROR, "reader: unexpected data length: %d", data.length()));
      ret = RETCODE_ERROR;
    } else if (data[0].key != expected_key_value) {
      ACE_ERROR((LM_ERROR, "reader: expected key value: %d, received: %d\n", expected_key_value, data[0].key));
      ret = RETCODE_ERROR;
    } else if (verbose) {
      ACE_DEBUG((LM_DEBUG, "reader: %d\n", data[0].key));
    }
  } else {
    ACE_ERROR((LM_ERROR, "ERROR: Reader: read_i returned %d\n", ret));
  }

  return ret;
}

template<typename T1, typename T2, typename T3>
ReturnCode_t read_union(const DataReader_var& dr, const T1& pdr,
  T2& data, T3 expected_value)
{
  ReturnCode_t ret = RETCODE_OK;
  if ((ret = read_i(dr, pdr, data)) == RETCODE_OK) {
    if (data.length() != 1) {
      ACE_ERROR((LM_ERROR, "reader: unexpected data length: %d", data.length()));
      ret = RETCODE_ERROR;
    } else {
      switch (data[0]._d()) {
      case E_KEY:
        if (data[0].key() != expected_value) {
          ACE_ERROR((LM_ERROR, "reader: expected union key value: %d, received: %d\n",
            expected_value, data[0].key()));
          ret = RETCODE_ERROR;
        }
        if (verbose) {
          ACE_DEBUG((LM_DEBUG, "reader: union key %d\n", data[0].key()));
        }
        break;
      case E_ADDITIONAL_FIELD:
        if (data[0].additional_field() != expected_value) {
          ACE_ERROR((LM_ERROR, "reader: expected additional_field value: %d, received: %d\n",
            expected_value, data[0].additional_field()));
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

ReturnCode_t read_property_1(const DataReader_var& dr)
{
  Property_1DataReader_var pdr = Property_1DataReader::_narrow(dr);
  ::Property_1Seq data;
  return read_struct(dr, pdr, data, PROPERTY_1_KEY);
}


ReturnCode_t read_property_2(const DataReader_var& dr)
{
  Property_2DataReader_var pdr = Property_2DataReader::_narrow(dr);
  ::Property_2Seq data;
  return read_struct(dr, pdr, data, PROPERTY_2_KEY);
}


ReturnCode_t read_appendable_struct(const DataReader_var& dr)
{
  AppendableStructDataReader_var pdr = AppendableStructDataReader::_narrow(dr);
  ::AppendableStructSeq data;
  return read_struct(dr, pdr, data, APPENDABLE_STRUCT_KEY);
}


ReturnCode_t read_additional_prefix_field_struct(const DataReader_var& dr)
{
  AdditionalPrefixFieldStructDataReader_var pdr = AdditionalPrefixFieldStructDataReader::_narrow(dr);
  ::AdditionalPrefixFieldStructSeq data;
  ReturnCode_t ret;
  ret = read_struct(dr, pdr, data, ADDITIONAL_PREFIX_FIELD_STRUCT_KEY);
  if (ret == RETCODE_OK) {
    ret = check_additional_field_value(data, ADDITIONAL_PREFIX_FIELD_STRUCT_AF);
  }
  return ret;
}


ReturnCode_t read_additional_postfix_field_struct(const DataReader_var& dr)
{
  AdditionalPostfixFieldStructDataReader_var pdr = AdditionalPostfixFieldStructDataReader::_narrow(dr);
  ::AdditionalPostfixFieldStructSeq data;
  ReturnCode_t ret;
  ret = read_struct(dr, pdr, data, ADDITIONAL_POSTFIX_FIELD_STRUCT_KEY);
  if (ret == RETCODE_OK) {
    ret = check_additional_field_value(data, ADDITIONAL_POSTFIX_FIELD_STRUCT_AF);
  }
  return ret;
}

ReturnCode_t read_mutable_struct(const DataReader_var& dr)
{
  MutableStructDataReader_var pdr = MutableStructDataReader::_narrow(dr);
  ::MutableStructSeq data;
  ReturnCode_t ret;
  ret = read_struct(dr, pdr, data, MUTABLE_STRUCT_KEY);
  if (ret == RETCODE_OK) {
    ret = check_additional_field_value(data, MUTABLE_STRUCT_AF);
  }
  return ret;
}

ReturnCode_t read_modified_mutable_struct(const DataReader_var& dr)
{
  ModifiedMutableStructDataReader_var pdr = ModifiedMutableStructDataReader::_narrow(dr);
  ::ModifiedMutableStructSeq data;
  ReturnCode_t ret;
  ret = read_struct(dr, pdr, data, MODIFIED_MUTABLE_STRUCT_KEY);
  if (ret == RETCODE_OK) {
    ret = check_additional_field_value(data, MODIFIED_MUTABLE_STRUCT_AF);
  }
  return ret;
}

ReturnCode_t read_mutable_union(const DataReader_var& dr)
{
  MutableUnionDataReader_var pdr = MutableUnionDataReader::_narrow(dr);
  ::MutableUnionSeq data;
  return read_union(dr, pdr, data, MUTABLE_UNION_KEY);
}

ReturnCode_t read_modified_mutable_union(const DataReader_var& dr)
{
  ModifiedMutableUnionDataReader_var pdr = ModifiedMutableUnionDataReader::_narrow(dr);
  ::ModifiedMutableUnionSeq data;
  return read_union(dr, pdr, data, MODIFIED_MUTABLE_UNION_AF);
}

ReturnCode_t read_trim20_struct(const DataReader_var& dr)
{
  Trim20StructDataReader_var pdr = Trim20StructDataReader::_narrow(dr);
  ::Trim20StructSeq data;
  return read_tryconstruct_struct(dr, pdr, data, STRING_20);
}


void write_property_1(const DataWriter_var& dw)
{
  Property_1DataWriter_var pdw = Property_1DataWriter::_narrow(dw);

  Property_1 p;
  p.key = PROPERTY_1_KEY;
  p.value = 1;
  pdw->write(p, HANDLE_NIL);
  if (verbose) {
    ACE_DEBUG((LM_DEBUG, "writer: Property_1\n"));
  }
}


void write_property_2(const DataWriter_var& dw)
{
  Property_2DataWriter_var pdw2 = Property_2DataWriter::_narrow(dw);

  Property_2 p2;
  p2.key = PROPERTY_2_KEY;
  p2.value = "Test";
  pdw2->write(p2, HANDLE_NIL);
  if (verbose) {
    ACE_DEBUG((LM_DEBUG, "writer: Property_2\n"));
  }
}


void write_appendable_struct(const DataWriter_var& dw)
{
  AppendableStructDataWriter_var typed_dw = AppendableStructDataWriter::_narrow(dw);

  AppendableStruct as;
  as.key = APPENDABLE_STRUCT_KEY;
  typed_dw->write(as, HANDLE_NIL);
  if (verbose) {
    ACE_DEBUG((LM_DEBUG, "writer: AppendableStruct\n"));
  }
}


void write_additional_prefix_field_struct(const DataWriter_var& dw)
{
  AdditionalPrefixFieldStructDataWriter_var typed_dw = AdditionalPrefixFieldStructDataWriter::_narrow(dw);

  AdditionalPrefixFieldStruct apfs;
  apfs.key = ADDITIONAL_PREFIX_FIELD_STRUCT_KEY;
  apfs.additional_field = ADDITIONAL_PREFIX_FIELD_STRUCT_AF;
  typed_dw->write(apfs, HANDLE_NIL);
  if (verbose) {
    ACE_DEBUG((LM_DEBUG, "writer: AdditionalPrefixFieldStruct\n"));
  }
}


void write_additional_postfix_field_struct(const DataWriter_var& dw)
{
  AdditionalPostfixFieldStructDataWriter_var typed_dw = AdditionalPostfixFieldStructDataWriter::_narrow(dw);

  AdditionalPostfixFieldStruct apfs;
  apfs.key = ADDITIONAL_POSTFIX_FIELD_STRUCT_KEY;
  apfs.additional_field = ADDITIONAL_POSTFIX_FIELD_STRUCT_AF;
  typed_dw->write(apfs, HANDLE_NIL);
  if (verbose) {
    ACE_DEBUG((LM_DEBUG, "writer: AdditionalPostfixFieldStruct\n"));
  }
}


void write_mutable_struct(const DataWriter_var& dw)
{
  MutableStructDataWriter_var typed_dw = MutableStructDataWriter::_narrow(dw);

  MutableStruct ms;
  ms.key = MUTABLE_STRUCT_KEY;
  ms.additional_field = MUTABLE_STRUCT_AF;
  typed_dw->write(ms, HANDLE_NIL);
  if (verbose) {
    ACE_DEBUG((LM_DEBUG, "writer: MutableStruct\n"));
  }
}


void write_modified_mutable_struct(const DataWriter_var& dw)
{
  ModifiedMutableStructDataWriter_var typed_dw = ModifiedMutableStructDataWriter::_narrow(dw);

  ModifiedMutableStruct ams;
  ams.key = MODIFIED_MUTABLE_STRUCT_KEY;
  ams.additional_field = MODIFIED_MUTABLE_STRUCT_AF;
  typed_dw->write(ams, HANDLE_NIL);
  if (verbose) {
    ACE_DEBUG((LM_DEBUG, "writer: ModifiedMutableStruct\n"));
  }
}


void write_mutable_union(const DataWriter_var& dw)
{
  MutableUnionDataWriter_var typed_dw = MutableUnionDataWriter::_narrow(dw);

  MutableUnion mu;
  mu.key(MUTABLE_UNION_KEY);
  typed_dw->write(mu, HANDLE_NIL);
  if (verbose) {
    ACE_DEBUG((LM_DEBUG, "writer: MutableUnion\n"));
  }
}


void write_modified_mutable_union(const DataWriter_var& dw)
{
  ModifiedMutableUnionDataWriter_var typed_dw = ModifiedMutableUnionDataWriter::_narrow(dw);

  ModifiedMutableUnion amu;
  amu.additional_field(MODIFIED_MUTABLE_UNION_AF);
  typed_dw->write(amu, HANDLE_NIL);
  if (verbose) {
    ACE_DEBUG((LM_DEBUG, "writer: ModifiedMutableUnion\n"));
  }
}


void write_trim64_struct(const DataWriter_var& dw)
{
  Trim64StructDataWriter_var typed_dw = Trim64StructDataWriter::_narrow(dw);

  Trim64Struct tcs;
  tcs.trim_string = STRING_26.c_str();
  typed_dw->write(tcs, HANDLE_NIL);
  if (verbose) {
    ACE_DEBUG((LM_DEBUG, "writer: Trim64Struct\n"));
  }
}


template<typename T>
void get_topic(T ts, const DomainParticipant_var dp, const char* topic_name,
  Topic_var& topic, const std::string& registered_type_name)
{
  ts->register_type(dp, registered_type_name.c_str());
  CORBA::String_var type_name = (registered_type_name.empty() ? ts->get_type_name() : registered_type_name.c_str());
  topic = dp->create_topic(topic_name, type_name,
    TOPIC_QOS_DEFAULT, 0, DEFAULT_STATUS_MASK);
}


bool check_inconsistent_topic_status(Topic_var topic)
{
  DDS::InconsistentTopicStatus status;
  DDS::ReturnCode_t retcode;

  retcode = topic->get_inconsistent_topic_status(status);
  if (retcode != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, "ERROR: get_inconsistent_topic_status failed\n"));
    return false;
  } else if (status.total_count != (expect_to_fail ? 1 : 0)) {
    ACE_ERROR((LM_ERROR, "ERROR: inconsistent topic count is %d\n", status.total_count));
    return false;
  }
  return true;
}


int ACE_TMAIN(int argc, ACE_TCHAR* argv[])
{
  DomainParticipantFactory_var dpf = TheParticipantFactoryWithArgs(argc, argv);
  DomainParticipant_var dp = dpf->create_participant(23,
    PARTICIPANT_QOS_DEFAULT, 0, DEFAULT_STATUS_MASK);

  bool writer = false;
  bool reader = false;
  std::string type;
  std::string registered_type_name = "";

  for (int i = 1; i < argc; ++i) {
    ACE_TString arg(argv[i]);
    if (arg == ACE_TEXT("--verbose")) {
      verbose = true;
    } else if (arg == ACE_TEXT("--writer")) {
      writer = true;
    } else if (arg == ACE_TEXT("--reader")) {
      reader = true;
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
    } else if (arg == ACE_TEXT("--expect_to_fail")) {
      expect_to_fail = true;
    } else {
      ACE_ERROR((LM_ERROR, "ERROR: Invalid argument: %s\n", argv[i]));
      return 1;
    }
  }

  bool failed = false;

  Topic_var topic;

  if (type == "Property_1") {
    Property_1TypeSupport_var ts = new Property_1TypeSupportImpl;
    get_topic(ts, dp, "Property_1_Topic", topic, registered_type_name);
  } else if (type == "Property_2") {
    Property_2TypeSupport_var ts = new Property_2TypeSupportImpl;
    get_topic(ts, dp, "Property_2_Topic", topic, registered_type_name);
  } else if (type == "AppendableStruct") {
    AppendableStructTypeSupport_var ts = new AppendableStructTypeSupportImpl;
    get_topic(ts, dp, "AppendableStruct_Topic", topic, registered_type_name);
  } else if (type == "AdditionalPrefixFieldStruct") {
    AdditionalPrefixFieldStructTypeSupport_var ts = new AdditionalPrefixFieldStructTypeSupportImpl;
    get_topic(ts, dp, "AppendableStruct_Topic", topic, registered_type_name);
  } else if (type == "AdditionalPostfixFieldStruct") {
    AdditionalPostfixFieldStructTypeSupport_var ts = new AdditionalPostfixFieldStructTypeSupportImpl;
    get_topic(ts, dp, "AppendableStruct_Topic", topic, registered_type_name);
  } else if (type == "MutableStruct") {
    MutableStructTypeSupport_var ts = new MutableStructTypeSupportImpl;
    get_topic(ts, dp, "MutableStruct_Topic", topic, registered_type_name);
  } else if (type == "ModifiedMutableStruct") {
    ModifiedMutableStructTypeSupport_var ts = new ModifiedMutableStructTypeSupportImpl;
    get_topic(ts, dp, "MutableStruct_Topic", topic, registered_type_name);
  } else if (type == "MutableUnion") {
    MutableUnionTypeSupport_var ts = new MutableUnionTypeSupportImpl;
    get_topic(ts, dp, "MutableUnion_Topic", topic, registered_type_name);
  } else if (type == "ModifiedMutableUnion") {
    ModifiedMutableUnionTypeSupport_var ts = new ModifiedMutableUnionTypeSupportImpl;
    get_topic(ts, dp, "MutableUnion_Topic", topic, registered_type_name);
  } else if (type == "Trim64Struct") {
    Trim64StructTypeSupport_var ts = new Trim64StructTypeSupportImpl;
    get_topic(ts, dp, "Tryconstruct_Topic", topic, registered_type_name);
  } else if (type == "Trim20Struct") {
    Trim20StructTypeSupport_var ts = new Trim20StructTypeSupportImpl;
    get_topic(ts, dp, "Tryconstruct_Topic", topic, registered_type_name);
  } else if (type == "FinalStruct") {
    FinalStructTypeSupport_var ts = new FinalStructTypeSupportImpl;
    get_topic(ts, dp, "FinalStruct_Topic", topic, registered_type_name);
  } else if (type == "ModifiedFinalStruct") {
    ModifiedFinalStructTypeSupport_var ts = new ModifiedFinalStructTypeSupportImpl;
    get_topic(ts, dp, "FinalStruct_Topic", topic, registered_type_name);
  } else {
    ACE_ERROR((LM_ERROR, "ERROR: Type %s is not supported\n", type.c_str()));
    return 1;
  }

  if (writer) {
    Publisher_var pub = dp->create_publisher(PUBLISHER_QOS_DEFAULT, 0,
                                             DEFAULT_STATUS_MASK);
    DataWriterQos dw_qos;
    pub->get_default_datawriter_qos(dw_qos);
    dw_qos.durability.kind = TRANSIENT_LOCAL_DURABILITY_QOS;

    DataWriter_var dw = pub->create_datawriter(topic, dw_qos, 0,
      DEFAULT_STATUS_MASK);

    ACE_OS::sleep(ACE_Time_Value(1, 0));

    failed = !check_inconsistent_topic_status(topic);

    if (!failed && !expect_to_fail) {
      if (type == "Property_1") {
        write_property_1(dw);
      } else if (type == "Property_2") {
        write_property_2(dw);
      } else if (type == "AppendableStruct") {
        write_appendable_struct(dw);
      } else if (type == "AdditionalPrefixFieldStruct") {
        write_additional_prefix_field_struct(dw);
      } else if (type == "AdditionalPostfixFieldStruct") {
        write_additional_postfix_field_struct(dw);
      } else if (type == "MutableStruct") {
        write_mutable_struct(dw);
      } else if (type == "ModifiedMutableStruct") {
        write_modified_mutable_struct(dw);
      } else if (type == "MutableUnion") {
        write_mutable_union(dw);
      } else if (type == "ModifiedMutableUnion") {
        write_modified_mutable_union(dw);
      } else if (type == "ModifiedMutableUnion") {
        write_modified_mutable_union(dw);
      } else if (type == "Trim64Struct") {
        write_trim64_struct(dw);
      } else {
        ACE_ERROR((LM_ERROR, "ERROR: Type %s is not supported\n", type.c_str()));
        failed = true;
      }
    }

    ACE_OS::sleep(ACE_Time_Value(0, 5000 * 1000));

    if (failed) {
      ACE_ERROR((LM_ERROR, "ERROR: Writer failed for type %s\n", type.c_str()));
    }
  } else if (reader) {
    ACE_DEBUG((LM_DEBUG, "Reader starting at %T\n"));

    Subscriber_var sub = dp->create_subscriber(SUBSCRIBER_QOS_DEFAULT, 0,
                                               DEFAULT_STATUS_MASK);
    DataReaderQos dr_qos;
    sub->get_default_datareader_qos(dr_qos);
    dr_qos.reliability.kind = RELIABLE_RELIABILITY_QOS;
    dr_qos.durability.kind = TRANSIENT_LOCAL_DURABILITY_QOS;

    DataReader_var dr = sub->create_datareader(topic, dr_qos, 0,
      DEFAULT_STATUS_MASK);

    ACE_OS::sleep(ACE_Time_Value(1, 0));

    failed = !check_inconsistent_topic_status(topic);

    if (!failed && !expect_to_fail) {
      if (type == "Property_1") {
        failed = !(read_property_1(dr) == RETCODE_OK);
      } else if (type == "Property_2") {
        failed = !(read_property_2(dr) == RETCODE_OK);
      } else if (type == "AppendableStruct") {
        failed = !(read_appendable_struct(dr) == RETCODE_OK);
      } else if (type == "AdditionalPrefixFieldStruct") {
        failed = !(read_additional_prefix_field_struct(dr) == RETCODE_OK);
      } else if (type == "AdditionalPostfixFieldStruct") {
        failed = !(read_additional_postfix_field_struct(dr) == RETCODE_OK);
      } else if (type == "MutableStruct") {
        failed = !(read_mutable_struct(dr) == RETCODE_OK);
      } else if (type == "ModifiedMutableStruct") {
        failed = !(read_modified_mutable_struct(dr) == RETCODE_OK);
      } else if (type == "MutableUnion") {
        failed = !(read_mutable_union(dr) == RETCODE_OK);
      } else if (type == "ModifiedMutableUnion") {
        failed = !(read_modified_mutable_union(dr) == RETCODE_OK);
      } else if (type == "Trim20Struct") {
        failed = !(read_trim20_struct(dr) == RETCODE_OK);
      } else {
        ACE_ERROR((LM_ERROR, "ERROR: Type %s is not supported\n", type.c_str()));
        failed = true;
      }
    }

    if (failed) {
      ACE_ERROR((LM_ERROR, "ERROR: Reader failed for type %s\n", type.c_str()));
    }
  } else {
    ACE_ERROR((LM_ERROR, "ERROR: Must pass either --writer or --reader\n"));
    failed = true;
  }

  topic = 0;
  dp->delete_contained_entities();
  dpf->delete_participant(dp);
  TheServiceParticipant->shutdown();
  return failed ? 1 : 0;
}
