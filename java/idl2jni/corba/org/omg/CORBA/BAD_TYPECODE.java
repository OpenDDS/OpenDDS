package org.omg.CORBA;

public final class BAD_TYPECODE extends SystemException {

  public BAD_TYPECODE() {
    this(null);
  }

  public BAD_TYPECODE(String s) {
    this(s, 0, CompletionStatus.COMPLETED_MAYBE);
  }

  public BAD_TYPECODE(int i, CompletionStatus c) {
    this(null, i, c);
  }

  public BAD_TYPECODE(String s, int i, CompletionStatus c) {
    super(s, i, c);
  }

}
