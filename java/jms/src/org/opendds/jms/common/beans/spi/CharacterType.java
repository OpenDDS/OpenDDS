/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package org.opendds.jms.common.beans.spi;

import org.opendds.jms.common.beans.UnsupportedTypeException;
import org.opendds.jms.common.lang.Strings;

/**
 * @author  Steven Stallion
 */
public class CharacterType implements Type {

    public Class[] supportedTypes() {
        return new Class[] { char.class, Character.class };
    }

    public Character defaultValue() {
        return 0;
    }

    public Character valueOf(Object o) {
        assert o != null;

        if (o instanceof String) {
            String s = (String) o;
            return !Strings.isEmpty(s) ? s.charAt(0) : defaultValue();
        }
        throw new UnsupportedTypeException(o);
    }
}
