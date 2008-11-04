/*
 * $Id$
 */

package org.opendds.jms.util;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public class Strings {

    public static String capitalize(String s) {
        assert s != null;

        StringBuffer sbuf = new StringBuffer();

        sbuf.append(s.trim());

        char first = sbuf.charAt(0);
        sbuf.setCharAt(0, Character.toUpperCase(first));

        return sbuf.toString();
    }

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
