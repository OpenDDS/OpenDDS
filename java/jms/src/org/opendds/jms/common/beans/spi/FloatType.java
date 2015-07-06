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
public class FloatType implements Type<Float> {

    public Class[] supportedTypes() {
        return new Class[] { float.class, Float.class };
    }

    public Float defaultValue() {
        return 0F;
    }

    public Float valueOf(Object o) {
        assert o != null;

        if (o instanceof Number) {
            return ((Number) o).floatValue();

        } else if (o instanceof String) {
            return Float.valueOf((String) o);
        }
        throw new UnsupportedTypeException(o);
    }
}
