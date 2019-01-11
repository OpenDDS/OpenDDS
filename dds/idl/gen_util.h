
#ifndef OPENDDS_IDL_GEN_UTIL_H
#define OPENDDS_IDL_GEN_UTIL_H

#include "be_extern.h"

namespace {
  inline
  std::string insert_cxx11_accessor_parens(
                std::string full_var_name, bool is_union_member) {
    const bool use_cxx11 = be_global->language_mapping() == BE_GlobalData::LANGMAP_CXX11;
    if (!use_cxx11 || is_union_member) return full_var_name;

    std::string::size_type n = 0;
    while ((n = full_var_name.find(".", n)) != std::string::npos) {
      if (full_var_name[n-1] != ']') {
        full_var_name.insert(n, "()");
        n += 3;
      } else {
        ++n;
      }
    }
    n = 0;
    while ((n = full_var_name.find("[", n)) != std::string::npos) {
      full_var_name.insert(n, "()");
      n += 3;
    }
    return full_var_name[full_var_name.length()-1] == ']'
      ? full_var_name : full_var_name + "()";
  }
}

#endif
