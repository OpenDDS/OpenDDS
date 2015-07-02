// -*- C++ -*-
/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef itl_generator_H
#define itl_generator_H

#include "dds_generator.h"

class itl_generator : public dds_generator {
public:
  itl_generator()
    : level_(0)
    , count_(0)
  {}

  bool do_included_files() const { return true; }

  void gen_prologue();
  void gen_epilogue();

  bool gen_enum(AST_Enum*, UTL_ScopedName* name,
                const std::vector<AST_EnumVal*>& contents, const char* repoid);

  bool gen_struct(AST_Structure*, UTL_ScopedName* name,
                  const std::vector<AST_Field*>& fields,
                  AST_Type::SIZE_TYPE size, const char* repoid);

  bool gen_typedef(AST_Typedef*, UTL_ScopedName*, AST_Type*, const char*);

  bool gen_union(AST_Union*, UTL_ScopedName*, const std::vector<AST_UnionBranch*>&,
                 AST_Type*, const char*);

  struct Open {
    itl_generator* generator;

    Open (itl_generator* g)
      : generator(g)
    { }
  };

  struct Close {
    itl_generator* generator;

    Close (itl_generator* g)
      : generator(g)
    { }
  };

  struct Indent {
    itl_generator* generator;

    Indent (itl_generator* g)
      : generator(g)
    { }
  };

  int level_;

private:
  size_t count_;

  void new_type();
};

#endif
