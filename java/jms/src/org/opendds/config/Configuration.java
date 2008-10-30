/*
 * $Id$
 */

package org.opendds.config;

import java.util.LinkedHashMap;
import java.util.Map;

import org.opendds.config.spi.PropertyFormat;
import org.opendds.util.Strings;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public class Configuration {
    public static final String DELIMITERS = ";, \t\r\n\f";

    private Map<String, Object> attributes =
        new LinkedHashMap<String, Object>();

    public <T> void set(String name, T value) {
        attributes.put(name, value);
    }

    @SuppressWarnings("unchecked")
    public <T> T get(String name) {
        return (T) attributes.get(name);
    }

    public boolean isSet(String name) {
        Object value = get(name);

        if (value == null) {
            return false;
        }

        if (value instanceof Boolean) {
            return (Boolean) value;
        }

        if (value instanceof String) {
            return !Strings.isEmpty((String) value);
        }

        return true;
    }

    public String[] toArgs() {
        return PropertyFormat.format(this);
    }
}
