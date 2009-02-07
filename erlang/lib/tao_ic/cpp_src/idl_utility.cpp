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
{
  ostringstream os;

  os << prefix << ':';
  UTL_IdListActiveIterator it(name);
  while (!it.is_done())
  {
    Identifier* item = it.item();
  
    string s(item->get_string());
    it.next(); // advance iterator

    if (s.empty()) continue;
      
    os << s;
    if (!it.is_done())
      os << '/';
  }
  os << ':' << version;

  str_ = os.str();
}

repo_identifier::~repo_identifier()
{
}

string
repo_identifier::str() const
{
  return str_;
}

repo_identifier::operator string() const
{
  return str_;
}

ostream&
operator<<(ostream& os, const repo_identifier& r)
{
  return os << r.str_;
}
