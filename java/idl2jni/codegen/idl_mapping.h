/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef idl2jni_idl_mapping_H
#define idl2jni_idl_mapping_H

#include "utl_scoped_name.h"
#include "ast.h"

#include <string>
#include <vector>

class idl_mapping {
public:
  virtual ~idl_mapping() = 0;

  virtual bool gen_const(UTL_ScopedName *name, bool nestedInInteface,
                         AST_Expression::ExprType type, AST_Expression::AST_ExprValue *value) = 0;

  virtual bool gen_enum(UTL_ScopedName *name,
                        const std::vector<AST_EnumVal *> &contents, const char *repoid) = 0;

  virtual bool gen_struct(UTL_ScopedName *name,
                          const std::vector<AST_Field *> &fields, const char *repoid) = 0;

  virtual bool gen_typedef(UTL_ScopedName *name, AST_Type *base,
                           const char *repoid) = 0;

  virtual bool gen_interf(UTL_ScopedName *name, bool local,
                          const std::vector<AST_Interface *> &inherits,
                          const std::vector<AST_Interface *> &inherits_flat,
                          const std::vector<AST_Attribute *> &attrs,
                          const std::vector<AST_Operation *> &ops, const char *repoid) = 0;

  virtual bool gen_interf_fwd(UTL_ScopedName *name) = 0;

  virtual bool gen_native(UTL_ScopedName *name, const char *repoid) = 0;

  virtual bool gen_union(UTL_ScopedName *name,
                         const std::vector<AST_UnionBranch *> &branches, AST_Type *discriminator,
                         AST_Expression::ExprType udisc_type,
                         const AST_Union::DefaultValue &default_value, const char *repoid) = 0;

  static std::string scoped_helper(UTL_ScopedName *sn, const char *sep,
                                   bool omit_local = false);
};

class composite_mapping : public idl_mapping {
public:
  bool gen_const(UTL_ScopedName *name, bool nestedInInteface,
                 AST_Expression::ExprType type, AST_Expression::AST_ExprValue *value);

  bool gen_enum(UTL_ScopedName *name,
                const std::vector<AST_EnumVal *> &contents, const char *repoid);

  bool gen_struct(UTL_ScopedName *name,
                  const std::vector<AST_Field *> &fields, const char *repoid);

  bool gen_typedef(UTL_ScopedName *name, AST_Type *base, const char *repoid);

  bool gen_interf(UTL_ScopedName *name, bool local,
                  const std::vector<AST_Interface *> &inherits,
                  const std::vector<AST_Interface *> &inherits_flat,
                  const std::vector<AST_Attribute *> &attrs,
                  const std::vector<AST_Operation *> &ops, const char *repoid);

  bool gen_interf_fwd(UTL_ScopedName *name);

  bool gen_native(UTL_ScopedName *name, const char *repoid);

  bool gen_union(UTL_ScopedName *name,
                 const std::vector<AST_UnionBranch *> &branches, AST_Type *discriminator,
                 AST_Expression::ExprType udisc_type,
                 const AST_Union::DefaultValue &default_value, const char *repoid);

  template <typename InputIterator>
  composite_mapping(InputIterator begin, InputIterator end)
  : components_(begin, end) {}

private:
  std::vector<idl_mapping *> components_;
};

class idl_mapping_java : public idl_mapping {
public:
  static std::string scoped(UTL_ScopedName *name);
  static std::string type(AST_Type *decl);

  bool gen_const(UTL_ScopedName *name, bool nestedInInteface,
                 AST_Expression::ExprType type, AST_Expression::AST_ExprValue *value);

  bool gen_enum(UTL_ScopedName *name,
                const std::vector<AST_EnumVal *> &contents, const char *repoid);

  bool gen_struct(UTL_ScopedName *name,
                  const std::vector<AST_Field *> &fields, const char *repoid);

  bool gen_typedef(UTL_ScopedName *name, AST_Type *base, const char *repoid);

  bool gen_interf(UTL_ScopedName *name, bool local,
                  const std::vector<AST_Interface *> &inherits,
                  const std::vector<AST_Interface *> &inherits_flat,
                  const std::vector<AST_Attribute *> &attrs,
                  const std::vector<AST_Operation *> &ops, const char *repoid);

  bool gen_interf_fwd(UTL_ScopedName *) {
    return true;
  }

  bool gen_native(UTL_ScopedName *name, const char *repoid);

  bool gen_union(UTL_ScopedName *name,
                 const std::vector<AST_UnionBranch *> &branches, AST_Type *discriminator,
                 AST_Expression::ExprType udisc_type,
                 const AST_Union::DefaultValue &default_value, const char *repoid);
};

class idl_mapping_jni : public idl_mapping {
public:
  static std::string scoped(UTL_ScopedName *name, bool omit_local = false);
  static std::string type(AST_Type *decl);   //"jint", "jlong", etc.
  static std::string jvmSignature(AST_Type *decl);  //"I", "J", etc.
  static std::string jniFnName(AST_Type *decl);   //"Int", "Long", etc.
  static std::string taoType(AST_Type *decl);
  static std::string taoParam(AST_Type *decl, AST_Argument::Direction dir,
                              bool return_type = false);

  bool gen_const(UTL_ScopedName *, bool,
                 AST_Expression::ExprType, AST_Expression::AST_ExprValue *) {
    return true;
  }

  bool gen_enum(UTL_ScopedName *name,
                const std::vector<AST_EnumVal *> &contents, const char *repoid);

  bool gen_struct(UTL_ScopedName *name,
                  const std::vector<AST_Field *> &fields, const char *repoid);

  bool gen_typedef(UTL_ScopedName *name, AST_Type *base, const char *repoid);

  bool gen_interf(UTL_ScopedName *name, bool local,
                  const std::vector<AST_Interface *> &inherits,
                  const std::vector<AST_Interface *> &inherits_flat,
                  const std::vector<AST_Attribute *> &attrs,
                  const std::vector<AST_Operation *> &ops, const char *repoid);

  bool gen_interf_fwd(UTL_ScopedName *name);

  bool gen_native(UTL_ScopedName *name, const char *repoid);

  bool gen_union(UTL_ScopedName *name,
                 const std::vector<AST_UnionBranch *> &branches, AST_Type *discriminator,
                 AST_Expression::ExprType udisc_type,
                 const AST_Union::DefaultValue &default_value, const char *repoid);

private:
  bool gen_jarray_copies(UTL_ScopedName *name, const std::string &jvmSig,
                         const std::string &jniFn, const std::string &jniType,
                         const std::string &jniArrayType, const std::string &taoTypeName,
                         bool sequence, const std::string &length, bool elementIsObjref = false);
};

#endif
