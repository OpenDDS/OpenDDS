/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package org.opendds.jms.common;

import DDS.PartitionQosPolicy;

import org.opendds.jms.common.lang.Strings;

/**
 * @author  Steven Stallion
 */
public class PartitionHelper {

    public static PartitionQosPolicy match(String ...s) {
        return new PartitionQosPolicy(s);
    }

    public static PartitionQosPolicy matchAll() {
        return match("*");
    }

    public static PartitionQosPolicy negate(String s) {
        int len = s.length();

        String[] parts = new String[len];
        for (int i = 0; i < len; ++i) {
            StringBuilder sb = new StringBuilder(len + 3);

            Strings.fill(sb, '?', i);

            sb.append("[!").append(s.charAt(i)).append("]");

            Strings.fill(sb, '?', len - i - 1);

            parts[i] = sb.toString();
        }
        return match(parts);
    }

    //

    private PartitionHelper() {}
}
