#include "XTypes.h"
#include "XTypesPubTypeSupportImpl.h"


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
  FinalStructPubDataWriter_var pdw = FinalStructPubDataWriter::_narrow(dw);

  FinalStructPub fs;
  fs.key = key_value;
  pdw->write(fs, HANDLE_NIL);
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
  pdw->write(mfs, HANDLE_NIL);
  if (verbose) {
    ACE_DEBUG((LM_DEBUG, "writer: ModifiedFinalStruct\n"));
  }
}


void write_appendable_struct_no_xtypes(const DataWriter_var& dw)
{
  AppendableStructNoXTypesDataWriter_var typed_dw = AppendableStructNoXTypesDataWriter::_narrow(dw);

  AppendableStructNoXTypes as;
  as.key = key_value;
  typed_dw->write(as, HANDLE_NIL);
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

  std::string type;
  std::string registered_type_name = "";

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
    } else {
      ACE_ERROR((LM_ERROR, "ERROR: Invalid argument: %s\n", argv[i]));
      return 1;
    }
  }

  bool failed = false;

  Topic_var topic;
  const std::string topic_name =
    !registered_type_name.empty() ? registered_type_name + "_Topic" : type + "_Topic";

  if (type == "PlainCdrStruct") {
    PlainCdrStructTypeSupport_var ts = new PlainCdrStructTypeSupportImpl;
    get_topic(ts, dp, topic_name, topic, registered_type_name);
  } else if (type == "FinalStructPub") {
    FinalStructPubTypeSupport_var ts = new FinalStructPubTypeSupportImpl;
    get_topic(ts, dp, topic_name, topic, registered_type_name);
  } else if (type == "ModifiedFinalStruct") {
    ModifiedFinalStructTypeSupport_var ts = new ModifiedFinalStructTypeSupportImpl;
    get_topic(ts, dp, topic_name, topic, registered_type_name);
  } else if (type == "AppendableStructNoXTypes") {
    AppendableStructNoXTypesTypeSupport_var ts = new AppendableStructNoXTypesTypeSupportImpl;
    get_topic(ts, dp, topic_name, topic, registered_type_name);
  } else if (type == "AdditionalPrefixFieldStruct") {
    AdditionalPrefixFieldStructTypeSupport_var ts = new AdditionalPrefixFieldStructTypeSupportImpl;
    get_topic(ts, dp, topic_name, topic, registered_type_name);
  } else if (type == "AdditionalPostfixFieldStruct") {
    AdditionalPostfixFieldStructTypeSupport_var ts = new AdditionalPostfixFieldStructTypeSupportImpl;
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
  } else if (type == "AppendableStructWithDependency") {
    AppendableStructWithDependencyTypeSupport_var ts = new AppendableStructWithDependencyTypeSupportImpl;
    get_topic(ts, dp, topic_name, topic, registered_type_name);
  } else {
    ACE_ERROR((LM_ERROR, "ERROR: Type %s is not supported\n", type.c_str()));
    return 1;
  }

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
    }
  }

  ACE_OS::sleep(ACE_Time_Value(3, 0));

  topic = 0;
  dp->delete_contained_entities();
  dpf->delete_participant(dp);
  TheServiceParticipant->shutdown();
  return failed ? 1 : 0;
}
