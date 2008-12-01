/*
 * $Id$
 */

package org.opendds.jms.persistence;

import java.io.Serializable;
import java.util.Properties;

import org.hibernate.SessionFactory;
import org.hibernate.cfg.Configuration;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public class PersistenceManager implements Serializable {
    private SessionFactory sessionFactory;
    private DurableSubscriptionStore durableSubscriptionStore;

    public PersistenceManager(Properties properties) {
        Configuration cfg = new Configuration();

        for (Class persistentClass : getPersistentClasses()) {
            cfg.addClass(persistentClass);
        }

        cfg.setProperties(properties);

        sessionFactory = cfg.buildSessionFactory();

        // Initialize persistence stores
        durableSubscriptionStore = new DurableSubscriptionStore(sessionFactory);
    }

    public Class[] getPersistentClasses() {
        return new Class[] {
            AcknowledgedMessage.class
        };
    }

    public SessionFactory getSessionFactory() {
        return sessionFactory;
    }

    public DurableSubscriptionStore getDurableSubscriptionStore() {
        return durableSubscriptionStore;
    }
}
