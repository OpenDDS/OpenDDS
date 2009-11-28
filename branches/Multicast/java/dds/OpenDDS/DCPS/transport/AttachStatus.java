/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package OpenDDS.DCPS.transport;

import java.io.Serializable;

public class AttachStatus implements Serializable {

  private static AttachStatus[] __values = {
    new AttachStatus(0),
    new AttachStatus(1),
    new AttachStatus(2),
    new AttachStatus(3)
  };

  public static final int _ATTACH_BAD_TRANSPORT = 0;
  public static final AttachStatus ATTACH_BAD_TRANSPORT = __values[0];

  public static final int _ATTACH_ERROR = 1;
  public static final AttachStatus ATTACH_ERROR = __values[1];

  public static final int _ATTACH_INCOMPATIBLE_QOS = 2;
  public static final AttachStatus ATTACH_INCOMPATIBLE_QOS = __values[2];

  public static final int _ATTACH_OK = 3;
  public static final AttachStatus ATTACH_OK = __values[3];

  public int value() { return _value; }
  private int _value;
  public static AttachStatus from_int(int value) {
    return __values[value];
  }
  protected AttachStatus(int value) { _value = value; }

  public Object readResolve()
      throws java.io.ObjectStreamException {
    return from_int(value());
  }
}
