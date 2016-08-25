package org.omg.CORBA;

public abstract class SystemException extends RuntimeException {

  public int minor;
  public CompletionStatus completed;

  protected SystemException(String s, int i, CompletionStatus c) {
    super(s);
    minor = i;
    completed = c;
  }

}
