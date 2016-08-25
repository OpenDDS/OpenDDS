package org.omg.CORBA;

public final class BAD_INV_ORDER extends SystemException {

  public BAD_INV_ORDER() {
    this("");
  }

  public BAD_INV_ORDER(String s) {
    this(s, 0, CompletionStatus.COMPLETED_NO);
  }

  public BAD_INV_ORDER(int i, CompletionStatus c) {
    this("", i, c);
  }

  public BAD_INV_ORDER(String s, int i, CompletionStatus c) {
    super(s, i, c);
  }

}
