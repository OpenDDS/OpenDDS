/*
 * $Id$
 */
 
package org.opendds.jms.common.beans.spi;

import java.util.ArrayList;
import java.util.Collection;
import java.util.Collections;
import java.util.Iterator;
import java.util.List;

import sun.misc.Service;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public class TypeRegistry {
    private List<Type> types = new ArrayList<Type>();

    public void registerAll() {
        Iterator itr = Service.providers(Type.class);
        while (itr.hasNext()) {
            register((Type) itr.next());
        }
    }

    public void register(Type type) {
        types.add(type);
    }

    public void deregisterAll() {
        types.clear();
    }

    public void deregister(Type type) {
        types.remove(type);
    }

    public Type findType(Class clazz) {
        assert clazz != null;

        for (Type type : types) {
            if (clazz.equals(type.getType())) {
                return type;
            }
        }
        return null;
    }

    public Collection<Type> getTypes() {
        return Collections.unmodifiableCollection(types);
    }
}
