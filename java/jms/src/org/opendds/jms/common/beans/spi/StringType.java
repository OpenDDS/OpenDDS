/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package org.opendds.jms.common.beans.spi;

/**
 * @author  Steven Stallion
 */
public class StringType implements Type<String> {

    public Class[] supportedTypes() {
        return new Class[] { String.class };
    }

    public String defaultValue() {
        return null;
    }

    public String valueOf(Object o) {
        assert o != null;

        return o.toString();
    }
}
