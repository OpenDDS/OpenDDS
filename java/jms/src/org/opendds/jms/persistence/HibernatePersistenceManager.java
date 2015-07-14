/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package org.opendds.jms.persistence;

import java.util.Properties;

import org.hibernate.SessionFactory;
import org.hibernate.cfg.Configuration;

/**
 * @author  Steven Stallion
 */
public class HibernatePersistenceManager implements PersistenceManager {
    private DurableSubscriptionStore durableSubscriptionStore;

    public HibernatePersistenceManager(Properties properties) {
        Configuration cfg = new Configuration();

        for (Class persistentClass : getPersistentClasses()) {
            cfg.addClass(persistentClass);
        }
        cfg.setProperties(properties);

        SessionFactory sessionFactory = cfg.buildSessionFactory();

        // Initialize persistence stores
        durableSubscriptionStore = new DurableSubscriptionStore(sessionFactory);
    }

    public Class[] getPersistentClasses() {
        return new Class[] {
            AcknowledgedMessage.class
        };
    }

    public DurableSubscriptionStore getDurableSubscriptionStore() {
        return durableSubscriptionStore;
    }
}
