/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "rapidjson_generator.h"

#include "be_extern.h"

#include <dds/DCPS/Definitions.h>

#include <utl_identifier.h>

using namespace AstTypeClassification;

void rapidjson_generator::gen_includes()
{
  be_global->add_include("dds/DCPS/RapidJsonWrapper.h", BE_GlobalData::STREAM_H);
  be_global->add_include("<sstream>", BE_GlobalData::STREAM_CPP);
}

bool rapidjson_generator::gen_enum(AST_Enum*, UTL_ScopedName*,
                            const std::vector<AST_EnumVal*>&, const char*)
{
  return true;
}

namespace {

  std::string getRapidJsonType(AST_Type* type,
                        AST_PredefinedType::PredefinedType* pt = 0)
  {
    const Classification cls = classify(type);
    if (cls & (CL_ENUM | CL_STRING)) return "String";
    if (cls & (CL_STRUCTURE | CL_UNION)) return "Object";
    if (cls & (CL_ARRAY | CL_SEQUENCE)) return "Array";
    if (cls & CL_PRIMITIVE) {
      AST_Type* actual = resolveActualType(type);
      AST_PredefinedType* p = dynamic_cast<AST_PredefinedType*>(actual);
      if (pt) *pt = p->pt();
      switch (p->pt()) {
      case AST_PredefinedType::PT_char:
      case AST_PredefinedType::PT_wchar:
        return "String";
      case AST_PredefinedType::PT_boolean:
        return "Bool";
#if OPENDDS_HAS_EXPLICIT_INTS
      case AST_PredefinedType::PT_int8:
#endif
      case AST_PredefinedType::PT_short:
      case AST_PredefinedType::PT_long:
        return "Int";
      case AST_PredefinedType::PT_octet:
#if OPENDDS_HAS_EXPLICIT_INTS
      case AST_PredefinedType::PT_uint8:
#endif
      case AST_PredefinedType::PT_ushort:
      case AST_PredefinedType::PT_ulong:
        return "Uint";
      case AST_PredefinedType::PT_longlong:
        return "Int64";
      case AST_PredefinedType::PT_ulonglong:
        return "Uint64";
      case AST_PredefinedType::PT_float:
        return "Float";
      case AST_PredefinedType::PT_double:
      case AST_PredefinedType::PT_longdouble:
        return "Double";
      case AST_PredefinedType::PT_any:
      case AST_PredefinedType::PT_object:
      case AST_PredefinedType::PT_value:
      case AST_PredefinedType::PT_abstract:
      case AST_PredefinedType::PT_void:
      case AST_PredefinedType::PT_pseudo:
        be_util::misc_error_and_abort("Unsupported type in getRapidJsonType");
      }
    }
    be_util::misc_error_and_abort("Unhandled type in getRapidJsonType");
    return "";
  }

  bool builtInSeq(AST_Type* type)
  {
    const std::string name = scoped(type->name());
    static const char PRE[] = " ::CORBA::";
    if (std::strncmp(name.c_str(), PRE, sizeof(PRE) - 1)) return false;
    return name.rfind("Seq") == name.size() - 3;
  }

  void gen_copyto(const char* tgt, const char* alloc, const char* src, AST_Type* type,
                  const char* prop, bool prop_index = false, bool push_back = false, std::ostream& strm = be_global->impl_)
  {
    const Classification cls = classify(type);
    AST_PredefinedType::PredefinedType pt = AST_PredefinedType::PT_void;
    const std::string rapidJsonType = getRapidJsonType(type, &pt),
      tgt_str = ((prop_index && !push_back) ? (std::string(tgt) + "[" + prop + "]") : std::string(tgt)),
      op_pre = (prop_index ? (push_back ? std::string(".PushBack(rapidjson::Value(") : std::string(" = rapidjson::Value(")) : (std::string(".AddMember(") + prop + std::string(", rapidjson::Value("))),
      op_post = ((prop_index && !push_back) ? (std::string(").Move()")) : std::string(").Move(), ") + alloc + std::string(")")),
      propVal = (prop_index ? (push_back ? (std::string(tgt) + ".PushBack(rapidjson::Value(0).Move(), " + alloc + ")[" + prop + "]") : (std::string(tgt) + "[" + prop + "]")) : (std::string(tgt) + std::string(".AddMember(") + prop + std::string(", rapidjson::Value(0).Move(), ") + alloc + std::string(")[") + prop + std::string("]")));

    if (cls & CL_SCALAR) {
      std::string prefix, suffix;

      if (cls & CL_ENUM) {
        const std::string underscores =
          dds_generator::scoped_helper(type->name(), "_"),
          array = "gen_" + underscores + "_names";
        strm <<
          "  {\n"
          "    const size_t index = static_cast<size_t>(" << src << ");\n"
          "    std::string str(index >= " << array << "_size ? \"<<invalid>>\" : " << array << "[index]);\n" <<
          "    " << tgt_str << op_pre << "str.c_str(), " << alloc << op_post << ";\n"
          "  }\n";
        return;

      } else if (cls & CL_STRING) {
        if (!std::strstr(src, "()")) {
          suffix = ".in()";
        }
        if (cls & CL_WIDE) {
          strm <<
            "  {\n"
            "    rapidjson::GenericStringStream<rapidjson::UTF16<> > ins(" << src << suffix << ");\n"
            "    rapidjson::GenericStringBuffer<rapidjson::UTF8<> > outs;\n"
            "    while (ins.Peek() != '\\0') {\n"
            "      rapidjson::Transcoder<rapidjson::UTF16<>, rapidjson::UTF8<> >::Transcode(ins, outs);\n"
            "    }\n"
            "    " << tgt_str << op_pre << "outs.GetString(), " << alloc << op_post << ";\n"
            "  }\n";
          return;
        } else {
          strm << "  " << tgt_str << op_pre << src << suffix << ", " << alloc << op_post << ";\n";
          return;
        }

      } else {
        switch (pt) {
        case AST_PredefinedType::PT_char:
          strm <<
            "  {\n"
            "    const char str[] = {" << src << ", 0};\n"
            "    " << tgt_str << op_pre << "&str[0], " << alloc << op_post << ";\n"
            "  }\n";
          return;

        case AST_PredefinedType::PT_wchar:
          strm <<
            "  {\n"
            "    const uint16_t str[] = {" << src << ", 0};\n"
            "    rapidjson::GenericStringStream<rapidjson::UTF16<> > ins(&str[0]);\n"
            "    rapidjson::GenericStringBuffer<rapidjson::UTF8<> > outs;\n"
            "    rapidjson::Transcoder<rapidjson::UTF16<>, rapidjson::UTF8<> >::Transcode(ins, outs);\n"
            "    " << tgt_str << op_pre << "outs.GetString(), " << alloc << op_post << ";\n"
            "  }\n";
          return;

        case AST_PredefinedType::PT_boolean:
        case AST_PredefinedType::PT_ulong:
        case AST_PredefinedType::PT_long:
        case AST_PredefinedType::PT_float:
          // prefix and suffix not needed
          break;

#if OPENDDS_HAS_EXPLICIT_INTS
        case AST_PredefinedType::PT_int8:
#endif
        case AST_PredefinedType::PT_short:
          prefix = "static_cast<int>(";
          suffix = ")";
          break;

        case AST_PredefinedType::PT_octet:
#if OPENDDS_HAS_EXPLICIT_INTS
        case AST_PredefinedType::PT_uint8:
#endif
        case AST_PredefinedType::PT_ushort:
          prefix = "static_cast<unsigned int>(";
          suffix = ")";
          break;

        case AST_PredefinedType::PT_longlong:
          prefix = "static_cast<int64_t>(";
          suffix = ")";
          break;

        case AST_PredefinedType::PT_ulonglong:
          prefix = "static_cast<uint64_t>(";
          suffix = ")";
          break;

        case AST_PredefinedType::PT_double:
        case AST_PredefinedType::PT_longdouble:
          prefix = "static_cast<double>(";
          suffix = ")";
          break;

        case AST_PredefinedType::PT_any:
        case AST_PredefinedType::PT_object:
        case AST_PredefinedType::PT_value:
        case AST_PredefinedType::PT_abstract:
        case AST_PredefinedType::PT_void:
        case AST_PredefinedType::PT_pseudo:
          be_util::misc_error_and_abort("Unsupported predefined type in gen_copyto");
        }
      }

      strm << "  " << tgt_str << op_pre << prefix << src << suffix << op_post << ";\n";

    } else if ((cls & CL_SEQUENCE) && builtInSeq(type)) {
      AST_Type* real_type = resolveActualType(type);
      AST_Sequence* seq = dynamic_cast<AST_Sequence*>(real_type);
      AST_Type* elem = seq->base_type();
      strm <<
        "  {\n"
        "    rapidjson::Value& valseq = " << propVal << ";\n"
        "    valseq.SetArray();\n"
        "    valseq.GetArray().Reserve(" << src << ".length(), " << alloc << ");\n"
        "    for (CORBA::ULong j = 0; j < " << src << ".length(); ++j) {\n";
      gen_copyto("valseq", alloc, (std::string(src) + "[j]").c_str(), elem, "j", true, true);
      strm <<
        "    }\n"
        "  }\n";

    } else { // struct, sequence, etc.
      strm <<
        "  {\n"
        "    rapidjson::Value& valzone = " << propVal << ".SetObject();\n"
        "    " << "copyToRapidJson(" << src << ", valzone, " << alloc << ");\n"
        "  }\n";
    }
  }

  void gen_copyfrom(const char* tgt, const char* src, AST_Type* type,
                    const char* prop, bool prop_index = false, bool fun_assign = false, std::ostream& strm = be_global->impl_)
  {
    const Classification cls = classify(type);
    AST_PredefinedType::PredefinedType pt = AST_PredefinedType::PT_void;
    const std::string rapidJsonType = getRapidJsonType(type, &pt);
    const std::string propName = prop_index ? std::string(tgt) + "[" + prop + "]" : std::string(tgt) + "." + prop,
      assign_prefix = fun_assign ? "(" : " = ",
      assign_suffix = fun_assign ? ")" : "";

    if (cls & CL_SCALAR) {

      std::string ip; // indent prefix

      if (prop_index) {
        strm <<
          "  {\n"
          "    if (" << src << ".IsArray()) {\n"
          "      const rapidjson::Value& val = " << src << ".GetArray()[" << prop << "];\n";
        ip = std::string(8, ' ');
      } else {
        strm <<
          "  {\n"
          "    rapidjson::Value::ConstMemberIterator it = " << src << ".FindMember(\"" << prop << "\");\n"
          "    if (it != " << src << ".MemberEnd()) {\n"
          "      const rapidjson::Value& val = it->value;\n";
        ip = std::string(8, ' ');
      }

      const std::string underscores =
        dds_generator::scoped_helper(type->name(), "_"),
        temp_type = scoped(type->name()),
        temp_name = "temp_" + underscores;
      const bool octet_type = (pt == AST_PredefinedType::PT_octet);

      if (cls & CL_ENUM) {
        const std::string underscores =
          dds_generator::scoped_helper(type->name(), "_"),
          array = "gen_" + underscores + "_names";
        strm <<
          ip << "if (val.IsNumber()) {\n" <<
          ip << "  " << propName << assign_prefix << "static_cast<" << scoped(type->name()) << ">(val.GetInt())" << assign_suffix << ";\n" <<
          ip << "}\n" <<
          ip << "if (val.IsString()) {\n" <<
          ip << "  std::string ss(val.GetString());\n" <<
          ip << "  for (uint32_t i = 0; i < " << array << "_size; ++i) {\n" <<
          ip << "    if (ss == " << array << "[i]) {\n" <<
          ip << "      " << propName << assign_prefix << "static_cast<" << scoped(type->name()) << ">(i)" << assign_suffix << ";\n" <<
          ip << "      break;\n" <<
          ip << "    }\n" <<
          ip << "  }\n" <<
          ip << "}\n";
      } else if (cls & CL_STRING) {
        strm <<
          ip << "if (val.IsString()) {\n";
        if (cls & CL_WIDE) {
          strm <<
            ip << "  rapidjson::GenericStringStream<rapidjson::UTF8<> > ins(val.GetString());\n" <<
            ip << "  rapidjson::GenericStringBuffer<rapidjson::UTF16<> > outs;\n" <<
            ip << "  while (ins.Peek() != '\\0') {\n" <<
            ip << "    rapidjson::Transcoder<rapidjson::UTF8<>, rapidjson::UTF16<> >::Transcode(ins, outs);\n" <<
            ip << "  }\n" <<
            ip << "  " << propName << assign_prefix << "outs.GetString()" << assign_suffix << ";\n";
        } else {
          strm <<
            ip << "  " << propName << assign_prefix << "val.GetString()" << assign_suffix << ";\n";
        }
        strm <<
          ip << "}\n";
      } else {
        switch (pt) {
        case AST_PredefinedType::PT_char:
          strm <<
            ip << "if (val.IsString()) {\n" <<
            ip << "  " << propName << assign_prefix << "val.GetString()[0]" << assign_suffix << ";\n" <<
            ip << "}\n";
          break;

        case AST_PredefinedType::PT_wchar:
          strm <<
            ip << "if (val.IsString()) {\n" <<
            ip << "  rapidjson::GenericStringStream<rapidjson::UTF8<> > ins(val.GetString());\n" <<
            ip << "  rapidjson::GenericStringBuffer<rapidjson::UTF16<> > outs;\n" <<
            ip << "  rapidjson::Transcoder<rapidjson::UTF8<>, rapidjson::UTF16<> >::Transcode(ins, outs);\n" <<
            ip << "  " << propName << assign_prefix << "outs.GetString()[0]" << assign_suffix << ";\n" <<
            ip << "}\n";
          break;

        case AST_PredefinedType::PT_longlong:
        case AST_PredefinedType::PT_ulonglong:
        case AST_PredefinedType::PT_octet:
#if OPENDDS_HAS_EXPLICIT_INTS
        case AST_PredefinedType::PT_uint8:
        case AST_PredefinedType::PT_int8:
#endif
        case AST_PredefinedType::PT_ushort:
        case AST_PredefinedType::PT_short:
        case AST_PredefinedType::PT_ulong:
        case AST_PredefinedType::PT_long:
          strm <<
            ip << "if (val.IsString()) {\n" <<
            ip << "  std::string ss(val.GetString());\n" <<
            ip << "  std::istringstream iss(ss);\n" <<
            ip << "  " << (octet_type ? "uint16_t" : temp_type.c_str()) << " " << temp_name << ";\n" <<
            ip << "  if (ss.find(\"0x\") != std::string::npos) {\n" <<
            ip << "    iss >> std::hex >> " << temp_name << ";\n" <<
            ip << "  } else {\n" <<
            ip << "    iss >> " << temp_name << ";\n" <<
            ip << "  }\n" <<
            ip << "  " << propName << assign_prefix;
          if (octet_type) {
            strm << "static_cast<" << temp_type.c_str() << ">(";
          }
          strm << temp_name << (octet_type ? ")" : "") << assign_suffix << ";\n" <<
            ip << "}\n" <<
            ip << "if (val.IsNumber()) {\n" <<
            ip << "  " << propName << assign_prefix << "val.Get" << rapidJsonType << "()" << assign_suffix << ";\n" <<
            ip << "}\n";
          break;

        case AST_PredefinedType::PT_float:
        case AST_PredefinedType::PT_double:
          strm <<
            ip << "if (val.IsNumber()) {\n" <<
            ip << "  " << propName << assign_prefix << "val.Get" << rapidJsonType << "()" << assign_suffix << ";\n" <<
            ip << "}\n";
          break;

        case AST_PredefinedType::PT_longdouble:
          strm <<
            ip << "if (val.IsNumber()) {\n" <<
            ip << "  ACE_CDR::LongDouble temp;\n" <<
            ip << "  ACE_CDR_LONG_DOUBLE_ASSIGNMENT(temp, val.Get" << rapidJsonType << "());\n" <<
            ip << "  " << propName << assign_prefix << "temp" << assign_suffix << ";\n" <<
            ip << "}\n";
          break;

        case AST_PredefinedType::PT_boolean:
          strm <<
            ip << "if (val.IsBool()) {\n" <<
            ip << "  " << propName << assign_prefix << "val.Get" << rapidJsonType << "()" << assign_suffix << ";\n" <<
            ip << "}\n";
          break;

        case AST_PredefinedType::PT_any:
        case AST_PredefinedType::PT_object:
        case AST_PredefinedType::PT_value:
        case AST_PredefinedType::PT_abstract:
        case AST_PredefinedType::PT_void:
        case AST_PredefinedType::PT_pseudo:
          be_util::misc_error_and_abort("Unsupported predefined type in gen_copyfrom");
        }
      }

      if (prop_index) {
        strm <<
          "    }\n"
          "  }\n";
      } else {
        strm <<
          "    }\n"
          "  }\n";
      }

    } else if ((cls & CL_SEQUENCE) && builtInSeq(type)) {
      AST_Type* real_type = resolveActualType(type);
      AST_Sequence* seq = dynamic_cast<AST_Sequence*>(real_type);
      AST_Type* elem = seq->base_type();
      if (prop_index) {
        strm <<
          "  {\n"
          "    if (" << src << ".IsArray()) {\n"
          "      const rapidjson::Value& valseq = " << src << ".GetArray()[" << prop << "];\n"
          "      uint32_t length = valseq.GetArray().Size();\n"
          "      " << scoped(type->name()) << "& temp = " << propName <<  ";\n"
          "      temp.length(length);\n"
          "      for (uint32_t i = 0; i < length; ++i) {\n"
          "      ";
        gen_copyfrom("temp", "valseq", elem, "i", true);
        strm <<
          "      }\n"
          "    }\n"
          "  }\n";
      } else {
        strm <<
          "  {\n"
          "    rapidjson::Value::ConstMemberIterator it = " << src << ".FindMember(\"" << prop << "\");\n"
          "    if (it != " << src << ".MemberEnd()) {\n"
          "      const rapidjson::Value& valseq = it->value;\n"
          "      if (valseq.IsArray()) {\n"
          "        uint32_t length = valseq.GetArray().Size();\n"
          "        " << scoped(type->name()) << "& temp = " << propName <<  ";\n"
          "        temp.length(length);\n"
          "        for (uint32_t i = 0; i < length; ++i) {\n"
          "        ";
        gen_copyfrom("temp", "valseq", elem, "i", true);
        strm <<
          "        }\n"
          "      }\n"
          "    }\n"
          "  }\n";
      }
    } else { // struct, sequence, etc.
      if (prop_index) {
        strm <<
          "  {\n"
          "    if (" << src << ".IsArray()) {\n"
          "      const rapidjson::Value& valzone = " << src << ".GetArray()[" << prop << "];\n"
          "      copyFromRapidJson(valzone, " << (fun_assign ? propName + "()" : propName) << ");\n"
          "    }\n"
          "  }\n";
      } else {
        strm <<
          "  {\n"
          "    rapidjson::Value::ConstMemberIterator it = " << src << ".FindMember(\"" << prop << "\");\n"
          "    if (it != " << src << ".MemberEnd()) {\n"
          "      const rapidjson::Value& valzone = it->value;\n"
          "      copyFromRapidJson(valzone, " << (fun_assign ? propName + "()" : propName) << ");\n"
          "    }\n"
          "  }\n";
      }
    }
  }

  struct gen_field_copyto {
    gen_field_copyto(const char* tgt, const char* alloc, const char* src) : tgt_(tgt), alloc_(alloc), src_(src) {}
    const std::string tgt_, alloc_, src_;
    void operator()(AST_Field* field) const
    {
      const std::string fieldName = field->local_name()->get_string(),
        source = src_ + '.' + fieldName,
        prop = "\"" + fieldName + "\"";
      gen_copyto(tgt_.c_str(), alloc_.c_str(), source.c_str(),
                 field->field_type(), prop.c_str());
    }
  };

  struct gen_field_copyfrom {
    gen_field_copyfrom(const char* tgt, const char* src) : tgt_(tgt), src_(src) {}
    const std::string tgt_, src_;
    void operator()(AST_Field* field) const
    {
      const std::string fieldName = field->local_name()->get_string();
      gen_copyfrom(tgt_.c_str(), src_.c_str(),
                 field->field_type(), fieldName.c_str());
    }
  };

  void gen_type_support(UTL_ScopedName* name) {
    be_global->add_include("dds/DCPS/RapidJsonTypeConverter.h",
                           BE_GlobalData::STREAM_CPP);
    be_global->impl_ << be_global->versioning_begin() << "\n";
    ScopedNamespaceGuard cppGuard(name, be_global->impl_);
    const std::string lname = name->last_component()->get_string(),
      ts_rj = lname + "TypeSupportRapidJsonImpl";
    be_global->impl_ <<
      "class " << ts_rj << "\n"
      "  : public virtual " << lname << "TypeSupportImpl\n"
      "  , public virtual OpenDDS::DCPS::RapidJsonTypeConverter {\n\n"
      "  void toRapidJson(const void* source, rapidjson::Value& dst, rapidjson::Value::AllocatorType& alloc) const\n"
      "  {\n"
      "    OpenDDS::DCPS::copyToRapidJson(*static_cast<const " << lname << "*>(source), dst, alloc);\n"
      "  }\n\n"
      "  void* fromRapidJson(const rapidjson::Value& source) const\n"
      "  {\n"
      "    " << lname << "* result = new " << lname << "();\n"
      "    OpenDDS::DCPS::copyFromRapidJson(source, *result);\n"
      "    return result;\n"
      "  }\n\n"
      "  void deleteFromRapidJsonResult(void* val) const\n"
      "  {\n"
      "    " << lname << "* delete_me = static_cast< " << lname << "*>(val);\n"
      "    delete delete_me;\n"
      "  }\n\n"
      "public:\n"
      "  struct Initializer {\n"
      "    Initializer()\n"
      "    {\n"
      "      " << lname << "TypeSupport_var ts = new " << ts_rj << ";\n"
      "      ts->register_type(0, \"\");\n"
      "    }\n"
      "  };\n"
      "};\n\n" <<
      ts_rj << "::Initializer init_tsrapidjson_" << lname << ";\n";
    be_global->impl_ << be_global->versioning_end() << "\n";
  }
} // namespace

bool rapidjson_generator::gen_struct(AST_Structure* node, UTL_ScopedName* name,
                              const std::vector<AST_Field*>& fields,
                              AST_Type::SIZE_TYPE, const char*)
{
  gen_includes();
  {
    NamespaceGuard ng;
    const std::string clazz = scoped(name);
    {
      Function ctv("copyToRapidJson", "void");
      ctv.addArg("src", "const " + clazz + '&');
      ctv.addArg("dst", "rapidjson::Value&");
      ctv.addArg("alloc", "rapidjson::Value::AllocatorType&");
      ctv.endArgs();
      std::for_each(fields.begin(), fields.end(), gen_field_copyto("dst", "alloc", "src"));
    }
    {
      Function vtc("copyFromRapidJson", "void");
      vtc.addArg("src", "const rapidjson::Value&");
      vtc.addArg("out", clazz + '&');
      vtc.endArgs();
      std::for_each(fields.begin(), fields.end(),
                    gen_field_copyfrom("out", "src"));
    }
  }

  if (idl_global->is_dcps_type(name) || be_global->is_topic_type(node)) {
    gen_type_support(name);
  }
  return true;
}

bool rapidjson_generator::gen_typedef(AST_Typedef*, UTL_ScopedName* name,
                               AST_Type* type, const char* /*repoid*/)
{
  gen_includes();
  switch (type->node_type()) {
  case AST_Decl::NT_sequence: {
    NamespaceGuard ng;
    AST_Sequence* seq = dynamic_cast<AST_Sequence*>(type);
    const std::string cxx = scoped(name);
    AST_Type* elem = seq->base_type();
    {
      Function ctv("copyToRapidJson", "void");
      ctv.addArg("src", "const " + cxx + '&');
      ctv.addArg("dst", "rapidjson::Value&");
      ctv.addArg("alloc", "rapidjson::Value::AllocatorType&");
      ctv.endArgs();
      be_global->impl_ <<
        "  dst.SetArray();\n"
        "  dst.GetArray().Reserve(src.length(), alloc);\n"
        "  for (CORBA::ULong i = 0; i < dst.Size(); ++i) {\n"
        "    ";
      gen_copyto("dst", "alloc", "src[i]", elem, "i", true);
      be_global->impl_ <<
        "  }\n"
        "  for (CORBA::ULong i = dst.Size(); i < src.length(); ++i) {\n"
        "    ";
      gen_copyto("dst", "alloc", "src[i]", elem, "i", true, true);
      be_global->impl_ <<
        "  }\n";
    }
    {
      Function vtc("copyFromRapidJson", "void");
      vtc.addArg("src", "const rapidjson::Value&");
      vtc.addArg("out", cxx + '&');
      vtc.endArgs();
      be_global->impl_ <<
        "  CORBA::ULong length = 0;\n"
        "  if (src.IsArray()) {\n"
        "    length = src.GetArray().Size();\n"
        "  }\n"
        "  out.length(length);\n"
        "  for (CORBA::ULong i = 0; i < length; ++i) {\n"
        "  ";
      gen_copyfrom("out", "src", elem, "i", true);
      be_global->impl_ <<
        "  }\n";
    }
    break;
  }
  case AST_Decl::NT_array: {
    NamespaceGuard ng;
    AST_Array* array = dynamic_cast<AST_Array*>(type);
    const std::string cxx = scoped(name);
    AST_Type* elem = array->base_type();
    {
      Function ctv("copyToRapidJson", "void");
      ctv.addArg("src", "const " + cxx + '&');
      ctv.addArg("dst", "rapidjson::Value&");
      ctv.addArg("alloc", "rapidjson::Value::AllocatorType&");
      ctv.endArgs();
      be_global->impl_ <<
        "  CORBA::ULong length = sizeof(src) / sizeof(src[0]);\n"
        "  dst.SetArray();\n"
        "  dst.GetArray().Reserve(length, alloc);\n"
        "  for (CORBA::ULong i = 0; i < dst.Size(); ++i) {\n"
        "  ";
      gen_copyto("dst", "alloc", "src[i]", elem, "i", true);
      be_global->impl_ <<
        "  }\n"
        "  for (CORBA::ULong i = dst.Size(); i < length; ++i) {\n"
        "    ";
      gen_copyto("dst", "alloc", "src[i]", elem, "i", true, true);
      be_global->impl_ <<
        "  }\n";
    }
    {
      Function vtc("copyFromRapidJson", "void");
      vtc.addArg("src", "const rapidjson::Value&");
      vtc.addArg("out", cxx + '&');
      vtc.endArgs();
      be_global->impl_ <<
        "  CORBA::ULong length = 0;\n"
        "  if (src.IsArray()) {\n"
        "    CORBA::ULong src_length = src.GetArray().Size();\n"
        "    CORBA::ULong out_length = (sizeof(out) / sizeof(out[0]));\n"
        "    length = (src_length <= out_length) ? src_length : out_length;\n"
        "  }\n"
        "  for (CORBA::ULong i = 0; i < length; ++i) {\n"
        "  ";
      gen_copyfrom("out", "src", elem, "i", true);
      be_global->impl_ <<
        "  }\n";
    }
    break;
  }
  default:
    return true;
  }
  return true;
}

namespace {
  std::string branchGenTo(const std::string&, const std::string& name, AST_Type* type,
                          const std::string&, bool, Intro&,
                          const std::string&, bool)
  {
    const std::string source = "src." + name + "()",
      prop = "\"" + name + "\"";
    std::ostringstream strm;
    strm << "  ";
    gen_copyto("dst", "alloc", source.c_str(), type, prop.c_str(), false, false,  strm);
    return strm.str();
  }
  std::string branchGenFrom(const std::string&, const std::string& name, AST_Type* type,
                            const std::string&, bool, Intro&,
                            const std::string&, bool)
  {
    std::ostringstream strm;
    const Classification cls = classify(type);
    if (cls & (CL_STRUCTURE | CL_UNION | CL_SEQUENCE | CL_FIXED)) {
      strm << "  out." << name << "(" << scoped(type->name()) << "());\n";
    }
    strm << "  ";
    gen_copyfrom("out", "src", type, name.c_str(), false, true, strm);
    return strm.str();
  }
}

bool rapidjson_generator::gen_union(AST_Union* node, UTL_ScopedName* name,
                             const std::vector<AST_UnionBranch*>& branches,
                             AST_Type* discriminator, const char* /*repoid*/)
{
  gen_includes();
  NamespaceGuard ng;
  const std::string clazz = scoped(name);
  {
    Function ctv("copyToRapidJson", "void");
    ctv.addArg("src", "const " + clazz + '&');
    ctv.addArg("dst", "rapidjson::Value&");
    ctv.addArg("alloc", "rapidjson::Value::AllocatorType&");
    ctv.endArgs();
    gen_copyto("dst", "alloc", "src._d()", discriminator, "\"_d\"");
    generateSwitchForUnion(node, "src._d()", branchGenTo, branches, discriminator, "", "", clazz.c_str(), false, false);
  }
  {
    Function vtc("copyFromRapidJson", "void");
    vtc.addArg("src", "const rapidjson::Value&");
    vtc.addArg("out", clazz + '&');
    vtc.endArgs();
    gen_copyfrom("out", "src", discriminator, "_d", false, true);
    generateSwitchForUnion(node, "out._d()", branchGenFrom, branches, discriminator, "", "", clazz.c_str(), false, false);
  }
  if (be_global->is_topic_type(node)) {
    gen_type_support(name);
  }
  return true;
}
