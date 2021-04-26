#include "testTypeSupportImpl.h"

int ACE_TMAIN(int, ACE_TCHAR**)
{
  the_module::the_struct a;
  a.the_field() = the_module::the_const;
  ACE_UNUSED_ARG(a);
  the_module::the_structDataWriter* a_dw = 0;
  ACE_UNUSED_ARG(a_dw);

  boolean::attribute b;
  b.component() = boolean::oneway;
  ACE_UNUSED_ARG(b);
  boolean::attributeDataWriter* b_dw = 0;
  ACE_UNUSED_ARG(b_dw);

  /* TODO(iguessthislldo): See test.idl
  _cxx_bool::_cxx_class c;
  c._cxx_else() = _cxx_bool::_cxx_continue;
  ACE_UNUSED_ARG(c);
  _cxx_bool::cxx_classDataWriter* c_dw = 0;
  ACE_UNUSED_ARG(c_dw);

  _cxx_case::_cxx_struct d;
  d._cxx_union() = _cxx_case::_cxx_typeid;
  ACE_UNUSED_ARG(d);
  _cxx_case::cxx_structDataWriter* d_dw = 0;
  ACE_UNUSED_ARG(d_dw);
  */

  return 0;
}
