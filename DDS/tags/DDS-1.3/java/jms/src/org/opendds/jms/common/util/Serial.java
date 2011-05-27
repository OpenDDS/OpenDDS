/*
 * $Id$
 */

package org.opendds.jms.common.util;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public class Serial {
    private boolean overflow;
    private int serial;

    public Serial() {
        this(0);
    }

    public Serial(int initial) {
        serial = initial;
    }

    public synchronized boolean overflowed() {
        return overflow;
    }

    public synchronized int next() {
        if (serial == Integer.MAX_VALUE) {
            overflow = true;
        }
        return ++serial;
    }
}
