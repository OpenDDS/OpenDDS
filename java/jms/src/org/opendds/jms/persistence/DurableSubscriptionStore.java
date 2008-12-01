/*
 * $Id$
 */

package org.opendds.jms.persistence;

import java.io.Serializable;

import javax.jms.JMSException;
import javax.jms.Message;

import org.hibernate.Session;
import org.hibernate.SessionFactory;
import org.hibernate.Transaction;

import org.opendds.jms.common.ExceptionHelper;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public class DurableSubscriptionStore implements Serializable {
    private SessionFactory sessionFactory;

    public DurableSubscriptionStore(SessionFactory sessionFactory) {
        this.sessionFactory = sessionFactory;
    }

    public void acknowledge(DurableSubscription subscription, Message message) throws JMSException {
        subscription.addAcknowledgedMessage(message);

        Session session = sessionFactory.openSession();
        Transaction transaction = null;
        try {
            transaction = session.beginTransaction();

            session.saveOrUpdate(subscription);
            transaction.commit();

        } catch (Exception e) {
            if (transaction != null) {
                transaction.rollback();
            }
            throw ExceptionHelper.wrap(e);

        } finally {
            session.close();
        }
    }

    public boolean acknowledged(DurableSubscription subscription, Message message) throws JMSException {
        Session session = sessionFactory.openSession();
        try {
            session.refresh(subscription);
            return subscription.isAcknowledged(message);

        } catch (Exception e) {
            throw ExceptionHelper.wrap(e);

        } finally {
            session.close();
        }
    }

    public void unsubscribe(DurableSubscription subscription) throws JMSException {
        Session session = sessionFactory.openSession();
        Transaction transaction = null;
        try {
            transaction = session.beginTransaction();

            session.delete(subscription);
            transaction.commit();

        } catch (Exception e) {
            if (transaction != null) {
                transaction.rollback();
            }
            throw ExceptionHelper.wrap(e);

        } finally {
            session.close();
        }
    }
}
