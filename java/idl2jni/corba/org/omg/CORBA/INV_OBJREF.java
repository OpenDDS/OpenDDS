package org.omg.CORBA;

public final class INV_OBJREF extends SystemException {

  public INV_OBJREF() {
    this(null);
  }

  public INV_OBJREF(String s) {
    this(s, 0, CompletionStatus.COMPLETED_MAYBE);
  }

  public INV_OBJREF(int i, CompletionStatus c) {
    this(null, i, c);
  }

  public INV_OBJREF(String s, int i, CompletionStatus c) {
    super(s, i, c);
  }

}
