/*
 * $Id$
 */

#include <sstream>

#include "utl_identifier.h"
#include "utl_string.h"

#include "idl_utility.h"

using namespace std;

const char* repo_identifier::prefix = "IDL";
const char* repo_identifier::version = "1.0";

repo_identifier::repo_identifier(UTL_ScopedName* name)
  : str_(to_str(name))
{
}

repo_identifier::~repo_identifier()
{
}

string
repo_identifier::str() const
{
  return str_;
}

string
repo_identifier::to_str(UTL_ScopedName* name)
{
  ostringstream os;

  os << prefix << ':';
  UTL_IdListActiveIterator it(name);
  while (!it.is_done())
  {
    Identifier* item = it.item();
    it.next();
  
    string s(item->get_string());
    
    if (s.empty()) continue; // ignore
      
    os << s;
    if (!it.is_done())
    {
      os << '/';
    }
  }
  os << ':' << version;

  return os.str();
}

repo_identifier::operator string() const
{
  return str_;
}

ostream&
operator<<(ostream& os, const repo_identifier& rhs)
{
  return os << rhs.str_;
}
