/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "v8_generator.h"
#include "be_extern.h"

#include "utl_identifier.h"

using namespace AstTypeClassification;

void v8_generator::gen_includes()
{
  be_global->add_include("<v8.h>", BE_GlobalData::STREAM_H);
  be_global->add_include("<nan.h>", BE_GlobalData::STREAM_CPP);
}

bool v8_generator::gen_enum(AST_Enum*, UTL_ScopedName*,
                            const std::vector<AST_EnumVal*>&, const char*)
{
  return true;
}

namespace {

  std::string getV8Type(AST_Type* type,
                        AST_PredefinedType::PredefinedType* pt = 0)
  {
    const Classification cls = classify(type);
    if (cls & (CL_ENUM | CL_STRING)) return "String";
    if (cls & (CL_STRUCTURE | CL_UNION)) return "Object";
    if (cls & (CL_ARRAY | CL_SEQUENCE)) return "Array";
    if (cls & CL_PRIMITIVE) {
      AST_Type* actual = resolveActualType(type);
      AST_PredefinedType* p = AST_PredefinedType::narrow_from_decl(actual);
      if (pt) *pt = p->pt();
      switch (p->pt()) {
      case AST_PredefinedType::PT_char:
      case AST_PredefinedType::PT_wchar:
      case AST_PredefinedType::PT_longlong:  // Can't fully fit in a JS "number"
      case AST_PredefinedType::PT_ulonglong: // Can't fully fit in a JS "number"
        return "String";
      case AST_PredefinedType::PT_boolean:
        return "Boolean";
      case AST_PredefinedType::PT_octet:
      case AST_PredefinedType::PT_ushort:
      case AST_PredefinedType::PT_short:
      case AST_PredefinedType::PT_ulong:
      case AST_PredefinedType::PT_long:
        return "Integer";
      default:
        return "Number";
      }
    }
    return "";
  }

  bool builtInSeq(AST_Type* type)
  {
    const std::string name = scoped(type->name());
    static const char PRE[] = "CORBA::";
    if (std::strncmp(name.c_str(), PRE, sizeof(PRE) - 1)) return false;
    return name.rfind("Seq") == name.size() - 3;
  }

  void gen_copyto(const char* tgt, const char* src, AST_Type* type,
                  const char* prop, std::ostream& strm = be_global->impl_)
  {
    const Classification cls = classify(type);
    AST_PredefinedType::PredefinedType pt = AST_PredefinedType::PT_void;
    const std::string v8Type = getV8Type(type, &pt),
      propName = std::string(tgt) + "->Set(" + prop + ", ";

    if (cls & CL_SCALAR) {
      std::string prefix, suffix, postNew;

      if (cls & CL_ENUM) {
        const std::string underscores =
          dds_generator::scoped_helper(type->name(), "_"),
          array = "gen_" + underscores + "_names";
        prefix = '(' + std::string(src) + " >= sizeof(" + array + ")/sizeof(" +
          array + "[0])) ? \"<<invalid>>\" : " + array + "[static_cast<int>(";
        suffix = ")]";
        postNew = ".ToLocalChecked()";

      } else if (cls & CL_STRING) {
        if (!std::strstr(src, "()")) {
          suffix = ".in()";
        }
        if (cls & CL_WIDE) {
          strm <<
            "  {\n"
            "    const size_t len = ACE_OS::strlen(" << src << suffix << ");\n"
            "    uint16_t* const str = new uint16_t[len + 1];\n"
            "    for (size_t i = 0; i <= len; ++i) {\n"
            "      str[i] = " << src << "[i];\n"
            "    }\n"
            "    " << propName << "Nan::New(str).ToLocalChecked());\n"
            "    delete[] str;\n"
            "  }\n";
          return;
        }
        postNew = ".ToLocalChecked()";

      } else if (pt == AST_PredefinedType::PT_char) {
        strm <<
          "  {\n"
          "    const char str[] = {" << src << ", 0};\n"
          "    " << propName << "Nan::New(str).ToLocalChecked());\n"
          "  }\n";
        return;

      } else if (pt == AST_PredefinedType::PT_wchar) {
        strm <<
          "  {\n"
          "    const uint16_t str[] = {" << src << ", 0};\n"
          "    " << propName << "Nan::New(str).ToLocalChecked());\n"
          "  }\n";
        return;

      } else if (pt == AST_PredefinedType::PT_longlong || pt == AST_PredefinedType::PT_ulonglong) {
        prefix = "std::to_string(";
        suffix = ").c_str()";
        postNew = ".ToLocalChecked()";

      } else if (v8Type == "Number") {
        prefix = "static_cast<double>(";
        suffix = ")";
      }

      strm <<
        "  " << propName << "Nan::New(" << prefix << src << suffix << ")"
             << postNew << ");\n";

    } else if ((cls & CL_SEQUENCE) && builtInSeq(type)) {
      AST_Type* real_type = resolveActualType(type);
      AST_Sequence* seq = AST_Sequence::narrow_from_decl(real_type);
      AST_Type* elem = seq->base_type();
      strm <<
        "  {\n"
        "    const v8::Local<v8::Array> seq = Nan::New<v8::Array>(" << src
                       << ".length());\n"
        "    for (CORBA::ULong j = 0; j < " << src << ".length(); ++j) {\n";
      gen_copyto("seq", (std::string(src) + "[j]").c_str(), elem, "j");
      strm <<
        "    }\n"
        "    " << propName << "seq" << ");\n"
        "  }\n";

    } else { // struct, sequence, etc.
      strm <<
        "  " << propName << "copyToV8(" << src << "));\n";
    }
  }

  void gen_copyfrom(const char* tgt, const char* src, AST_Type* type,
                    const char* prop, bool prop_index = false, bool prop_no_str = false, std::ostream& strm = be_global->impl_)
  {
    const Classification cls = classify(type);
    AST_PredefinedType::PredefinedType pt = AST_PredefinedType::PT_void;
    const std::string v8Type = getV8Type(type, &pt),
      propName = prop_index ? std::string(tgt) + "[" + prop + "]" : std::string(tgt) + "." + prop;

    if (cls & CL_SCALAR) {
      std::string clines;

      if (cls & CL_ENUM) {
        const std::string underscores =
          dds_generator::scoped_helper(type->name(), "_"),
          array = "gen_" + underscores + "_names";
        clines += "      if (lv->IsNumber()) {\n";
        clines += "        v8::Local<v8::Integer> li = Nan::To<v8::Integer>(lv).ToLocalChecked();\n";
        clines += "        " + propName + " = static_cast<" + scoped(type->name()) + ">(li->Value());\n";
        clines += "      }\n";
        clines += "      if (lv->IsString()) {\n";
        clines += "        v8::Local<v8::String> ls = Nan::To<v8::String>(lv).ToLocalChecked();\n";
        clines += "        std::string ss(ls->Length(), ' ');\n";
        clines += "        ls->WriteUtf8(&ss[0]);\n";
        clines += "        for (uint32_t i = 0; i < sizeof(" + array + ")/sizeof(" + array + "[0]); ++i) {\n";
        clines += "          if (ss == " + array + "[i]) {\n";
        clines += "            " + propName + " = static_cast<" + scoped(type->name()) + ">(i);\n";
        clines += "            continue;\n";
        clines += "          }\n";
        clines += "        }\n";
        clines += "      }\n";
      } else if (cls & CL_STRING) {
        clines += "      if (lv->IsString()) {\n";
        clines += "        v8::Local<v8::String> ls = Nan::To<v8::String>(lv).ToLocalChecked();\n";
        if (cls & CL_WIDE) {
          clines += "        std::vector<uint16_t> vwc(ls->Length(), 0);\n";
          clines += "        ls->Write(&vwc[0]);\n";
          clines += "        std::wstring ws(ls->Length(), ' ');\n";
          clines += "        for (size_t i = 0; i < vwc.size(); ++i) {\n";
          clines += "          ws[i] = vwc[i];\n";
          clines += "        }\n";
          clines += "        " + propName + " = ws.c_str();\n";
        } else {
          clines += "        std::string ss(ls->Length(), ' ');\n";
          clines += "        ls->WriteUtf8(&ss[0]);\n";
          clines += "        " + propName + " = ss.c_str();\n";
        }
        clines += "      }\n";
      } else if (pt == AST_PredefinedType::PT_char) {
        clines += "      if (lv->IsString()) {\n";
        clines += "        v8::Local<v8::String> ls = Nan::To<v8::String>(lv).ToLocalChecked();\n";
        clines += "        ls->WriteUtf8(&" + propName + ", 1);\n";
        clines += "      }\n";
      } else if (pt == AST_PredefinedType::PT_wchar) {
        clines += "      if (lv->IsString()) {\n";
        clines += "        v8::Local<v8::String> ls = Nan::To<v8::String>(lv).ToLocalChecked();\n";
        clines += "        ls->Write(&" + propName + ", 1);\n";
        clines += "      }\n";
      } else if (pt == AST_PredefinedType::PT_longlong
              || pt == AST_PredefinedType::PT_ulonglong) {
        clines += "      if (lv->IsString()) {\n";
        clines += "        v8::Local<v8::String> ls = Nan::To<v8::String>(lv).ToLocalChecked();\n";
        clines += "        std::string ss(ls->Length(), ' ');\n";
        clines += "        ls->WriteUtf8(&ss[0]);\n";
        clines += "        std::istringstream iss(ss);";
        clines += "        if (ss.find(\"0x\") != std::string::npos) {\n";
        clines += "          iss >> std::hex >> " + propName + ";\n";
        clines += "        } else {\n";
        clines += "          iss >> " + propName + ";\n";
        clines += "        }\n";
        clines += "      }\n";
        clines += "      if (lv->IsNumber()) {\n";
        clines += "        v8::Local<v8::Number> ln = Nan::To<v8::Number>(lv).ToLocalChecked();\n";
        clines += "        " + propName + " = ln->IntegerValue();\n";
        clines += "      }\n";
      } else if (pt == AST_PredefinedType::PT_octet
              || pt == AST_PredefinedType::PT_ushort
              || pt == AST_PredefinedType::PT_short
              || pt == AST_PredefinedType::PT_ulong
              || pt == AST_PredefinedType::PT_long) {
        clines += "      if (lv->IsNumber()) {\n";
        clines += "        v8::Local<v8::Number> ln = Nan::To<v8::Number>(lv).ToLocalChecked();\n";
        clines += "        " + propName + " = ln->IntegerValue();\n";
        clines += "      }\n";
      } else if (pt == AST_PredefinedType::PT_float
              || pt == AST_PredefinedType::PT_double
              || pt == AST_PredefinedType::PT_longdouble) {
        clines += "      if (lv->IsNumber()) {\n";
        clines += "        v8::Local<v8::Number> ln = Nan::To<v8::Number>(lv).ToLocalChecked();\n";
        clines += "        " + propName + " = ln->Value();\n";
        clines += "      }\n";
      } else if (pt == AST_PredefinedType::PT_boolean) {
        clines += "      if (lv->IsBoolean()) {\n";
        clines += "        v8::Local<v8::Boolean> lb = Nan::To<v8::Boolean>(lv).ToLocalChecked();\n";
        clines += "        " + propName + " = lb->Value();\n";
        clines += "      }\n";
      }

      if (prop_no_str) {
        strm <<
          "  {\n"
          "      v8::Local<v8::Value> lv = " << src << "->Get(" << prop << ");\n"
          << clines <<
          "  }\n";
      } else {
        strm <<
          "  {\n"
          "    v8::Local<v8::String> field_str = Nan::New(\"" << prop << "\").ToLocalChecked();\n"
          "    if (" << src << "->Has(field_str)) {\n"
          "      v8::Local<v8::Value> lv = " << src << "->Get(field_str);\n"
          << clines <<
          "    }\n"
          "  }\n";
      }

    } else if ((cls & CL_SEQUENCE) && builtInSeq(type)) {
      AST_Type* real_type = resolveActualType(type);
      AST_Sequence* seq = AST_Sequence::narrow_from_decl(real_type);
      AST_Type* elem = seq->base_type();
      if (prop_no_str) {
        strm <<
          "  {\n"
          "      v8::Local<v8::Value> lv = " << src << "->Get(" << prop << ");\n"
          "      if (lv->IsArray()) {\n"
          "        uint32_t length = 0;\n"
          "        v8::Local<v8::Object> lo = Nan::To<v8::Object>(lv).ToLocalChecked();\n"
          "        length = lo->Get(Nan::New(\"length\").ToLocalChecked())->ToObject()->Uint32Value();\n"
          "        " << scoped(type->name()) << "& temp = " << propName <<  ";\n"
          "        temp.length(length);\n"
          "        for (uint32_t i = 0; i < length; ++i) {\n"
          "        ";
          gen_copyfrom("temp", "lo", elem, "i", true, true);
          strm <<
          "        }\n"
          "      }\n"
          "  }\n";
      } else {
        strm <<
          "  {\n"
          "    v8::Local<v8::String> field_str = Nan::New(\"" << prop << "\").ToLocalChecked();\n"
          "    if (" << src << "->Has(field_str)) {\n"
          "      v8::Local<v8::Value> lv = " << src << "->Get(field_str);\n"
          "      if (lv->IsArray()) {\n"
          "        uint32_t length = 0;\n"
          "        v8::Local<v8::Object> lo = Nan::To<v8::Object>(lv).ToLocalChecked();\n"
          "        length = lo->Get(Nan::New(\"length\").ToLocalChecked())->ToObject()->Uint32Value();\n"
          "        " << scoped(type->name()) << "& temp = " << propName <<  ";\n"
          "        temp.length(length);\n"
          "        for (uint32_t i = 0; i < length; ++i) {\n"
          "        ";
          gen_copyfrom("temp", "lo", elem, "i", true, true);
          strm <<
          "        }\n"
          "      }\n"
          "    }\n"
          "  }\n";
      }
    } else { // struct, sequence, etc.
      if (prop_no_str) {
        strm <<
          "  {\n"
          "    v8::Local<v8::Value> lv = " << src << "->Get(" << prop << ");\n"
          "    v8::Local<v8::Object> lo = Nan::To<v8::Object>(lv).ToLocalChecked();\n"
          "    copyFromV8(lo, " << propName << ");\n"
          "  }\n";
      } else {
        strm <<
          "  {\n"
          "    v8::Local<v8::String> field_str = Nan::New(\"" << prop << "\").ToLocalChecked();\n"
          "    if (" << src << "->Has(field_str)) {\n"
          "      v8::Local<v8::Value> lv = " << src << "->Get(field_str);\n"
          "      v8::Local<v8::Object> lo = Nan::To<v8::Object>(lv).ToLocalChecked();\n"
          "      copyFromV8(lo, " << propName << ");\n"
          "    }\n"
          "  }\n";
      }
    }
  }

  struct gen_field_copyto {
    gen_field_copyto(const char* tgt, const char* src) : tgt_(tgt), src_(src) {}
    const std::string tgt_, src_;
    void operator()(AST_Field* field) const
    {
      const std::string fieldName = field->local_name()->get_string(),
        source = src_ + '.' + fieldName,
        prop = "Nan::New<v8::String>(\"" + fieldName + "\").ToLocalChecked()";
      gen_copyto(tgt_.c_str(), source.c_str(),
                 field->field_type(), prop.c_str());
    }
  };

  struct gen_field_copyfrom {
    gen_field_copyfrom(const char* tgt, const char* src) : tgt_(tgt), src_(src) {}
    const std::string tgt_, src_;
    void operator()(AST_Field* field) const
    {
      const std::string fieldName = field->local_name()->get_string(),
        source = src_,
        prop = fieldName;
      gen_copyfrom(tgt_.c_str(), source.c_str(),
                 field->field_type(), prop.c_str());
    }
  };
}

bool v8_generator::gen_struct(AST_Structure*, UTL_ScopedName* name,
                              const std::vector<AST_Field*>& fields,
                              AST_Type::SIZE_TYPE, const char*)
{
  gen_includes();
  {
    NamespaceGuard ng;
    const std::string clazz = scoped(name);
    {
      Function ctv("copyToV8", "v8::Local<v8::Object>");
      ctv.addArg("src", "const " + clazz + '&');
      ctv.endArgs();
      be_global->impl_ <<
        "  const v8::Local<v8::Object> stru = Nan::New<v8::Object>();\n";
      std::for_each(fields.begin(), fields.end(),
                    gen_field_copyto("stru", "src"));
      be_global->impl_ <<
        "  return stru;\n";
    }
    {
      Function vtc("copyFromV8", "void");
      vtc.addArg("src", "const v8::Local<v8::Object>&");
      vtc.addArg("out", clazz + '&');
      vtc.endArgs();
      std::for_each(fields.begin(), fields.end(),
                    gen_field_copyfrom("out", "src"));
      be_global->impl_ <<
        "  return;\n";
    }
  }

  if (idl_global->is_dcps_type(name)) {
    be_global->add_include("dds/DCPS/V8TypeConverter.h",
                           BE_GlobalData::STREAM_CPP);
    ScopedNamespaceGuard cppGuard(name, be_global->impl_);
    const std::string lname = name->last_component()->get_string(),
      tsv8 = lname + "TypeSupportV8Impl";
    be_global->impl_ <<
      "class " << tsv8 << "\n"
      "  : public virtual " << lname << "TypeSupportImpl\n"
      "  , public virtual OpenDDS::DCPS::V8TypeConverter {\n\n"
      "  v8::Local<v8::Object> toV8(const void* source) const\n"
      "  {\n"
      "    return OpenDDS::DCPS::copyToV8(*static_cast<const " << lname
               << "*>(source));\n"
      "  }\n\n"
      "  void* fromV8(const v8::Local<v8::Object>& source) const\n"
      "  {\n"
      "    " << lname << "* result = new " << lname << "();\n"
      "    OpenDDS::DCPS::copyFromV8(source, *result);\n"
      "    return result;\n"
      "  }\n\n"
      "  DDS::InstanceHandle_t register_instance_helper(DDS::DataWriter* dw, const void* data) const\n"
      "  {\n"
      "    " << lname << "DataWriter* dw_t = dynamic_cast<" << lname << "DataWriter*>(dw);\n"
      "    return dw_t->register_instance(*reinterpret_cast<const " << lname << "*>(data));\n"
      "  }\n\n"
      "  DDS::ReturnCode_t write_helper(DDS::DataWriter* dw, const void* data, DDS::InstanceHandle_t inst) const\n"
      "  {\n"
      "    " << lname << "DataWriter* dw_t = dynamic_cast<" << lname << "DataWriter*>(dw);\n"
      "    return dw_t->write(*reinterpret_cast<const " << lname << "*>(data), inst);\n"
      "  }\n\n"
      "  DDS::ReturnCode_t unregister_instance_helper(DDS::DataWriter* dw, const void* data, DDS::InstanceHandle_t inst) const\n"
      "  {\n"
      "    " << lname << "DataWriter* dw_t = dynamic_cast<" << lname << "DataWriter*>(dw);\n"
      "    return dw_t->unregister_instance(*reinterpret_cast<const " << lname << "*>(data), inst);\n"
      "  }\n\n"
      "  DDS::ReturnCode_t dispose_helper(DDS::DataWriter* dw, const void* data, DDS::InstanceHandle_t inst) const\n"
      "  {\n"
      "    " << lname << "DataWriter* dw_t = dynamic_cast<" << lname << "DataWriter*>(dw);\n"
      "    return dw_t->dispose(*reinterpret_cast<const " << lname << "*>(data), inst);\n"
      "  }\n\n"
      "public:\n"
      "  struct Initializer {\n"
      "    Initializer()\n"
      "    {\n"
      "      " << lname << "TypeSupport_var ts = new " << tsv8 << ";\n"
      "      ts->register_type(0, \"\");\n"
      "    }\n"
      "  };\n"
      "};\n\n" <<
      tsv8 << "::Initializer init_tsv8_" << lname << ";\n";
  }
  return true;
}

bool v8_generator::gen_typedef(AST_Typedef*, UTL_ScopedName* name,
                               AST_Type* type, const char* /*repoid*/)
{
  gen_includes();
  switch (type->node_type()) {
  case AST_Decl::NT_sequence: {
    NamespaceGuard ng;
    AST_Sequence* seq = AST_Sequence::narrow_from_decl(type);
    const std::string cxx = scoped(name);
    AST_Type* elem = seq->base_type();
    {
      Function ctv("copyToV8", "v8::Local<v8::Object>");
      ctv.addArg("src", "const " + cxx + '&');
      ctv.endArgs();
      be_global->impl_ <<
        "  const v8::Local<v8::Array> tgt(Nan::New<v8::Array>(src.length()));\n"
        "  for (CORBA::ULong i = 0; i < src.length(); ++i) {\n"
        "  ";
      gen_copyto("tgt", "src[i]", elem, "i");
      be_global->impl_ <<
        "  }\n"
        "  return tgt;\n";
    }
    {
      Function vtc("copyFromV8", "void");
      vtc.addArg("src", "const v8::Local<v8::Object>&");
      vtc.addArg("out", cxx + '&');
      vtc.endArgs();
      be_global->impl_ <<
        "  uint32_t length = 0;\n"
        "  if (src->IsArray()) {\n"
        "    length = src->Get(Nan::New(\"length\").ToLocalChecked())->ToObject()->Uint32Value();\n"
        "  }\n"
        "  out.length(length);\n"
        "  for (uint32_t i = 0; i < length; ++i) {\n"
        "  ";
      gen_copyfrom("out", "src", elem, "i", true, true);
      be_global->impl_ <<
        "  }\n"
        "  return;\n";
    }
    break;
  }
  case AST_Decl::NT_array: {
    //TODO: support arrays
    break;
  }
  default:
    return true;
  }
  return true;
}

namespace {
  std::string branchGen(const std::string& name, AST_Type* type,
                        const std::string&, std::string&,
                        const std::string&)
  {
    const std::string source = "src." + name + "()",
      prop = "Nan::New<v8::String>(\"" + name + "\").ToLocalChecked()";
    std::ostringstream strm;
    strm << "  ";
    gen_copyto("uni", source.c_str(), type, prop.c_str(), strm);
    return strm.str();
  }
}

bool v8_generator::gen_union(AST_Union*, UTL_ScopedName* name,
                             const std::vector<AST_UnionBranch*>& branches,
                             AST_Type* discriminator, const char* /*repoid*/)
{
  gen_includes();
  NamespaceGuard ng;
  const std::string clazz = scoped(name);
  {
    Function ctv("copyToV8", "v8::Local<v8::Object>");
    ctv.addArg("src", "const " + clazz + '&');
    ctv.endArgs();
    be_global->impl_ <<
      "  const v8::Local<v8::Object> uni = Nan::New<v8::Object>();\n";
    gen_copyto("uni", "src._d()", discriminator, "Nan::New<v8::String>(\"_d\").ToLocalChecked()");
    generateSwitchForUnion("src._d()", branchGen, branches, discriminator,
                           "", "", clazz.c_str(), false, false);
    be_global->impl_ <<
      "  return uni;\n";
  }
  {
    Function vtc("copyFromV8", "void");
    vtc.addArg("src", "const v8::Local<v8::Object>&");
    vtc.addArg("out", clazz + '&');
    vtc.endArgs();
// TODO FIXME
//    be_global->impl_ <<
//      "  const v8::Local<v8::Object> uni = Nan::New<v8::Object>();\n";
//    gen_copyto("uni", "src._d()", discriminator, "Nan::New<v8::String>(\"_d\").ToLocalChecked()");
//    generateSwitchForUnion("src._d()", branchGen, branches, discriminator,
//                           "", "", clazz.c_str(), false, false);
//    be_global->impl_ <<
//      "  return uni;\n";
  }
  return true;
}
