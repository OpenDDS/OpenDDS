/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef marshal_generator_H
#define marshal_generator_H

#include "dds_generator.h"
#include "ace/Bound_Ptr.h"
#include <map>

class marshal_generator : public dds_generator {
public:
  marshal_generator();

  bool gen_enum(AST_Enum* node, UTL_ScopedName* name,
                const std::vector<AST_EnumVal*>& contents, const char* repoid);

  bool gen_struct(AST_Structure* node, UTL_ScopedName* name,
                  const std::vector<AST_Field*>& fields,
                  AST_Type::SIZE_TYPE size, const char* repoid);

  bool gen_typedef(AST_Typedef* node, UTL_ScopedName* name, AST_Type* base, const char* repoid);

  bool gen_union(AST_Union* node, UTL_ScopedName* name,
                 const std::vector<AST_UnionBranch*>& branches,
                 AST_Type* discriminator,
                 const char* repoid);

private:

  enum SPECIAL_T
  {
    SPECIAL_SEQUENCE,
    SPECIAL_STRUCT,
    SPECIAL_UNION,
  };

  struct is_special_case
  {
    typedef ACE_Strong_Bound_Ptr<is_special_case, ACE_Null_Mutex> ptr;

    virtual ~is_special_case() {}
    virtual bool operator()(const std::string& cxx) const = 0;
  };

  struct gen_special_case
  {
    typedef ACE_Strong_Bound_Ptr<gen_special_case, ACE_Null_Mutex> ptr;

    virtual ~gen_special_case() {}
    virtual bool operator()(const std::string& cxx) = 0;
  };

  struct special_case
  {
    is_special_case::ptr check;
    gen_special_case::ptr gen;
  };

  std::map<SPECIAL_T, std::vector<special_case> > special_case_handlers;

};

#endif
