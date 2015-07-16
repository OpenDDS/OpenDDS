/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package org.opendds.jms.common.beans.spi;

import java.util.HashMap;
import java.util.Map;

import org.opendds.jms.common.spi.ServiceRegistry;

/**
 * @author  Steven Stallion
 */
public class TypeRegistry extends ServiceRegistry<Type> {
    private Map<Class, Type> types =
        new HashMap<Class, Type>();

    protected Class<Type> getProviderClass() {
        return Type.class;
    }

    @Override
    public void register(Type type) {
        super.register(type);

        for (Class clazz : type.supportedTypes()) {
            types.put(clazz, type);
        }
    }

    public Type findType(Class clazz) {
        assert clazz != null;

        return types.get(clazz);
    }
}
