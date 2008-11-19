/*
 * $Id$
 */

package org.opendds.jms.common.beans.spi;

import org.opendds.jms.common.spi.ServiceRegistry;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public class TypeRegistry extends ServiceRegistry<Type> {

    protected Class<Type> getProviderClass() {
        return Type.class;
    }

    public Type findType(Class clazz) {
        assert clazz != null;

        for (Type type : providers) {
            if (clazz.equals(type.getType())) {
                return type;
            }
        }
        return null;
    }
}
