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
  Unions::U u;
  OpenDDS::DCPS::set_default(u);
  Unions::T t;
  OpenDDS::DCPS::set_default(t);
#ifndef OPENDDS_SAFETY_PROFILE
  Unions::S s;
  OpenDDS::DCPS::set_default(s);
  Unions::R r;
  OpenDDS::DCPS::set_default(r);
#endif
  Unions::Q q;
  OpenDDS::DCPS::set_default(q);
  Unions::P p;
  OpenDDS::DCPS::set_default(p);
  return 0;
}

