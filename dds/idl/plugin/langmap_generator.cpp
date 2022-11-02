/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "langmap_generator.h"
#include "langmap_generator_helper.h"

#include "field_info.h"
#include "be_extern.h"

#include <dds/DCPS/Definitions.h>

#ifdef ACE_HAS_CDR_FIXED
#  include <ast_fixed.h>
#endif
#include <utl_identifier.h>

#include <ace/CDR_Base.h>

#include <map>
#include <iostream>

using namespace AstTypeClassification;

std::map<AST_PredefinedType::PredefinedType, std::string> GeneratorBase::primtype_;

std::map<GeneratorBase::Helper, std::string> GeneratorBase::helpers_;

namespace {
  GeneratorBase* generator_ = 0;
}

std::string GeneratorBase::exporter()
{
  return be_builtin_global->export_macro().empty() ? ""
    : be_builtin_global->export_macro().c_str() + std::string(" ");
}

std::string GeneratorBase::array_zero_indices(AST_Array* arr)
{
  std::string indices;
  for (ACE_CDR::ULong i = 1; i < arr->n_dims(); ++i) {
    indices += "[0]";
  }
  return indices;
}

std::string GeneratorBase::array_dims(AST_Type* type, ACE_CDR::ULong& elems)
{
  AST_Array* const arr = dynamic_cast<AST_Array*>(type);
  std::string ret = array_zero_indices(arr);
  elems *= array_element_count(arr);
  AST_Type* base = resolveActualType(arr->base_type());
  if (dynamic_cast<AST_Array*>(base)) {
    ret += "[0]" + array_dims(base, elems);
  }
  return ret;
}

GeneratorBase::~GeneratorBase() {}

std::string GeneratorBase::string_ns()
{
  return "::CORBA";
}

void GeneratorBase::enum_entry(AST_Enum* e, AST_Union* the_union, AST_Union::DefaultValue& dv, std::stringstream& first_label)
{
  first_label << scoped(e->value_to_name(dv.u.enum_val));
}

std::string GeneratorBase::get_typedef(const std::string& name, const std::string& type)
{
  return "typedef " + type + " " + name;
}

bool GeneratorBase::gen_interf_fwd(UTL_ScopedName* name)
{
  const ScopedNamespaceGuard namespaces(name, be_builtin_global->lang_header_);

  be_builtin_global->add_include("<tao/Objref_VarOut_T.h>", BE_BuiltinGlobalData::STREAM_LANG_H);
  const char* const nm = name->last_component()->get_string();
  be_builtin_global->lang_header_ <<
    "class " << nm << ";\n"
    "typedef " << nm << '*' << nm << "_ptr;\n"
    "typedef TAO_Objref_Var_T<" << nm << "> " << nm << "_var;\n"
    "typedef TAO_Objref_Out_T<" << nm << "> " << nm << "_out;\n";

  return true;
}

void GeneratorBase::gen_typecode(UTL_ScopedName* name)
{
  if (!be_builtin_global->suppress_typecode()) {
    const char* const nm = name->last_component()->get_string();
    be_builtin_global->lang_header_ <<
      "extern " << exporter() << "const ::CORBA::TypeCode_ptr _tc_" << nm
      << ";\n";
    const ScopedNamespaceGuard cppNs(name, be_builtin_global->impl_);
    be_builtin_global->impl_ <<
      "const ::CORBA::TypeCode_ptr _tc_" << nm << " = 0;\n";
  }
}

std::string GeneratorBase::const_keyword(AST_Expression::ExprType)
{
  return "const";
}

std::string GeneratorBase::map_type(AST_Type* type)
{
  if (dynamic_cast<AST_Typedef*>(type)) {
    return scoped(type->name());
  }
  const Classification cls = classify(type);
  if (cls & CL_PRIMITIVE) {
    AST_Type* actual = resolveActualType(type);
    return primtype_[dynamic_cast<AST_PredefinedType*>(actual)->pt()];
  }
  if (cls & CL_STRING) {
    const AST_PredefinedType::PredefinedType chartype = (cls & CL_WIDE)
      ? AST_PredefinedType::PT_wchar : AST_PredefinedType::PT_char;
    return map_type_string(chartype, false);
  }
  if (cls & (CL_STRUCTURE | CL_UNION | CL_SEQUENCE | CL_ARRAY | CL_ENUM | CL_FIXED)) {
    return scoped(type->name());
  }
  if (cls & CL_INTERFACE) {
    return scoped(type->name()) + "_var";
  }
  return "<<unknown>>";
}

std::string GeneratorBase::map_type(AST_Field* field)
{
  FieldInfo af(*field);
  return (af.type_->anonymous() && af.as_base_) ? af.type_name_ : map_type(af.type_);
}

std::string GeneratorBase::map_type_string(AST_PredefinedType::PredefinedType chartype, bool constant)
{
  return primtype_[chartype] + '*' + (constant ? " const" : "");
}

void GeneratorBase::add_fixed_include()
{
}

std::string GeneratorBase::map_type(AST_Expression::ExprType type)
{
  AST_PredefinedType::PredefinedType pt = AST_PredefinedType::PT_void;
  switch (type) {
#if OPENDDS_HAS_EXPLICIT_INTS
  case AST_Expression::EV_int8: pt = AST_PredefinedType::PT_int8; break;
  case AST_Expression::EV_uint8: pt = AST_PredefinedType::PT_uint8; break;
#endif
  case AST_Expression::EV_short: pt = AST_PredefinedType::PT_short; break;
  case AST_Expression::EV_ushort: pt = AST_PredefinedType::PT_ushort; break;
  case AST_Expression::EV_long: pt = AST_PredefinedType::PT_long; break;
  case AST_Expression::EV_ulong: pt = AST_PredefinedType::PT_ulong; break;
  case AST_Expression::EV_longlong: pt = AST_PredefinedType::PT_longlong; break;
  case AST_Expression::EV_ulonglong: pt = AST_PredefinedType::PT_ulonglong; break;
  case AST_Expression::EV_float: pt = AST_PredefinedType::PT_float; break;
  case AST_Expression::EV_double: pt = AST_PredefinedType::PT_double; break;
  case AST_Expression::EV_longdouble: pt = AST_PredefinedType::PT_longdouble; break;
  case AST_Expression::EV_char: pt = AST_PredefinedType::PT_char; break;
  case AST_Expression::EV_wchar: pt = AST_PredefinedType::PT_wchar; break;
  case AST_Expression::EV_octet: pt = AST_PredefinedType::PT_octet; break;
  case AST_Expression::EV_bool: pt = AST_PredefinedType::PT_boolean; break;
  case AST_Expression::EV_string: pt = AST_PredefinedType::PT_char; break;
  case AST_Expression::EV_wstring: pt = AST_PredefinedType::PT_wchar; break;
#ifdef ACE_HAS_CDR_FIXED
  case AST_Expression::EV_fixed:
    add_fixed_include();
    return helpers_[HLP_FIXED_CONSTANT];
#endif
  default:
    be_util::misc_error_and_abort("Unhandled ExprType value in map_type");
  }

  if (type == AST_Expression::EV_string || type == AST_Expression::EV_wstring)
    return map_type_string(pt, true);

  return primtype_[pt];
}

void GeneratorBase::gen_simple_out(const char* nm)
{
  be_builtin_global->lang_header_ <<
    "typedef " << nm << "& " << nm << "_out;\n";
}

bool GeneratorBase::scoped_enum() { return false; }
std::string GeneratorBase::enum_base() { return ""; }

void GeneratorBase::struct_decls(UTL_ScopedName* name, AST_Type::SIZE_TYPE size, const char* struct_or_class)
{
  be_builtin_global->add_include("<tao/VarOut_T.h>", BE_BuiltinGlobalData::STREAM_LANG_H);
  const char* const nm = name->last_component()->get_string();
  be_builtin_global->lang_header_ <<
    struct_or_class << ' ' << nm << ";\n";
  switch (size) {
  case AST_Type::SIZE_UNKNOWN:
    be_builtin_global->lang_header_ << "/* Unknown size */\n";
    break;
  case AST_Type::FIXED:
    be_builtin_global->lang_header_ <<
      "typedef " << helpers_[HLP_FIX_VAR] << '<' << nm << "> " << nm << "_var;\n" <<
      "typedef " << nm << "& " << nm << "_out;\n";
    break;
  case AST_Type::VARIABLE:
    be_builtin_global->lang_header_ <<
      "typedef " << helpers_[HLP_VAR_VAR] << '<' << nm << "> " << nm << "_var;\n" <<
      "typedef " << helpers_[HLP_OUT] << '<' << nm << "> " << nm << "_out;\n";
    break;
  }
}

std::string GeneratorBase::generateDefaultValue(AST_Union* the_union)
{
  std::stringstream first_label;
  AST_Union::DefaultValue dv;
  if (the_union->default_value(dv) == -1) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: generateDefaultValue::")
      ACE_TEXT(" computing default value failed\n")));
    return "";
  }

  switch (the_union->udisc_type ())
    {
#if OPENDDS_HAS_EXPLICIT_INTS
    case AST_Expression::EV_int8:
      first_label << signed(dv.u.char_val);
      break;
    case AST_Expression::EV_uint8:
      first_label << unsigned(dv.u.char_val);
      break;
#endif
    case AST_Expression::EV_short:
      first_label << dv.u.short_val;
      break;
    case AST_Expression::EV_ushort:
      first_label << dv.u.ushort_val;
      break;
    case AST_Expression::EV_long:
      first_label << dv.u.long_val;
      break;
    case AST_Expression::EV_ulong:
      first_label << dv.u.ulong_val;
      break;
    case AST_Expression::EV_char:
      first_label << (int)dv.u.char_val;
      break;
    case AST_Expression::EV_bool:
      first_label << (dv.u.bool_val == 0 ? "false" : "true");
      break;
    case AST_Expression::EV_enum:
      {
        if (generator_ != 0) {
          AST_Enum* e = dynamic_cast<AST_Enum*>(the_union->disc_type());
          generator_->enum_entry(e, the_union, dv, first_label);
        }
        break;
      }
    case AST_Expression::EV_longlong:
      first_label << dv.u.longlong_val;
      break;
    case AST_Expression::EV_ulonglong:
      first_label << dv.u.ulonglong_val;
      break;
    default:
      be_util::misc_error_and_abort("Unhandled ExprType value in generateDefaultValue");
    }

  return first_label.str();
}

  void GeneratorBase::GenerateUnionAccessors::operator()(AST_UnionBranch* branch)
  {
    const char* field_name = branch->local_name()->get_string();
    std::stringstream first_label;
    {
      AST_UnionLabel* label = branch->label(0);
      if (label->label_kind() == AST_UnionLabel::UL_default) {
        first_label << generateDefaultValue(the_union);
      } else if (discriminator->node_type() == AST_Decl::NT_enum) {
        first_label << getEnumLabel(label->label_val(), discriminator);
      } else {
        first_label << *label->label_val()->ev();
      }
    }

    AST_Type* field_type = branch->field_type();
    const std::string field_type_string = generator_->map_type(field_type);
    AST_Type* actual_field_type = resolveActualType(field_type);
    const Classification cls = classify(actual_field_type);
    if (cls & (CL_PRIMITIVE | CL_ENUM)) {
      be_builtin_global->lang_header_ <<
        "  void " << field_name << " (" << field_type_string << " x) {\n"
        "    _reset();\n"
        "    this->_u." << field_name << " = x;\n"
        "    _discriminator = " << first_label.str() << ";\n"
        "  }\n"
        "  " << field_type_string << ' ' << field_name << " () const {\n"
        "    return this->_u." << field_name << ";\n"
        "  }\n";
    } else if (cls & CL_STRING) {
      const std::string& primtype = (cls & CL_WIDE) ? primtype_[AST_PredefinedType::PT_wchar] : primtype_[AST_PredefinedType::PT_char];
      const std::string& helper = (cls & CL_WIDE) ? helpers_[HLP_WSTR_VAR] : helpers_[HLP_STR_VAR];
      be_builtin_global->lang_header_ <<
        "  void " << field_name << " (" << primtype << "* x) {\n"
        "    _reset();\n" <<
        "    this->_u." << field_name << " = x;\n"
        "    _discriminator = " << first_label.str() << ";\n"
        "  }\n"
        "  void " << field_name << " (const " << primtype << "* x) {\n"
        "    _reset();\n"
        "    this->_u." << field_name << " = " << generator_->string_ns() << "::string_dup(x);\n"
        "    _discriminator = " << first_label.str() << ";\n"
        "  }\n"
        "  void " << field_name << " (const " << helper << "& x) {\n"
        "    _reset();\n" <<
        "    this->_u." << field_name << " = " << generator_->string_ns() << "::string_dup(x.in());\n"
        "    _discriminator = " << first_label.str() << ";\n"
        "  }\n"
        "  const " << primtype << "* " << field_name << " () const {\n"
        "    return this->_u." << field_name << ";\n"
        "  }\n";
    } else if (cls & CL_ARRAY) {
      be_builtin_global->lang_header_ <<
        "  void " << field_name << " (" << field_type_string << " x) {\n"
        "    _reset();\n" <<
        "    this->_u." << field_name << " = " << field_type_string << "_dup(x);\n"
        "    _discriminator = " << first_label.str() << ";\n"
        "  }\n"
        "  " << field_type_string << "_slice* " << field_name << " () const {\n"
        "    return this->_u." << field_name << ";\n"
        "  }\n";
    } else if (cls & (CL_STRUCTURE | CL_UNION | CL_SEQUENCE | CL_FIXED)) {
      be_builtin_global->lang_header_ <<
        "  void " << field_name << " (const " << field_type_string << "& x) {\n"
        "    _reset();\n"
        "    this->_u." << field_name << " = new " << field_type_string << "(x);\n"
        "    _discriminator = " << first_label.str() << ";\n"
        "  }\n"
        "  const " << field_type_string << "& " << field_name << " () const {\n"
        "    return *this->_u." << field_name << ";\n"
        "  }\n"
        "  " << field_type_string << "& " << field_name << " () {\n"
        "    return *this->_u." << field_name << ";\n"
        "  }\n";
    } else {
      std::cerr << "Unsupported type for union element\n";
    }
  }

bool GeneratorBase::hasDefaultLabel (const std::vector<AST_UnionBranch*>& branches)
{
  for (std::vector<AST_UnionBranch*>::const_iterator pos = branches.begin(), limit = branches.end();
        pos != limit;
        ++pos) {
    AST_UnionBranch* branch = *pos;
    for (unsigned long j = 0; j < branch->label_list_length(); ++j) {
      AST_UnionLabel* label = branch->label(j);
      if (label->label_kind() == AST_UnionLabel::UL_default) {
        return true;
      }
    }
  }
  return false;
}

size_t GeneratorBase::countLabels (const std::vector<AST_UnionBranch*>& branches)
{
  size_t count = 0;

  for (std::vector<AST_UnionBranch*>::const_iterator pos = branches.begin(), limit = branches.end();
        pos != limit;
        ++pos) {
    count += (*pos)->label_list_length();
  }

  return count;
}

bool GeneratorBase::needsDefault (const std::vector<AST_UnionBranch*>& branches, AST_Type* discriminator)
{
  return !hasDefaultLabel(branches) && needSyntheticDefault(discriminator, countLabels(branches));
}

void GeneratorBase::generate_union_field(AST_UnionBranch* branch)
{
  AST_Type* field_type = branch->field_type();
  AST_Type* actual_field_type = resolveActualType(field_type);
  const Classification cls = classify(actual_field_type);
  const std::string lang_field_type = generator_->map_type(field_type);
  if (cls & (CL_PRIMITIVE | CL_ENUM)) {
    be_builtin_global->lang_header_ <<
      "    " << lang_field_type << ' ' << branch->local_name()->get_string() << ";\n";
  } else if (cls & CL_STRING) {
    const AST_PredefinedType::PredefinedType chartype = (cls & CL_WIDE)
      ? AST_PredefinedType::PT_wchar : AST_PredefinedType::PT_char;
    be_builtin_global->lang_header_ <<
      "    " << primtype_[chartype] << "* " << branch->local_name()->get_string() << ";\n";
  } else if (cls & CL_ARRAY) {
    be_builtin_global->lang_header_ <<
      "    " << lang_field_type << "_slice* " << branch->local_name()->get_string() << ";\n";
  } else if (cls & (CL_STRUCTURE | CL_UNION | CL_SEQUENCE | CL_FIXED)) {
    be_builtin_global->lang_header_ <<
      "    " << lang_field_type << "* " << branch->local_name()->get_string() << ";\n";
  } else {
    std::cerr << "Unsupported type for union element\n";
  }
}

std::string GeneratorBase::generateCopyCtor(const std::string&, const std::string& name, AST_Type* field_type,
                                            const std::string&, bool, Intro&,
                                            const std::string&)
{
  std::stringstream ss;
  AST_Type* actual_field_type = resolveActualType(field_type);
  const Classification cls = classify(actual_field_type);
  const std::string lang_field_type = generator_->map_type(field_type);
  if (cls & (CL_PRIMITIVE | CL_ENUM)) {
    ss <<
      "    this->_u." << name << " = other._u." << name << ";\n";
  } else if (cls & CL_STRING) {
    ss <<
      "    this->_u." << name << " = (other._u." << name << ") ? " << generator_->string_ns() << "::string_dup(other._u." << name << ") : 0 ;\n";
  } else if (cls & CL_ARRAY) {
    ss <<
      "    this->_u." << name << " = (other._u." << name << ") ? " << lang_field_type << "_dup(other._u." << name << ") : 0 ;\n";
  } else if (cls & (CL_STRUCTURE | CL_UNION | CL_SEQUENCE | CL_FIXED)) {
    ss <<
      "    this->_u." << name << " = (other._u." << name << ") ? new " << lang_field_type << "(*other._u." << name << ") : 0;\n";
  } else {
    std::cerr << "Unsupported type for union element\n";
  }

  return ss.str();
}

std::string GeneratorBase::generateAssign(const std::string&, const std::string& name, AST_Type* field_type,
                                          const std::string&, bool, Intro&,
                                          const std::string&)
{
  std::stringstream ss;
  AST_Type* actual_field_type = resolveActualType(field_type);
  const Classification cls = classify(actual_field_type);
  const std::string lang_field_type = generator_->map_type(field_type);
  if (cls & (CL_PRIMITIVE | CL_ENUM)) {
    ss <<
      "    this->_u." << name << " = other._u." << name << ";\n";
  } else if (cls & CL_STRING) {
    ss <<
      "    this->_u." << name << " = (other._u." << name << ") ? " << generator_->string_ns() << "::string_dup(other._u." << name << ") : 0 ;\n";
  } else if (cls & CL_ARRAY) {
    ss <<
      "    this->_u." << name << " = (other._u." << name << ") ? " << lang_field_type << "_dup(other._u." << name << ") : 0 ;\n";
  } else if (cls & (CL_STRUCTURE | CL_UNION | CL_SEQUENCE | CL_FIXED)) {
    ss <<
      "    this->_u." << name << " = (other._u." << name << ") ? new " << lang_field_type << "(*other._u." << name << ") : 0;\n";
  } else {
    std::cerr << "Unsupported type for union element\n";
  }

  return ss.str();
}

std::string GeneratorBase::generateEqual(const std::string&, const std::string& name, AST_Type* field_type,
                                          const std::string&, bool, Intro&,
                                          const std::string&)
{
  std::stringstream ss;

  AST_Type* actual_field_type = resolveActualType(field_type);
  const Classification cls = classify(actual_field_type);
  if (cls & (CL_PRIMITIVE | CL_ENUM)) {
    ss <<
      "    return this->_u." << name << " == rhs._u." << name << ";\n";
  } else if (cls & CL_STRING) {
    ss <<
      "    return std::strcmp (this->_u." << name << ", rhs._u." << name << ") == 0 ;\n";
  } else if (cls & CL_ARRAY) {
    // TODO
    ss <<
      "    return false;\n";
  } else if (cls & (CL_STRUCTURE | CL_UNION | CL_SEQUENCE | CL_FIXED)) {
    ss <<
      "    return *this->_u." << name << " == *rhs._u." << name << ";\n";
  } else {
    std::cerr << "Unsupported type for union element\n";
  }

  return ss.str();
}

std::string GeneratorBase::generateReset(const std::string&, const std::string& name, AST_Type* field_type,
                                          const std::string&, bool, Intro&,
                                          const std::string&)
{
  std::stringstream ss;

  AST_Type* actual_field_type = resolveActualType(field_type);
  const Classification cls = classify(actual_field_type);
  if (cls & (CL_PRIMITIVE | CL_ENUM)) {
    // Do nothing.
  } else if (cls & CL_STRING) {
    ss <<
      "    " << generator_->string_ns() << "::string_free(this->_u." << name << ");\n"
      "    this->_u." << name << " = 0;\n";
  } else if (cls & CL_ARRAY) {
    ss <<
      "    " << generator_->map_type(field_type) << "_free(this->_u." << name << ");\n"
      "    this->_u." << name << " = 0;\n";
  } else if (cls & (CL_STRUCTURE | CL_UNION | CL_SEQUENCE | CL_FIXED)) {
    ss <<
      "    delete this->_u." << name << ";\n"
      "    this->_u." << name << " = 0;\n";
  } else {
    std::cerr << "Unsupported type for union element\n";
  }

  return ss.str();
}

bool GeneratorBase::gen_union(AST_Union* u, UTL_ScopedName* name,
                              const std::vector<AST_UnionBranch*>& branches, AST_Type* discriminator)
{
  const ScopedNamespaceGuard namespaces(name, be_builtin_global->lang_header_);
  const char* const nm = name->last_component()->get_string();
  struct_decls(name, u->size_type(), "class");
  be_builtin_global->lang_header_ <<
    "\n"
    "class " << exporter() << nm << "\n"
    "{\n"
    " public:\n"
    "  typedef " << nm << "_var _var_type;\n"
    "  typedef " << nm << "_out _out_type;\n"
    "  " << nm << "();\n"
    "  " << nm << "(const " << nm << "&);\n"
    "  ~" << nm << "() { _reset(); };\n"
    "  " << nm << "& operator=(const " << nm << "&);\n"
    "  void _d(" << scoped(discriminator->name()) << " d) { _discriminator = d; }\n"
    "  " << scoped(discriminator->name()) << " _d() const { return _discriminator; }\n";

  std::for_each (branches.begin(), branches.end(), GenerateUnionAccessors(u, discriminator));

  if (needsDefault(branches, discriminator)) {
    be_builtin_global->lang_header_ <<
      "  void _default() {\n"
      "    _reset();\n"
      "    _discriminator = " << generateDefaultValue(u) << ";\n"
      "  }\n";
  }

  be_builtin_global->lang_header_ <<
    "  bool operator==(const " << nm << "& rhs) const;\n"
    "  bool operator!=(const " << nm << "& rhs) const { return !(*this == rhs); }\n"
    "  OPENDDS_POOL_ALLOCATION_HOOKS\n";

  be_builtin_global->lang_header_ <<
    " private:\n"
    "  " << scoped(discriminator->name()) << " _discriminator;\n"
    "  union {\n";

  std::for_each (branches.begin(), branches.end(), generate_union_field);

  be_builtin_global->lang_header_ <<
    "  } _u;\n";

  be_builtin_global->lang_header_ <<
    "  void _reset();\n"
    "};\n\n";

  be_builtin_global->add_include("dds/DCPS/PoolAllocationBase.h", BE_BuiltinGlobalData::STREAM_LANG_H);
  be_builtin_global->add_include("<ace/CDR_Stream.h>", BE_BuiltinGlobalData::STREAM_LANG_H);

  be_builtin_global->lang_header_ <<
    exporter() << "ACE_CDR::Boolean operator<< (ACE_OutputCDR& os, const " << nm << "& x);\n\n";
  be_builtin_global->lang_header_ <<
    exporter() << "ACE_CDR::Boolean operator>> (ACE_InputCDR& os, " << nm << "& x);\n\n";

  {
    const ScopedNamespaceGuard guard(name, be_builtin_global->impl_);

    be_builtin_global->impl_ <<
      nm << "::" << nm << "() { std::memset (this, 0, sizeof (" << nm << ")); }\n\n";

    be_builtin_global->impl_ <<
      nm << "::" << nm << "(const " << nm << "& other)\n"
      "{\n"
      "  this->_discriminator = other._discriminator;\n";
    generateSwitchForUnion(u, "this->_discriminator", generateCopyCtor, branches, discriminator, "", "", "", false, false);
    be_builtin_global->impl_ <<
      "}\n\n";

    be_builtin_global->impl_ <<
      nm << "& " << nm << "::operator=(const " << nm << "& other)\n"
      "{\n" <<
      "  if (this == &other) {\n"
      "    return *this;\n"
      "  }\n\n"
      "  _reset();\n"
      "  this->_discriminator = other._discriminator;\n";
    generateSwitchForUnion(u, "this->_discriminator", generateAssign, branches, discriminator, "", "", "", false, false);
    be_builtin_global->impl_ <<
      "  return *this;\n"
      "}\n\n";

    be_builtin_global->impl_ <<
      "bool " << nm << "::operator==(const " << nm << "& rhs) const\n"
      "{\n"
      "  if (this->_discriminator != rhs._discriminator) return false;\n";
    if (generateSwitchForUnion(u, "this->_discriminator", generateEqual, branches, discriminator, "", "", "", false, false)) {
      be_builtin_global->impl_ <<
        "  return false;\n";
    }
    be_builtin_global->impl_ <<
      "}\n\n";

    be_builtin_global->impl_ <<
      "void " << nm << "::_reset()\n"
      "{\n";
    generateSwitchForUnion(u, "this->_discriminator", generateReset, branches, discriminator, "", "", "", false, false);
    be_builtin_global->impl_ <<
      "}\n\n";

    be_builtin_global->impl_ <<
      "ACE_CDR::Boolean operator<<(ACE_OutputCDR&, const " << nm << "&) { return true; }\n\n";
    be_builtin_global->impl_ <<
      "ACE_CDR::Boolean operator>>(ACE_InputCDR&, " << nm << "&) { return true; }\n\n";
  }

  generator_->gen_typecode(name);
  return true;
}

void GeneratorBase::gen_array(UTL_ScopedName* tdname, AST_Array* arr)
{
  be_builtin_global->add_include("<tao/Array_VarOut_T.h>", BE_BuiltinGlobalData::STREAM_LANG_H);
  be_builtin_global->add_include("dds/DCPS/SafetyProfilePool.h", BE_BuiltinGlobalData::STREAM_LANG_H);
  const char* const nm = tdname->last_component()->get_string();
  AST_Type* elem = arr->base_type();
  const Classification elem_cls = classify(elem);
  const Helper var = (elem->size_type() == AST_Type::VARIABLE)
    ? HLP_ARR_VAR_VAR : HLP_ARR_FIX_VAR,
    out = HLP_ARR_OUT,
    forany = HLP_ARR_FORANY;

  std::ostringstream bound, nofirst, total;
  const std::string zeros = array_zero_indices(arr);
  for (ACE_CDR::ULong dim = 0; dim < arr->n_dims(); ++dim) {
    const ACE_CDR::ULong extent = arr->dims()[dim]->ev()->u.ulval;
    bound << '[' << extent << ']';
    if (dim) {
      nofirst << '[' << extent << ']';
      total << " * ";
    }
    total << extent;
  }

  std::string elem_type = map_type(elem);
  if (elem_cls & CL_STRING) {
    elem_type = helpers_[(elem_cls & CL_WIDE) ? HLP_WSTR_MGR : HLP_STR_MGR];
  }

  std::string out_type = nm;
  if (elem->size_type() == AST_Type::VARIABLE) {
    out_type = helpers_[out] + '<' + nm + ", " + nm + "_var, " + nm +
      "_slice, " + nm + "_tag>";
  }

  be_builtin_global->lang_header_ <<
    "typedef " << elem_type << ' ' << nm << bound.str() << ";\n"
    "typedef " << elem_type << ' ' << nm << "_slice" << nofirst.str() << ";\n"
    "struct " << nm << "_tag {};\n"
    "typedef " << helpers_[var] << '<' << nm << ", " << nm << "_slice, "
    << nm << "_tag> " << nm << "_var;\n"
    "typedef " << out_type << ' ' << nm << "_out;\n"
    "typedef " << helpers_[forany] << '<' << nm << ", " << nm << "_slice, "
    << nm << "_tag> " << nm << "_forany;\n\n" <<
    exporter() << nm << "_slice* " << nm << "_alloc();\n" <<
    exporter() << "void " << nm << "_init_i(" << elem_type << "* begin);\n" <<
    exporter() << "void " << nm << "_fini_i(" << elem_type << "* begin);\n" <<
    exporter() << "void " << nm << "_free(" << nm << "_slice* slice);\n" <<
    exporter() << nm << "_slice* " << nm << "_dup(const " << nm
    << "_slice* slice);\n" <<
    exporter() << "void " << nm << "_copy(" << nm << "_slice* dst, const "
    << nm << "_slice* src);\n\n";
  const ScopedNamespaceGuard namespaces(tdname, be_builtin_global->impl_);
  be_builtin_global->impl_ <<
    nm << "_slice* " << nm << "_alloc()\n"
    "{\n"
    "  void* const raw = ACE_Allocator::instance()->malloc"
    "(sizeof(" << nm << "));\n"
    "  " << nm << "_slice* const slice = static_cast<" << nm << "_slice*"
    << ">(raw);\n"
    "  " << nm << "_init_i(slice" << zeros << ");\n"
    "  return slice;\n"
    "}\n\n"
    "void " << nm << "_init_i(" << elem_type << "* begin)\n"
    "{\n";

  if (elem_cls & (CL_PRIMITIVE | CL_ENUM)) {
    be_builtin_global->impl_ << "  ACE_UNUSED_ARG(begin);\n";
  } else if (elem_cls & CL_ARRAY) {
    std::string indent = "  ";
    const NestedForLoops nfl("ACE_CDR::ULong", "i", arr, indent);
    be_builtin_global->impl_ <<
      indent << elem_type << "_init_i(begin" << nfl.index_ << ");\n";
  } else {
    be_builtin_global->impl_ <<
      "  std::uninitialized_fill_n(begin, " << total.str() << ", "
      << elem_type << "());\n";
  }

  be_builtin_global->impl_ <<
    "}\n\n"
    "void " << nm << "_fini_i(" << elem_type << "* begin)\n"
    "{\n";

  if (elem_cls & (CL_PRIMITIVE | CL_ENUM)) {
    be_builtin_global->impl_ << "  ACE_UNUSED_ARG(begin);\n";
  } else if (elem_cls & CL_ARRAY) {
    std::string indent = "  ";
    const NestedForLoops nfl("ACE_CDR::ULong", "i", arr, indent);
    be_builtin_global->impl_ <<
      indent << elem_type << "_fini_i(begin" << nfl.index_ << ");\n";
  } else {
    const std::string::size_type idx_last = elem_type.rfind("::");
    const std::string elem_last =
      (elem_cls & CL_STRING) ? "StringManager" :
      ((idx_last == std::string::npos) ? elem_type
        : elem_type.substr(idx_last + 2));
    be_builtin_global->impl_ <<
      "  for (int i = 0; i < " << total.str() << "; ++i) {\n"
      "    begin[i]."
#ifdef __SUNPRO_CC
      << elem_type << "::"
#endif
      "~" << elem_last << "();\n"
      "  }\n";
  }

  be_builtin_global->impl_ <<
    "}\n\n"
    "void " << nm << "_free(" << nm << "_slice* slice)\n"
    "{\n"
    "  if (!slice) return;\n"
    "  " << nm << "_fini_i(slice" << zeros << ");\n"
    "  ACE_Allocator::instance()->free(slice);\n"
    "}\n\n" <<
    nm << "_slice* " << nm << "_dup(const " << nm << "_slice* slice)\n"
    "{\n"
    "  " << nm << "_slice* const arr = " << nm << "_alloc();\n"
    "  if (arr) " << nm << "_copy(arr, slice);\n"
    "  return arr;\n"
    "}\n\n"
    "void " << nm << "_copy(" << nm << "_slice* dst, const " << nm
    << "_slice* src)\n"
    "{\n"
    "  if (!src || !dst) return;\n";

  {
    std::string indent = "  ";
    const NestedForLoops nfl("ACE_CDR::ULong", "i", arr, indent);
    if (elem_cls & CL_ARRAY) {
      be_builtin_global->impl_ <<
        indent << elem_type << "_copy(dst" << nfl.index_ << ", src"
        << nfl.index_ << ");\n";
    } else {
      be_builtin_global->impl_ <<
        indent << "dst" << nfl.index_ << " = src" << nfl.index_ << ";\n";
    }
  }

  be_builtin_global->impl_ <<
    "}\n\n";

  be_builtin_global->lang_header_ <<
    "inline ACE_CDR::Boolean operator<<(ACE_OutputCDR &, const " << nm << "_forany&) { return true; }\n\n"
    "inline ACE_CDR::Boolean operator>>(ACE_InputCDR &, " << nm << "_forany&) { return true; }\n\n";
}

// Outside of user's namespace: add Traits for arrays so that they can be
// used in Sequences and Array_var/_out/_forany.
void GeneratorBase::gen_array_traits(UTL_ScopedName* tdname, AST_Array* arr)
{
  const std::string nm = scoped(tdname);
  const std::string zeros = array_zero_indices(arr);
  be_builtin_global->lang_header_ <<
    "TAO_BEGIN_VERSIONED_NAMESPACE_DECL\nnamespace TAO {\n"
    "template <>\n"
    "struct " << exporter() << "Array_Traits<" << nm << "_forany>\n"
    "{\n"
    "  static void free(" << nm << "_slice* slice)\n"
    "  {\n"
    "    " << nm << "_free(slice);\n"
    "  }\n\n"
    "  static " << nm << "_slice* dup(const " << nm << "_slice* slice)\n"
    "  {\n"
    "    return " << nm << "_dup(slice);\n"
    "  }\n\n"
    "  static void copy(" << nm << "_slice* dst, const " << nm
    << "_slice* src)\n"
    "  {\n"
    "    " << nm << "_copy(dst, src);\n"
    "  }\n\n"
    "  static " << nm << "_slice* alloc()\n"
    "  {\n"
    "    return " << nm << "_alloc();\n"
    "  }\n\n"
    "  static void zero(" << nm << "_slice* slice)\n"
    "  {\n"
    "    " << nm << "_fini_i(slice" << zeros << ");\n"
    "    " << nm << "_init_i(slice" << zeros << ");\n"
    "  }\n"
    "  static void construct(" << nm << "_slice* slice)\n"
    "  {\n"
    "    " << nm << "_init_i(slice" << zeros << ");\n"
    "  }\n"
    "  static void destroy(" << nm << "_slice* slice)\n"
    "  {\n"
    "    " << nm << "_fini_i(slice" << zeros << ");\n"
    "  }\n"
    "};\n}\nTAO_END_VERSIONED_NAMESPACE_DECL\n\n";
}

void GeneratorBase::gen_array_typedef(const char* nm, AST_Type* base)
{
  be_builtin_global->lang_header_ <<
    "typedef " << map_type(base) << "_var " << nm << "_var;\n" <<
    "typedef " << map_type(base) << "_slice " << nm << "_slice;\n" <<
    "typedef " << map_type(base) << "_forany " << nm << "_forany;\n\n" <<
    "inline " << nm << "_slice *" << nm << "_alloc() { return " << map_type(base) << "_alloc(); }\n" <<
    "inline " << nm << "_slice* " << nm << "_dup(" << nm << "_slice *a) { return " << map_type(base) << "_dup(a); }\n" <<
    "inline void " << nm << "_copy(" << nm << "_slice* to, const " << nm << "_slice* from) { " << map_type(base) << "_copy(to, from); }\n" <<
    "inline void " << nm << "_free(" << nm << "_slice *a) { " << map_type(base) << "_free(a); }\n";
}

void GeneratorBase::gen_typedef_varout(const char* nm, AST_Type* base)
{
  const Classification cls = classify(base);
  if (cls & CL_STRING) {
    const Helper var = (cls & CL_WIDE) ? HLP_WSTR_VAR : HLP_STR_VAR,
      out = (cls & CL_WIDE) ? HLP_WSTR_OUT : HLP_STR_OUT;
    be_builtin_global->lang_header_ <<
      "typedef " << helpers_[var] << ' ' << nm << "_var;\n"
      "typedef " << helpers_[out] << ' ' << nm << "_out;\n";
  } else {
    be_builtin_global->lang_header_ <<
      "typedef " << generator_->map_type(base) << "_out " << nm << "_out;\n";
  }
}

void GeneratorBase::gen_fixed(UTL_ScopedName* name, AST_Fixed* fixed)
{
#ifdef ACE_HAS_CDR_FIXED
  add_fixed_include();
  const char* const nm = name->last_component()->get_string();
  be_builtin_global->lang_header_ <<
    "typedef " << helpers_[HLP_FIXED] << '<' << *fixed->digits()->ev()
    << ", " << *fixed->scale()->ev() << "> " << nm << ";\n"
    "typedef " << nm << "& " << nm << "_out;\n";
#endif
}

const char* GeneratorBase::generate_ts(AST_Decl* decl, UTL_ScopedName* name)
{
  // There is no error, so return nothing.
  return 0;
}

void langmap_generator::init()
{
  generator_ = be_builtin_global->language_mapping()->getGeneratorHelper();
  if (generator_ != 0) {
    generator_->init();
  }
}

bool langmap_generator::gen_const(UTL_ScopedName* name, bool,
                                  AST_Constant* constant)
{
  const ScopedNamespaceGuard namespaces(name, be_builtin_global->lang_header_);
  const char* const nm = name->last_component()->get_string();

  const AST_Expression::ExprType type = constant->et();
  const bool is_enum = (type == AST_Expression::EV_enum);
  const std::string type_name = is_enum
    ? scoped(constant->enum_full_name()) : generator_->map_type(type);
  be_builtin_global->lang_header_ <<
    generator_->const_keyword(type) << ' ' << type_name << ' ' << nm << " = ";

  if (is_enum) {
    UTL_ScopedName* const enumerator = constant->constant_value()->n();
    if (generator_->scoped_enum()) {
      be_builtin_global->lang_header_ << type_name << "::"
        << to_string(enumerator->last_component()) << ";\n";
    } else {
      be_builtin_global->lang_header_ << dds_generator::scoped_helper(enumerator, "::") << ";\n";
    }
  } else {
    be_builtin_global->lang_header_ << *constant->constant_value()->ev() << ";\n";
  }
  return true;
}

bool langmap_generator::gen_enum(AST_Enum*, UTL_ScopedName* name,
                                 const std::vector<AST_EnumVal*>& contents,
                                 const char*)
{
  const ScopedNamespaceGuard namespaces(name, be_builtin_global->lang_header_);
  const char* const nm = name->last_component()->get_string();
  const char* scoped_enum = generator_->scoped_enum() ? "class " : "";
  const std::string enum_base = generator_->enum_base();
  be_builtin_global->lang_header_ <<
    "enum " << scoped_enum << nm << enum_base << " {\n";
  for (size_t i = 0; i < contents.size(); ++i) {
    be_builtin_global->lang_header_ <<
      "  " << contents[i]->local_name()->get_string()
      << ((i < contents.size() - 1) ? ",\n" : "\n");
  }
  be_builtin_global->lang_header_ <<
    "};\n\n";
  generator_->gen_simple_out(nm);
  generator_->gen_typecode(name);
  return true;
}

bool langmap_generator::gen_struct_fwd(UTL_ScopedName* name,
                                       AST_Type::SIZE_TYPE size)
{
  const ScopedNamespaceGuard namespaces(name, be_builtin_global->lang_header_);
  generator_->struct_decls(name, size);
  return true;
}

bool langmap_generator::gen_struct(AST_Structure* s, UTL_ScopedName* name,
                                   const std::vector<AST_Field*>& fields,
                                   AST_Type::SIZE_TYPE size,
                                   const char* x)
{
  return generator_->gen_struct(s, name, fields, size, x);
}

bool langmap_generator::gen_typedef(AST_Typedef*, UTL_ScopedName* name, AST_Type* base,
                                    const char*)
{
  AST_Array* arr = 0;
  {
    const ScopedNamespaceGuard namespaces(name, be_builtin_global->lang_header_);
    const char* const nm = name->last_component()->get_string();

    switch (base->node_type()) {
    case AST_Decl::NT_sequence:
      generator_->gen_sequence(name, dynamic_cast<AST_Sequence*>(base));
      break;
    case AST_Decl::NT_array:
      generator_->gen_array(name, arr = dynamic_cast<AST_Array*>(base));
      break;
    case AST_Decl::NT_fixed:
# ifdef ACE_HAS_CDR_FIXED
      generator_->gen_fixed(name, dynamic_cast<AST_Fixed*>(base));
      break;
# else
      std::cerr << "ERROR: fixed data type (for " << nm << ") is not supported"
        " with this version of ACE+TAO\n";
      return false;
# endif
    default:
       be_builtin_global->lang_header_
         << generator_->get_typedef(nm, generator_->map_type(base)) << ";\n";

      generator_->gen_typedef_varout(nm, base);

      AST_Type* actual_base = resolveActualType(base);
      if (actual_base->node_type() == AST_Decl::NT_array) {
        generator_->gen_array_typedef(nm, base);
      }

      break;
    }

    generator_->gen_typecode(name);
  }
  if (arr) generator_->gen_array_traits(name, arr);
  return true;
}

bool langmap_generator::gen_union_fwd(AST_UnionFwd* node,
                                      UTL_ScopedName* name,
                                      AST_Type::SIZE_TYPE)
{
  const ScopedNamespaceGuard namespaces(name, be_builtin_global->lang_header_);
  generator_->struct_decls(name, node->full_definition()->size_type(), "class");
  return true;
}

bool langmap_generator::gen_union(AST_Union* u, UTL_ScopedName* name,
                                  const std::vector<AST_UnionBranch*>& branches,
                                  AST_Type* discriminator,
                                  const char*)
{
  return generator_->gen_union(u, name, branches, discriminator);
}

bool langmap_generator::gen_interf_fwd(UTL_ScopedName* name)
{
  return generator_->gen_interf_fwd(name);
}
