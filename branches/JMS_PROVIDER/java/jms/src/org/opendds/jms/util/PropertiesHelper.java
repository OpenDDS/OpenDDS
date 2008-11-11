/*
 * $Id$
 */

package org.opendds.jms.util;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.Properties;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public class PropertiesHelper {

    public static Properties forResource(String resource) {
        try {
            InputStream in = ClassLoaders.getResourceAsStream(resource);

            Properties properties = new Properties();
            properties.load(in);

            return properties;

        } catch (IOException e) {
            throw new IllegalArgumentException(e);
        }
    }

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

    public boolean hasProperty(String key) {
        return properties.containsKey(key);
    }

    public String getProperty(String key) {
        return getProperty(key, null);
    }

    public String getProperty(String key, String defaultValue) {
        return properties.getProperty(key, defaultValue);
    }

    public String requireProperty(String key) {
        String value = properties.getProperty(key);
        if (Strings.isEmpty(value)) {
            throw new IllegalArgumentException(key + " is a required property!");
        }
        return value;
    }

    public Integer getIntProperty(String key) {
        return getIntProperty(key, 0);
    }

    public Integer getIntProperty(String key, Integer defaultValue) {
        String value = getProperty(key);

        if (value == null) {
            return defaultValue;
        }

        return Integer.valueOf(value);
    }

    public Integer requireIntProperty(String key) {
        if (!hasProperty(key)) {
            throw new IllegalArgumentException(key + " is a required property!");
        }
        return getIntProperty(key);
    }
}
