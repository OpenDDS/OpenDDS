/*
 * $Id$
 */

package org.opendds.jms.common.beans.spi;

import org.opendds.jms.common.beans.UnsupportedTypeException;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public class DoubleType implements Type<Double> {

    public Class<Double> getType() {
        return Double.class;
    }

    public Double defaultValue() {
        return 0D;
    }

    public Double valueOf(Object o) {
        if (o instanceof Number) {
            return ((Number) o).doubleValue();

        } else if (o instanceof String) {
            return Double.valueOf((String) o);
        }
        throw new UnsupportedTypeException(o);
    }
}
