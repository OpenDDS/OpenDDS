/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package org.opendds.esb.helpers;

import org.jboss.soa.esb.helpers.ConfigTree;

/**
 * @author  Steven Stallion
 */
public class ConfigTreeHelper {
    public static String requireAttribute(ConfigTree config, String name) {
        String value = config.getAttribute(name);
        if (value == null) {
            throw new IllegalArgumentException(name + " is a required attribute!");
        }
        return value;
    }

    public static Integer getAttribute(ConfigTree config, String name) {
        return getAttribute(config, name, 0);
    }

    public static Integer getAttribute(ConfigTree config, String name, Integer defaultValue) {
        String value = config.getAttribute(name);
        if (value != null) {
            return Integer.valueOf(value);
        }
        return defaultValue;
    }

    //

    private ConfigTreeHelper() {}
}
