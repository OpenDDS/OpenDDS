package org.opendds.jms.util;

public class Objects {
    public static <T> T ensureNotNull(T t) {
        if (t == null) {
            throw new NullPointerException();
        }
        return t;
    }
}
