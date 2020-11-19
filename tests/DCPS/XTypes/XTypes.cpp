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
bool expect_to_match = true;

int key_value = -1;

enum AdditionalFieldValue {
  FINAL_STRUCT_AF,
  APPENDABLE_STRUCT_AF,
  MUTABLE_STRUCT_AF,
  NESTED_STRUCT_AF
};

const std::string STRING_26 = "abcdefghijklmnopqrstuvwxyz";
const std::string STRING_20 = "abcdefghijklmnopqrst";

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
  FinalStructDataReader_var pdr = FinalStructDataReader::_narrow(dr);
  ::FinalStructSeq data;
  return read_struct(dr, pdr, data);
}


ReturnCode_t read_modified_final_struct(const DataReader_var& dr)
{
  ModifiedFinalStructDataReader_var pdr = ModifiedFinalStructDataReader::_narrow(dr);
  ::ModifiedFinalStructSeq data;
  return read_struct(dr, pdr, data);
}


ReturnCode_t read_appendable_struct(const DataReader_var& dr)
{
  AppendableStructDataReader_var pdr = AppendableStructDataReader::_narrow(dr);
  ::AppendableStructSeq data;
  return read_struct(dr, pdr, data);
}


ReturnCode_t read_additional_prefix_field_struct(const DataReader_var& dr)
{
  AdditionalPrefixFieldStructDataReader_var pdr = AdditionalPrefixFieldStructDataReader::_narrow(dr);
  ::AdditionalPrefixFieldStructSeq data;
  ReturnCode_t ret;
  ret = read_struct(dr, pdr, data);
  return ret;
}


ReturnCode_t read_additional_postfix_field_struct(const DataReader_var& dr)
{
  AdditionalPostfixFieldStructDataReader_var pdr = AdditionalPostfixFieldStructDataReader::_narrow(dr);
  ::AdditionalPostfixFieldStructSeq data;
  ReturnCode_t ret;
  ret = read_struct(dr, pdr, data);
  return ret;
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

ReturnCode_t read_modified_mutable_struct(const DataReader_var& dr)
{
  ModifiedMutableStructDataReader_var pdr = ModifiedMutableStructDataReader::_narrow(dr);
  ::ModifiedMutableStructSeq data;
  ReturnCode_t ret;
  ret = read_struct(dr, pdr, data);
  return ret;
}

ReturnCode_t read_mutable_union(const DataReader_var& dr)
{
  MutableUnionDataReader_var pdr = MutableUnionDataReader::_narrow(dr);
  ::MutableUnionSeq data;
  return read_union(dr, pdr, data);
}

ReturnCode_t read_modified_mutable_union(const DataReader_var& dr)
{
  ModifiedMutableUnionDataReader_var pdr = ModifiedMutableUnionDataReader::_narrow(dr);
  ::ModifiedMutableUnionSeq data;
  return read_union(dr, pdr, data);
}

ReturnCode_t read_trim20_struct(const DataReader_var& dr)
{
  Trim20StructDataReader_var pdr = Trim20StructDataReader::_narrow(dr);
  ::Trim20StructSeq data;
  return read_tryconstruct_struct(dr, pdr, data, STRING_20);
}

ReturnCode_t read_appendable_struct_with_dependency(const DataReader_var& dr)
{
  AppendableStructWithDependencyDataReader_var pdr = AppendableStructWithDependencyDataReader::_narrow(dr);
  ::AppendableStructWithDependencySeq data;
  return read_struct(dr, pdr, data);
}


void write_plain_cdr_struct(const DataWriter_var& dw)
{
  PlainCdrStructDataWriter_var pdw = PlainCdrStructDataWriter::_narrow(dw);

  PlainCdrStruct pcs;
  pcs.key = key_value;
  pcs.value = 1;
  pdw->write(pcs, HANDLE_NIL);
  if (verbose) {
    ACE_DEBUG((LM_DEBUG, "writer: PlainCdrStruct\n"));
  }
}


void write_final_struct(const DataWriter_var& dw)
{
  FinalStructDataWriter_var pdw = FinalStructDataWriter::_narrow(dw);

  FinalStruct fs;
  fs.key = key_value;
  pdw->write(fs, HANDLE_NIL);
  if (verbose) {
    ACE_DEBUG((LM_DEBUG, "writer: FinalStruct\n"));
  }
}


void write_modified_final_struct(const DataWriter_var& dw)
{
  ModifiedFinalStructDataWriter_var pdw = ModifiedFinalStructDataWriter::_narrow(dw);

  ModifiedFinalStruct mfs;
  mfs.key = key_value;
  mfs.additional_field = FINAL_STRUCT_AF;
  pdw->write(mfs, HANDLE_NIL);
  if (verbose) {
    ACE_DEBUG((LM_DEBUG, "writer: ModifiedFinalStruct\n"));
  }
}


void write_appendable_struct(const DataWriter_var& dw)
{
  AppendableStructDataWriter_var typed_dw = AppendableStructDataWriter::_narrow(dw);

  AppendableStruct as;
  as.key = key_value;
  typed_dw->write(as, HANDLE_NIL);
  if (verbose) {
    ACE_DEBUG((LM_DEBUG, "writer: AppendableStruct\n"));
  }
}


void write_additional_prefix_field_struct(const DataWriter_var& dw)
{
  AdditionalPrefixFieldStructDataWriter_var typed_dw = AdditionalPrefixFieldStructDataWriter::_narrow(dw);

  AdditionalPrefixFieldStruct apfs;
  apfs.key = key_value;
  apfs.additional_field = APPENDABLE_STRUCT_AF;
  typed_dw->write(apfs, HANDLE_NIL);
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
  typed_dw->write(apfs, HANDLE_NIL);
  if (verbose) {
    ACE_DEBUG((LM_DEBUG, "writer: AdditionalPostfixFieldStruct\n"));
  }
}


void write_mutable_struct(const DataWriter_var& dw)
{
  MutableStructDataWriter_var typed_dw = MutableStructDataWriter::_narrow(dw);

  MutableStruct ms;
  ms.key = key_value;
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
  ams.key = key_value;
  ams.additional_field = MUTABLE_STRUCT_AF;
  typed_dw->write(ams, HANDLE_NIL);
  if (verbose) {
    ACE_DEBUG((LM_DEBUG, "writer: ModifiedMutableStruct\n"));
  }
}


void write_mutable_union(const DataWriter_var& dw)
{
  MutableUnionDataWriter_var typed_dw = MutableUnionDataWriter::_narrow(dw);

  MutableUnion mu;
  mu.key(key_value);
  typed_dw->write(mu, HANDLE_NIL);
  if (verbose) {
    ACE_DEBUG((LM_DEBUG, "writer: MutableUnion\n"));
  }
}


void write_modified_mutable_union(const DataWriter_var& dw)
{
  ModifiedMutableUnionDataWriter_var typed_dw = ModifiedMutableUnionDataWriter::_narrow(dw);

  ModifiedMutableUnion mmu;
  mmu.key(key_value);
  typed_dw->write(mmu, HANDLE_NIL);
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


void write_appendable_struct_with_dependency(const DataWriter_var& dw)
{
  AppendableStructWithDependencyDataWriter_var typed_dw = AppendableStructWithDependencyDataWriter::_narrow(dw);

  AppendableStructWithDependency as;
  as.key = key_value;
  as.additional_nested_struct.additional_field = NESTED_STRUCT_AF;
  typed_dw->write(as, HANDLE_NIL);
  if (verbose) {
    ACE_DEBUG((LM_DEBUG, "writer: AppendableStructWithDependency\n"));
  }
}

template<typename T>
void get_topic(T ts, const DomainParticipant_var dp, const std::string& topic_name,
  Topic_var& topic, const std::string& registered_type_name)
{
  ts->register_type(dp, registered_type_name.c_str());
  CORBA::String_var type_name = (registered_type_name.empty() ? ts->get_type_name() : registered_type_name.c_str());
  topic = dp->create_topic(topic_name.c_str(), type_name,
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
  } else if (status.total_count != (expect_to_match ? 0 : 1)) {
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
    } else if (arg == ACE_TEXT("--key_val")) {
      if (i + 1 < argc) {
        key_value = ACE_OS::atoi(argv[++i]);
      } else {
        ACE_ERROR((LM_ERROR, "ERROR: Invalid key value argument"));
        return 1;
      }
    } else if (arg == ACE_TEXT("--expect_to_fail")) {
      expect_to_match = false;
    } else {
      ACE_ERROR((LM_ERROR, "ERROR: Invalid argument: %s\n", argv[i]));
      return 1;
    }
  }

  if (writer == reader) {
    ACE_ERROR((LM_ERROR, "ERROR: Must pass either --writer or --reader: writer %d, reader %d\n", writer, reader));
    return 1;
  }

  bool failed = false;

  Topic_var topic;
  const std::string topic_name =
    !registered_type_name.empty() ? registered_type_name + "_Topic" : type + "_Topic";

  if (type == "PlainCdrStruct") {
    PlainCdrStructTypeSupport_var ts = new PlainCdrStructTypeSupportImpl;
    get_topic(ts, dp, topic_name, topic, registered_type_name);
  } else if (type == "FinalStruct") {
    FinalStructTypeSupport_var ts = new FinalStructTypeSupportImpl;
    get_topic(ts, dp, topic_name, topic, registered_type_name);
  } else if (type == "ModifiedFinalStruct") {
    ModifiedFinalStructTypeSupport_var ts = new ModifiedFinalStructTypeSupportImpl;
    get_topic(ts, dp, topic_name, topic, registered_type_name);
  } else if (type == "AppendableStruct") {
    AppendableStructTypeSupport_var ts = new AppendableStructTypeSupportImpl;
    get_topic(ts, dp, topic_name, topic, registered_type_name);
  } else if (type == "AdditionalPrefixFieldStruct") {
    AdditionalPrefixFieldStructTypeSupport_var ts = new AdditionalPrefixFieldStructTypeSupportImpl;
    get_topic(ts, dp, topic_name, topic, registered_type_name);
  } else if (type == "AdditionalPostfixFieldStruct") {
    AdditionalPostfixFieldStructTypeSupport_var ts = new AdditionalPostfixFieldStructTypeSupportImpl;
    get_topic(ts, dp, topic_name, topic, registered_type_name);
  } else if (type == "MutableStruct") {
    MutableStructTypeSupport_var ts = new MutableStructTypeSupportImpl;
    get_topic(ts, dp, topic_name, topic, registered_type_name);
  } else if (type == "ModifiedMutableStruct") {
    ModifiedMutableStructTypeSupport_var ts = new ModifiedMutableStructTypeSupportImpl;
    get_topic(ts, dp, topic_name, topic, registered_type_name);
  } else if (type == "ModifiedIdMutableStruct") {
    ModifiedIdMutableStructTypeSupport_var ts = new ModifiedIdMutableStructTypeSupportImpl;
    get_topic(ts, dp, topic_name, topic, registered_type_name);
  } else if (type == "ModifiedTypeMutableStruct") {
    ModifiedTypeMutableStructTypeSupport_var ts = new ModifiedTypeMutableStructTypeSupportImpl;
    get_topic(ts, dp, topic_name, topic, registered_type_name);
  } else if (type == "ModifiedNameMutableStruct") {
    ModifiedNameMutableStructTypeSupport_var ts = new ModifiedNameMutableStructTypeSupportImpl;
    get_topic(ts, dp, topic_name, topic, registered_type_name);
  } else if (type == "MutableUnion") {
    MutableUnionTypeSupport_var ts = new MutableUnionTypeSupportImpl;
    get_topic(ts, dp, topic_name, topic, registered_type_name);
  } else if (type == "ModifiedMutableUnion") {
    ModifiedMutableUnionTypeSupport_var ts = new ModifiedMutableUnionTypeSupportImpl;
    get_topic(ts, dp, topic_name, topic, registered_type_name);
  } else if (type == "ModifiedDiscMutableUnion") {
    ModifiedDiscMutableUnionTypeSupport_var ts = new ModifiedDiscMutableUnionTypeSupportImpl;
    get_topic(ts, dp, topic_name, topic, registered_type_name);
  } else if (type == "ModifiedTypeMutableUnion") {
    ModifiedTypeMutableUnionTypeSupport_var ts = new ModifiedTypeMutableUnionTypeSupportImpl;
    get_topic(ts, dp, topic_name, topic, registered_type_name);
  } else if (type == "ModifiedNameMutableUnion") {
    ModifiedNameMutableUnionTypeSupport_var ts = new ModifiedNameMutableUnionTypeSupportImpl;
    get_topic(ts, dp, topic_name, topic, registered_type_name);
  } else if (type == "Trim64Struct") {
    Trim64StructTypeSupport_var ts = new Trim64StructTypeSupportImpl;
    get_topic(ts, dp, topic_name, topic, registered_type_name);
  } else if (type == "Trim20Struct") {
    Trim20StructTypeSupport_var ts = new Trim20StructTypeSupportImpl;
    get_topic(ts, dp, topic_name, topic, registered_type_name);
  } else if (type == "AppendableStructWithDependency") {
    AppendableStructWithDependencyTypeSupport_var ts = new AppendableStructWithDependencyTypeSupportImpl;
    get_topic(ts, dp, topic_name, topic, registered_type_name);
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

    if (failed) {
      ACE_ERROR((LM_ERROR, "ERROR: Writer failed for type %s\n", type.c_str()));
    } else if (expect_to_match) {
      if (type == "PlainCdrStruct") {
        write_plain_cdr_struct(dw);
      } else if (type == "FinalStruct") {
        write_final_struct(dw);
      } else if (type == "ModifiedFinalStruct") {
        write_modified_final_struct(dw);
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
      } else if (type == "Trim64Struct") {
        write_trim64_struct(dw);
      } else if (type == "AppendableStructWithDependency") {
        write_appendable_struct_with_dependency(dw);
      }
    }

    ACE_OS::sleep(ACE_Time_Value(3, 0));

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

    if (failed) {
      ACE_ERROR((LM_ERROR, "ERROR: Reader failed for type %s\n", type.c_str()));
    } else if (expect_to_match) {
      if (type == "PlainCdrStruct") {
        failed = !(read_plain_cdr_struct(dr) == RETCODE_OK);
      } else if (type == "AppendableStruct") {
        failed = !(read_appendable_struct(dr) == RETCODE_OK);
      } else if (type == "FinalStruct") {
        failed = !(read_final_struct(dr) == RETCODE_OK);
      } else if (type == "ModifiedFinalStruct") {
        failed = !(read_modified_final_struct(dr) == RETCODE_OK);
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
      } else if (type == "AppendableStructWithDependency") {
        failed = !(read_appendable_struct_with_dependency(dr) == RETCODE_OK);
      }
    }
  }

  topic = 0;
  dp->delete_contained_entities();
  dpf->delete_participant(dp);
  TheServiceParticipant->shutdown();
  return failed ? 1 : 0;
}
