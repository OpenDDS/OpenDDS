/*
 * $Id$
 */

package org.opendds.esb.helpers;

import org.jboss.soa.esb.helpers.ConfigTree;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public class ConfigTreeHelper {

    public static String requireProperty(ConfigTree config, String name) {
        String value = config.getAttribute(name);
        if (value == null) {
            throw new IllegalArgumentException(name + " is a required attribute!");
        }
        return value;
    }

    //

    private ConfigTreeHelper() {}
}
