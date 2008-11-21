/*
 * $Id$
 */

package org.opendds.jms.common.beans.spi;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public class StringType implements Type<String> {

    public Class[] supportedTypes() {
        return new Class[] { String.class };
    }

    public String defaultValue() {
        return null;
    }

    public String valueOf(Object o) {
        return o.toString();
    }
}
