/*
 * $Id$
 */
 
package org.opendds.jms.common;

import DDS.PartitionQosPolicy;

/**
 * @author  Steven Stallion
 * @version $Revision$
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
            StringBuffer sb = new StringBuffer();

            for (int j = 0; j < i; ++j) {
                sb.append('?');
            }

            sb.append("[!").append(s.charAt(i)).append("]");

            for (int j = i + 1; j < len; ++j) {
                sb.append('?');
            }

            parts[i] = sb.toString();
        }
        return match(parts);
    }


    //

    private PartitionHelper() {}
}
