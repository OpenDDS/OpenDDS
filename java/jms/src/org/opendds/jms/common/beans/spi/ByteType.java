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
public class ByteType implements Type<Byte> {

    public Class[] supportedTypes() {
        return new Class[] { byte.class, Byte.class };
    }

    public Byte defaultValue() {
        return 0;
    }

    public Byte valueOf(Object o) {
        assert o != null;

        if (o instanceof Boolean) {
            return ((Boolean) o) ? (byte) 1 : 0;

        } else if (o instanceof Number) {
            return ((Number) o).byteValue();

        } else if (o instanceof String) {
            return Byte.valueOf((String) o);
        }
        throw new UnsupportedTypeException(o);
    }
}
