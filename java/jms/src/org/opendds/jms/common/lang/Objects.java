/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package org.opendds.jms.common.lang;

/**
 * @author  Weiqi Gao
 * @author  Steven Stallion
 */
public class Objects {

    public static <T> T ensureNotNull(T t) {
        if (t == null) {
            throw new NullPointerException();
        }
        return t;
    }

    public static boolean equals(Object o1, Object o2) {
        return o1 == null && o2 == null || o1 != null && o1.equals(o2);
    }

    public static int hashCode(Object ...objs) {
        int value = 0;

        for (Object o : objs) {
            if (o != null) {
                value ^= o.hashCode();
            }
        }
        return value;
    }

    public static String toString(Object o) {
        assert o != null;

        StringBuilder sb = new StringBuilder();

        sb.append(o.getClass().getName());
        sb.append('@');
        sb.append(Integer.toHexString(o.hashCode()));

        return sb.toString();
    }

    //

    private Objects() {}
}
