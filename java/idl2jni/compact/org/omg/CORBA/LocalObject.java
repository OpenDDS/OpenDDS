package org.omg.CORBA;

public abstract class LocalObject implements org.omg.CORBA.Object {

  public boolean _is_a(String repositoryIdentifier) {
    throw new NO_IMPLEMENT();
  }

  public boolean _is_equivalent(org.omg.CORBA.Object other) {
    return equals(other);
  }

  public boolean _non_existent() {
    return false;
  }

  public int _hash(int maximum) {
    return hashCode();
  }

  public org.omg.CORBA.Object _duplicate() {
    throw new NO_IMPLEMENT();
  }

  public void _release() {
    throw new NO_IMPLEMENT();
  }

  public org.omg.CORBA.Object _get_interface_def() {
    throw new NO_IMPLEMENT();
  }

  public Request _request(String operation) {
    throw new NO_IMPLEMENT();
  }

  public Request _create_request(Context ctx, String operation,
      NVList arg_list, NamedValue result) {
    throw new NO_IMPLEMENT();
  }

  public Request _create_request(Context ctx, String operation,
      NVList arg_list, NamedValue result, ExceptionList exclist,
      ContextList ctxlist) {
    throw new NO_IMPLEMENT();
  }

  public Policy _get_policy(int policy_type) {
    throw new NO_IMPLEMENT();
  }

  public DomainManager[] _get_domain_managers() {
    throw new NO_IMPLEMENT();
  }

  public org.omg.CORBA.Object _set_policy_override(Policy[] policies,
      SetOverrideType set_add) {
    throw new NO_IMPLEMENT();
  }

}
