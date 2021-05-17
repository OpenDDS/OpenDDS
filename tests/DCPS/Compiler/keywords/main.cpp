#include <testTypeSupportImpl.h>

#if CPP11_MAPPING
#  define REF(WHAT) (WHAT)()
#else
#  define REF(WHAT) (WHAT)
#endif

int ACE_TMAIN(int, ACE_TCHAR**)
{
  the_module::the_struct a;
  REF(a.the_field) = the_module::the_const;
  ACE_UNUSED_ARG(a);
  the_module::the_structDataWriter* a_dw = 0;
  ACE_UNUSED_ARG(a_dw);

  the_module::the_union au;
  au.the_field(the_module::the_const);
  ACE_UNUSED_ARG(au);
  the_module::the_unionDataWriter* au_dw = 0;
  ACE_UNUSED_ARG(au_dw);

  boolean::attribute b;
  REF(b.component) = boolean::oneway;
  ACE_UNUSED_ARG(b);
  boolean::attributeDataWriter* b_dw = 0;
  ACE_UNUSED_ARG(b_dw);

  boolean::primarykey bu;
  bu.truncatable(boolean::oneway);
  ACE_UNUSED_ARG(bu);
  boolean::primarykeyDataWriter* bu_dw = 0;
  ACE_UNUSED_ARG(bu_dw);

  _cxx_bool::_cxx_class c;
  REF(c._cxx_else) = _cxx_bool::_cxx_continue;
  ACE_UNUSED_ARG(c);
  _cxx_bool::classDataWriter* c_dw = 0;
  ACE_UNUSED_ARG(c_dw);

  _cxx_bool::_cxx_goto cu;
  cu._cxx_asm(_cxx_bool::_cxx_continue);
  ACE_UNUSED_ARG(cu);
  _cxx_bool::gotoDataWriter* cu_dw = 0;
  ACE_UNUSED_ARG(cu_dw);

  _cxx_case::_cxx_struct d;
  REF(d._cxx_union) = _cxx_case::_cxx_typeid;
  ACE_UNUSED_ARG(d);
  _cxx_case::structDataWriter* d_dw = 0;
  ACE_UNUSED_ARG(d_dw);

  _cxx_case::_cxx_private du;
  du._cxx_public(_cxx_case::_cxx_typeid);
  ACE_UNUSED_ARG(du);
  _cxx_case::privateDataWriter* du_dw = 0;
  ACE_UNUSED_ARG(du_dw);

  return 0;
}
