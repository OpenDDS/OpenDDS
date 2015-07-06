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
public class LongType implements Type<Long> {

    public Class[] supportedTypes() {
        return new Class[] { long.class, Long.class };
    }

    public Long defaultValue() {
        return 0L;
    }

    public Long valueOf(Object o) {
        assert o != null;

        if (o instanceof Number) {
            return ((Number) o).longValue();

        } else if (o instanceof String) {
            return Long.valueOf((String) o);
        }
        throw new UnsupportedTypeException(o);
    }
}
