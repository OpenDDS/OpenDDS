/*
 * $Id$
 */

#ifndef TAO_IC_IDL_UTILITY_H
#define TAO_IC_IDL_UTILITY_H

#include <ostream>
#include <string>

#include "utl_scoped_name.h"

class repo_identifier
{
public:
  static const char* prefix;
  static const char* version;

  repo_identifier(UTL_ScopedName* name);

  ~repo_identifier();

  std::string str() const;

  operator std::string() const;

private:
  std::string str_;
  
  friend std::ostream& operator<<(std::ostream& os, const repo_identifier& r);
};

#endif /* TAO_IC_IDL_UTILITY_H */
