/*
 * $Id$
 */

package org.opendds.jms.jmx.config;

import java.util.LinkedHashMap;
import java.util.Map;

import org.opendds.jms.jmx.config.spi.AttributeFormat;
import org.opendds.jms.util.Strings;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public class Attributes {
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
        return AttributeFormat.format(this);
    }
}
