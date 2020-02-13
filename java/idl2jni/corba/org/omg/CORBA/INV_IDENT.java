package org.omg.CORBA;

public final class INV_IDENT extends SystemException {

  public INV_IDENT() {
    this(null);
  }

  public INV_IDENT(String s) {
    this(s, 0, CompletionStatus.COMPLETED_MAYBE);
  }

  public INV_IDENT(int i, CompletionStatus c) {
    this(null, i, c);
  }

  public INV_IDENT(String s, int i, CompletionStatus c) {
    super(s, i, c);
  }

}
