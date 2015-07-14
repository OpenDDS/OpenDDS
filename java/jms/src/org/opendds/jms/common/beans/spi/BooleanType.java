/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package org.opendds.jms.common.beans.spi;

import org.opendds.jms.common.beans.UnsupportedTypeException;

/**
 * @author  Steven Stallion
 */
public class BooleanType implements Type<Boolean> {

    public Class[] supportedTypes() {
        return new Class[] { boolean.class, Boolean.class };
    }

    public Boolean defaultValue() {
        return false;
    }

    public Boolean valueOf(Object o) {
        assert o != null;

        if (o instanceof Number) {
            return ((Number) o).intValue() != 0;

        } else if (o instanceof String) {
            return Boolean.valueOf((String) o);
        }
        throw new UnsupportedTypeException(o);
    }
}
