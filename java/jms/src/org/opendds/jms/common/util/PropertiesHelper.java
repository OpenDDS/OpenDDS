/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package org.opendds.jms.common.util;

import java.io.ByteArrayInputStream;
import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.util.Enumeration;
import java.util.Map;
import java.util.Properties;
import java.util.regex.Pattern;

import org.opendds.jms.common.lang.ClassLoaders;
import org.opendds.jms.common.lang.Strings;

/**
 * @author  Steven Stallion
 */
public class PropertiesHelper {

    public static PropertiesHelper getSystemPropertiesHelper() {
        return new PropertiesHelper(System.getProperties());
    }

    public static Properties forName(String name) {
        try {
            InputStream in = ClassLoaders.getResourceAsStream(name);

            Properties properties = new Properties();
            properties.load(in);

            return properties;

        } catch (IOException e) {
            throw new IllegalArgumentException(e);
        }
    }

    public static Properties remap(Properties properties, Map<String, String> names) {
        assert properties != null;
        assert names != null;

        Enumeration en = properties.propertyNames();
        while (en.hasMoreElements()) {
            String name = (String) en.nextElement();

            properties.setProperty(names.get(name), properties.getProperty(name));
            properties.remove(name);
        }
        return properties;
    }

    public static Properties valueOf(String value) {
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

    //

    private Properties properties;

    public PropertiesHelper(Properties properties) {
        assert properties != null;

        this.properties = properties;
    }

    public Property find(String key) {
        return new Property(key);
    }

    public Property require(String key) {
        if (!hasProperty(key)) {
            throw new IllegalArgumentException(key + " is a required property!");
        }
        return find(key);
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

    public Boolean getBooleanProperty(String key) {
        return getBooleanProperty(key, Boolean.FALSE);
    }

    public Boolean getBooleanProperty(String key, Boolean defaultValue) {
        String value = getProperty(key);

        if (value == null) {
            return defaultValue;
        }

        return Boolean.valueOf(value);
    }

    public byte[] getBytesProperty(String key) {
        return getBytesProperty(key, null);
    }

    public byte[] getBytesProperty(String key, byte[] defaultValue) {
        String value = getProperty(key);

        if (value == null) {
            return defaultValue;
        }

        return value.getBytes();
    }

    public File getFileProperty(String key) {
        return getFileProperty(key, null);
    }

    public File getFileProperty(String key, File defaultValue) {
        String value = getProperty(key);

        if (value == null) {
            return defaultValue;
        }

        return new File(value);
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

    //

    public class Property {
        private String key;

        protected Property(String key) {
            this.key = key;
        }

        public boolean exists() {
            return hasProperty(key);
        }

        public String getKey() {
            return key;
        }

        public String getValue() {
            return getProperty(key);
        }

        public String getValue(String defaultValue) {
            return getProperty(key, defaultValue);
        }

        public Boolean asBoolean() {
            return getBooleanProperty(key);
        }

        public Boolean asBoolean(Boolean defaultValue) {
            return getBooleanProperty(key, defaultValue);
        }

        public byte[] asBytes() {
            return getBytesProperty(key);
        }

        public byte[] asBytes(byte[] defaultValue) {
            return getBytesProperty(key, defaultValue);
        }

        public File asFile() {
            return getFileProperty(key);
        }

        public File asFile(File defaultValue) {
            return getFileProperty(key, defaultValue);
        }

        public Integer asInt() {
            return getIntProperty(key);
        }

        public Integer asInt(Integer defaultValue) {
            return getIntProperty(key, defaultValue);
        }

        public boolean equals(String value) {
            return value != null && value.equals(getValue());
        }

        public boolean matches(String pattern) {
            return Pattern.matches(pattern, getValue());
        }
    }
}
