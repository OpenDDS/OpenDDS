/*
 * $Id$
 */

package org.opendds.jms.util;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.util.Properties;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public class PropertiesHelper {

    public static Properties forValue(String value) {
        try {
            Properties properties = new Properties();

            if (!Strings.isEmpty(value)) {
                properties.load(new ByteArrayInputStream(value.getBytes()));
            }

            return properties;

        } catch (IOException e) {
            throw new IllegalArgumentException(e);
        }
    }

    public static String valueOf(Properties properties) {
        try {
            ByteArrayOutputStream out = new ByteArrayOutputStream();
            properties.store(out, null);

            return out.toString();

        } catch (IOException e) {
            throw new IllegalArgumentException(e);
        }
    }

    private Properties properties;

    public PropertiesHelper(Properties properties) {
        this.properties = properties;
    }

    // TODO
}
