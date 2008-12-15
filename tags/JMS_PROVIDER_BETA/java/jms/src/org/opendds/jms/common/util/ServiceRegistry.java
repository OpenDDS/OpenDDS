/*
 * $Id$
 */

package org.opendds.jms.common.util;

import java.util.ArrayList;
import java.util.Collection;
import java.util.Collections;
import java.util.Iterator;
import java.util.List;

import sun.misc.Service;

/**
 * @author Steven Stallion
 * @version $Revision$
 */
public abstract class ServiceRegistry<T> {
    protected List<T> providers = new ArrayList<T>();

    protected abstract Class<T> getProviderClass();

    @SuppressWarnings("unchecked")
    public void registerAll() {
        Iterator itr = Service.providers(getProviderClass());
        while (itr.hasNext()) {
            register((T) itr.next());
        }
    }

    public void register(T t) {
        assert t != null;

        providers.add(t);
    }

    public void deregisterAll() {
        providers.clear();
    }

    public void deregister(T t) {
        providers.remove(t);
    }

    public Collection<T> installedProviders() {
        return Collections.unmodifiableCollection(providers);
    }
}
