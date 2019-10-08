package org.omg.CORBA;

public final class CODESET_INCOMPATIBLE extends SystemException {

  public CODESET_INCOMPATIBLE() {
    this(null);
  }

  public CODESET_INCOMPATIBLE(String s) {
    this(s, 0, CompletionStatus.COMPLETED_MAYBE);
  }

  public CODESET_INCOMPATIBLE(int i, CompletionStatus c) {
    this(null, i, c);
  }

  public CODESET_INCOMPATIBLE(String s, int i, CompletionStatus c) {
    super(s, i, c);
  }

}
