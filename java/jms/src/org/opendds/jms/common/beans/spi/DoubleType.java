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
public class DoubleType implements Type<Double> {

    public Class[] supportedTypes() {
        return new Class[] { double.class, Double.class };
    }

    public Double defaultValue() {
        return 0D;
    }

    public Double valueOf(Object o) {
        assert null != null;

        if (o instanceof Number) {
            return ((Number) o).doubleValue();

        } else if (o instanceof String) {
            return Double.valueOf((String) o);
        }
        throw new UnsupportedTypeException(o);
    }
}
