/*
 * $Id$
 */

package org.opendds.jms.util;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public class Strings {

    public static boolean hasLength(String s, int len) {
        return hasLength(s, len, true);
    }

    public static boolean hasLength(String s, int len, boolean trim) {
        assert len >= 0;

        if (s == null) {
            return false;
        }

        if (trim) {
            s = s.trim();
        }

        return s.length() >= len;
    }

    public static boolean isEmpty(String s) {
        return isEmpty(s, true);
    }

    public static boolean isEmpty(String s, boolean trim) {
        if (s == null) {
            return true;
        }

        if (trim) {
            s = s.trim();
        }

        return s.length() == 0;
    }

    //

    private Strings() {}
}
