/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef LANGMAP_GENERATOR_HELPER_H
#define LANGMAP_GENERATOR_HELPER_H

#include "opendds_idl_plugin_export.h"
#include <ast.h>
#include <ast_union_fwd.h>

#include <string>
#include <vector>
#include <map>

struct Intro;

class opendds_idl_plugin_Export GeneratorBase
{
public:
  virtual ~GeneratorBase();
  virtual void init() = 0;
  virtual void gen_sequence(UTL_ScopedName* tdname, AST_Sequence* seq) = 0;
  virtual bool gen_struct(AST_Structure* s, UTL_ScopedName* name, const std::vector<AST_Field*>& fields, AST_Type::SIZE_TYPE size, const char* x) = 0;

  virtual std::string string_ns();

  virtual void enum_entry(AST_Enum* e, AST_Union* the_union, AST_Union::DefaultValue& dv, std::stringstream& first_label);

  virtual std::string get_typedef(const std::string& name, const std::string& type);

  virtual bool gen_interf_fwd(UTL_ScopedName* name);

  virtual void gen_typecode(UTL_ScopedName* name);

  virtual std::string const_keyword(AST_Expression::ExprType);

  std::string map_type(AST_Type* type);

  std::string map_type(AST_Field* field);

  virtual std::string map_type_string(AST_PredefinedType::PredefinedType chartype, bool constant);

  virtual void add_fixed_include();

  std::string map_type(AST_Expression::ExprType type);

  virtual void gen_simple_out(const char* nm);

  virtual bool scoped_enum();
  virtual std::string enum_base();

  virtual void struct_decls(UTL_ScopedName* name, AST_Type::SIZE_TYPE size, const char* struct_or_class = "struct");

  static std::string generateDefaultValue(AST_Union* the_union);

  struct GenerateUnionAccessors
  {
    AST_Union* the_union;
    AST_Type* discriminator;

    GenerateUnionAccessors(AST_Union* u, AST_Type* d)
      : the_union(u)
      , discriminator(d)
    { }

    void operator()(AST_UnionBranch* branch);
  };

  static bool hasDefaultLabel(const std::vector<AST_UnionBranch*>& branches);

  static size_t countLabels(const std::vector<AST_UnionBranch*>& branches);

  static bool needsDefault(const std::vector<AST_UnionBranch*>& branches, AST_Type* discriminator);

  static void generate_union_field(AST_UnionBranch* branch);

  static std::string generateCopyCtor(const std::string&, const std::string& name, AST_Type* field_type,
                                      const std::string&, bool, Intro&, const std::string&);

  static std::string generateAssign(const std::string&, const std::string& name, AST_Type* field_type,
                                    const std::string&, bool, Intro&, const std::string&);

  static std::string generateEqual(const std::string&, const std::string& name, AST_Type* field_type,
                                   const std::string&, bool, Intro&, const std::string&);

  static std::string generateReset(const std::string&, const std::string& name, AST_Type* field_type,
                                   const std::string&, bool, Intro&, const std::string&);

  virtual bool gen_union(AST_Union* u, UTL_ScopedName* name,
                         const std::vector<AST_UnionBranch*>& branches,
                         AST_Type* discriminator);

  virtual void gen_array(UTL_ScopedName* tdname, AST_Array* arr);

  // Outside of user's namespace: add Traits for arrays so that they can be
  // used in Sequences and Array_var/_out/_forany.
  virtual void gen_array_traits(UTL_ScopedName* tdname, AST_Array* arr);

  virtual void gen_array_typedef(const char* nm, AST_Type* base);

  virtual void gen_typedef_varout(const char* nm, AST_Type* base);

  void gen_fixed(UTL_ScopedName* name, AST_Fixed* fixed);

  virtual const char* generate_ts(AST_Decl* decl, UTL_ScopedName* name);

protected:
  static std::string exporter();
  static std::string array_zero_indices(AST_Array* arr);
  static std::string array_dims(AST_Type* type, ACE_CDR::ULong& elems);

  static std::map<AST_PredefinedType::PredefinedType, std::string> primtype_;

  enum Helper {
    HLP_STR_VAR, HLP_STR_OUT, HLP_WSTR_VAR, HLP_WSTR_OUT,
    HLP_STR_MGR, HLP_WSTR_MGR,
    HLP_FIX_VAR, HLP_VAR_VAR, HLP_OUT,
    HLP_SEQ, HLP_SEQ_NS, HLP_SEQ_VAR_VAR, HLP_SEQ_FIX_VAR, HLP_SEQ_OUT,
    HLP_ARR_VAR_VAR, HLP_ARR_FIX_VAR, HLP_ARR_OUT, HLP_ARR_FORANY,
    HLP_FIXED, HLP_FIXED_CONSTANT
  };
  static std::map<Helper, std::string> helpers_;
};

#endif /* LANGMAP_GENERATOR_HELPER_H */
