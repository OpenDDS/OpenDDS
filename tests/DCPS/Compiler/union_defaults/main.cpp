#include "union_defaultsTypeSupportImpl.h"

int ACE_TMAIN(int, ACE_TCHAR**)
{
  Unions::Z z;
  OpenDDS::DCPS::set_default(z);
  Unions::Y y;
  OpenDDS::DCPS::set_default(y);
  Unions::X x;
  OpenDDS::DCPS::set_default(x);
  Unions::V v;
  OpenDDS::DCPS::set_default(v);
  //Unions::U u;
  //OpenDDS::DCPS::set_default(u);
  //Unions::T t;
  //OpenDDS::DCPS::set_default(t);
  return 0;
}

