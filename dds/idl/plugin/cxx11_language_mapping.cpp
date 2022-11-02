#include "cxx11_language_mapping.h"
#include "langmap_generator_helper.h"
#include "dds_generator.h"
#include "field_info.h"

using namespace AstTypeClassification;

class Cxx11Generator;
namespace {
  Cxx11Generator* generator_;
  std::string union_activate(const std::string& indent, const std::string& name,
                             AST_Type* type, const std::string& prefix,
                             bool wrap_nested_key_only, Intro& intro,
                             const std::string& last);

  std::string union_reset(const std::string& indent, const std::string& name,
                          AST_Type* type, const std::string& prefix,
                          bool wrap_nested_key_only, Intro& intro,
                          const std::string& last);
}

class Cxx11Generator : public GeneratorBase
{
public:
  virtual void enum_entry(AST_Enum* e, AST_Union* the_union, AST_Union::DefaultValue& dv, std::stringstream& first_label)
  {
    first_label << scoped(e->name()) << "::";
    UTL_ScopedName* default_name;
    if (dv.u.enum_val < static_cast<ACE_CDR::ULong>(e->member_count())) {
      default_name = e->value_to_name(dv.u.enum_val);
    }
    else {
      const Fields fields(the_union);
      AST_UnionBranch* ub = dynamic_cast<AST_UnionBranch*>(*(fields.begin()));
      AST_Expression::AST_ExprValue* ev = ub->label(0)->label_val()->ev();
      default_name = e->value_to_name(ev->u.eval);
    }
    first_label << default_name->last_component()->get_string();
  }

  virtual std::string get_typedef(const std::string& name, const std::string& type)
  {
    return "using " + name + " = " + type;
  }

  virtual bool gen_interf_fwd(UTL_ScopedName* name)
  {
    return true;
  }

  virtual void gen_typecode(UTL_ScopedName* name)
  {
  }

  void init()
  {
    be_builtin_global->add_include("<cstdint>", BE_BuiltinGlobalData::STREAM_LANG_H);
    be_builtin_global->add_include("<string>", BE_BuiltinGlobalData::STREAM_LANG_H);
    primtype_[AST_PredefinedType::PT_long] = "int32_t";
    primtype_[AST_PredefinedType::PT_ulong] = "uint32_t";
    primtype_[AST_PredefinedType::PT_longlong] = "int64_t";
    primtype_[AST_PredefinedType::PT_ulonglong] = "uint64_t";
    primtype_[AST_PredefinedType::PT_short] = "int16_t";
    primtype_[AST_PredefinedType::PT_ushort] = "uint16_t";
#if OPENDDS_HAS_EXPLICIT_INTS
    primtype_[AST_PredefinedType::PT_int8] = "int8_t";
    primtype_[AST_PredefinedType::PT_uint8] = "uint8_t";
#endif
    primtype_[AST_PredefinedType::PT_float] = "float";
    primtype_[AST_PredefinedType::PT_double] = "double";
    primtype_[AST_PredefinedType::PT_longdouble] = "long double";
    primtype_[AST_PredefinedType::PT_char] = "char";
    primtype_[AST_PredefinedType::PT_wchar] = "wchar_t";
    primtype_[AST_PredefinedType::PT_boolean] = "bool";
    primtype_[AST_PredefinedType::PT_octet] = "uint8_t";
    helpers_[HLP_STR_VAR] = "std::string";
    helpers_[HLP_STR_OUT] = "std::string";
    helpers_[HLP_WSTR_VAR] = "std::wstring";
    helpers_[HLP_WSTR_OUT] = "std::wstring";
    helpers_[HLP_STR_MGR] = "std::string";
    helpers_[HLP_WSTR_MGR] = "std::wstring";
    helpers_[HLP_FIX_VAR] = "<<fixed-size var>>";
    helpers_[HLP_VAR_VAR] = "<<variable-size var>>";
    helpers_[HLP_OUT] = "<<out>>";
    helpers_[HLP_SEQ] = "std::vector";
    helpers_[HLP_SEQ_NS] = "std";
    helpers_[HLP_SEQ_VAR_VAR] = "<<variable sequence var>>";
    helpers_[HLP_SEQ_FIX_VAR] = "<<fixed sequence var>>";
    helpers_[HLP_SEQ_OUT] = "<<sequence out>>";
    helpers_[HLP_ARR_VAR_VAR] = "<<variable array var>>";
    helpers_[HLP_ARR_FIX_VAR] = "<<fixed array var>>";
    helpers_[HLP_ARR_OUT] = "<<array out>>";
    helpers_[HLP_ARR_FORANY] = "<<array forany>>";
    helpers_[HLP_FIXED] = "IDL::Fixed_T";
    helpers_[HLP_FIXED_CONSTANT] = "IDL::Fixed_T";
  }

  static void gen_typecode_ptrs(const std::string& type)
  {
    if (!be_builtin_global->suppress_typecode()) {
      be_builtin_global->add_include("tao/Basic_Types.h", BE_BuiltinGlobalData::STREAM_LANG_H);
      be_builtin_global->lang_header_ << "extern const ::CORBA::TypeCode_ptr _tc_" << type << ";\n";
      be_builtin_global->impl_ << "const ::CORBA::TypeCode_ptr _tc_" << type << " = nullptr;\n";
    }
  }

  std::string map_type_string(AST_PredefinedType::PredefinedType chartype, bool)
  {
    return chartype == AST_PredefinedType::PT_char ? "std::string" : "std::wstring";
  }

  std::string const_keyword(AST_Expression::ExprType type)
  {
    switch (type) {
    case AST_Expression::EV_string:
    case AST_Expression::EV_wstring:
      return "const";
    default:
      return "constexpr";
    }
  }

  void gen_simple_out(const char*) {}

  bool scoped_enum() { return true; }
  std::string enum_base() { return " : uint32_t"; }

  void struct_decls(UTL_ScopedName* name, AST_Type::SIZE_TYPE, const char*)
  {
    be_builtin_global->lang_header_ <<
      "class " << name->last_component()->get_string() << ";\n";
  }

  static void gen_array(AST_Array* arr, const std::string& type, const std::string& elem, const std::string& ind = "")
  {
    std::string array;
    std::ostringstream bounds;
    for (ACE_CDR::ULong dim = arr->n_dims(); dim; --dim) {
      array += "std::array<";
      bounds << ", " << arr->dims()[dim - 1]->ev()->u.ulval << '>';
    }
    be_builtin_global->add_include("<array>", BE_BuiltinGlobalData::STREAM_LANG_H);
    be_builtin_global->lang_header_ << ind << "using " << type << " = " << array << elem << bounds.str() << ";\n";
  }

  void gen_array(UTL_ScopedName* tdname, AST_Array* arr)
  {
    gen_array(arr, tdname->last_component()->get_string(), map_type(arr->base_type()));
  }

  void gen_array_traits(UTL_ScopedName*, AST_Array*) {}
  void gen_array_typedef(const char*, AST_Type*) {}
  void gen_typedef_varout(const char*, AST_Type*) {}

  static void gen_sequence(const std::string& type, const std::string& elem, const std::string& ind = "")
  {
    be_builtin_global->add_include("<vector>", BE_BuiltinGlobalData::STREAM_LANG_H);
    be_builtin_global->lang_header_ << ind << "using " << type << " = std::vector<" << elem << ">;\n";
  }

  void gen_sequence(UTL_ScopedName* tdname, AST_Sequence* seq)
  {
    gen_sequence(tdname->last_component()->get_string(), map_type(seq->base_type()));
  }

  static void gen_common_strunion_pre(const char* nm)
  {
    be_builtin_global->lang_header_ <<
      "\n"
      "class " << exporter() << nm << "\n"
      "{\n"
      "public:\n\n";
  }

  static void gen_common_strunion_post(const char* nm)
  {
    be_builtin_global->lang_header_ <<
      "};\n\n"
      << exporter() << "void swap(" << nm << "& lhs, " << nm << "& rhs);\n\n";
  }

  void gen_struct_members(AST_Field* field)
  {
    FieldInfo af(*field);
    if (af.type_->anonymous() && af.as_base_) {
      const std::string elem_type = map_type(af.as_base_);
      if (af.arr_) {
        gen_array(af.arr_, af.type_name_, elem_type, "  ");
      }
      else if (af.seq_) {
        gen_sequence(af.type_name_, elem_type, "  ");
      }
    }

    const std::string lang_field_type = map_type(field);
    const std::string assign_pre = "{ _" + af.name_ + " = ",
      assign = assign_pre + "val; }\n",
      move = assign_pre + "std::move(val); }\n",
      ret = "{ return _" + af.name_ + "; }\n";
    std::string initializer;
    if (af.cls_ & (CL_PRIMITIVE | CL_ENUM)) {
      be_builtin_global->lang_header_ <<
        "  void " << af.name_ << '(' << lang_field_type << " val) " << assign <<
        "  " << lang_field_type << ' ' << af.name_ << "() const " << ret <<
        "  " << lang_field_type << "& " << af.name_ << "() " << ret;
      if (af.cls_ & CL_ENUM) {
        AST_Enum* enu = dynamic_cast<AST_Enum*>(af.act_);
        for (UTL_ScopeActiveIterator it(enu, UTL_Scope::IK_decls); !it.is_done(); it.next()) {
          if (it.item()->node_type() == AST_Decl::NT_enum_val) {
            initializer = '{' + map_type(af.type_)
              + "::" + it.item()->local_name()->get_string() + '}';
            break;
          }
        }
      }
      else {
        initializer = "{}";
      }
    }
    else {
      if (af.cls_ & CL_ARRAY) {
        initializer = "{}";
      }
      be_builtin_global->add_include("<utility>", BE_BuiltinGlobalData::STREAM_LANG_H);
      be_builtin_global->lang_header_ <<
        "  void " << af.name_ << "(const " << lang_field_type << "& val) " << assign <<
        "  void " << af.name_ << '(' << lang_field_type << "&& val) " << move <<
        "  const " << lang_field_type << "& " << af.name_ << "() const " << ret <<
        "  " << lang_field_type << "& " << af.name_ << "() " << ret;
    }
    be_builtin_global->lang_header_ <<
      "  " << lang_field_type << " _" << af.name_ << initializer << ";\n\n";
  }

  bool gen_struct(AST_Structure*, UTL_ScopedName* name,
    const std::vector<AST_Field*>& fields,
    AST_Type::SIZE_TYPE, const char*)
  {
    const ScopedNamespaceGuard namespaces(name, be_builtin_global->lang_header_);
    const ScopedNamespaceGuard namespaces2(name, be_builtin_global->impl_);
    const char* const nm = name->last_component()->get_string();
    gen_common_strunion_pre(nm);

    for (std::vector<AST_Field*>::const_iterator first = fields.begin();
         first != fields.end(); ++first) {
      gen_struct_members(*first);
    }

    be_builtin_global->lang_header_ <<
      "  " << nm << "() = default;\n"
      "  " << (fields.size() == 1 ? "explicit " : "") << nm << '(';
    be_builtin_global->impl_ <<
      nm << "::" << nm << '(';

    std::string init_list, swaps;
    for (size_t i = 0; i < fields.size(); ++i) {
      const std::string fn = fields[i]->local_name()->get_string();
      const std::string ft = map_type(fields[i]);
      const Classification cls = classify(fields[i]->field_type());
      const bool by_ref = (cls & (CL_PRIMITIVE | CL_ENUM)) == 0;
      const std::string param = (by_ref ? "const " : "") + ft + (by_ref ? "&" : "")
        + ' ' + fn + (i < fields.size() - 1 ? ",\n    " : ")");
      be_builtin_global->lang_header_ << param;
      be_builtin_global->impl_ << param;
      init_list += '_' + fn + '(' + fn + ')';
      if (i < fields.size() - 1) init_list += "\n  , ";
      swaps += "  swap(lhs._" + fn + ", rhs._" + fn + ");\n";
    }
    be_builtin_global->lang_header_ << ";\n\n";
    be_builtin_global->impl_ << "\n  : " << init_list << "\n{}\n\n";

    gen_common_strunion_post(nm);
    be_builtin_global->impl_ <<
      "void swap(" << nm << "& lhs, " << nm << "& rhs)\n"
      "{\n"
      "  using std::swap;\n"
      << swaps << "}\n\n";
    gen_typecode_ptrs(nm);
    return true;
  }

  void union_field(AST_UnionBranch* branch)
  {
    AST_Type* field_type = branch->field_type();
    const std::string lang_field_type = map_type(field_type);
    be_builtin_global->lang_header_ <<
      "    " << lang_field_type << " _" << branch->local_name()->get_string()
      << ";\n";
  }

  void union_accessors(AST_UnionBranch* branch)
  {
    AST_Type* field_type = branch->field_type();
    AST_Type* actual_field_type = resolveActualType(field_type);
    const std::string lang_field_type = map_type(field_type);
    const Classification cls = classify(actual_field_type);
    const char* nm = branch->local_name()->get_string();

    AST_UnionLabel* label = branch->label(0);
    AST_Union* union_ = dynamic_cast<AST_Union*>(branch->defined_in());
    AST_Type* dtype = resolveActualType(union_->disc_type());
    const std::string disc_type = map_type(dtype);

    std::string dval;
    if (label->label_kind() == AST_UnionLabel::UL_default) {
      dval = generateDefaultValue(union_);
    }
    else if (dtype->node_type() == AST_Decl::NT_enum) {
      dval = getEnumLabel(label->label_val(), dtype);
    }
    else {
      std::ostringstream strm;
      strm << *label->label_val()->ev();
      dval = strm.str();
    }

    std::string disc_param, disc_name = dval;
    if (label->label_kind() == AST_UnionLabel::UL_default ||
      branch->label_list_length() > 1) {
      disc_name = "disc";
      disc_param = ", " + disc_type + " disc = " + dval;
    }

    const std::string assign_pre = "{ _activate(" + disc_name + "); _"
      + std::string(nm) + " = ",
      assign = assign_pre + "val; }\n",
      move = assign_pre + "std::move(val); }\n",
      ret = "{ return _" + std::string(nm) + "; }\n";
    if (cls & (CL_PRIMITIVE | CL_ENUM)) {
      be_builtin_global->lang_header_ <<
        "  void " << nm << '(' << lang_field_type << " val" << disc_param
        << ") " << assign <<
        "  " << lang_field_type << ' ' << nm << "() const " << ret <<
        "  " << lang_field_type << "& " << nm << "() " << ret << "\n";
    }
    else {
      be_builtin_global->add_include("<utility>", BE_BuiltinGlobalData::STREAM_LANG_H);
      be_builtin_global->lang_header_ <<
        "  void " << nm << "(const " << lang_field_type << "& val" << disc_param
        << ") " << assign <<
        "  void " << nm << '(' << lang_field_type << "&& val" << disc_param
        << ") " << move <<
        "  const " << lang_field_type << "& " << nm << "() const " << ret <<
        "  " << lang_field_type << "& " << nm << "() " << ret << "\n";
    }
  }

  static std::string union_copy(const std::string&, const std::string& name, AST_Type*,
    const std::string&, bool, Intro&,
    const std::string&)
  {
    return "    _" + name + " = rhs._" + name + ";\n";
  }

  static std::string union_move(const std::string&, const std::string& name, AST_Type*,
    const std::string&, bool, Intro&,
    const std::string&)
  {
    return "    _" + name + " = std::move(rhs._" + name + ");\n";
  }

  static std::string union_assign(const std::string&, const std::string& name, AST_Type*,
    const std::string&, bool, Intro&,
    const std::string&)
  {
    return "    " + name + "(rhs._" + name + ");\n";
  }

  static std::string union_move_assign(const std::string&, const std::string& name, AST_Type*,
    const std::string&, bool, Intro&,
    const std::string&)
  {
    return "    " + name + "(std::move(rhs._" + name + "));\n";
  }

  std::string union_activate(const std::string&, const std::string& name, AST_Type* type,
    const std::string&, bool, Intro&,
    const std::string&)
  {
    AST_Type* actual_field_type = resolveActualType(type);
    const std::string lang_field_type = map_type(type);
    const Classification cls = classify(actual_field_type);
    if (!(cls & (CL_PRIMITIVE | CL_ENUM))) {
      return "    new(&_" + name + ") " + lang_field_type + ";\n";
    }
    return "";
  }

  std::string union_reset(const std::string&, const std::string& name, AST_Type* type,
    const std::string&, bool, Intro&,
    const std::string&)
  {
    AST_Type* actual_field_type = resolveActualType(type);
    const std::string lang_field_type = map_type(type);
    const Classification cls = classify(actual_field_type);
    if (cls & CL_STRING) {
      return "    _" + name + ".~basic_string();\n";
    }
    else if (!(cls & (CL_PRIMITIVE | CL_ENUM))) {
      const size_t idx = lang_field_type.rfind("::");
      const std::string dtor_name = (idx == std::string::npos) ? lang_field_type
        : lang_field_type.substr(idx + 2);
      return "    _" + name + ".~" + dtor_name + "();\n";
    }
    return "";
  }

  bool gen_union(AST_Union* u, UTL_ScopedName* name,
    const std::vector<AST_UnionBranch*>& branches, AST_Type* discriminator)
  {
    const ScopedNamespaceGuard namespaces(name, be_builtin_global->lang_header_);
    const char* const nm = name->last_component()->get_string();
    const std::string d_type = map_type(discriminator);
    const std::string defVal = generateDefaultValue(u);

    gen_common_strunion_pre(nm);

    be_builtin_global->lang_header_ <<
      "  " << nm << "() { _activate(" << defVal << "); }\n"
      "  " << nm << "(const " << nm << "& rhs);\n"
      "  " << nm << "(" << nm << "&& rhs);\n"
      "  " << nm << "& operator=(const " << nm << "& rhs);\n"
      "  " << nm << "& operator=(" << nm << "&& rhs);\n"
      "  ~" << nm << "() { _reset(); }\n\n"
      "  " << d_type << " _d() const { return _disc; }\n"
      "  void _d(" << d_type << " d) { _disc = d; }\n\n";

    for (std::vector<AST_UnionBranch*>::const_iterator first = branches.begin();
      first != branches.end(); ++first) {
      union_accessors(*first);
    }

    if (needsDefault(branches, discriminator)) {
      be_builtin_global->lang_header_ <<
        "  void _default() { _reset(); _activate(" << defVal << "); }\n\n";
    }

    be_builtin_global->lang_header_ <<
      "private:\n"
      "  bool _set = false;\n"
      "  " << d_type << " _disc;\n\n"
      "  union {\n";

    for (std::vector<AST_UnionBranch*>::const_iterator first = branches.begin();
      first != branches.end(); ++first) {
      union_field(*first);
    }

    be_builtin_global->lang_header_ <<
      "  };\n\n"
      "  void _activate(" << d_type << " d);\n"
      "  void _reset();\n";

    gen_common_strunion_post(nm);

    const ScopedNamespaceGuard namespacesCpp(name, be_builtin_global->impl_);
    be_builtin_global->impl_ <<
      nm << "::" << nm << "(const " << nm << "& rhs)\n"
      "{\n"
      "  _activate(rhs._disc);\n";
    generateSwitchForUnion(u, "_disc", union_copy, branches, discriminator, "", "", "", false, false);
    be_builtin_global->impl_ <<
      "}\n\n" <<
      nm << "::" << nm << '(' << nm << "&& rhs)\n"
      "{\n"
      "  _activate(rhs._disc);\n";
    generateSwitchForUnion(u, "_disc", union_move, branches, discriminator, "", "", "", false, false);
    be_builtin_global->impl_ <<
      "}\n\n" <<
      nm << "& " << nm << "::operator=(const " << nm << "& rhs)\n"
      "{\n"
      "  if (this == &rhs) {\n"
      "    return *this;\n"
      "  }\n";
    generateSwitchForUnion(u, "rhs._disc", union_assign, branches, discriminator, "", "", "", false, false);
    be_builtin_global->impl_ <<
      "  _disc = rhs._disc;\n"
      "  return *this;\n"
      "}\n\n" <<
      nm << "& " << nm << "::operator=(" << nm << "&& rhs)\n"
      "{\n"
      "  if (this == &rhs) {\n"
      "    return *this;\n"
      "  }\n";
    generateSwitchForUnion(u, "rhs._disc", union_move_assign, branches, discriminator, "", "", "", false, false);
    be_builtin_global->impl_ <<
      "  _disc = rhs._disc;\n"
      "  return *this;\n"
      "}\n\n" <<
      "void " << nm << "::_activate(" << d_type << " d)\n"
      "{\n"
      "  if (_set && d != _disc) {\n"
      "    _reset();\n"
      "  }\n";
    // Set the generator for the two static functions to the generator
    // object.  The generateSwitchForUnion() function calls for a static
    // function.
    generator_ = this;
    generateSwitchForUnion(u, "d", ::union_activate, branches, discriminator, "", "", "", false, false);
    be_builtin_global->impl_ <<
      "  _set = true;\n"
      "  _disc = d;\n"
      "}\n\n"
      "void " << nm << "::_reset()\n"
      "{\n"
      "  if (!_set) return;\n";
    generateSwitchForUnion(u, "_disc", ::union_reset, branches, discriminator, "", "", "", false, false);
    be_builtin_global->impl_ <<
      "  _set = false;\n"
      "}\n\n"
      "void swap(" << nm << "& lhs, " << nm << "& rhs)\n"
      "{\n"
      "  std::swap(lhs, rhs);\n"
      "}\n\n";

    gen_typecode_ptrs(nm);
    return true;
  }

};

namespace {
  std::string union_activate(const std::string& indent, const std::string& name,
                             AST_Type* type, const std::string& prefix,
                             bool wrap_nested_key_only, Intro& intro,
                             const std::string& last)
  {
    return generator_->union_activate(indent, name, type, prefix,
                                      wrap_nested_key_only, intro, last);
  }

  std::string union_reset(const std::string& indent, const std::string& name,
                          AST_Type* type, const std::string& prefix,
                          bool wrap_nested_key_only, Intro& intro,
                          const std::string& last)
  {
    return generator_->union_reset(indent, name, type, prefix,
                                   wrap_nested_key_only, intro, last);
  }
}

Cxx11LanguageMapping::~Cxx11LanguageMapping()
{
}

bool Cxx11LanguageMapping::cxx11() const
{
  return true;
}

bool Cxx11LanguageMapping::none() const
{
  return false;
}

bool Cxx11LanguageMapping::needSequenceTypeSupportImplHeader() const
{
  return true;
}

GeneratorBase* Cxx11LanguageMapping::getGeneratorHelper() const
{
  static Cxx11Generator generator;
  return &generator;
}
