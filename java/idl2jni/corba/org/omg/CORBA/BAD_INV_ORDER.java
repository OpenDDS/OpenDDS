package org.omg.CORBA;

public final class BAD_INV_ORDER extends SystemException {

  public BAD_INV_ORDER() {
    this(null);
  }

  public BAD_INV_ORDER(String s) {
    this(s, 0, CompletionStatus.COMPLETED_MAYBE);
  }

  public BAD_INV_ORDER(int i, CompletionStatus c) {
    this(null, i, c);
  }

  public BAD_INV_ORDER(String s, int i, CompletionStatus c) {
    super(s, i, c);
  }

}
