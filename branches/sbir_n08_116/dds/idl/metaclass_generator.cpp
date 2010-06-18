/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "metaclass_generator.h"
#include "be_extern.h"

#include "utl_identifier.h"

namespace {
  struct ContentSubscriptionGuard {
    ContentSubscriptionGuard()
    {
      be_global->header_ << "#ifndef OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE\n";
      be_global->impl_ << "#ifndef OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE\n";
    }
    ~ContentSubscriptionGuard()
    {
      be_global->header_ << "#endif\n";
      be_global->impl_ << "#endif\n";
    }
  };
}

bool metaclass_generator::gen_enum(UTL_ScopedName* name,
  const std::vector<AST_EnumVal*>& contents, const char*)
{
  ContentSubscriptionGuard csg;
  NamespaceGuard ng;
  std::string decl = "const char* gen_" + scoped_helper(name, "_") + "_names[]";
  be_global->header_ << "extern " << decl << ";\n";
  be_global->impl_ << decl << " = {\n";
  for (size_t i = 0; i < contents.size(); ++i) {
    be_global->impl_ << "  \"" << contents[i]->local_name()->get_string()
      << ((i < contents.size() - 1) ? "\",\n" : "\"\n");
  }
  be_global->impl_ << "};\n";
  return true;
}

namespace {
  void unTypeDef(AST_Type*& element)
  {
    if (element->node_type() == AST_Decl::NT_typedef) {
      AST_Typedef* td = AST_Typedef::narrow_from_decl(element);
      element = td->primitive_base_type();
    }
  }

  enum Classification {
    CL_UNKNOWN = 0,
    CL_SCALAR = 1,
    CL_STRUCTURE = 2,
    CL_STRING = 4,
    CL_ENUM = 8
  };

  Classification classify(AST_Type* type)
  {
    unTypeDef(type);
    switch (type->node_type()) {
    case AST_Decl::NT_pre_defined: {
      AST_PredefinedType* p = AST_PredefinedType::narrow_from_decl(type);
      if (p->pt() == AST_PredefinedType::PT_any
          || p->pt() == AST_PredefinedType::PT_object) {
        return CL_UNKNOWN;
      } else {
        return CL_SCALAR;
      }
    }
    case AST_Decl::NT_string:
    case AST_Decl::NT_wstring:
      return static_cast<Classification>(CL_SCALAR | CL_STRING);
    case AST_Decl::NT_struct:
      return CL_STRUCTURE;
    case AST_Decl::NT_enum:
      return static_cast<Classification>(CL_SCALAR | CL_ENUM);
    default:
      return CL_UNKNOWN;
    }
  }

  void gen_field_getValue(AST_Field* field)
  {
    Classification cls = classify(field->field_type());
    const std::string fieldName = field->local_name()->get_string();
    if (cls & CL_SCALAR) {
      std::string prefix, suffix;
      if (cls & CL_ENUM) {
        prefix = "gen_" +
          dds_generator::scoped_helper(field->field_type()->name(), "_")
          + "_names[";
        suffix = "]";
      }
      be_global->impl_ <<
        "    if (std::strcmp(field, \"" << fieldName << "\") == 0) {\n"
        "      return " + prefix + "typed." + fieldName
        + (cls & CL_STRING ? ".in()" : "") + suffix + ";\n"
        "    }\n";
      be_global->add_include("<cstring>", BE_GlobalData::STREAM_CPP);
    } else if (cls & CL_STRUCTURE) {
      size_t n = fieldName.size() + 1 /* 1 for the dot */;
      std::string fieldType = scoped(field->field_type()->name());
      be_global->impl_ <<
        "    if (std::strncmp(field, \"" << fieldName << ".\", " << n
        << ") == 0) {\n"
        "      return getMetaStruct<" << fieldType << ">().getValue(&typed."
        << fieldName << ", field + " << n << ");\n"
        "    }\n";
    }
  }

  struct gen_field_createQC {
    std::string scoped_;
    gen_field_createQC(const std::string& scoped)
      : scoped_(scoped)
    {}

    void operator()(AST_Field* field) const
    {
      Classification cls = classify(field->field_type());
      const std::string fieldName = field->local_name()->get_string();
      if (cls & CL_SCALAR) {
        be_global->impl_ <<
          "    if (std::strcmp(field, \"" << fieldName << "\") == 0) {\n"
          "      return make_field_cmp(&" << scoped_ << "::" << fieldName
          << ", next);\n"
          "    }\n";
      } else if (cls & CL_STRUCTURE) {
        size_t n = fieldName.size() + 1 /* 1 for the dot */;
        std::string fieldType = scoped(field->field_type()->name());
        be_global->impl_ <<
          "    if (std::strncmp(field, \"" << fieldName << ".\", " << n
          << ") == 0) {\n"
          "      return make_struct_cmp(&" << scoped_ << "::" << fieldName
          << ", getMetaStruct<" << fieldType << ">().create_qc_comparator("
          "field + " << n << ", 0), next);\n"
          "    }\n";
      }
    }
  };
}

bool metaclass_generator::gen_struct(UTL_ScopedName* name,
  const std::vector<AST_Field*>& fields, const char*)
{
  ContentSubscriptionGuard csg;
  NamespaceGuard ng;
  be_global->add_include("dds/DCPS/FilterEvaluator.h",
    BE_GlobalData::STREAM_CPP);
  if (first_struct_) {
    be_global->header_ <<
      "class MetaStruct;\n\n"
      "template<typename T>\n"
      "const MetaStruct& getMetaStruct();\n\n";
    first_struct_ = false;
  }
  std::string clazz = scoped(name);
  be_global->impl_ <<
    "template<>\n"
    "struct MetaStructImpl<" << clazz << "> : MetaStruct {\n"
    "  Value getValue(const void* stru, const char* field) const\n"
    "  {\n"
    "    const " << clazz << "& typed = *static_cast<const " << clazz
    << "*>(stru);\n";
  std::for_each(fields.begin(), fields.end(), gen_field_getValue);
  std::string decl = "const MetaStruct& getMetaStruct<" + clazz + ">()",
    exp = be_global->export_macro().c_str();
  be_global->header_ << "template<>\n" << exp << (exp.length() ? "\n" : "")
    << decl << ";\n";
  std::string exception =
    "    throw std::runtime_error(\"Field \" + std::string(field) + \" not "
    "found or its type is not supported (in Struct " + clazz + ")\");\n";
  be_global->impl_ <<
    "    ACE_UNUSED_ARG(typed);\n" <<
    exception <<
    "  }\n\n"
    "  ComparatorBase::Ptr create_qc_comparator(const char* field, "
    "ComparatorBase::Ptr next) const\n"
    "  {\n"
    "    ACE_UNUSED_ARG(next);\n";
  std::for_each(fields.begin(), fields.end(), gen_field_createQC(clazz));
  be_global->impl_ <<
    exception <<
    "  }\n"
    "};\n\n"
    "template<>\n"
    << decl << "\n"
    "{\n"
    "  static MetaStructImpl<" << clazz << "> msi;\n"
    "  return msi;\n"
    "}\n\n";
  be_global->add_include("stdexcept", BE_GlobalData::STREAM_CPP);
  return true;
}
