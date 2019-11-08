package org.omg.CORBA;

public final class DATA_CONVERSION extends SystemException {

  public DATA_CONVERSION() {
    this(null);
  }

  public DATA_CONVERSION(String s) {
    this(s, 0, CompletionStatus.COMPLETED_MAYBE);
  }

  public DATA_CONVERSION(int i, CompletionStatus c) {
    this(null, i, c);
  }

  public DATA_CONVERSION(String s, int i, CompletionStatus c) {
    super(s, i, c);
  }

}
