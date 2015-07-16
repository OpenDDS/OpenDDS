/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package org.opendds.jms.common.lang;

import java.util.UUID;

/**
 * @author  Steven Stallion
 */
public class Strings {

    public static String asIdentity(Object o) {
        assert o != null;

        return String.format("%s -> %s", Objects.toString(o), o.toString());
    }

    public static String capitalize(String s) {
        assert s != null;

        StringBuffer sbuf = new StringBuffer();

        sbuf.append(s.trim());

        char first = sbuf.charAt(0);
        sbuf.setCharAt(0, Character.toUpperCase(first));

        return sbuf.toString();
    }

    public static StringBuilder fill(StringBuilder sb, char c, int len) {
        assert sb != null;
        assert len > -1;

        for (int i = 0; i < len; ++i) {
            sb.append(c);
        }
        return sb;
    }

    public static boolean hasLength(String s, int len) {
        return hasLength(s, len, true);
    }

    public static boolean hasLength(String s, int len, boolean trim) {
        assert len > -1;

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

    public static String randomUuid() {
        return UUID.randomUUID().toString();
    }

    //

    private Strings() {}
}
