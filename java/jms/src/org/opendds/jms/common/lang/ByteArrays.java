/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package org.opendds.jms.common.lang;

/**
 * @author  Weiqi Gao
 */
public class ByteArrays {
    public static byte[] extractSubBytes(byte[] bytes, int i, int i1) {
        byte[] subBytes = new byte[i1];
        System.arraycopy(bytes, i, subBytes, 0, i1);
        return subBytes;
    }

    /**
     * Fill the destination array with enough of the source array
     * @param src
     * @param dst
     */
    public static int fillByteArray(byte[] src, int offset,  byte[] dst) {
        int filled = Math.min(src.length - offset, dst.length);
        if (filled > 0) System.arraycopy(src, offset, dst, 0, filled);
        return filled;
    }
}
