/*
 * $Id$
 */
 
package org.opendds.jms.common.beans.spi;

import org.opendds.jms.common.beans.UnsupportedTypeException;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public class LongType implements Type<Long> {

    public Class<Long> getType() {
        return Long.class;
    }

    public Long defaultValue() {
        return 0L;
    }

    public Long valueOf(Object o) {
        if (o instanceof Number) {
            return ((Number) o).longValue();

        } else if (o instanceof String) {
            return Long.valueOf((String) o);
        }

        throw new UnsupportedTypeException(o);
    }
}
