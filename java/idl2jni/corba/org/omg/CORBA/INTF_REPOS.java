package org.omg.CORBA;

public final class INTF_REPOS extends SystemException {

  public INTF_REPOS() {
    this(null);
  }

  public INTF_REPOS(String s) {
    this(s, 0, CompletionStatus.COMPLETED_MAYBE);
  }

  public INTF_REPOS(int i, CompletionStatus c) {
    this(null, i, c);
  }

  public INTF_REPOS(String s, int i, CompletionStatus c) {
    super(s, i, c);
  }

}
