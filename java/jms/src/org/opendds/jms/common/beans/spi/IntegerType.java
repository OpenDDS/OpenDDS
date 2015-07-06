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
public class IntegerType implements Type<Integer> {

    public Class[] supportedTypes() {
        return new Class[] { int.class, Integer.class };
    }

    public Integer defaultValue() {
        return 0;
    }

    public Integer valueOf(Object o) {
        assert o != null;

        if (o instanceof Boolean) {
            return ((Boolean) o) ? 1 : 0;

        } else if (o instanceof Number) {
            return ((Number) o).intValue();

        } else if (o instanceof String) {
            return Integer.valueOf((String) o);
        }
        throw new UnsupportedTypeException(o);
    }
}
