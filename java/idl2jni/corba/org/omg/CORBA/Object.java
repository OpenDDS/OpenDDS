package org.omg.CORBA;

public interface Object {
  boolean _is_a(String Identifier);
  boolean _is_equivalent(org.omg.CORBA.Object that);
  boolean _non_existent();
  int _hash(int maximum);
  org.omg.CORBA.Object _duplicate();
  void _release();
  org.omg.CORBA.Object _get_interface_def();
  Request _request(String s);
  Request _create_request(Context ctx, String operation,
                          NVList arg_list, NamedValue result);
  Request _create_request(Context ctx, String operation, NVList arg_list,
                          NamedValue result, ExceptionList exclist,
                          ContextList ctxlist);
  Policy _get_policy(int policy_type);
  DomainManager[] _get_domain_managers();
  org.omg.CORBA.Object _set_policy_override(Policy[] policies,
                                            SetOverrideType set_add);
}
