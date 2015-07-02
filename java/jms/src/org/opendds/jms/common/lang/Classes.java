/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package org.opendds.jms.common.lang;

import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.List;

/**
 * @author  Steven Stallion
 */
public class Classes {

    public static Method findMethod(Class<?> clazz,
                                    String methodName,
                                    Object[] parameters) throws NoSuchMethodException {
        assert clazz != null;

        Class[] parameterTypes = null;
        if (parameters != null) {
            parameterTypes = getTypes(parameters);
        }

        return clazz.getMethod(methodName, parameterTypes);
    }

    public static Class[] getTypes(Object[] parameters) {
        assert parameters != null;

        List<Class> types = new ArrayList<Class>();

        for (Object parameter : parameters) {
            types.add(parameter.getClass());
        }

        return types.toArray(new Class[types.size()]);
    }

    //

    private Classes() {}
}
