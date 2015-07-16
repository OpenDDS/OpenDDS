/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package org.opendds.jms.common.spi;

import java.util.ArrayList;
import java.util.Collection;
import java.util.Collections;
import java.util.Iterator;
import java.util.List;

/**
 * @author Steven Stallion
 */
public abstract class ServiceRegistry<T> {
    protected List<T> providers =
        new ArrayList<T>();

    protected abstract Class<T> getProviderClass();

    public void registerAll() {
        Iterator<T> itr = Service.providers(getProviderClass());
        while (itr.hasNext()) {
            register(itr.next());
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
